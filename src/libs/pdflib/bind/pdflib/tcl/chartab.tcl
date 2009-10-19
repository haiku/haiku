#!/bin/sh
# $Id: chartab.tcl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: character table example in Tcl
#

# Hide the exec to Tcl but not to the shell by appending a backslash\
exec tclsh "$0" ${1+"$@"}

# The lappend line is unnecessary if PDFlib has been installed
# in the Tcl package directory
set auto_path [linsert $auto_path 0 .libs .]

package require pdflib 5.0

# Adjust as you need.
set fontname   "LuciduxSans-Oblique"
set searchpath "../data"
set encnames   "iso8859-1 iso8859-2 iso8859-15"
set embedflag  "embedding"
set embednote  "embedded"
set fontsize 16
set top 700
set left 50
set xincr [expr {2 * $fontsize}]
set yincr [expr {2 * $fontsize}]

# create a new PDFlib object
set p [PDF_new]

# open new PDF file
if {[PDF_open_file $p "chartab.pdf"] == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
    exit
}

if { [catch {

    PDF_set_info $p "Creator" "chartab.tcl"
    PDF_set_info $p "Author" "Thomas Merz"
    PDF_set_info $p "Title" "Character table (Tcl)"

    PDF_set_parameter $p "openaction" "fitpage"
    PDF_set_parameter $p "fontwarning" "true"
    PDF_set_parameter $p "SearchPath" $searchpath

    foreach encoding $encnames {

        # start a new page
        PDF_begin_page $p 595 842

        set font [PDF_load_font $p "Helvetica" "winansi" ""]
        PDF_setfont $p $font $fontsize

        # title and bookmark
        set text "$fontname ($encoding) $embednote"
        PDF_show_xy $p $text [expr {$left - $xincr}] [expr {$top + 3 * $yincr}]
        PDF_add_bookmark $p $text 0 0

        # print the row and column captions
        PDF_setfont $p $font [expr {2 * $fontsize / 3}]

        # character code labels
        for {set row 0} {$row < 16} {incr row} {

            set text [format "x%X" $row]
            PDF_show_xy $p $text \
                [expr {$left + $row * $xincr}] [expr {$top + $yincr}]

            set text [format "%Xx" $row]
            PDF_show_xy $p $text \
                [expr {$left - $xincr}] [expr {$top - $row * $yincr}]
        }

        # print the character table
        set font [PDF_load_font $p $fontname $encoding $embedflag]
        PDF_setfont $p $font $fontsize

        set x $left
        set y $top
        for {set row 0} {$row < 16} {incr row} {
            for {set col 0} {$col < 16} {incr col} {
                set text [format "%c" [expr {int(16*$row + $col)}]]
                PDF_show_xy $p $text $x $y
                incr x $xincr
            }
            set x $left
            incr y -$yincr
        }

        PDF_end_page $p
    }

    # close PDF document
    PDF_close $p

} result] } {
    puts stderr "$result"
}

# delete the PDFlib object
PDF_delete $p
