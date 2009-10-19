#!/bin/sh
# $Id: hello.tcl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: hello example in Tcl
#

# Hide the exec to Tcl but not to the shell by appending a backslash\
exec tclsh "$0" ${1+"$@"}

# The lappend line is unnecessary if PDFlib has been installed
# in the Tcl package directory
set auto_path [linsert $auto_path 0 .libs .]

package require pdflib 5.0

# create a new PDFlib object
set p [PDF_new]

# open new PDF file
if {[PDF_open_file $p "hello.pdf"] == -1} {
    puts stderr "Error:  [PDF_get_errmsg $p]"
    exit
}

PDF_set_info $p "Creator" "hello.tcl"
PDF_set_info $p "Author" "Thomas Merz"
PDF_set_info $p "Title" "Hello world (Tcl)"

# start a new page
PDF_begin_page $p 595 842

set font [PDF_load_font $p "Helvetica-Bold" "winansi" ""]

PDF_setfont $p $font 24.0
PDF_set_text_pos $p 50 700
PDF_show $p "Hello world!"
PDF_continue_text $p "(says Tcl)"
# close page
PDF_end_page $p

# close PDF document
PDF_close $p

# delete the PDFlib object
PDF_delete $p
