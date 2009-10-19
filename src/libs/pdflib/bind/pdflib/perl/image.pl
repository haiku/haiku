#!/usr/bin/perl
# $Id: image.pl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: image example in Perl
#

use pdflib_pl 5.0;

# This is where font/image/PDF input files live. Adjust as necessary.
$searchpath = "../data";
$imagefile = "nesrin.jpg";

$p = PDF_new();

eval {
    if (PDF_open_file($p, "image.pdf") == -1){
	printf("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    PDF_set_parameter($p, "SearchPath", $searchpath);

    # This line is required to avoid problems on Japanese systems
    PDF_set_parameter($p, "hypertextencoding", "winansi");

    PDF_set_info($p, "Creator", "image.pl");
    PDF_set_info($p, "Author", "Thomas Merz");
    PDF_set_info($p, "Title", "image sample (Perl)");


    $image = PDF_load_image($p, "auto", $imagefile, "");
    die "Couldn't open image '$imagefile'" if ($image == -1);

    # dummy page size, will be adjusted by PDF_fit_image()
    PDF_begin_page($p, 10, 10);
    PDF_fit_image($p, $image, 0, 0, "adjustpage");
    PDF_close_image($p, $image);
    PDF_end_page($p);		# close page

    PDF_close($p);			# close PDF document
};
if ($@) {
    printf("image: PDFlib Exception occurred:\n");
    printf(" $@\n");
    exit;
}

PDF_delete($p); 		# delete the PDFlib object
