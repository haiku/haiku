#!/bin/sh
# $Id: image.tcl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: image example in Tcl
#

# Hide the exec to Tcl but not to the shell by appending a backslash\
exec tclsh "$0" ${1+"$@"}

# The lappend line is unnecessary if PDFlib has been installed
# in the Tcl package directory
set auto_path [linsert $auto_path 0 .libs .]

package require pdflib 5.0

set p [PDF_new]
set imagefile "nesrin.jpg"

# This is where font/image/PDF input files live. Adjust as necessary.
set searchpath "../data"

if {[PDF_open_file $p "image.pdf"] == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

PDF_set_parameter $p "SearchPath" $searchpath

PDF_set_info $p "Creator" "image.tcl"
PDF_set_info $p "Author" "Thomas Merz"
PDF_set_info $p "Title" "image sample (Tcl)"

set image [PDF_load_image $p "auto" $imagefile ""]

if {$image == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

# We generate a page with arbitrary dimensions
PDF_begin_page $p 10 10
PDF_fit_image $p $image 0 0 "adjustpage"
PDF_close_image $p $image
PDF_end_page $p

PDF_close $p

PDF_delete $p
