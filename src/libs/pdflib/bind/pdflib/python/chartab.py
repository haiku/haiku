#!/usr/bin/python
# $Id: chartab.py 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: hello character table in Python
#

from sys import *
from pdflib_py import *

# change these as required
fontname = "LuciduxSans-Oblique"

# This is where font/image/PDF input files live. Adjust as necessary.
searchpath = "../data"

# list of encodings to use
encoding = ["iso8859-1", "iso8859-2", "iso8859-15"]

# whether or not to embed the font
embed = ""
# embed = "not "
embedding = "embedding"
# embedding = ""

ENCODINGS       = 3
FONTSIZE        = 16
TOP             = 700
LEFT            = 50
YINCR           = 2*FONTSIZE
XINCR           = 2*FONTSIZE

# create a new PDFlib object
p = PDF_new()

# open new PDF file
if PDF_open_file(p, "chartab.pdf") == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

PDF_set_parameter(p, "openaction", "fitpage")
PDF_set_parameter(p, "fontwarning", "true")

PDF_set_parameter(p, "SearchPath", searchpath)

# This line is required to avoid problems on Japanese systems
PDF_set_parameter(p, "hypertextencoding", "winansi")

PDF_set_info(p, "Creator", "chartab.c")
PDF_set_info(p, "Author", "Thomas Merz")
PDF_set_info(p, "Title", "Character table (C)")

# loop over all encodings
for page in range(0, ENCODINGS, 1):
    PDF_begin_page(p, 595, 842)		# start a new page

    # print the heading and generate the bookmark
    font = PDF_load_font(p, "Helvetica", "winansi", "")
    PDF_setfont(p, font, FONTSIZE)
    buf = fontname + " (" + encoding[page] + ") " + embed + "embedded"

    PDF_show_xy(p, buf, LEFT - XINCR, TOP + 3 * YINCR)
    PDF_add_bookmark(p, buf, 0, 0)

    # print the row and column captions
    PDF_setfont(p, font, 2 * FONTSIZE/3)

    for row in range(0, 16, 1):
	buf = "x" + repr(row)
	PDF_show_xy(p, buf, LEFT + row*XINCR, TOP + YINCR)

	buf = "x" + repr(row)
	PDF_show_xy(p, buf, LEFT - XINCR, TOP - row * YINCR)

    # print the character table
    font = PDF_load_font(p, fontname, encoding[page], embedding)
    PDF_setfont(p, font, FONTSIZE)

    y = TOP
    x = LEFT

    for row in range(0, 16, 1):
	for col in range(0, 16, 1):
	    val = (16*row + col)
	    if val != 0:
		buf = chr(val)
		PDF_show_xy(p, buf, x, y)
	    x += XINCR
	x = LEFT
	y -= YINCR

    PDF_end_page(p)                        # close page

PDF_close(p)                               # close PDF document

PDF_delete(p)                              # delete the PDFlib object
