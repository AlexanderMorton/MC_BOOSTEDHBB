// -*- C++ -*-
#include "MC_BOOSTEDHBB.hh"

#include <iostream>
#include <map>
#include <string>

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


// TODO
// global variables. ick
const string ptlab = "$p_T$ / GeV";
const string mlab = "mass / GeV";
const string drlab = "$\\Delta R";
const string etalab = "$\\eta";
const string philab = "$\\phi";

namespace Rivet {

/// @name Analysis methods
//@{

/// Book histograms and initialise projections before the run
void MC_BOOSTEDHBB::init() {
    getLog().setLevel(Log::DEBUG);

    ChargedLeptons clfs(FinalState(-2.5, 2.5, 25*GeV));
    addProjection(clfs, "ChargedLeptons");

    // TODO
    // minimum pt cutoff?
    MissingMomentum mmfs(FinalState(-4.2, 4.2, 0*GeV));
    addProjection(mmfs, "MissingMomentum");

    // calo jets constituents
    // TODO
    // don't include high-pt neutrinos or leptons in jets
    // include electrons?
    IdentifiedFinalState nufs(FinalState(-2.5, 2.5, 25*GeV));
    nufs.acceptNeutrinos();

    MergedFinalState leptonsAndNeutrinos(clfs, nufs);
    addProjection(leptonsAndNeutrinos, "LeptonsAndNeutrinos");

    VetoedFinalState caloParts(FinalState(-4.2, 4.2));
    caloParts.addVetoOnThisFinalState(leptonsAndNeutrinos);

    // "track" jets constituents
    VetoedFinalState trackParts(ChargedFinalState(-2.5, 2.5, 0.5*GeV));
    trackParts.addVetoOnThisFinalState(leptonsAndNeutrinos);


    // register 0.4 calo jets
    string s = "AntiKt04CaloJets";
    jetColls.push_back(s);
    bookFourMomColl(s);
    minJetPtCut[s] = 25*GeV;
    addProjection(FastJets(caloParts, FastJets::ANTIKT, 0.4), s);

    // and corresponding b-tagged jets
    bookFourMomColl("AntiKt04CaloJetsB");
    bookFourMomPair("LeadingAntiKt04CaloJetsB");


    // register 1.0 calo jets
    s = "AntiKt10CaloJets";
    jetColls.push_back(s);
    bookFourMomColl(s);
    minJetPtCut[s] = 250*GeV;
    addProjection(FastJets(caloParts, FastJets::ANTIKT, 1.0), s);

    // and corresponding b-tagged jets
    bookFourMomColl("AntiKt10CaloJetsB");

    // register 0.3 track jets
    s = "AntiKt03TrackJets";
    jetColls.push_back(s);
    bookFourMomColl(s);
    minJetPtCut[s] = 25*GeV;
    addProjection(FastJets(trackParts, FastJets::ANTIKT, 0.3), s);

    // and corresponding b-tagged jets
    bookFourMomColl("AntiKt03TrackJetsB");
    bookFourMomPair("LeadingAntiKt03TrackJetsB");


    // register leptons and met
    bookFourMomColl("Leptons");
    bookFourMomPair("Dilepton");
    bookFourMom("MissingMomentum");


    // register special collections
    bookFourMom("Higgs");
    bookFourMomPair("HiggsTrackJets");

    bookFourMomPair("MinDeltaRLeptonTrackJetB");
    bookFourMomPair("MinMassLeptonTrackJetB");

    return;
}


/// Perform the per-event analysis
void MC_BOOSTEDHBB::analyze(const Event& event) {
    const double weight = event.weight();

    // leptons
    // TODO
    // isolation?
    const Particles &leptons =
        applyProjection<ChargedLeptons>(event, "ChargedLeptons").particles();

    if (!leptons.size())
        vetoEvent;

    const Particle& mm =
        Particle(0, -applyProjection<MissingMomentum>(event, "MissingMomentum").visibleMomentum());

    // TODO
    // mass window cuts?
    if (leptons.size() >= 2) {
        const FourMomentum &zmom = leptons[0].momentum() + leptons[1].momentum();
        if (abs(zmom.mass() - 90*GeV) > 15*GeV)
            vetoEvent;
    }

    fillFourMomColl("Leptons", leptons, weight);
    if (leptons.size() >= 2)
        fillFourMomPair("Dilepton", leptons[0], leptons[1], weight);

    fillFourMom("MissingMomentum", mm, weight);

    foreach (const string &name, jetColls) {
        const FastJets &fj =
            applyProjection<FastJets>(event, name);
        const Jets &jets = fj.jetsByPt(minJetPtCut[name]);
        fillFourMomColl(name, jets, weight);

        Jets bjets;
        foreach (const Jet& jet, jets)
            if (jet.bTags().size()) bjets.push_back(jet);

        fillFourMomColl(name + "B", bjets, weight);
    }


    // search for higgs: highest-pt akt10 jet matched to two b-tagged track jets
    Jets antiKt10CaloJets =
        applyProjection<FastJets>(event, "AntiKt10CaloJets").jetsByPt(250*GeV);
    Jets antiKt03TrackJets =
        applyProjection<FastJets>(event, "AntiKt03TrackJets").jetsByPt(25*GeV);

    Jet higgs;
    Jets matchedTrackJets;
    foreach (const Jet &calojet, antiKt10CaloJets) {
        matchedTrackJets.clear();

        foreach (const Jet &trackjet, antiKt03TrackJets) {
            // is it near the calojet?
            if (Rivet::deltaR(calojet, trackjet) > 1.0)
                continue;

            // is it b-tagged?
            if (!trackjet.bTags().size())
                continue;

            matchedTrackJets.push_back(trackjet);
        }

        if (matchedTrackJets.size() >= 2) {
            higgs = calojet;
            break;
        }
    }

    if (higgs.pT()) {
        fillFourMom("Higgs", higgs, weight);
        fillFourMomPair("HiggsTrackJets", matchedTrackJets[0], matchedTrackJets[1], weight);
    }

    double drMin = -1, massMin = -1;
    Jet minLepDrJet;
    Jet minLepMassJet;
    foreach(Jet &trackjet, antiKt03TrackJets) {
        if (!trackjet.bTags().size())
            continue;

        if (drMin < 0) {
            drMin = Rivet::deltaR(trackjet, leptons[0]);
            massMin = (trackjet.momentum() + leptons[0].momentum()).mass();
            continue;
        }

        double dr = Rivet::deltaR(trackjet, leptons[0]);
        double mass = (trackjet.momentum() + leptons[0].momentum()).mass();

        minLepDrJet = dr < drMin ? trackjet : minLepDrJet;
        minLepMassJet = mass < massMin ? trackjet : minLepMassJet;
    }

    if (minLepDrJet.pT()) {
        fillFourMomPair("MinDeltaRLeptonTrackJetB", leptons[0], minLepDrJet, weight);
        fillFourMomPair("MinMassLeptonTrackJetB", leptons[0], minLepMassJet, weight);
    }

    return;
}


/// Normalise histograms etc., after the run
void MC_BOOSTEDHBB::finalize() {

    double norm = crossSection()/sumOfWeights();
    for (map<string, map<string, Histo1DPtr> >::iterator p = histos1D.begin(); p != histos1D.end(); ++p) {
        for (map<string, Histo1DPtr>::iterator q = p->second.begin(); q != p->second.end(); ++q) {
            q->second->scaleW(norm); // norm to cross section
        }
    }

    for (map<string, map<string, Histo2DPtr> >::iterator p = histos2D.begin(); p != histos2D.end(); ++p) {
        for (map<string, Histo2DPtr>::iterator q = p->second.begin(); q != p->second.end(); ++q) {
            q->second->scaleW(norm); // norm to cross section
        }
    }

    minLeptonTrackJetB_m->scaleW(norm);
    minLeptonTrackJetB_dr->scaleW(norm);

    return;
}


Histo1DPtr MC_BOOSTEDHBB::bookHisto(const string& name, const string& title,
        const string& xlabel, int nxbins, double xmin, double xmax) {

    double xbinwidth = (xmax - xmin)/nxbins;

    char buff[100];
    sprintf(buff, "events / %.2f", xbinwidth);
    string ylabel = buff;

    return bookHisto1D(name, nxbins, xmin, xmax, xlabel, ylabel, title);
}


Histo2DPtr MC_BOOSTEDHBB::bookHisto(const string& name, const string& title,
        const string& xlabel, int nxbins, double xmin, double xmax,
        const string& ylabel, int nybins, double ymin, double ymax) {

    double xbinwidth = (xmax - xmin)/nxbins;
    double ybinwidth = (ymax - ymin)/nybins;

    char buff[100];
    sprintf(buff, "events / %.2f / %.2f", xbinwidth, ybinwidth);
    string zlabel = buff;

    return bookHisto2D(name, nxbins, xmin, xmax, nybins, ymin, ymax, xlabel, ylabel, zlabel, title);
}


void MC_BOOSTEDHBB::bookFourMom(const string &name) {
    MSG_DEBUG("Booking " << name << " histograms.");

    histos1D[name]["pt"] = bookHisto(name + "_pt", name, ptlab, 50, 0, 2000*GeV);
    histos1D[name]["eta"] = bookHisto(name + "_eta", name, "$\\eta$", 50, -5, 5);
    histos1D[name]["m"] = bookHisto(name + "_m", name, mlab, 50, 0, 500*GeV);

    histos2D[name]["m_pt"] = bookHisto(name + "_m_pt", name,
            ptlab, 50, 0, 2000*GeV,
            mlab, 50, 0, 500*GeV);

    return;
}


void MC_BOOSTEDHBB::bookFourMomPair(const string &name) {
    // pairs of particles also are "particles"
    bookFourMom(name);

    // extra histograms for pairs of particles
    histos1D[name]["dr"] = bookHisto(name + "_dr", drlab, "", 50, 0, 5);

    histos2D[name]["dr_pt"] = bookHisto(name + "_dr_pt", name,
            ptlab, 50, 0, 2000*GeV,
            drlab, 50, 0, 5);

    histos2D[name]["pt_pt"] = bookHisto(name + "_pt_pt", name,
            ptlab, 50, 0, 2000*GeV,
            ptlab, 50, 0, 2000*GeV);

    return;
}


void MC_BOOSTEDHBB::bookFourMomColl(const string &name) {
    bookFourMom(name);

    // histos1D[name] = map<string, Histo1DPtr>();
    histos1D[name]["n"] = bookHisto(name + "_n", "multiplicity", "", 10, 0, 10);

    return;
}


template <class T>
void MC_BOOSTEDHBB::fillFourMom(const string &name, const T &part, double weight) {
    MSG_DEBUG("Filling " << name << " histograms");

    histos1D[name]["pt"]->fill(part.pT(), weight);
    histos1D[name]["eta"]->fill(part.eta(), weight);
    histos1D[name]["m"]->fill(part.mass(), weight);
    histos2D[name]["m_pt"]->fill(part.pT(), part.mass(), weight);

    return;
}


template <class T, class S>
void MC_BOOSTEDHBB::fillFourMomPair(const string &name,const T &p1, const S &p2, double weight) {
    fillFourMom(name, p1.momentum() + p2.momentum(), weight);

    double dr = Rivet::deltaR(p1.momentum(), p2.momentum());
    double pt = (p1.momentum(), p2.momentum()).pT();

    histos1D[name]["dr"]->fill(dr, weight);
    histos2D[name]["dr_pt"]->fill(pt, dr);
    histos2D[name]["pt_pt"]->fill(p1.momentum().pT(), p2.momentum().pT());

    return;
}


template <class T>
void MC_BOOSTEDHBB::fillFourMomColl(const string &name, const vector<T> &parts, double weight) {
    histos1D[name]["n"]->fill(parts.size(), weight);

    foreach (const T &part, parts)
        fillFourMom(name, part, weight);

    return;
}

//@}

} // Rivet
