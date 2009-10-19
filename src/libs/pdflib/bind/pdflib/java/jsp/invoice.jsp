<%@page import="java.io.*, 
		javax.servlet.*, 
		java.text.*, 
		java.util.*,
		com.pdflib.pdflib"
		%><%

   /* $Id: invoice.jsp 14574 2005-10-29 16:27:43Z bonefish $
    *
    * invoice.jsp
    */

    pdflib p = null ;
    int         i, form, pdipage, regularfont, boldfont;
    String      infile = "stationery.pdf";
    /* This is where font/image/PDF input files live. Adjust as necessary.*/
    String      searchpath = "../data";
    final float col1 = 55;
    final float col2 = 100;
    final float col3 = 330;
    final float col4 = 430;
    final float col5 = 530;
    float       fontsize = 12, leading, y;
    float       sum, total;
    float       pagewidth = 595, pageheight = 842;
    Date now = new Date();
    DateFormat fulldate = DateFormat.getDateInstance(DateFormat.LONG);

    byte[] buf;
    ServletOutputStream output;

    String      closingtext =
	"30 days warranty starting at the day of sale. " +
	"This warranty covers defects in workmanship only. " +
	"Kraxi Systems, Inc. will, at its option, repair or replace the " +
	"product under the warranty. This warranty is not transferable. " +
	"No returns or exchanges will be accepted for wet products.";

    String[][] data = {
	{ "Super Kite",         "20",     "2"},
	{ "Turbo Flyer",        "40",     "5"},
	{ "Giga Trash",         "180",    "1"},
	{ "Bare Bone Kit",      "50",     "3"},
	{ "Nitty Gritty",       "20",     "10"},
	{ "Pretty Dark Flyer",  "75",     "1"},
	{ "Free Gift",          "0",      "1"},
    };

    String[] months = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
    };

    p = new pdflib();

    // Generate a PDF in memory; insert a file name to create PDF on disk
    if (p.open_file("") == -1) {
        System.err.println("Error: " + p.get_errmsg());
	System.exit(1);
    }

    p.set_parameter("SearchPath", searchpath);

    p.set_info("Creator", "invoice.jsp");
    p.set_info("Author", "Rainer Ploeckl");
    p.set_info("Title", "PDFlib invoice generation demo (Java/JSP)");

    form = p.open_pdi(infile, "", 0);
    if (form == -1) {
	System.err.println("Couldn't open infile '" + infile + "'.\n");
	System.exit(1);
    }

    pdipage = p.open_pdi_page(form, 1, "");
    if (pdipage == -1) {
	System.err.println("Error: " + p.get_errmsg());
	System.exit(1);
    }

    boldfont = p.load_font("Helvetica-Bold", "winansi", "");
    regularfont = p.load_font("Helvetica", "winansi", "");
    leading = fontsize + 2;

    // Establish coordinates with the origin in the upper left corner.
    p.set_parameter("topdown", "true");

    p.begin_page(pagewidth, pageheight);       // A4 page

    p.fit_pdi_page(pdipage, 0, pageheight, "");
    p.close_pdi_page(pdipage);

    p.setfont(regularfont, fontsize);

    // Print the address
    y = 170;
    p.set_value("leading", leading);

    p.show_xy("John Q. Doe", col1, y);
    p.continue_text("255 Customer Lane");
    p.continue_text("Suite B");
    p.continue_text("12345 User Town");
    p.continue_text("Everland");

    // Print the header and date

    p.setfont(boldfont, fontsize);
    y = 300;
    p.show_xy("INVOICE", col1, y);

    p.fit_textline(fulldate.format(now), col5, y, "position {100 0}");

    // Print the invoice header line
    p.setfont(boldfont, fontsize);

    // "position {0 0}" is left-aligned, "position {100 0}" right-aligned
    y = 370;
    p.fit_textline("ITEM",             col1, y, "position {0 0}");
    p.fit_textline("DESCRIPTION",      col2, y, "position {0 0}");
    p.fit_textline("QUANTITY",         col3, y, "position {100 0}");
    p.fit_textline("PRICE",            col4, y, "position {100 0}");
    p.fit_textline("AMOUNT",           col5, y, "position {100 0}");

    // Print the article list

    p.setfont(regularfont, fontsize);
    y += 2*leading;
    total = 0;

    for (i = 0; i < data.length; i++) {
	p.show_xy(Integer.toString(i+1), col1, y);
	p.show_xy(data[i][0], col2, y);
	p.fit_textline(data[i][2], col3, y, "position {100 0}");
	p.fit_textline(data[i][1], col4, y, "position {100 0}");
	sum = 0;

	sum = Integer.parseInt(data[i][2]) * Integer.parseInt(data[i][1]);
	p.fit_textline(Float.toString(sum), col5, y, "position {100 0}");

	y += leading;
	total += sum;
    }

    y += leading;
    p.setfont(boldfont, fontsize);
    p.fit_textline(Float.toString(total), col5, y, "position {100 0}");

    // Print the closing text

    y += 5*leading;
    p.setfont(regularfont, fontsize);
    p.set_value("leading", leading);
    p.show_boxed(closingtext,
	col1, y + 4*leading, col5-col1, 4*leading, "justify", "");

    p.end_page();
    p.close();
    p.close_pdi(form);

    buf = p.get_buffer();

    response.setContentType("application/pdf");
    response.setContentLength(buf.length);

    output = response.getOutputStream();
    output.write(buf);
    output.close();
%>
