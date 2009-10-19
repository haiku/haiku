// $Id: quickreference.cpp 14574 2005-10-29 16:27:43Z bonefish $
//
// PDFlib/PDI client: mini imposition demo
//
#include <iostream>

#include "pdflib.hpp"

int
main(void)
{
    try {
	PDFlib *p;			// pointer to the PDFlib class
	int manual, page;
	int font, row, col;
	const int maxrow = 2;
	const int maxcol = 2;
	char optlist[128];
	int startpage = 1, endpage = 4;
	const float width = 500, height = 770;
	int pageno;
	const string infile = "reference.pdf";
	/* This is where font/image/PDF input files live. Adjust as necessary.*/
	const string searchpath = "../data";

	p = new PDFlib();

	// open new PDF file
	if (p->open_file("quickreference.pdf") == -1) {
	    cerr << "Error: " << p->get_errmsg() << endl;
	    return 2;
	}

	p->set_parameter("SearchPath", searchpath);

	// This line is required to avoid problems on Japanese systems
	p->set_parameter("hypertextencoding", "host");

	p->set_info("Creator", "quickreference.cpp");
	p->set_info("Author", "Thomas Merz");
	p->set_info("Title", "mini imposition demo (C++)");

	manual = p->open_pdi(infile, "", 0);
	if (manual == -1) {
	    cerr << "Error: " << p->get_errmsg() << endl;
	    return 2;
	}

	row = 0;
	col = 0;

	p->set_parameter("topdown", "true");

	for (pageno = startpage; pageno <= endpage; pageno++) {
	    if (row == 0 && col == 0) {
		p->begin_page(width, height);
		font = p->load_font("Helvetica-Bold", "host", "");
		p->setfont(font, 18);
		p->set_text_pos(24, 24);
		p->show("PDFlib Quick Reference");
	    }

	    page = p->open_pdi_page(manual, pageno, "");

	    if (page == -1) {
		cerr << "Error: " << p->get_errmsg() << endl;
		return 2;
	    }

	    sprintf(optlist, "scale %f", (float) 1/maxrow);
	    p->fit_pdi_page(page, width/maxcol*col,
			(row + 1) *  height/maxrow, optlist);
	    p->close_pdi_page(page);

	    col++;
	    if (col == maxcol) {
		col = 0;
		row++;
	    }
	    if (row == maxrow) {
		row = 0;
		p->end_page();
	    }
	}

	// finish the last partial page
	if (row != 0 || col != 0)
	    p->end_page();

	p->close();
	p->close_pdi(manual);

    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in quickreference sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 2;
    }

    return 0;
}
