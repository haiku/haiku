<%@page import="java.io.*, 
		javax.servlet.*, 
		java.text.*,
		java.util.*,
		com.pdflib.pdflib" %><%
    /* $Id: pdfclock.jsp 14574 2005-10-29 16:27:43Z bonefish $
     *
     * PDFclock.jsp
     */

    pdflib p;
    int tm_hour, tm_min, tm_sec, alpha;
    float RADIUS = 200.0f;
    float MARGIN = 20.0f;
    SimpleDateFormat format;
    Date now = new Date();
    byte[] buf;
    ServletOutputStream output;

    p = new pdflib();

    // Generate a PDF in memory; insert a file name to create PDF on disk
    if (p.open_file("") == -1) {
        System.err.println("Error: " + p.get_errmsg());
	System.exit(1);
    }

    p.set_info("Creator", "pdfclock.jsp");
    p.set_info("Author", "Rainer Ploeckl");
    p.set_info("Title", "PDF clock (Java/JSP)");

    p.begin_page(  (int) (2 * (RADIUS + MARGIN)),
			    (int) (2 * (RADIUS + MARGIN)));

    p.translate(RADIUS + MARGIN, RADIUS + MARGIN);
    p.setcolor("fillstroke", "rgb", 0.0f, 0.0f, 1.0f, 0.0f);
    p.save();

    // minute strokes 
    p.setlinewidth(2.0f);
    for (alpha = 0; alpha < 360; alpha += 6)
    {
	p.rotate(6.0f);
	p.moveto(RADIUS, 0.0f);
	p.lineto(RADIUS-MARGIN/3, 0.0f);
	p.stroke();
    }

    p.restore();
    p.save();

    // 5 minute strokes
    p.setlinewidth(3.0f);
    for (alpha = 0; alpha < 360; alpha += 30)
    {
	p.rotate(30.0f);
	p.moveto(RADIUS, 0.0f);
	p.lineto(RADIUS-MARGIN, 0.0f);
	p.stroke();
    }

    format = new SimpleDateFormat("hh");
    tm_hour= Integer.parseInt(format.format(now));
    format = new SimpleDateFormat("mm");
    tm_min = Integer.parseInt(format.format(now));
    format = new SimpleDateFormat("ss");
    tm_sec = Integer.parseInt(format.format(now));

    // draw hour hand 
    p.save();
    p.rotate((-((tm_min/60.0f) + tm_hour - 3.0f) * 30.0f));
    p.moveto(-RADIUS/10, -RADIUS/20);
    p.lineto(RADIUS/2, 0.0f);
    p.lineto(-RADIUS/10, RADIUS/20);
    p.closepath();
    p.fill();
    p.restore();

    // draw minute hand
    p.save();
    p.rotate((-((tm_sec/60.0f) + tm_min - 15.0f) * 6.0f));
    p.moveto(-RADIUS/10, -RADIUS/20);
    p.lineto(RADIUS * 0.8f, 0.0f);
    p.lineto(-RADIUS/10, RADIUS/20);
    p.closepath();
    p.fill();
    p.restore();

    // draw second hand
    p.setcolor("fillstroke", "rgb", 1.0f, 0.0f, 0.0f, 0.0f);
    p.setlinewidth(2);
    p.save();
    p.rotate(-((tm_sec - 15.0f) * 6.0f));
    p.moveto(-RADIUS/5, 0.0f);
    p.lineto(RADIUS, 0.0f);
    p.stroke();
    p.restore();

    // draw little circle at center
    p.circle(0f, 0f, RADIUS/30);
    p.fill();

    p.restore();
    p.end_page();
    p.close();

    buf = p.get_buffer();

    response.setContentType("application/pdf");
    response.setContentLength(buf.length);
    output = response.getOutputStream();
    output.write(buf);
    output.close();
%>
