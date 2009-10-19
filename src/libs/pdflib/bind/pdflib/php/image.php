<?php
# $Id: image.php 14574 2005-10-29 16:27:43Z bonefish $

/* This is where font/image/PDF input files live. Adjust as necessary. */ 
$searchpath = "../data"; 

$p = PDF_new();				/* create a new PDFlib object */

PDF_set_parameter($p, "SearchPath", $searchpath);

/*  open new PDF file; insert a file name to create the PDF on disk */
if (PDF_open_file($p, "") == 0) {
    die("Error: " . PDF_get_errmsg($p));
}

/* This line is required to avoid problems on Japanese systems */
PDF_set_parameter($p, "hypertextencoding", "winansi");

PDF_set_info($p, "Creator", "image.php");
PDF_set_info($p, "Author", "Rainer Schaaf");
PDF_set_info($p, "Title", "image sample (PHP)");

$imagefile = "nesrin.jpg";

$image = PDF_load_image($p, "auto", $imagefile, "");
if (!$image) {
    die("Error: " . PDF_get_errmsg($p));
}

/* dummy page size, will be adjusted by PDF_fit_image() */
PDF_begin_page($p, 10, 10);
PDF_fit_image($p, $image, 0, 0, "adjustpage");
PDF_close_image($p, $image);
PDF_end_page($p);			/* close page           */

PDF_close($p);				/* close PDF document   */

$buf = PDF_get_buffer($p);
$len = strlen($buf);

header("Content-type: application/pdf");
header("Content-Length: $len");
header("Content-Disposition: inline; filename=image.pdf");
print $buf;

PDF_delete($p);				/* delete the PDFlib object */
?>
