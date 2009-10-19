<?php
/*
 * $Id: chartab.php 14574 2005-10-29 16:27:43Z bonefish $
 * PDFlib client: chartab example in PHP
 */

/* change these as required */
$fontname = "LuciduxSans-Oblique";

/* This is where font/image/PDF input files live. Adjust as necessary. */ 
$searchpath = "../data"; 

/* list of encodings to use */
$encodings = array( "iso8859-1", "iso8859-2", "iso8859-15" );

/* whether or not to embed the font */
$embed = 1;

define("FONTSIZE", 	16);
define("TOP",		700);
define("LEFT", 		50);
define("YINCR", 	2*FONTSIZE);
define("XINCR",		2*FONTSIZE);

/* create a new PDFlib object */
$p = PDF_new();


/* open new PDF file */
if (PDF_open_file($p, "") == 0) {
    die("Error: " . PDF_get_errmsg($p));
}

PDF_set_parameter($p, "openaction", "fitpage");
PDF_set_parameter($p, "fontwarning", "true");
PDF_set_parameter($p, "SearchPath", $searchpath);

/* This line is required to avoid problems on Japanese systems */
PDF_set_parameter($p, "hypertextencoding", "winansi");

PDF_set_info($p, "Creator", "chartab.php");
PDF_set_info($p, "Author", "Thomas Merz");
PDF_set_info($p, "Title", "Character table (PHP)");

/* loop over all encodings */
for ($page = 0; $page < count($encodings); $page++)
{
    PDF_begin_page($p, 595, 842);  /* start a new page */

    /* print the heading and generate the bookmark */
    $font = PDF_load_font($p, "Helvetica", "winansi", "");
    PDF_setfont($p, $font, FONTSIZE);
    $buf = sprintf("%s (%s) %sembedded",
	$fontname, $encodings[$page], $embed ? "" : "not ");

    PDF_show_xy($p, $buf, LEFT - XINCR, TOP + 3 * YINCR);
    PDF_add_bookmark($p, $buf, 0, 0);

    /* print the row and column captions */
    PDF_setfont($p, $font, 2 * FONTSIZE/3);

    for ($row = 0; $row < 16; $row++)
    {
	$buf = sprintf("x%X", $row);
	PDF_show_xy($p, $buf, LEFT + $row*XINCR, TOP + YINCR);

	$buf = sprintf("%Xx", $row);
	PDF_show_xy($p, $buf, LEFT - XINCR, TOP - $row * YINCR);
    }

    /* print the character table */
    $font = PDF_load_font($p, $fontname, $encodings[$page],
	$embed ? "embedding": "");
    PDF_setfont($p, $font, FONTSIZE);

    $y = TOP;
    $x = LEFT;

    for ($row = 0; $row < 16; $row++)
    {
	for ($col = 0; $col < 16; $col++) {
	    $buf = sprintf("%c", 16*$row + $col);
	    PDF_show_xy($p, $buf, $x, $y);
	    $x += XINCR;
	}
	$x = LEFT;
	$y -= YINCR;
    }

    PDF_end_page($p);			/* close page */
}


PDF_close($p);				/* close PDF document	*/

$buf = PDF_get_buffer($p);
$len = strlen($buf);

header("Content-type: application/pdf");
header("Content-Length: $len");
header("Content-Disposition: inline; filename=hello.pdf");
print $buf;

PDF_delete($p);				/* delete the PDFlib object */
?>
