// -*- C++ -*-
#ifndef RIVET_MC_BOOSTEDHBB_HH
#define RIVET_MC_BOOSTEDHBB_HH

#include <iostream>
#include <map>
#include <string>

#include "Rivet/Analysis.hh"
#include "Rivet/Tools/Logging.hh"

#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/VisibleFinalState.hh"
#include "Rivet/Projections/MergedFinalState.hh"
#include "Rivet/Projections/VetoedFinalState.hh"
#include "Rivet/Projections/IdentifiedFinalState.hh"
#include "Rivet/Projections/ChargedLeptons.hh"
#include "Rivet/Projections/MissingMomentum.hh"

#include "Rivet/Jet.hh"
#include "Rivet/Projections/FastJets.hh"

using std::map;
using std::string;

namespace Rivet {


    class MC_BOOSTEDHBB : public Analysis {
        public:

            /// Constructor
            MC_BOOSTEDHBB()
                : Analysis("MC_BOOSTEDHBB") {

                    return;
                }


        public:

            /// @name Analysis methods
            //@{

            /// Book histograms and initialise projections before the run
            void init() {

                ChargedLeptons clfs(FinalState(-2.5, 2.5, 25*GeV));
                addProjection(clfs, "ChargedLeptons");

                MissingMomentum mmfs(FinalState(-4.2, 4.2, 0*GeV));
                addProjection(mmfs, "MissingMomentum");

                // calo jets constituents
                // don't include high-pt neutrinos or leptons in jets
                // TODO
                // include electrons?
                IdentifiedFinalState nufs(FinalState(-2.5, 2.5, 25*GeV));
                nufs.acceptNeutrinos();

                MergedFinalState leptonsAndNeutrinos(clfs, nufs);
                addProjection(leptonsAndNeutrinos, "LeptonsAndNeutrinos");

                VetoedFinalState caloParts(FinalState(-4.2, 4.2));
                caloParts.addVetoOnThisFinalState(leptonsAndNeutrinos);

                // "track" jets constituents
                ChargedFinalState trackParts(-2.5, 2.5, 0.5*GeV);


                char buff[100];
                string s;
                for (unsigned int i = 2; i <= 10; i += 2) {

                    // register calo jets
                    sprintf(buff, "AntiKt%02dCaloJets", i);
                    s = string(buff);
                    jetCollections.push_back(s);
                    addJetCollection(s);
                    minJetPtCut[s] = 25*GeV;
                    addProjection(FastJets(caloParts, FastJets::ANTIKT, i/10.0), s);

                    // and corresponding b-tagged jets
                    sprintf(buff, "AntiKt%02dCaloJetsB", i);
                    s = string(buff);
                    addJetCollection(s);


                    // register track jets
                    sprintf(buff, "AntiKt%02dTrackJets", i);
                    s = string(buff);
                    jetCollections.push_back(s);
                    addJetCollection(s);
                    minJetPtCut[s] = 25*GeV;
                    addProjection(FastJets(trackParts, FastJets::ANTIKT, i/10.0), s);

                    // and corresponding b-tagged jets
                    sprintf(buff, "AntiKt%02dTrackJetsB", i);
                    s = string(buff);
                    addJetCollection(s);
                }

                
                return;
            }


            /// Perform the per-event analysis
            void analyze(const Event& event) {
                const double weight = event.weight();

                // leptons
                const Particles &leptons =
                    applyProjection<ChargedLeptons>(event, "ChargedLeptons").particles();


                foreach (const string &name, jetCollections) {
                    const FastJets &fj =
                        applyProjection<FastJets>(event, name);
                    const Jets &jets = fj.jetsByPt(minJetPtCut[name]);
                    fillJetCollection(name, jets, weight);

                    Jets bjets;
                    foreach (const Jet& jet, jets)
                        if (jet.bTags().size()) bjets.push_back(jet);

                    fillJetCollection(name + "B", bjets, weight);
                }

                return;
            }


