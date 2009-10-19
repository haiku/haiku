#!/usr/bin/perl
# $Id: hello.pl 14574 2005-10-29 16:27:43Z bonefish $
#
# PDFlib client: hello example in Perl
#

use pdflib_pl 5.0;

# create a new PDFlib object
$p = PDF_new();

eval {
    # open new PDF file
    if (PDF_open_file($p, "hello.pdf") == -1) {
	printf("Error: %s\n", PDF_get_errmsg($p));
	exit;
    }

    # This line is required to avoid problems on Japanese systems
    PDF_set_parameter($p, "hypertextencoding", "winansi");

    PDF_set_info($p, "Creator", "hello.pl");
    PDF_set_info($p, "Author", "Thomas Merz");
    PDF_set_info($p, "Title", "Hello world (Perl)!");

    PDF_begin_page($p, 595, 842);		# start a new page

    $font = PDF_load_font($p, "Helvetica-Bold", "winansi", "");

    PDF_setfont($p, $font, 24.0);
    PDF_set_text_pos($p, 50, 700);
    PDF_show($p, "Hello world!");
    PDF_continue_text($p, "(says Perl)");
    PDF_end_page($p);				# close page

    PDF_close($p);				# close PDF document
};

if ($@) {
    printf("hello: PDFlib Exception occurred:\n");
    printf(" $@\n");
    exit;
}

PDF_delete($p);					# delete the PDFlib object
