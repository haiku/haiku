/* $Id: chartabServlet.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib client: hello servlet example in Java
 */

import java.io.*;
import javax.servlet.*;

import com.pdflib.pdflib;
import com.pdflib.PDFlibException;

public class chartabServlet extends GenericServlet
{
    /* change these as required */
    static final String fontname = "LuciduxSans-Oblique";

    /* This is where font/image/PDF input files live. Adjust as necessary. */
    static final String searchpath = "../data";

    /* list of encodings to use */
    static final String encodings[] = { "iso8859-1", "iso8859-2", "iso8859-15" };
    static final int  ENCODINGS = 3;
    static final float FONTSIZE	= 16;
    static final float TOP	= 700;
    static final float LEFT	= 50;
    static final float YINCR	= 2*FONTSIZE;
    static final float XINCR	= 2*FONTSIZE;

    public void service(ServletRequest request, ServletResponse response)
    {
	/* whether or not to embed the font */
	int embed = 1;

	String buf;
	float x, y;
	int row, col, font, page;

	pdflib p = null ;
	byte[] outbuf;
	ServletOutputStream out;

	try{
	    p = new pdflib();

	// Generate a PDF in memory; insert a file name to create PDF on disk
	    if (p.open_file("") == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }

	    p.set_parameter("openaction", "fitpage");
	    p.set_parameter("fontwarning", "true");

	    p.set_parameter("SearchPath", searchpath);

	    p.set_info("Creator", "chartabServlet.java");
	    p.set_info("Author", "Thomas Merz");
	    p.set_info("Title", "Character table (Java/Servlet)");

	    /* loop over all encodings */
	    for (page = 0; page < ENCODINGS; page++)
	    {
		p.begin_page(595, 842);  /* start a new page */

		/* print the heading and generate the bookmark */
		font = p.load_font("Helvetica", "winansi", "");
		p.setfont(font, FONTSIZE);
		if (embed == 1) {
		    buf = fontname + " (" + encodings[page] + ") embedded";
		} else{
		    buf = fontname + " (" + encodings[page] + ") not  embedded";
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
		font = p.load_font(fontname, encodings[page],buf);
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

	    out = response.getOutputStream();
	    out.write(outbuf);
	    out.close();

        } catch (PDFlibException e) {
            System.err.print("PDFlib exception occurred in chartab sample:\n");
            System.err.print("[" + e.get_errnum() + "] " + e.get_apiname() +
                            ": " + e.getMessage() + "\n");
        } catch (Exception e) {
            System.err.println(e.getMessage());
        } finally {
            if (p != null) {
                p.delete();                     /* delete the PDFlib object */
            }
        }
    }
}
