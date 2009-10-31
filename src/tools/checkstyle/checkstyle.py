#
# Copyright 2009, Alexandre Deckner, alex@zappotek.com
# Distributed under the terms of the MIT License.
#
import re, sys
from utils import *


def processMatches(matches, name, text, highlights):
    for match in matches:
        printMatch(name, match, text)
        highlights.append((match.start(), match.end(), name))


def run(sourceFile, rules):
    file = open(sourceFile, 'r')
    text = file.read()

    highlights = []

    for name, regexp in rules.items():
        processMatches(regexp.finditer(text), name, text, highlights)

    highlights.sort()
    highlights = checkHighlights(highlights)

    file.close()
    renderHtml(text, highlights, sourceFile, "styleviolations.html")


cppRules = {}
cppRules["Line over 80 char"] = re.compile('[^\n]{81,}')
cppRules["Spaces instead of tabs"] = re.compile('   ')
cppRules["Missing space after for/if/select/while"] = re.compile('(for|if|select|while)\(')
cppRules["Missing space at comment start"] = re.compile('//[a-zA-Z0-9]')
cppRules["Missing space after operator"] \
    = re.compile('[a-zA-Z0-9](==|[,=>/+\-*;\|])[a-zA-Z0-9]')
cppRules["Operator at line end"] = re.compile('([*=/+\-\|\&\?]|\&&|\|\|)\n')
cppRules["Missing space"] = re.compile('\){')
cppRules["Mixed tabs/spaces"] = re.compile('( \t]|\t )+')
cppRules["Malformed else"] = re.compile('}[ \t]*\n[ \t]*else')
cppRules["Lines between functions > 2"] = re.compile('\n}([ \t]*\n){4,}')
cppRules["Lines between functions < 2"] = re.compile('\n}([ \t]*\n){0,2}.')

# TODO: ignore some rules in comments
#cppRules["-Comment 1"] = re.compile('[^/]/\*(.|[\r\n])*?\*/')
#cppRules["-Comment 2"] = re.compile('(//)[^\n]*')


if len(sys.argv) == 2 and sys.argv[1] != "--help":
    run(sys.argv[1], cppRules)
else:
	print "Usage: python checkstyle.py file.cpp\n"
	print "Checks a c++ source file against the Haiku Coding Guidelines."
	print "Outputs an highlighted html report in the styleviolations.html file.\n"
