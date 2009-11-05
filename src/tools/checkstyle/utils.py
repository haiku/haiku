#
# Copyright 2009, Alexandre Deckner, alex@zappotek.com
# Distributed under the terms of the MIT License.
#
from cgi import escape


# prints match to stdout
def printMatch(name, match, source):
    start = match.start()
    end = match.end()
    startLine = source.count('\n', 0, start)
    startColumn = start - source.rfind('\n', 0, start)
    print name + " (line " + str(startLine + 1) + ", " + str(startColumn) \
        + "): '" + match.group().replace('\n','\\n') + "'"


def openHtml(fileList, outputFileName):
    file = open(outputFileName, 'w')
    file.write("""
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
    <head>
        <title>Style violations</title>
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
        <style type="text/css">""" + cssStyle() + """</style>
    </head>
    <body>
        <p><b>File list:</b><br>""")
    for fileName in fileList:
        file.write(fileName + "<br>")
    file.write("</p>")
    file.close()


def closeHtml(outputFileName):
    file = open(outputFileName, 'a')
    file.write("""
        </pre>
    </body>
    </html>""")

    file.close()


# render in html
def renderHtml(text, highlights, sourceFileName, outputFileName):
    splittedText = highlightSplit(text, highlights)

    file = open(outputFileName, 'a')
    file.write("<hr/><p><b>" + sourceFileName + "</b></p>")

    # insert highlight tags in a temp buffer
    temp = ""
    count = 0
    for slice in splittedText:
        if count % 2 == 0:
            temp += escape(slice) + '<span class="highlight tooltip">'
        else:
            temp += escape(slice) + "<em>" + highlights[(count - 1) / 2][2] \
                + "</em></span>"
        count += 1

    temp += "</span>" # close the superfluous last highlight

    file.write('<table><tr><td><pre class="code"><span class="linenumber">')
    count = 1
    for line in temp.split('\n'):
        file.write(str(count).rjust(4)+"<br>")
        count += 1

    file.write('</span></pre></td><td><pre class="code">')

    for line in temp.split('\n'):
        file.write('<span class="linehead"> </span>' + line.replace('\r', ' ') \
             + '<br>')

    file.write("</pre></td></tr></table>")

    file.close()


# highlight overlap check
def highlightOverlaps(highlight1, highlight2):
    #print "hl1", highlight1, "hl2", highlight2
    return not(highlight2[0] > highlight1[1] or highlight1[0] > highlight2[1])


# splits the string in three parts before, between and after the highlight
def splitByHighlight(string, highlight):
    return (string[:highlight[0]], string[highlight[0]:highlight[1]], \
        string[highlight[1]:])


# splits the source text on highlights boundaries so that we can escape to html
# without the need to recalculate the highlights positions
def highlightSplit(string, highlights):
    splittedString = []
    text = string
    offset = 0
    lastEnd = 0
    for (start, end, name) in highlights:
         if start >= lastEnd:
            (before, between, after) = splitByHighlight( \
                text, (start - offset, end - offset))
            splittedString.append(before)
            splittedString.append(between)
            text = after
            lastEnd = end
            offset += len(before + between)
         else:
            print "overlap ", (start, end, name)
    splittedString.append(text)
    return splittedString


# checkHighlights() checks for highlights overlaps
def checkHighlights(highlights):
    highlights.sort()

    index = 0
    lastHighlight = (-2, -1, '')

    # merge overlapping highlights
    for highlight in highlights:
        if highlightOverlaps(highlight, lastHighlight):

            newStart = min(lastHighlight[0], highlight[0])
            newEnd = max(lastHighlight[1], highlight[1])
            newComment = lastHighlight[2]

            if (newComment.find(highlight[2]) == -1):
                newComment += " + " + highlight[2]

            highlight = (newStart, newEnd, newComment)
            highlights[index] = highlight

            # mark highlight to be deleted
            highlights[index - 1] = (0, 0, "")

        lastHighlight = highlight
        index += 1

    # remove "to be deleted" highlights
    return [ (start, end, comment) for (start, end, comment) in highlights \
        if (start, end, comment) != (0, 0, "") ]


def cssStyle():
    return """
    .highlight {
        background: #ffff00;
        color: #000000;
    }

    .linehead {
        background: #ddd;
        font-size: 1px;
    }

    .highlight .linehead {
        background: #ffff00;;
        color: #999;
        text-align: right;
        font-size: 8px;
    }

    .linenumber {
        background: #eee;
        color: #999;
        text-align: right;
    }

    td {
        border-spacing: 0px;
        border-width: 0px;
        padding: 0px
    }

    div.code pre {
        font-family: monospace;
    }

    .tooltip em {
        display:none;
    }

    .tooltip:hover {
        border: 0;
        position: relative;
        z-index: 500;
        text-decoration:none;
    }

    .tooltip:hover em {
        font-style: normal;
        display: block;
        position: absolute;
        top: 20px;
        left: -10px;
        padding: 5px;
        color: #000;
        border: 1px solid #bbb;
        background: #ffc;
        width: auto;
    }"""
