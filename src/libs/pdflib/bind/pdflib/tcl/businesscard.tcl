#!/bin/sh
# $Id: businesscard.tcl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: block processing example in tcl
#

# Hide the exec to Tcl but not to the shell by appending a backslash\
exec tclsh "$0" ${1+"$@"}

# The lappend line is unnecessary if PDFlib has been installed
# in the Tcl package directory
set auto_path [linsert $auto_path 0 .libs .]

package require pdflib 5.0

set infile "boilerplate.pdf"

global blockName blockValue

proc block_add {id name value} {
    global blockName blockValue
    set blockName($id) $name
    set blockValue($id) $value
}

block_add 0 "name"                    "Victor Kraxi"
block_add 1 "business.title"          "Chief Paper Officer"
block_add 2 "business.address.line1"  "17, Aviation Road"
block_add 3 "business.address.city"   "Paperfield"
block_add 4 "business.telephone.voice" "phone +1 234 567-89"
block_add 5 "business.telephone.fax"  "fax +1 234 567-98"
block_add 6 "business.email"          "victor@kraxi.com"
block_add 7 "business.homepage"       "www.kraxi.com"

set BLOCKCOUNT 8

set p [PDF_new]

# This is where font/image/PDF input files live. Adjust as necessary.
#
# Note that this directory must also contain the LuciduxSans font outline
# and metrics files.
#
PDF_set_parameter $p "SearchPath" "../data"

if {[PDF_open_file $p "businesscard.pdf"] == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

PDF_set_info $p "Creator" "businesscard.tcl"
PDF_set_info $p "Author" "Thomas Merz"
PDF_set_info $p "Title" "PDFlib block processing sample (Tcl)"

set blockcontainer [PDF_open_pdi $p $infile "" 0]
if {$blockcontainer == -1} {
    puts stderr "Error: % [PDF_get_errmsg $p]"
    exit
}

set page [PDF_open_pdi_page $p $blockcontainer 1 ""]
if {$page == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

# dummy page size
PDF_begin_page $p 20 20

# This will adjust the page size to the block container's size.
PDF_fit_pdi_page $p $page 0 0 "adjustpage"

# Fill all text blocks with dynamic data
for { set i 0} {$i < $BLOCKCOUNT} {set i [expr $i + 1]} {
    if {[PDF_fill_textblock $p $page $blockName($i) $blockValue($i) \
        "embedding encoding=winansi"] == -1} {
	puts stderr "Warning: [PDF_get_errmsg $p]"
    }
}

# close page
PDF_end_page $p
PDF_close_pdi_page $p $page

# close PDF document
PDF_close $p
PDF_close_pdi $p $blockcontainer

# delete the PDFlib object
PDF_delete $p
