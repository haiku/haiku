// $Id: businesscard.cpp 14574 2005-10-29 16:27:43Z bonefish $
// PDFlib client: hello example in C++
//
//

#include <iostream>

#include "pdflib.hpp"

int
main(void)
{
    try {
	PDFlib p;				// the PDFlib object
	int         i, blockcontainer, page;
	const string infile = "boilerplate.pdf";
        /* This is where font/image/PDF input files live. Adjust as necessary.
         *
         * Note that this directory must also contain the LuciduxSans font
         * outline and metrics files.
         */
	const string searchpath = "../data";
	struct blockdata {
	    blockdata(string n, string v): name(n), value(v){}
	    string name;
	    string value;
	};

	blockdata data[] = {
	   blockdata("name",                   "Victor Kraxi"),
	   blockdata("business.title",         "Chief Paper Officer"),
	   blockdata("business.address.line1", "17, Aviation Road"),
	   blockdata("business.address.city",  "Paperfield"),
	   blockdata("business.telephone.voice","phone +1 234 567-89"),
	   blockdata("business.telephone.fax", "fax +1 234 567-98"),
	   blockdata("business.email",         "victor@kraxi.com"),
	   blockdata("business.homepage",      "www.kraxi.com"),
	};

#define BLOCKCOUNT (sizeof(data)/sizeof(data[0]))

	// open new PDF file 
        if (p.open_file("businesscard.pdf") == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
            return(2);
        }

	p.set_parameter("SearchPath", searchpath);

	// This line is required to avoid problems on Japanese systems
	p.set_parameter("hypertextencoding", "host");

        p.set_info("Creator", "businesscard.cpp");
        p.set_info("Author", "Thomas Merz");
        p.set_info("Title","PDFlib block processing sample (C++)");

        blockcontainer = p.open_pdi(infile, "", 0);
        if (blockcontainer == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
            return(2);
        }

        page = p.open_pdi_page(blockcontainer, 1, "");
        if (page == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
            return(2);
        }

        p.begin_page(20, 20);              // dummy page size

        // This will adjust the page size to the block container's size.
        p.fit_pdi_page(page, 0, 0, "adjustpage");

        // Fill all text blocks with dynamic data 
        for (i = 0; i < (int) BLOCKCOUNT; i++) {
            if (p.fill_textblock(page, data[i].name, data[i].value,
		"embedding encoding=host") == -1) {
		cerr << "Error: " << p.get_errmsg() << endl;
            }
        }

        p.end_page();                        // close page
	p.close_pdi_page(page);

        p.close();                           // close PDF document
	p.close_pdi(blockcontainer);
    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in businesscard sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 99;
    }

    return 0;
}
