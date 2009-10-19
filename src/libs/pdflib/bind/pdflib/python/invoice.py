#!/usr/bin/python
# $Id: invoice.py 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: invoice generation demo
#

from sys import *
import time
import fpformat
from pdflib_py import *

infile = "stationery.pdf"

# This is where font/image/PDF input files live. Adjust as necessary.
searchpath = "../data"

col1 = 55
col2 = 100
col3 = 330
col4 = 430
col5 = 530
fontsize = 12
pagewidth = 595
pageheight = 842
closingtext = \
    "30 days warranty starting at the day of sale. " +\
    "This warranty covers defects in workmanship only. " +\
    "Kraxi Systems, Inc. will, at its option, repair or replace the " +\
    "product under the warranty. This warranty is not transferable. " +\
    "No returns or exchanges will be accepted for wet products."

data_name = [
 "Super Kite",
 "Turbo Flyer",
 "Giga Trash",
 "Bare Bone Kit",
 "Nitty Gritty",
 "Pretty Dark Flyer",
 "Free Gift" ]
data_price = [ 20, 40, 180, 50, 20, 75, 0]
data_quantity = [ 2, 5, 1, 3, 10, 1, 1]


ARTICLECOUNT = 6

p = PDF_new()

PDF_set_parameter(p, "tracefile", "invoice.trace")
PDF_set_parameter(p, "debug", "t")
# open new PDF file
if PDF_open_file(p, "invoice.pdf") == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

PDF_set_parameter(p, "SearchPath", searchpath)

# This line is required to avoid problems on Japanese systems
PDF_set_parameter(p, "hypertextencoding", "winansi")

PDF_set_info(p, "Creator", "invoice.c")
PDF_set_info(p, "Author", "Thomas Merz")
PDF_set_info(p, "Title", "PDFlib invoice generation demo (C)")

form = PDF_open_pdi(p, infile, "", 0)
if form == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

page = PDF_open_pdi_page(p, form, 1, "")
if page == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

boldfont = PDF_load_font(p, "Helvetica-Bold", "winansi", "")
regularfont = PDF_load_font(p, "Helvetica", "winansi", "")
leading = fontsize + 2

# Establish coordinates with the origin in the upper left corner.
PDF_set_parameter(p, "topdown", "true")

PDF_begin_page(p, pagewidth, pageheight)	# A4 page

PDF_fit_pdi_page(p, page, 0, pageheight, "")
PDF_close_pdi_page(p, page)

PDF_setfont(p, regularfont, fontsize)

# Print the address
y = 170
PDF_set_value(p, "leading", leading)

PDF_show_xy(p, "John Q. Doe", col1, y)
PDF_continue_text(p, "255 Customer Lane")
PDF_continue_text(p, "Suite B")
PDF_continue_text(p, "12345 User Town")
PDF_continue_text(p, "Everland")

# Print the header and date

PDF_setfont(p, boldfont, fontsize)
y = 300
PDF_show_xy(p, "INVOICE",	col1, y)

buf = time.strftime("%x", time.gmtime(time.time()))
PDF_fit_textline(p, buf, col5, y, "position {100 0}")

# Print the invoice header line
PDF_setfont(p, boldfont, fontsize)

# "position {0 0}" is left-aligned, "position {100 0}" right-aligned
y = 370
PDF_fit_textline(p, "ITEM",		col1, y, "position {0 0}")
PDF_fit_textline(p, "DESCRIPTION",	col2, y, "position {0 0}")
PDF_fit_textline(p, "QUANTITY",		col3, y, "position {100 0}")
PDF_fit_textline(p, "PRICE",		col4, y, "position {100 0}")
PDF_fit_textline(p, "AMOUNT",		col5, y, "position {100 0}")

# Print the article list

PDF_setfont(p, regularfont, fontsize)
y += 2*leading
total = 0

for i in range(0, ARTICLECOUNT, 1):
    buf = repr(i)
    i +=1
    PDF_show_xy(p, buf, col1, y)

    PDF_show_xy(p, data_name[i], col2, y)

    buf = repr(data_quantity[i])
    PDF_fit_textline(p, buf, col3, y, "position {100 0}")

    buf = fpformat.fix(data_price[i], 2)
    PDF_fit_textline(p, buf, col4, y, "position {100 0}")

    sum = data_price[i] * data_quantity[i]
    buf =  fpformat.fix(sum, 2)
    PDF_fit_textline(p, buf, col5, y, "position {100 0}")

    y += leading
    total += sum

y += leading
PDF_setfont(p, boldfont, fontsize)
buf =  fpformat.fix(total, 2)
PDF_fit_textline(p, buf, col5, y, "position {100 0}")

# Print the closing text

y += 5*leading
PDF_setfont(p, regularfont, fontsize)
PDF_set_value(p, "leading", leading)
PDF_show_boxed(p, closingtext,
    col1, y + 4*leading, col5-col1, 4*leading, "justify", "")

PDF_end_page(p)
PDF_close(p)
PDF_close_pdi(p, form)

PDF_delete(p)
