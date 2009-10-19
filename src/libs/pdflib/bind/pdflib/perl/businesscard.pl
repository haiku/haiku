#!/usr/bin/perl
# $Id: businesscard.pl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: block processing example in C
#

use pdflib_pl 5.0;

$infile = "boilerplate.pdf";
# This is where font/image/PDF input files live. Adjust as necessary.
#
# Note that this directory must also contain the LuciduxSans font outline
# and metrics files.
#
$searchpath = "../data";

%data = (   "name"			=> "Victor Kraxi",
	    "business.title"		=> "Chief Paper Officer",
	    "business.address.line1" 	=> "17, Aviation Road",
	    "business.address.city"	=> "Paperfield",
	    "business.telephone.voice"	=> "phone +1 234 567-89",
	    "business.telephone.fax"	=> "fax +1 234 567-98",
	    "business.email"		=> "victor\@kraxi.com",
	    "business.homepage"		=> "www.kraxi.com"
	);

# create a new PDFlib object
$p = PDF_new();

eval {
    # open new PDF file
    if (PDF_open_file($p, "businesscard.pdf") == -1){
	printf ("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    PDF_set_parameter($p, "SearchPath", $searchpath);

    # This line is required to avoid problems on Japanese systems
    PDF_set_parameter($p, "hypertextencoding", "winansi");

    PDF_set_info($p, "Creator", "businesscard.pl");
    PDF_set_info($p, "Author", "Thomas Merz");
    PDF_set_info($p, "Title", "PDFlib block processing sample (Perl)");

    $blockcontainer = PDF_open_pdi($p, $infile, "", 0);
    if ($blockcontainer == -1){
	printf ("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    $page = PDF_open_pdi_page($p, $blockcontainer, 1, "");
    if ($page == -1){
	printf ("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    PDF_begin_page($p, 20, 20);		# dummy page size

    # This will adjust the page size to the block container's size.
    PDF_fit_pdi_page($p, $page, 0, 0, "adjustpage");

    # Fill all text blocks with dynamic data
    foreach $elem(keys %data){
	if (PDF_fill_textblock($p, $page, $elem, $data{$elem},
            "embedding encoding=winansi") == -1) {
	    printf ("Warning: %s\n", PDF_get_errmsg($p));
	}
    }

    PDF_end_page($p);			# close page
    PDF_close_pdi_page($p, $page);

    PDF_close($p);			# close PDF document
    PDF_close_pdi($p, $blockcontainer);
};
if ($@) {
    printf("businesscard: PDFlib Exception occurred:\n");
    printf(" $@\n");
    exit;
}
PDF_delete($p); 			# delete the PDFlib object
