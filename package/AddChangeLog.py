#!/usr/bin/python
#

#print 2+2

import sys, os

fInName = sys.argv[1] + '/015_changelog.html'
fOutName = sys.argv[1] + '/015_changelog_chg.html'

fIn = open(fInName, 'r')
fOut = open(fOutName, 'w')
log = open('changelog.txt', 'r')

#txt = fin.readlines()

for line in fIn:
    #print line,
    if "CHG_LOG_REPL</pre>\n" == line:
    #if "abc\n" == line:
        for logLine in log:
            fOut.write(logLine)
        fOut.write("</pre>\n")
    else:
        fOut.write(line)

os.rename(fOutName, fInName)

#CHG_LOG_REPL
