/* $Id: quickreference.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib/PDI client: mini imposition demo
 */

import java.io.*;
import com.pdflib.pdflib;
import com.pdflib.PDFlibException;

public class quickreference
{
    public static void main (String argv[])
    {
	int manual, page;
	int font, row, col;
	final int maxrow = 2, maxcol = 2;
	int i, startpage = 1, endpage = 4;
	final float width = 500, height = 770;
	int pageno;
	pdflib p = null;
	String infile = "reference.pdf";
	/* This is where font/image/PDF input files live. Adjust as necessary.*/
	String searchpath = "../data";

	try{
	    p = new pdflib();

	    if (p.open_file("quickreference.pdf") == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }

	    p.set_parameter("SearchPath", searchpath);

	    p.set_info("Creator", "quickreference.java");
	    p.set_info("Author", "Thomas Merz");
	    p.set_info("Title", "imposition demo (Java)");

	    manual = p.open_pdi(infile, "", 0);
	    if (manual == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }

	    row = 0;
	    col = 0;

	    p.set_parameter("topdown", "true");

	    for (pageno = startpage; pageno <= endpage; pageno++) {
		if (row == 0 && col == 0) {
		    p.begin_page(width, height);
		    font = p.load_font("Helvetica-Bold", "winansi", "");
		    p.setfont(font, 18);
		    p.set_text_pos(24, 24);
		    p.show("PDFlib Quick Reference");
		}

		page = p.open_pdi_page(manual, pageno, "");

		if (page == -1) {
		    throw new Exception("Error: " + p.get_errmsg());
		}

		p.fit_pdi_page(manual, width/maxcol*col,
		    (row + 1) * height/maxrow, "scale " + (float)1/maxrow);
		p.close_pdi_page(page);

		col++;
		if (col == maxcol) {
		    col = 0;
		    row++;
		}
		if (row == maxrow) {
		    row = 0;
		    p.end_page();
		}
	    }

	    // finish the last partial page
	    if (row != 0 || col != 0)
		p.end_page();

	    p.close();
	    p.close_pdi(manual);

        } catch (PDFlibException e) {
	    System.err.print("PDFlib exception occurred in quickreference sample:\n");
	    System.err.print("[" + e.get_errnum() + "] " + e.get_apiname() +
			    ": " + e.getMessage() + "\n");
        } catch (Exception e) {
            System.err.println(e.getMessage());
        } finally {
            if (p != null) {
		p.delete();			/* delete the PDFlib object */
            }
        }
    }
}