            /// Normalise histograms etc., after the run
            void finalize() {

                for (map<string, map<string, Histo1DPtr> >::iterator p = jet1DHistos.begin(); p != jet1DHistos.end(); ++p) {
                    for (map<string, Histo1DPtr>::iterator q = p->second.begin(); q != p->second.end(); ++q) {
                        scale(q->second, crossSection()/sumOfWeights()); // norm to cross section
                    }
                }

                return;
            }

            //@}


        private:

            vector<string> jetCollections;
            map<string, map<string, Histo1DPtr> > jet1DHistos;
            map<string, map<string, Histo2DPtr> > jet2DHistos;
            map<string, double> minJetPtCut;

            void addJetCollection(const string &name) {
                jet1DHistos[name] = map<string, Histo1DPtr>();
                jet2DHistos[name] = map<string, Histo2DPtr>();

                MSG_DEBUG("Adding jet collection " << name);

                map<string, Histo1DPtr> &mh1D = jet1DHistos.at(name);
                map<string, Histo2DPtr> &mh2D = jet2DHistos.at(name);

                mh1D["_n"] = bookHisto1D(name + "_n", 10, 0, 10);
                mh1D["_pt"] = bookHisto1D(name + "_pt", 50, 0, 2000*GeV);
                mh1D["_eta"] = bookHisto1D(name + "_eta", 50, -5, 5);
                mh1D["_phi"] = bookHisto1D(name + "_phi", 50, 0, 2*PI);
                mh1D["_E"] = bookHisto1D(name + "_E", 50, 0, 2000*GeV);
                mh1D["_m"] = bookHisto1D(name + "_m", 50, 0, 500*GeV);

                mh1D["0_pt"] = bookHisto1D(name + "0_pt", 50, 0, 2000*GeV);
                mh1D["0_eta"] = bookHisto1D(name + "0_eta", 50, -5, 5);
                mh1D["0_phi"] = bookHisto1D(name + "0_phi", 50, 0, 2*PI);
                mh1D["0_E"] = bookHisto1D(name + "0_E", 50, 0, 2000*GeV);
                mh1D["0_m"] = bookHisto1D(name + "0_m", 50, 0, 500*GeV);

                mh1D["1_pt"] = bookHisto1D(name + "1_pt", 50, 0, 2000*GeV);
                mh1D["1_eta"] = bookHisto1D(name + "1_eta", 50, -5, 5);
                mh1D["1_phi"] = bookHisto1D(name + "1_phi", 50, 0, 2*PI);
                mh1D["1_E"] = bookHisto1D(name + "1_E", 50, 0, 2000*GeV);
                mh1D["1_m"] = bookHisto1D(name + "1_m", 50, 0, 500*GeV);


                mh1D["01_dr"] = bookHisto1D(name + "01_dr", 50, 0, 8);
                mh1D["01_deta"] = bookHisto1D(name + "01_deta", 50, 0, 8);
                mh1D["01_dphi"] = bookHisto1D(name + "01_dphi", 50, 0, PI);

                mh1D["01_pt"] = bookHisto1D(name + "01_pt", 50, 0, 2000*GeV);
                mh1D["01_eta"] = bookHisto1D(name + "01_eta", 50, -5, 5);
                mh1D["01_phi"] = bookHisto1D(name + "01_phi", 50, 0, 2*PI);
                mh1D["01_E"] = bookHisto1D(name + "01_E", 50, 0, 2000*GeV);
                mh1D["01_m"] = bookHisto1D(name + "01_m", 50, 0, 500*GeV);

                mh2D["_m_pt"] = bookHisto2D(name + "_m_pt", 50, 0, 2000*GeV, 50, 0, 500*GeV);

                mh2D["0_m_pt"] = bookHisto2D(name + "0_m_pt", 50, 0, 2000*GeV, 50, 0, 500*GeV);

                mh2D["1_m_pt"] = bookHisto2D(name + "1_m_pt", 50, 0, 2000*GeV, 50, 0, 500*GeV);

                mh2D["0_pt_1_pt"] = bookHisto2D(name + "0_pt_1_pt", 50, 0, 2000*GeV, 50, 0, 2000*GeV);

                mh2D["01_dr_01_pt"] = bookHisto2D(name + "01_dr_01_pt", 50, 0, 2000*GeV, 50, 0, 5);

                mh2D["01_deta_01_pt"] = bookHisto2D(name + "01_deta_01_pt", 50, 0, 2000*GeV, 50, 0, 5);

                mh2D["01_m_01_pt"] = bookHisto2D(name + "01_m_01_pt", 50, 0, 2000*GeV, 50, 0, 500*GeV);

                return;
            }


