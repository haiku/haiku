<?php
# $Id: hello.php 14574 2005-10-29 16:27:43Z bonefish $

/* create a new PDFlib object */
$p = PDF_new();

/*  open new PDF file; insert a file name to create the PDF on disk */
if (PDF_open_file($p, "") == 0) {
    die("Error: " . PDF_get_errmsg($p));
}

/* This line is required to avoid problems on Japanese systems */
PDF_set_parameter($p, "hypertextencoding", "winansi");

PDF_set_info($p, "Creator", "hello.php");
PDF_set_info($p, "Author", "Rainer Schaaf");
PDF_set_info($p, "Title", "Hello world (PHP)!");

PDF_begin_page($p, 595, 842); 		/* start a new page     */

$font = PDF_load_font($p, "Helvetica-Bold", "winansi", "");

PDF_setfont($p, $font, 24.0);
PDF_set_text_pos($p, 50, 700);
PDF_show($p, "Hello world!");
PDF_continue_text($p, "(says PHP)");
PDF_end_page($p);			/* close page		*/

PDF_close($p);				/* close PDF document	*/

$buf = PDF_get_buffer($p);
$len = strlen($buf);

header("Content-type: application/pdf");
header("Content-Length: $len");
header("Content-Disposition: inline; filename=hello.pdf");
print $buf;

PDF_delete($p);				/* delete the PDFlib object */
?>
