/* $Id: quickreferenceServlet.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib/PDI client: mini imposition demo
 */

import java.io.*;
import javax.servlet.*;
import com.pdflib.pdflib;
import com.pdflib.PDFlibException;

public class quickreferenceServlet extends GenericServlet
{
    public void service (ServletRequest request, ServletResponse response)
    {
	int font, row = 0 , col = 0 , i;
	int manual, pages;
	final int maxrow=2, maxcol=2;
	int startpage = 1, endpage = 4;
	final float width = 500, height = 770;
	int pageno;
	String infile = "reference.pdf";
	/* This is where font/image/PDF input files live. Adjust as necessary.*/
	String searchpath = "../data";
	byte[] buf;
	ServletOutputStream output;
	pdflib p = null;

	try{
	    p = new pdflib();

	// Generate a PDF in memory; insert a file name to create PDF on disk
	    if (p.open_file("") == -1) {
		throw new Exception("Couldn't create PDF output.\n");
	    }

	    p.set_parameter("SearchPath", searchpath);

	    p.set_info("Creator", "quickreferenceServlet.java");
	    p.set_info("Author", "Rainer Ploeckl");
	    p.set_info("Title", "imposition demo (Java/Servlet)");

	    manual = p.open_pdi(infile, "", 0);
	    i = 0;

	    if (manual == -1){
		throw new Exception("Error: " + p.get_errmsg());
	    }

	    p.set_parameter("topdown", "true");

	    for (pageno = startpage; pageno <= endpage; pageno++) {
		if (row == 0 && col == 0) {
		    i++;
		    p.begin_page(width, height);
		    font = p.load_font("Helvetica-Bold", "winansi", "");
		    p.setfont(font, 18);
		    p.set_text_pos(24, 24);
		    p.show("PDFlib Quick Reference");
		}

		pages = p.open_pdi_page(manual, pageno, "");

		if (pages == -1) {
		    throw new Exception("Error: " + p.get_errmsg());
		}

		p.fit_pdi_page(manual, width/maxcol*col,
			(row + 1) * height/maxrow, "scale " + (float)1/maxrow);
		p.close_pdi_page(pages);

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
	    buf = p.get_buffer();

	    response.setContentType("application/pdf");
	    response.setContentLength(buf.length);

	    output = response.getOutputStream();
	    output.write(buf);
	    output.close();
        } catch (PDFlibException e) {
            System.err.print("PDFlib exception occurred in quickreference sample:\n");
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
