#!/usr/bin/python
# $Id: hello.py 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: hello example in Python
#

from sys import *
from pdflib_py import *

# create a new PDFlib object
p = PDF_new()

# open new PDF file
if PDF_open_file(p, "hello.pdf") == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

# This line is required to avoid problems on Japanese systems
PDF_set_parameter(p, "hypertextencoding", "winansi")

PDF_set_info(p, "Author", "Thomas Merz")
PDF_set_info(p, "Creator", "hello.py")
PDF_set_info(p, "Title", "Hello world (Python)")

PDF_begin_page(p, 595, 842)		# start a new page

font = PDF_load_font(p, "Helvetica-Bold", "winansi", "")

PDF_setfont(p, font, 24)
PDF_set_text_pos(p, 50, 700)
PDF_show(p, "Hello world!")
PDF_continue_text(p, "(says Python)")
PDF_end_page(p)				# close page

PDF_close(p)				# close PDF document

PDF_delete(p)				# delete the PDFlib object