            void fillJetCollection(const string &name, const Jets &jets, double weight) {
                MSG_DEBUG("Filling jet collection " << name);

                map<string, Histo1DPtr> &mh1D = jet1DHistos[name];
                map<string, Histo2DPtr> &mh2D = jet2DHistos[name];

                mh1D["_n"]->fill(jets.size(), weight);

                foreach (const Jet &jet, jets) {
                    mh1D["_pt"]->fill(jet.pT(), weight);
                    mh1D["_eta"]->fill(jet.eta(), weight);
                    mh1D["_phi"]->fill(jet.phi(), weight);
                    mh1D["_E"]->fill(jet.E(), weight);
                    mh1D["_m"]->fill(jet.mass(), weight);

                    mh2D["_m_pt"]->fill(jet.pT(), jet.mass(), weight);
                }

                if (jets.size()) {
                    mh1D["0_pt"]->fill(jets[0].pT(), weight);
                    mh1D["0_eta"]->fill(jets[0].eta(), weight);
                    mh1D["0_phi"]->fill(jets[0].phi(), weight);
                    mh1D["0_E"]->fill(jets[0].energy(), weight);
                    mh1D["0_m"]->fill(jets[0].mass(), weight);

                    mh2D["0_m_pt"]->fill(jets[0].pT(), jets[0].mass(), weight);
                }

                if (jets.size() > 1) {
                    mh1D["1_pt"]->fill(jets[1].pT(), weight);
                    mh1D["1_eta"]->fill(jets[1].eta(), weight);
                    mh1D["1_phi"]->fill(jets[1].phi(), weight);
                    mh1D["1_E"]->fill(jets[1].energy(), weight);
                    mh1D["1_m"]->fill(jets[1].mass(), weight);

                    mh2D["1_m_pt"]->fill(jets[1].pT(), jets[1].mass(), weight);


                    mh1D["01_dr"]->fill(Rivet::deltaR(jets[0], jets[1]), weight);
                    mh1D["01_deta"]->fill(Rivet::deltaEta(jets[0], jets[1]), weight);
                    mh1D["01_dphi"]->fill(Rivet::deltaPhi(jets[0], jets[1]), weight);

                    FourMomentum p = jets[0].momentum() + jets[1].momentum();
                    mh1D["01_pt"]->fill(p.pT(), weight);
                    mh1D["01_eta"]->fill(p.eta(), weight);
                    mh1D["01_phi"]->fill(p.phi(), weight);
                    mh1D["01_E"]->fill(p.E(), weight);
                    mh1D["01_m"]->fill(p.mass(), weight);


                    mh2D["0_pt_1_pt"]->fill(jets[0].pT(), jets[1].pT(), weight);
                    mh2D["01_m_01_pt"]->fill(p.pT(), p.mass(), weight);
                    mh2D["01_dr_01_pt"]->fill(p.pT(), Rivet::deltaR(jets[0], jets[1]), weight);
                    mh2D["01_deta_01_pt"]->fill(p.pT(), Rivet::deltaEta(jets[0], jets[1]), weight);
                }

                return;
            }

    };



    // The hook for the plugin system
    DECLARE_RIVET_PLUGIN(MC_BOOSTEDHBB);

}

#endif
