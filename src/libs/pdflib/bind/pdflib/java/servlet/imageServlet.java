/* $Id: imageServlet.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib client: image example in JavaServlet
 */

import java.io.*;
import javax.servlet.*;
import com.pdflib.pdflib;
import com.pdflib.PDFlibException;

public class imageServlet extends GenericServlet
{
    public void service (ServletRequest request, ServletResponse response)
    {
	int image;
	float width, height;
	pdflib p = null;
	String imagefile = "nesrin.jpg";
	byte[] buf;
	ServletOutputStream output;
	int manual, page;
	/* This is where font/image/PDF input files live. Adjust as necessary.*/
	String searchpath = "../data";

	try{

	    p = new pdflib();

	// Generate a PDF in memory; insert a file name to create PDF on disk
	    if (p.open_file("") == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }
	    
	    p.set_parameter("SearchPath", searchpath);

	    p.set_info("Creator", "imageServlet.java");
	    p.set_info("Author", "Rainer Ploeckl");
	    p.set_info("Title", "image sample (JavaServlet)");

	    if (request.getParameter("image") != null){
		imagefile = request.getParameter("image") ;
	    }

	    image = p.load_image("auto", imagefile, "");

	    if (image == -1) {
		throw new Exception("Error: " + p.get_errmsg());
	    }


	    /* dummy page size, will be adjusted by PDF_fit_image() */
	    p.begin_page(10, 10);
	    p.fit_image(image, (float) 0.0, (float) 0.0, "adjustpage");
	    p.close_image(image);
	    p.end_page();

	    p.close();
	    buf = p.get_buffer();

	    response.setContentType("application/pdf");
	    response.setContentLength(buf.length);

	    output = response.getOutputStream();
	    output.write(buf);
	    output.close();

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
