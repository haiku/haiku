/* $Id: helloServlet.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib client: hello servlet example in Java
 */

import java.io.*;
import javax.servlet.*;

import com.pdflib.pdflib;
import com.pdflib.PDFlibException;

public class helloServlet extends GenericServlet
{
    public void service(ServletRequest request, ServletResponse response)
    {
	int font;
	pdflib p = null;
	byte[] buf;
	ServletOutputStream out;

	try{
	    p = new pdflib();
	// Generate a PDF in memory; insert a file name to create PDF on disk
	    if (p.open_file("") == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }

	    p.set_info("Creator", "helloServlet.java");
	    p.set_info("Author", "Thomas Merz");
	    p.set_info("Title", "Hello world (Java/Servlet)!");

	    p.begin_page(595, 842);		/* start a new page     */

	    font = p.load_font("Helvetica-Bold", "winansi", "");

	    p.setfont(font, 18);

	    p.set_text_pos(50, 700);
	    p.show("Hello world!");
	    p.continue_text("(says Java/Servlet)");
	    p.end_page();			/* close page           */

	    p.close();				/* close PDF document   */

	    buf = p.get_buffer();

	    response.setContentType("application/pdf");
	    response.setContentLength(buf.length);

	    out = response.getOutputStream();
	    out.write(buf);
	    out.close();
        } catch (PDFlibException e) {
            System.err.print("PDFlib exception occurred in hello sample:\n");
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
