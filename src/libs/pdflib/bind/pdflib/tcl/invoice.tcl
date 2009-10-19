#!/bin/sh
# $Id: invoice.tcl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: invoice generation demo
#

# Hide the exec to Tcl but not to the shell by appending a backslash\
exec tclsh "$0" ${1+"$@"}

# The lappend line is unnecessary if PDFlib has been installed
# in the Tcl package directory
set auto_path [linsert $auto_path 0 .libs .]

package require pdflib 5.0

set infile "stationery.pdf"
set searchpath  "../data"
set col1 55.0
set col2 100.0
set col3 330.0
set col4 430.0
set col5 530.0
set fontsize 12.0
set pagewidth 595.0
set pageheight 842.0

set closingtext  \
"30 days warranty starting at the day of sale. \
This warranty covers defects in workmanship only. \
Kraxi Systems, Inc. will, at its option, repair or replace the \
product under the warranty. This warranty is not transferable. \
No returns or exchanges will be accepted for wet products."

global articleName articlePrice articleQuantity

proc article_add {id name price quantity} {
    global articleName articlePrice articleQuantity
    set articleName($id) $name
    set articlePrice($id) $price
    set articleQuantity($id) $quantity
}

article_add 0 "Super Kite"		20	2
article_add 1 "Turbo Flyer"		40	5
article_add 2 "Giga Trash"		180	1
article_add 3 "Bare Bone Kit"		50	3
article_add 4 "Nitty Gritty"		20	10
article_add 5 "Pretty Dark Flyer"	75	1
article_add 6 "Free Gift"		0	1

set ARTICLECOUNT 7

# create a new PDFlib object
set p [PDF_new]

# open new PDF file
if {[PDF_open_file $p "invoice.pdf"] == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

PDF_set_parameter $p "SearchPath" $searchpath

PDF_set_info $p "Creator" "invoice.c"
PDF_set_info $p "Author" "Thomas Merz"
PDF_set_info $p "Title" "PDFlib invoice generation demo (TCL)"

set form [PDF_open_pdi $p $infile "" 0]
if {$form == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

set page [PDF_open_pdi_page $p $form 1 ""]
if {$page == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

set boldfont [PDF_load_font $p "Helvetica-Bold" "winansi" ""]
set regularfont [PDF_load_font $p "Helvetica" "winansi" ""]
set leading [expr $fontsize + 2]

# Establish coordinates with the origin in the upper left corner.
PDF_set_parameter $p "topdown" "true"

# A4 page
PDF_begin_page $p $pagewidth $pageheight

PDF_fit_pdi_page $p $page 0 $pageheight ""
PDF_close_pdi_page $p $page

PDF_setfont $p $regularfont $fontsize

# Print the address
set y 170
PDF_set_value $p "leading" $leading

PDF_show_xy $p "John Q. Doe" $col1 $y
PDF_continue_text $p "255 Customer Lane"
PDF_continue_text $p "Suite B"
PDF_continue_text $p "12345 User Town"
PDF_continue_text $p "Everland"

# Print the header and date

PDF_setfont $p $boldfont $fontsize
set y 300
PDF_show_xy $p "INVOICE" $col1 $y

set buf [clock format [clock seconds] -format "%B %d, %Y"]
PDF_fit_textline $p $buf $col5 $y "position {100 0}"

# Print the invoice header line
PDF_setfont $p $boldfont $fontsize

# "position {0 0}" is left-aligned, "position {100 0}" right-aligned
set y 370
PDF_fit_textline $p "ITEM"		$col1 $y "position {0 0}"
PDF_fit_textline $p "DESCRIPTION"	$col2 $y "position {0 0}"
PDF_fit_textline $p "QUANTITY"		$col3 $y "position {100 0}"
PDF_fit_textline $p "PRICE"		$col4 $y "position {100 0}"
PDF_fit_textline $p "AMOUNT"		$col5 $y "position {100 0}"

# Print the article list

PDF_setfont  $p $regularfont $fontsize
set y [expr $y + 2*$leading]
set total 0

for {set i 0} {$i < $ARTICLECOUNT} {set i [expr $i + 1]} {
    PDF_show_xy $p [expr $i + 1] $col1 $y

    PDF_show_xy $p $articleName($i) $col2 $y

    PDF_fit_textline $p $articleName($i) $col3 $y "position {100 0}"

    set buf [format "%.2f" $articlePrice($i)]
    PDF_fit_textline $p $buf $col4 $y "position {100 0}"

    set sum [ expr $articlePrice($i) * $articleQuantity($i)]
    set buf [format "%.2f" $sum]
    PDF_fit_textline $p $buf $col5 $y "position {100 0}"

    set y [expr $y + $leading]
    set total [expr $total + $sum]
}

set y [expr $y + $leading]
PDF_setfont $p $boldfont $fontsize
set buf [format "%.2f" $total]
PDF_fit_textline $p $buf $col5 $y "position {100 0}"

# Print the closing text

set y [expr $y + 5*$leading]
PDF_setfont $p $regularfont $fontsize
PDF_set_value $p "leading" $leading
PDF_show_boxed $p $closingtext \
    $col1 [expr $y + 4*$leading] [expr $col5-$col1] [expr 4*$leading]\
    "justify" ""

PDF_end_page $p
PDF_close $p
PDF_close_pdi $p $form

PDF_delete $p
