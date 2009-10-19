<%@page import="java.io.*, javax.servlet.*, com.pdflib.pdflib" %><%
/* $Id: image.jsp 14574 2005-10-29 16:27:43Z bonefish $
 *
 * image.jsp
 */

	int image;
	float width, height;
	pdflib p;
	String imagefile = "nesrin.jpg";
	byte[] buf;
	ServletOutputStream output;
	/* This is where font/image/PDF input files live. Adjust as necessary.*/
	String searchpath = "../data";

	p = new pdflib();

	// Generate a PDF in memory; insert a file name to create PDF on disk
	if (p.open_file("") == -1) {
	    System.err.println("Error: " + p.get_errmsg());
	    System.exit(1);
	}

	p.set_parameter("SearchPath", searchpath);

	p.set_info("Creator", "image.jsp");
	p.set_info("Author", "Rainer Ploeckl");
	p.set_info("Title", "image sample (JSP)");

	image = p.load_image("auto", imagefile, "");

	if (image == -1) {
	    System.err.println("Error: " + p.get_errmsg());
	    System.exit(1);
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
%>
