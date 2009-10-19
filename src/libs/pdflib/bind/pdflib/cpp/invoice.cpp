// $Id: invoice.cpp 14574 2005-10-29 16:27:43Z bonefish $
// PDFlib client: invoice example in C++
//
//

#include <iostream>

#include <time.h>

#if !defined(WIN32) && !defined(MAC)
#include <unistd.h>
#endif

#include "pdflib.hpp"

int
main(void)
{
    try {
	int         i, form, page, regularfont, boldfont;
	string      infile = "stationery.pdf";
	// This is where font/image/PDF input files live. Adjust as necessary.
	string      searchpath = "../data";
	const float col1 = 55;
	const float col2 = 100;
	const float col3 = 330;
	const float col4 = 430;
	const float col5 = 530;
	time_t      timer;
	struct tm   ltime;
	float       fontsize = 12, leading, y;
	float       sum, total;
	float       pagewidth = 595, pageheight = 842;
	char        buf[128];
	PDFlib      p;
	string      closingtext =
	    "30 days warranty starting at the day of sale. "
	    "This warranty covers defects in workmanship only. "
	    "Kraxi Systems, Inc. will, at its option, repair or replace the "
	    "product under the warranty. This warranty is not transferable. "
	    "No returns or exchanges will be accepted for wet products.";

	struct articledata {
	    articledata(string n, float pr, int q):
		name(n), price(pr), quantity(q){}
	    string name;
	    float price;
	    int quantity;
	};

	articledata data[] = {
	    articledata("Super Kite",         20,     2),
	    articledata("Turbo Flyer",        40,     5),
	    articledata("Giga Trash",         180,    1),
	    articledata("Bare Bone Kit",      50,     3),
	    articledata("Nitty Gritty",       20,     10),
	    articledata("Pretty Dark Flyer",  75,     1),
	    articledata("Free Gift",          0,      1),
	};

#define ARTICLECOUNT (sizeof(data)/sizeof(data[0]))

	static const string months[] = {
	    "January", "February", "March", "April", "May", "June",
	    "July", "August", "September", "October", "November", "December"
	};


        // open new PDF file
        if (p.open_file("invoice.pdf") == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
            return(2);
        }

	p.set_parameter("SearchPath", searchpath);

	// This line is required to avoid problems on Japanese systems
	p.set_parameter("hypertextencoding", "host");

        p.set_info("Creator", "invoice.cpp");
        p.set_info("Author", "Thomas Merz");
        p.set_info("Title", "PDFlib invoice generation demo (C++)");

        form = p.open_pdi(infile, "", 0);
        if (form == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
            return(2);
        }

        page = p.open_pdi_page(form, 1, "");
        if (page == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
            return(2);
        }

        boldfont = p.load_font("Helvetica-Bold", "host", "");
        regularfont = p.load_font("Helvetica", "host", "");
        leading = fontsize + 2;

        // Establish coordinates with the origin in the upper left corner.
        p.set_parameter("topdown", "true");

        p.begin_page(pagewidth, pageheight);       // A4 page

        p.fit_pdi_page(page, 0, pageheight, "");
        p.close_pdi_page(page);

        p.setfont(regularfont, 12);

        // Print the address
        y = 170;
        p.set_value("leading", leading);

        p.show_xy("John Q. Doe", col1, y);
        p.continue_text("255 Customer Lane");
        p.continue_text("Suite B");
        p.continue_text("12345 User Town");
        p.continue_text("Everland");

        // Print the header and date

        p.setfont(boldfont, 12);
        y = 300;
        p.show_xy("INVOICE",       col1, y);

        time(&timer);
        ltime = *localtime(&timer);
        sprintf(buf, "%s %d, %d", months[ltime.tm_mon].c_str(),
			    ltime.tm_mday, ltime.tm_year + 1900);
        p.fit_textline(buf, col5, y, "position {100 0}");

        // Print the invoice header line
        p.setfont(boldfont, 12);

        // "position {0 0}" is left-aligned, "position {100 0}" right-aligned
        y = 370;
        p.fit_textline("ITEM",             col1, y, "position {0 0}");
        p.fit_textline("DESCRIPTION",      col2, y, "position {0 0}");
        p.fit_textline("QUANTITY",         col3, y, "position {100 0}");
        p.fit_textline("PRICE",            col4, y, "position {100 0}");
        p.fit_textline("AMOUNT",           col5, y, "position {100 0}");

        // Print the article list

        p.setfont(regularfont, 12);
        y += 2*leading;
        total = 0;

        for (i = 0; i < (int)ARTICLECOUNT; i++) {
            sprintf(buf, "%d", i+1);
            p.show_xy(buf, col1, y);

            p.show_xy(data[i].name, col2, y);

            sprintf(buf, "%d", data[i].quantity);
            p.fit_textline(buf, col3, y, "position {100 0}");

            sprintf(buf, "%.2f", data[i].price);
            p.fit_textline(buf, col4, y, "position {100 0}");

            sum = data[i].price * data[i].quantity;
            sprintf(buf, "%.2f", sum);
            p.fit_textline(buf, col5, y, "position {100 0}");

            y += leading;
            total += sum;
        }

        y += leading;
        p.setfont(boldfont, 12);
        sprintf(buf, "%.2f", total);
        p.fit_textline(buf, col5, y, "position {100 0}");

        // Print the closing text

        y += 5*leading;
        p.setfont(regularfont, 12);
        p.set_value("leading", leading);
        p.show_boxed(closingtext,
            col1, y + 4*leading, col5-col1, 4*leading, "justify", "");

        p.end_page();
        p.close();
        p.close_pdi(form);
    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in invoice sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 2;
    }

    return 0;
}
