// $Id: chartab.cpp 14574 2005-10-29 16:27:43Z bonefish $
// PDFlib client: chartab example in C++
//
//

#include <iostream>

#include "pdflib.hpp"

int
main(void)
{
    /* change these as required */
    const char *fontname = "LuciduxSans-Oblique";

    /* This is where font/image/PDF input files live. Adjust as necessary. */
    char *searchpath = "../data";

    /* list of encodings to use */
    const char *encodings[] = { "iso8859-1", "iso8859-2", "iso8859-15" };

    /* whether or not to embed the font */
    int embed = 1;


    char buf[256];
    float x, y;
    int row, col, font, page;

    static const int ENCODINGS = 3;
    static const float FONTSIZE	= 16;
    static const float TOP		= 700;
    static const float LEFT		= 50;
    static const float YINCR	= 2*FONTSIZE;
    static const float XINCR	= 2*FONTSIZE;

    try {
	/* create a new PDFlib object */
	PDFlib p;				// the PDFlib object

	/* open new PDF file */
	if (p.open_file("chartab.pdf") == -1) {
	    cerr << "Error: " << p.get_errmsg() << endl;
	    return(2);
	}

        p.set_parameter("openaction", "fitpage");
        p.set_parameter("fontwarning", "true");

	p.set_parameter("SearchPath", searchpath);

	// This line is required to avoid problems on Japanese systems
	p.set_parameter("hypertextencoding", "host");

	p.set_info("Creator", "chartab.c");
	p.set_info("Author", "Thomas Merz");
	p.set_info("Title", "Character table (C++)");

	/* loop over all encodings */
	for (page = 0; page < ENCODINGS; page++)
	{
	    p.begin_page(a4_width, a4_height);  /* start a new page */

	    /* print the heading and generate the bookmark */
	    // Change "host" encoding to "winansi" or whatever you need!
	    font = p.load_font("Helvetica", "host", "");
	    p.setfont(font, FONTSIZE);
	    sprintf(buf, "%s (%s) %sembedded",
		fontname, encodings[page], embed ? "" : "not ");

	    p.show_xy(buf, LEFT - XINCR, TOP + 3 * YINCR);
	    p.add_bookmark(buf, 0, 0);

	    /* print the row and column captions */
	    p.setfont(font, 2 * FONTSIZE/3);

	    for (row = 0; row < 16; row++)
	    {
		sprintf(buf, "x%X", row);
		p.show_xy(buf, LEFT + row*XINCR, TOP + YINCR);

		sprintf(buf, "%Xx", row);
		p.show_xy(buf, LEFT - XINCR, TOP - row * YINCR);
	    }

	    /* print the character table */
	    font = p.load_font(fontname, encodings[page],
		embed ? "embedding": "");
	    p.setfont(font, FONTSIZE);

	    y = TOP;
	    x = LEFT;

	    for (row = 0; row < 16; row++)
	    {
		for (col = 0; col < 16; col++) {
		    sprintf(buf, "%c", 16*row + col);
		    p.show_xy(buf, x, y);
		    x += XINCR;
		}
		x = LEFT;
		y -= YINCR;
	    }

	    p.end_page();			/* close page */
	}

	p.close();				/* close PDF document	*/


    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in chartab sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 2;
    }

    return 0;
}
