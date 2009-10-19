<%@page import="java.io.*, javax.servlet.*, com.pdflib.pdflib" %><%

   /* $Id: chartab.jsp 14574 2005-10-29 16:27:43Z bonefish $
    *
    * chartab.jsp
    */

    /* change these as required */
    String fontname = "LuciduxSans-Oblique";

    /* This is where font/image/PDF input files live. Adjust as necessary. */
    String searchpath = "../data";

    /* list of encodings to use */
    String encodings[] = { "iso8859-1", "iso8859-2", "iso8859-15" };
    int  ENCODINGS = 3;
    float FONTSIZE= 16;
    float TOP	= 700;
    float LEFT	= 50;
    float YINCR	= 2*FONTSIZE;
    float XINCR	= 2*FONTSIZE;

    /* whether or not to embed the font */
    int embed = 1;

    String buf;
    float x, y;
    int row, col, font, codepage;

    pdflib p = null ;
    byte[] outbuf;
    ServletOutputStream output;

    p = new pdflib();

    // Generate a PDF in memory; insert a file name to create PDF on disk
    if (p.open_file("") == -1) {
        System.err.println("Error: " + p.get_errmsg());
	System.exit(1);
    }

    p.set_parameter("openaction", "fitpage");
    p.set_parameter("fontwarning", "true");

    p.set_parameter("SearchPath", searchpath);

    p.set_info("Creator", "chartab.jsp");
    p.set_info("Author", "Thomas Merz");
    p.set_info("Title", "Character table (Java/JSP)");

    /* loop over all encodings */
    for (codepage = 0; codepage < ENCODINGS; codepage++)
    {
	p.begin_page(595, 842);  /* start a new page */

	/* print the heading and generate the bookmark */
	font = p.load_font("Helvetica", "winansi", "");
	p.setfont(font, FONTSIZE);
	if (embed == 1) {
	    buf = fontname + " (" + encodings[codepage] + ") embedded";
	} else{
	    buf = fontname + " (" + encodings[codepage] + ") not  embedded";
	}

	p.show_xy(buf, LEFT - XINCR, TOP + 3 * YINCR);
	p.add_bookmark(buf, 0, 0);

	/* print the row and column captions */
	p.setfont(font, 2 * FONTSIZE/3);

	for (row = 0; row < 16; row++)
	{
	    buf ="x" + (Integer.toHexString(row)).toUpperCase();
	    p.show_xy(buf, LEFT + row*XINCR, TOP + YINCR);

	    buf = (Integer.toHexString(row)).toUpperCase() + "x";
	    p.show_xy(buf, LEFT - XINCR, TOP - row * YINCR);
	}

	/* print the character table */
	if (embed == 1) {
	    buf = "embedding";
	} else{
	    buf = "";
	}
	font = p.load_font(fontname, encodings[codepage],buf);
	p.setfont(font, FONTSIZE);

	y = TOP;
	x = LEFT;

	for (row = 0; row < 16; row++)
	{
	    for (col = 0; col < 16; col++) {
		buf = String.valueOf((char)(16*row + col));
		p.show_xy(buf, x, y);
		x += XINCR;
	    }
	    x = LEFT;
	    y -= YINCR;
	}

	p.end_page();			/* close page */
    }
    p.close();				/* close PDF document   */

    outbuf = p.get_buffer();

    response.setContentType("application/pdf");
    response.setContentLength(outbuf.length);

    output = response.getOutputStream();
    output.write(outbuf);
    output.close();
%>
