<?php
/* $Id: invoice.php 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib client: invoice example in PHP
 */

$col1 = 55;
$col2 = 100;
$col3 = 330;
$col4 = 430;
$col5 = 530;
$fontsize = 12;
$pagewidth = 595;
$pageheight = 842;
$fontsize = 12;
$infile = "stationery.pdf";
/* This is where font/image/PDF input files live. Adjust as necessary. */
$searchpath = "../data";
$closingtext = 
	"30 days warranty starting at the day of sale. " .
	"This warranty covers defects in workmanship only. " .
	"Kraxi Systems, Inc. will, at its option, repair or replace the " .
	"product under the warranty. This warranty is not transferable. " .
	"No returns or exchanges will be accepted for wet products.";

$data = array(  array("name"=>"Super Kite", 	"price"=>20,	"quantity"=>2),
		array("name"=>"Turbo Flyer", 	"price"=>40, 	"quantity"=>5),
		array("name"=>"Giga Trasch", 	"price"=>180, 	"quantity"=>1),
		array("name"=>"Bare Bone Kit", 	"price"=>50, 	"quantity"=>3),
		array("name"=>"Nitty Gritty", 	"price"=>20, 	"quantity"=>10),
		array("name"=>"Pretty Dark Flyer","price"=>75, 	"quantity"=>1),
		array("name"=>"Free Gift", 	"price"=>0, 	"quantity"=>1)
	    );

$months = array( "January", "February", "March", "April", "May", "June",
	    "July", "August", "September", "October", "November", "December");

$p = PDF_new();

/*  open new PDF file; insert a file name to create the PDF on disk */
if (PDF_open_file($p, "") == 0) {
    die("Error: " . PDF_get_errmsg($p));
}

PDF_set_parameter($p, "SearchPath", $searchpath);

/* This line is required to avoid problems on Japanese systems */
PDF_set_parameter($p, "hypertextencoding", "winansi");

PDF_set_info($p, "Creator", "invoice.php");
PDF_set_info($p, "Author", "Thomas Merz");
PDF_set_info($p, "Title", "PDFlib invoice generation demo (PHP)");

$form = PDF_open_pdi($p, $infile, "", 0);
if ($form == 0){
    die("Error: " . PDF_get_errmsg($p));
}

$page = PDF_open_pdi_page($p, $form, 1, "");
if ($page == 0){
    die("Error: " . PDF_get_errmsg($p));
}


$boldfont = PDF_load_font($p, "Helvetica-Bold", "winansi", "");
$regularfont = PDF_load_font($p, "Helvetica", "winansi", "");
$leading = $fontsize + 2;

/* Establish coordinates with the origin in the upper left corner. */
PDF_set_parameter($p, "topdown", "true");

PDF_begin_page($p, $pagewidth, $pageheight);		/* A4 page */

PDF_fit_pdi_page($p, $page, 0, $pageheight, "");
PDF_close_pdi_page($p, $page);

PDF_setfont($p, $regularfont, $fontsize);

/* print the address */
$y = 170;
PDF_set_value($p, "leading", $leading);

PDF_show_xy($p, "John Q. Doe", $col1, $y);
PDF_continue_text($p, "255 Customer Lane");
PDF_continue_text($p, "Suite B");
PDF_continue_text($p, "12345 User Town");
PDF_continue_text($p, "Everland");

/* print the header and date */

PDF_setfont($p, $boldfont, $fontsize);
$y = 300;
PDF_show_xy($p, "INVOICE", $col1, $y);
$time = localtime();
$buf = sprintf("%s %d, %d", $months[$time[4]], $time[3], $time[5]+1900);
PDF_fit_textline($p, $buf, $col5, $y, "position {100 0}");


/* print the invoice header line */
PDF_setfont($p, $boldfont, $fontsize);

/* "position {0 0}" is left-aligned, "position {100 0}" right-aligned */
$y = 370;
PDF_fit_textline($p, "ITEM",             $col1, $y, "position {0 0}");
PDF_fit_textline($p, "DESCRIPTION",      $col2, $y, "position {0 0}");
PDF_fit_textline($p, "QUANTITY",         $col3, $y, "position {100 0}");
PDF_fit_textline($p, "PRICE",            $col4, $y, "position {100 0}");
PDF_fit_textline($p, "AMOUNT",           $col5, $y, "position {100 0}");

PDF_setfont($p, $regularfont, $fontsize);
$y += 2*$leading;
$total = 0;

for ($i = 0; $i < count($data); $i++){
    PDF_show_xy($p, $i+1, $col1, $y);

    PDF_show_xy($p, $data[$i]{"name"}, $col2, $y);
    PDF_fit_textline($p, $data[$i]{"quantity"}, $col3, $y, "position {100 0}");
    PDF_fit_textline($p, $data[$i]{"price"}, $col4, $y, "position {100 0}");
    $sum = $data[$i]{"price"}*$data[$i]{"quantity"};
    $buf = sprintf("%.2f", $sum);
    PDF_fit_textline($p, $buf, $col5, $y, "position {100 0}");
    $y += $leading;
    $total +=$sum;
}

$y += $leading;
PDF_setfont($p, $boldfont, $fontsize);
PDF_fit_textline($p,sprintf("%.2f",$total), $col5, $y, "position {100 0}");

/* Print the closing text */

$y +=5*$leading;
PDF_setfont($p, $regularfont, $fontsize);
PDF_set_value($p, "leading", $leading);
PDF_show_boxed($p, $closingtext,
    $col1, $y + 4*$leading, $col5-$col1, 4*$leading, "justify", "");

PDF_end_page($p);
PDF_close($p);
PDF_close_pdi($p, $form);

$buf = PDF_get_buffer($p);
$len = strlen($buf);

header("Content-type: application/pdf");
header("Content-Length: $len");
header("Content-Disposition: inline; filename=hello.pdf");
print $buf;

PDF_delete($p);
?>
