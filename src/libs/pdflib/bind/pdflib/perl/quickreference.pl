#!/usr/bin/perl
# $Id: quickreference.pl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib/PDI client: mini imposition demo
#

use pdflib_pl 5.0;

$infile = "reference.pdf";
# This is where font/image/PDF input files live. Adjust as necessary.
$searchpath = "../data";
$maxrow = 2;
$maxcol = 2;
$pagecount = 4;
$width = 500.0;
$height = 770.0;
$startpage = 1;
$endpage = 4;

$p = PDF_new();
eval{
    if (PDF_open_file($p, "quickreference.pdf") == -1) {
	printf("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    PDF_set_parameter($p, "SearchPath", $searchpath);

    # This line is required to avoid problems on Japanese systems
    PDF_set_parameter($p, "hypertextencoding", "winansi");

    PDF_set_info($p, "Creator", "quickreference.pl");
    PDF_set_info($p, "Author", "Thomas Merz");
    PDF_set_info($p, "Title", "mini imposition demo (Perl)");

    $manual = PDF_open_pdi($p, $infile, "", 0);
    if ($manual == -1) {
	printf("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    $row = 0;
    $col = 0;

    PDF_set_parameter($p, "topdown", "true");

    for ($pageno = $startpage; $pageno <= $endpage; $pageno++) {
	if ($row == 0 && $col == 0) {
	    PDF_begin_page($p, $width, $height);
	    $font = PDF_load_font($p, "Helvetica-Bold", "winansi", "");
	    PDF_setfont($p, $font, 18);
	    PDF_set_text_pos($p, 24, 24);
	    PDF_show($p, "PDFlib Quick Reference");
	}

	$page = PDF_open_pdi_page($p, $manual, $pageno, "");

	if ($page == -1) {
	    printf("Error: %s\n", PDF_get_errmsg($p));
	    exit;
	}

	PDF_fit_pdi_page($p, $page, 
	    $width/$maxcol*$col, ($row + 1) * $height/$maxrow, "scale ". 1/$maxrow);
	PDF_close_pdi_page($p, $page);

	$col++;
	if ($col == $maxcol) {
	    $col = 0;
	    $row++;
	}
	if ($row == $maxrow) {
	    $row = 0;
	    PDF_end_page($p);
	}
    }

    # finish the last partial page
    if ($row != 0 || $col != 0) {
	PDF_end_page($p);
    }

    PDF_close($p);
    PDF_close_pdi($p, $manual);
};
if ($@) {
    printf("quickreference: PDFlib Exception occurred:\n");
    printf(" $@\n");
    exit;
}

PDF_delete($p);
