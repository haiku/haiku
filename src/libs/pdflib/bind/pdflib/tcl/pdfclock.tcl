#!/bin/sh
# $Id: pdfclock.tcl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: pdfclock example in Tcl
#

# Hide the exec to Tcl but not to the shell by appending a backslash\
exec tclsh "$0" ${1+"$@"}

# The lappend line is unnecessary if PDFlib has been installed
# in the Tcl package directory
set auto_path [linsert $auto_path 0 .libs .]

package require pdflib 5.0

set RADIUS 200.0
set MARGIN 20.0

set p [PDF_new]

if {[PDF_open_file $p "pdfclock.pdf"] == -1} {
    puts stderr "Error: [PDF_get_errmsg $p]"
}


PDF_set_info $p "Creator" "pdfclock.tcl"
PDF_set_info $p "Author" "Thomas Merz"
PDF_set_info $p "Title" "PDF clock (Tcl)"

PDF_begin_page $p [expr 2 * ($RADIUS + $MARGIN)] [expr  2 * ($RADIUS + $MARGIN)]

PDF_translate $p [expr $RADIUS + $MARGIN] [expr $RADIUS + $MARGIN]
PDF_setcolor $p "fillstroke" "rgb" 0.0 0.0 1.0 0.0
PDF_save $p

# minute strokes
PDF_setlinewidth $p 2.0
for {set alpha  0} {$alpha < 360} {set alpha [expr $alpha + 6]} {
    PDF_rotate $p 6.0
    PDF_moveto $p $RADIUS 0.0
    PDF_lineto $p [expr $RADIUS-$MARGIN/3] 0.0
    PDF_stroke $p
}

PDF_restore $p
PDF_save $p

# 5 minute strokes
PDF_setlinewidth $p 3.0
for {set alpha  0} {$alpha < 360} {set alpha [expr $alpha + 30]} {
    PDF_rotate $p 30.0
    PDF_moveto $p $RADIUS 0.0
    PDF_lineto $p [expr $RADIUS-$MARGIN] 0.0
    PDF_stroke $p
}

set tm_hour [ clock format [clock seconds] -format "%I" ]
set tm_min  [ clock format [clock seconds] -format "%M" ]
set tm_sec  [ clock format [clock seconds] -format "%S" ]

# This is a Tcl-itis: when the clock returns "08" seconds, tm_sec
# won't be recognized as an integer. Therefore we "cast" it to integer.
# Note that this doesn't happen with, for example, the value "07"
# because this is a valid octal number...

scan $tm_hour %d tm_hour 
scan $tm_min  %d tm_min 
scan $tm_sec  %d tm_sec 

# draw hour hand
PDF_save $p
PDF_rotate $p [expr -(($tm_min/60.0) + $tm_hour - 3.0) * 30.0]
PDF_moveto $p [expr -$RADIUS/10] [expr -$RADIUS/20]
PDF_lineto $p [expr $RADIUS/2] 0.0
PDF_lineto $p [expr -$RADIUS/10] [expr $RADIUS/20]
PDF_closepath $p
PDF_fill $p
PDF_restore $p

# draw minute hand
PDF_save $p
PDF_rotate $p [expr -(($tm_sec/60.0) + $tm_min - 15.0) * 6.0]
PDF_moveto $p [expr -$RADIUS/10] [expr -$RADIUS/20]
PDF_lineto $p [expr $RADIUS * 0.8] 0.0
PDF_lineto $p [expr -$RADIUS/10] [expr $RADIUS/20]
PDF_closepath $p
PDF_fill $p
PDF_restore $p

# draw second hand
PDF_setcolor $p "fillstroke" "rgb" 1.0 0.0 0.0 0.0
PDF_setlinewidth $p 2
PDF_save $p
PDF_rotate $p [expr -(($tm_sec - 15.0) * 6.0)]
PDF_moveto $p [expr -$RADIUS/5] 0.0
PDF_lineto $p $RADIUS 0.0
PDF_stroke $p
PDF_restore $p

# draw little circle at center
PDF_circle $p 0 0 [expr $RADIUS/30]
PDF_fill $p

PDF_restore $p

PDF_end_page $p
PDF_close $p

PDF_delete $p
