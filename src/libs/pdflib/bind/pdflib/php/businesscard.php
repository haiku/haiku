<?php
/* $Id: businesscard.php 14574 2005-10-29 16:27:43Z bonefish $
 * PDFlib client: businesscard example in PHP
 *
 */


$infile = "boilerplate.pdf";

/* This is where font/image/PDF input files live. Adjust as necessary.
 *
 * Note that this directory must also contain the LuciduxSans font outline
 * and metrics files.
 */
$searchpath = "../data";

$data = array(  "name"				=> "Victor Kraxi",
		"business.title"		=> "Chief Paper Officer",
		"business.address.line1" 	=> "17, Aviation Road",
		"business.address.city"		=> "Paperfield",
		"business.telephone.voice"	=> "phone +1 234 567-89",
		"business.telephone.fax"	=> "fax +1 234 567-98",
		"business.email"		=> "victor@kraxi.com",
		"business.homepage"		=> "www.kraxi.com"
	);

$p = PDF_new();

/*  open new PDF file; insert a file name to create the PDF on disk */
if (PDF_open_file($p, "") == 0) {
    die("Error: " . PDF_get_errmsg($p));
}

PDF_set_parameter($p, "SearchPath", $searchpath);

/* This line is required to avoid problems on Japanese systems */
PDF_set_parameter($p, "hypertextencoding", "winansi");

PDF_set_info($p, "Creator", "businesscard.php");
PDF_set_info($p, "Author", "Thomas Merz");
PDF_set_info($p, "Title", "PDFlib block processing sample (PHP)");

$blockcontainer = PDF_open_pdi($p, $infile, "", 0);
if ($blockcontainer == 0){
    die ("Error: " . PDF_get_errmsg($p));
}

$page = PDF_open_pdi_page($p, $blockcontainer, 1, "");
if ($page == 0){
    die ("Error: " . PDF_get_errmsg($p));
}

PDF_begin_page($p, 20, 20);		/* dummy page size */

/* This will adjust the page size to the block container's size. */
PDF_fit_pdi_page($p, $page, 0, 0, "adjustpage");

/* Fill all text blocks with dynamic data */
foreach ($data as $key => $value){
    if (PDF_fill_textblock($p, $page, $key, $value,
        "embedding encoding=winansi") == 0) {
	printf("Warning: %s\n ", PDF_get_errmsg($p));
    }
}

PDF_end_page($p);			/* close page */
PDF_close_pdi_page($p, $page);

PDF_close($p);				/* close PDF document */
PDF_close_pdi($p, $blockcontainer);

$buf = PDF_get_buffer($p);
$len = strlen($buf);

header("Content-type: application/pdf");
header("Content-Length: $len");
header("Content-Disposition: inline; filename=businesscard.pdf");
print $buf;

PDF_delete($p);				/* delete the PDFlib object */
?>
