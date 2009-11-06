#
# Copyright 2009, Alexandre Deckner, alex@zappotek.com
# Distributed under the terms of the MIT License.
#
import re, sys, os
from utils import *


def processMatches(matches, name, text, highlights):
    for match in matches:
        printMatch(name, match, text)
        highlights.append((match.start(), match.end(), name))


def run(fileSet, rules, outputFileName):
    openHtml(fileSet, outputFileName)

    for fileName in fileSet:
        print "\nChecking " + fileName + ":"
        file = open(fileName, 'r')
        text = file.read()

        highlights = []

        for name, regexp in rules.items():
            processMatches(regexp.finditer(text), name, text, highlights)

        highlights.sort()
        highlights = checkHighlights(highlights)

        file.close()

        renderHtml(text, highlights, fileName, outputFileName)

    closeHtml(outputFileName)


def visit(result, dir, names):
    extensions = [".cpp", ".h"]
    names.remove(".svn")
    for name in names:
        path = os.path.join(dir, name)
        if os.path.isfile(path) and os.path.splitext(name)[1] in extensions:
            print "adding", path
            result.append(path)


cppRules = {}
cppRules["Line over 80 char"] = re.compile('[^\n]{81,}')
cppRules["Spaces instead of tabs"] = re.compile('   ')
cppRules["Missing space after control statement"] \
    = re.compile('(for|if|while|switch)\(')
cppRules["Missing space at comment start"] = re.compile('//\w')
cppRules["Missing space after operator"] \
    = re.compile('\w(==|[,=>/+\-*;\|])\w')
cppRules["Operator at line end"] = re.compile('([*=/+\-\|\&\?]|\&&|\|\|)(?=\n)')
cppRules["Missing space"] = re.compile('\){')
cppRules["Mixed tabs/spaces"] = re.compile('( \t]|\t )+')
cppRules["Malformed else"] = re.compile('}[ \t]*\n[ \t]*else')
cppRules["Lines between functions > 2"] \
    = re.compile('(?<=\n})([ \t]*\n){3,}(?=\n)')
cppRules["Lines between functions < 2"] \
    = re.compile('(?<=\n})([ \t]*\n){0,2}(?=.)')
cppRules["Windows Line Ending"] = re.compile('\r')
cppRules["Bad pointer/reference style"] \
    = re.compile('(?<=\w) [*&](?=(\w|[,\)]))')

# TODO: ignore some rules in comments
#cppRules["-Comment 1"] = re.compile('[^/]/\*(.|[\r\n])*?\*/')
#cppRules["-Comment 2"] = re.compile('(//)[^\n]*')


if len(sys.argv) >= 2 and sys.argv[1] != "--help":
    files = []
    for arg in sys.argv[1:]:
        if os.path.isfile(arg):
            files.append(arg)
        else:
            os.path.walk(arg, visit, files)
    run(files, cppRules, "styleviolations.html")
else:
    print "Usage: python checkstyle.py file.cpp [file2.cpp] [directory]\n"
    print "Checks c++ source files against the Haiku Coding Guidelines."
    print "Outputs an html report in the styleviolations.html file.\n"
