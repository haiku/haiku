// $Id: pdfclock.cpp 14574 2005-10-29 16:27:43Z bonefish $
// A little PDFlib application to draw an analog clock.
//
//

#include <iostream>

#include <time.h>

#if !defined(WIN32) && !defined(MAC)
#include <unistd.h>
#endif

#include "pdflib.hpp"

#define RADIUS		200.0f
#define MARGIN		20.0f

int
main()
{
    try {
	PDFlib		*p;
	float		alpha;
	time_t		timer;
	struct tm	ltime;
	
	// Create a new PDFlib object
	p = new PDFlib();

	// Open new PDF file
	if (p->open_file("pdfclock.pdf") == -1) {
	    cerr << "Error: " << p->get_errmsg() << endl;
	    return 2;
	}

	// This line is required to avoid problems on Japanese systems
	p->set_parameter("hypertextencoding", "host");

	p->set_info("Creator", "pdfclock.cpp");
	p->set_info("Author", "Thomas Merz");
	p->set_info("Title", "PDF clock (C++)");

	p->begin_page((unsigned int) (2 * (RADIUS + MARGIN)),
			  (unsigned int) (2 * (RADIUS + MARGIN)));
	
	p->translate(RADIUS + MARGIN, RADIUS + MARGIN);
	p->setcolor("fillstroke", "rgb", 0, 0, 1, 0);
	p->save();

	// minute strokes
	p->setlinewidth(2);
	for (alpha = 0; alpha < 360; alpha += 6)
	{
	    p->rotate(6);
	    p->moveto(RADIUS, 0);
	    p->lineto((float) (RADIUS-MARGIN/3), 0);
	    p->stroke();
	}

	p->restore();
	p->save();

	// 5 minute strokes
	p->setlinewidth(3);
	for (alpha = 0; alpha < 360; alpha += 30)
	{
	    p->rotate(30);
	    p->moveto(RADIUS, 0);
	    p->lineto(RADIUS-MARGIN, 0);
	    p->stroke();
	}

	time(&timer);
	ltime = *localtime(&timer);

	// draw hour hand
	p->save();
	p->rotate(
		(float)(-((ltime.tm_min/60) + ltime.tm_hour - 3.0) * 30));
	p->moveto(-RADIUS/10, -RADIUS/20);
	p->lineto(RADIUS/2, 0);
	p->lineto(-RADIUS/10, RADIUS/20);
	p->closepath();
	p->fill();
	p->restore();

	// draw minute hand
	p->save();
	p->rotate((float) (-((ltime.tm_sec/60.0) + ltime.tm_min - 15.0) * 6.0));
	p->moveto(-RADIUS/10, -RADIUS/20);
	p->lineto(RADIUS * 0.8, 0);
	p->lineto(-RADIUS/10, RADIUS/20);
	p->closepath();
	p->fill();
	p->restore();

	// draw second hand
	p->setcolor("fillstroke", "rgb", 1, 0, 0, 0);
	p->setlinewidth(2);
	p->save();
	p->rotate((float) -((ltime.tm_sec - 15) * 6));
	p->moveto(-RADIUS/5, 0);
	p->lineto(RADIUS, 0);
	p->stroke();
	p->restore();

	// draw little circle at center
	p->circle(0, 0, (float) RADIUS/30);
	p->fill();

	p->restore();

	p->end_page();

	p->close();
    }
    catch (PDFlib::Exception &ex) {
	cerr << "PDFlib exception occurred in pdfclock sample: " << endl;
	cerr << "[" << ex.get_errnum() << "] " << ex.get_apiname()
	    << ": " << ex.get_errmsg() << endl;
	return 2;
    }

    return 0;
}
