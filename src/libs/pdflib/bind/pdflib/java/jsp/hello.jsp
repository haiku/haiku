<%@page import="java.io.*, javax.servlet.*, com.pdflib.pdflib" %><%

   /* $Id: hello.jsp 14574 2005-10-29 16:27:43Z bonefish $
    *
    * hello.jsp
    */

    int font;
    pdflib p;
    byte[] buf;
    ServletOutputStream output;

    p = new pdflib();

    // Generate a PDF in memory; insert a file name to create PDF on disk
    if (p.open_file("") == -1) {
        System.err.println("Error: " + p.get_errmsg());
	System.exit(1);
    }

    p.set_info("Creator", "hello.jsp");
    p.set_info("Author", "Rainer Ploeckl");
    p.set_info("Title", "Hello world (Java/JSP)!");

    p.begin_page(595, 842);		/* start a new page     */

    font = p.load_font("Helvetica-Bold", "winansi", "");

    p.setfont(font, 18);

    p.set_text_pos(50, 700);
    p.show("Hello world!");
    p.continue_text("(says Java/JSP)");
    p.end_page();			/* close page           */

    p.close();				/* close PDF document   */

    buf = p.get_buffer();

    response.setContentType("application/pdf");
    response.setContentLength(buf.length);

    output = response.getOutputStream();
    output.write(buf);
    output.close();
%>
