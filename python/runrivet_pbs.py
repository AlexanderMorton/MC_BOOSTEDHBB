#!/usr/bin/env python

from subprocess import Popen
from sys import argv, stdout
from pbssubmit import pbssubmit
from time import sleep

def runrivet_pbs(fhepmcnames):
    running = []
    for fhepmc in fhepmcnames:
        fyoda = fhepmc.replace("hepmc", "yoda")
        flog = fhepmc.replace("hepmc", "rivet.log")

        cmd = "rivet --pwd -a MC_BOOSTEDHBB -H %s %s" % \
                (fyoda, fhepmc)

        running.append(pbssubmit("rivet.%s" % fhepmc,  cmd,
            outfile=flog))

        sleep(1)
        continue

    for p in running:
        p.wait()
        print p.stdout.read()
        print p.stderr.read()
    
    return


def main(args):
    if len(argv) < 2:
        print "no hepmc output specified"
        exit()

    runrivet_pbs(args[1:])

    return 0


if __name__ == "__main__":
    main(argv)
