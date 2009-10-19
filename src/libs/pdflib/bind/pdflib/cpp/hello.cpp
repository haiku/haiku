// $Id: hello.cpp 14574 2005-10-29 16:27:43Z bonefish $
// PDFlib client: hello example in C++
//
//

#include <iostream>

#include "pdflib.hpp"

int
main(void)
{
    try {
	int font;
	PDFlib p;				// the PDFlib object

	// open new PDF file
	if (p.open_file("hello.pdf") == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
	    return 2;
	}

	// This line is required to avoid problems on Japanese systems
	p.set_parameter("hypertextencoding", "host");

	p.set_info("Creator", "hello.cpp");
	p.set_info("Author", "Thomas Merz");
	p.set_info("Title", "Hello, world (C++)!");

	// start a new page
	p.begin_page((float) a4_width, (float) a4_height);

	// Change "host" encoding to "winansi" or whatever you need!
	font = p.load_font("Helvetica-Bold", "host", "");

	p.setfont(font, 24);
	p.set_text_pos(50, 700);
	p.show("Hello, world!");
	p.continue_text("(says C++)");
	p.end_page();				// close page
	p.close();				// close PDF document
    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in hello sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 2;
    }

    return 0;
}
