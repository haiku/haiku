// $Id: image.cpp 14574 2005-10-29 16:27:43Z bonefish $
// PDFlib client: image example in C++
//
//

#include <iostream>

#include "pdflib.hpp"

int
main(void)
{
    try {
	PDFlib *p;			// pointer to the PDFlib class
	int image;
	char *imagefile = (char *) "nesrin.jpg";
	// This is where font/image/PDF input files live. Adjust as necessary. 
	char *searchpath = (char *) "../data";

	p = new PDFlib();

	// Open new PDF file
	if (p->open_file("image.pdf") == -1) {
	    cerr << "Error: " << p->get_errmsg() << endl;
	    return 2;
	}

	p->set_parameter("SearchPath", searchpath);

	// This line is required to avoid problems on Japanese systems
	p->set_parameter("hypertextencoding", "host");

	p->set_info("Creator", "image.cpp");
	p->set_info("Author", "Thomas Merz");
	p->set_info("Title", "image sample (C++)!");

	image = p->load_image("auto", imagefile, "");

	if (image == -1) {
	    cerr << "Error: " << p->get_errmsg() << endl;
	    exit(3);
	}

	// dummy page size, will be adjusted by PDF_fit_image()
	p->begin_page(10, 10);
	p->fit_image(image, (float) 0.0,(float) 0.0, "adjustpage");
	p->close_image(image);
	p->end_page();				// close page

	p->close();				// close PDF document
    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in hello sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 2;
    }

    return 0;
}
