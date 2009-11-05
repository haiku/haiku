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


# make a flat list of files recursively from the arg list
def makeFileSet(args):
    files = []
    extensions = [".cpp", ".h"]
    for arg in args:
        if os.path.isfile(arg) and os.path.splitext(arg)[1] in extensions:
            files.append(arg)
        elif os.path.isdir(arg):
            for content in os.listdir(arg):
                path = os.path.join(arg, content)
                if os.path.isfile(path) \
                    and os.path.splitext(path)[1] in extensions:
                    files.append(path)
                elif os.path.isdir(path) and os.path.basename(path) != ".svn":
                    files.extend(makeFileSet([path]))
    return files


cppRules = {}
cppRules["Line over 80 char"] = re.compile('[^\n]{81,}')
cppRules["Spaces instead of tabs"] = re.compile('   ')
cppRules["Missing space after for/if/select/while"] \
    = re.compile('(for|if|select|while)\(')
cppRules["Missing space at comment start"] = re.compile('//[a-zA-Z0-9]')
cppRules["Missing space after operator"] \
    = re.compile('[a-zA-Z0-9](==|[,=>/+\-*;\|])[a-zA-Z0-9]')
cppRules["Operator at line end"] = re.compile('([*=/+\-\|\&\?]|\&&|\|\|)(?=\n)')
cppRules["Missing space"] = re.compile('\){')
cppRules["Mixed tabs/spaces"] = re.compile('( \t]|\t )+')
cppRules["Malformed else"] = re.compile('}[ \t]*\n[ \t]*else')
cppRules["Lines between functions > 2"] \
    = re.compile('(?<=\n})([ \t]*\n){3,}(?=\n)')
cppRules["Lines between functions < 2"] \
    = re.compile('(?<=\n})([ \t]*\n){0,2}(?=.)')
cppRules["Windows Line Ending"] = re.compile('\r')

# TODO: ignore some rules in comments
#cppRules["-Comment 1"] = re.compile('[^/]/\*(.|[\r\n])*?\*/')
#cppRules["-Comment 2"] = re.compile('(//)[^\n]*')


if len(sys.argv) >= 2 and sys.argv[1] != "--help":
    run(makeFileSet(sys.argv[1:]), cppRules, "styleviolations.html")
else:
    print "Usage: python checkstyle.py file.cpp [file2.cpp] [directory]\n"
    print "Checks c++ source files against the Haiku Coding Guidelines."
    print "Outputs an html report in the styleviolations.html file.\n"
