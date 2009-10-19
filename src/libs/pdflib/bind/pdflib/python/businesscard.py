#!/usr/bin/python
# $Id: businesscard.py 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: block processing example in Python
#

from sys import *
from pdflib_py import *

infile = "boilerplate.pdf"

# This is where font/image/PDF input files live. Adjust as necessary.
#
# Note that this directory must also contain the LuciduxSans font outline
# and metrics files.
#
searchpath = "../data"

data_name = [
 "name",
 "business.title",
 "business.address.line1",
 "business.address.city",
 "business.telephone.voice",
 "business.telephone.fax",
 "business.email",
 "business.homepage" ]

data_value = [
 "Victor Kraxi",
 "Chief Paper Officer",
 "17, Aviation Road",
 "Paperfield",
 "phone +1 234 567-89",
 "fax +1 234 567-98",
 "victor@kraxi.com",
 "www.kraxi.com" ]

BLOCKCOUNT = 8

p = PDF_new()

if PDF_open_file(p, "businesscard.pdf") == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

# Set the search path for fonts and PDF files
PDF_set_parameter(p, "SearchPath", searchpath)

# This line is required to avoid problems on Japanese systems
PDF_set_parameter(p, "hypertextencoding", "winansi")

PDF_set_info(p, "Creator", "businesscard.c")
PDF_set_info(p, "Author", "Thomas Merz")
PDF_set_info(p, "Title","PDFlib block processing sample (C)")

blockcontainer = PDF_open_pdi(p, infile, "", 0)
if blockcontainer == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

page = PDF_open_pdi_page(p, blockcontainer, 1, "")
if page == -1:
    print "Error: " + PDF_get_errmsg(p) + "\n"
    exit(2)

PDF_begin_page(p, 20, 20)		# dummy page size

# This will adjust the page size to the block container's size.
PDF_fit_pdi_page(p, page, 0, 0, "adjustpage")

# Fill all text blocks with dynamic data
for i in range(0, BLOCKCOUNT, 1):
    if PDF_fill_textblock(p, page, data_name[i], data_value[i], \
		"embedding encoding=winansi") == -1:
	print "Warning: " + PDF_get_errmsg(p) + "\n"

PDF_end_page(p)			# close page		*/
PDF_close_pdi_page(p, page)

PDF_close(p)				# close PDF document	*/
PDF_close_pdi(p, blockcontainer)

PDF_delete(p)				# delete the PDFlib object
