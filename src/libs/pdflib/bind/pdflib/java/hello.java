/* $Id: hello.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib client: hello example in Java
 */

import java.io.*;
import com.pdflib.pdflib;
import com.pdflib.PDFlibException;

public class hello
{
    public static void main (String argv[])
    {
	int font;
	pdflib p = null;

	try{
	    p = new pdflib();

	    /* open new PDF file */
	    if (p.open_file("hello.pdf") == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }

	    p.set_info("Creator", "hello.java");
	    p.set_info("Author", "Thomas Merz");
	    p.set_info("Title", "Hello world (Java)!");

	    p.begin_page(595, 842);		/* start a new page     */

	    font = p.load_font("Helvetica-Bold", "winansi", "");

	    p.setfont(font, 18);

	    p.set_text_pos(50, 700);
	    p.show("Hello world!");
	    p.continue_text("(says Java)");
	    p.end_page();			/* close page           */

	    p.close();				/* close PDF document   */

        } catch (PDFlibException e) {
	    System.err.print("PDFlib exception occurred in hello sample:\n");
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
