/* $Id: invoice.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib/PDI client: invoice generation demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pdflib.h"

int
main(void)
{
    PDF *	p;
    int		i, form, page, regularfont, boldfont;
    char *	infile = "stationery.pdf";

    /* This is where font/image/PDF input files live. Adjust as necessary. */
    char *searchpath = "../data";

    const float	col1 = 55;
    const float	col2 = 100;
    const float	col3 = 330;
    const float	col4 = 430;
    const float	col5 = 530;
    time_t	timer;
    struct tm	ltime;
    float	fontsize = 12, leading, y;
    float	sum, total;
    float	pagewidth = 595, pageheight = 842;
    char	buf[128];
    char	*closingtext =
	"30 days warranty starting at the day of sale. "
	"This warranty covers defects in workmanship only. "
	"Kraxi Systems, Inc. will, at its option, repair or replace the "
	"product under the warranty. This warranty is not transferable. "
	"No returns or exchanges will be accepted for wet products.";

    typedef struct { char *name; float price; int quantity; } articledata;

    articledata data[] = {
	{ "Super Kite",		20,	2},
	{ "Turbo Flyer",	40,	5},
	{ "Giga Trash",		180,	1},
	{ "Bare Bone Kit",	50,	3},
	{ "Nitty Gritty",	20,	10},
	{ "Pretty Dark Flyer",	75,	1},
	{ "Free Gift",		0,	1},
    };

#define ARTICLECOUNT (sizeof(data)/sizeof(data[0]))

    static const char *months[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
    };

    /* create a new PDFlib object */
    if ((p = PDF_new()) == (PDF *) 0)
    {
        printf("Couldn't create PDFlib object (out of memory)!\n");
        return(2);
    }

    PDF_TRY(p) {
	/* open new PDF file */
	if (PDF_open_file(p, "invoice.pdf") == -1) {
	    printf("Error: %s\n", PDF_get_errmsg(p));
	    return(2);
	}

	PDF_set_parameter(p, "SearchPath", searchpath);

	/* This line is required to avoid problems on Japanese systems */
	PDF_set_parameter(p, "hypertextencoding", "host");

	PDF_set_info(p, "Creator", "invoice.c");
	PDF_set_info(p, "Author", "Thomas Merz");
	PDF_set_info(p, "Title", "PDFlib invoice generation demo (C)");

	form = PDF_open_pdi(p, infile, "", 0);
	if (form == -1) {
	    printf("Error: %s\n", PDF_get_errmsg(p));
	    return(2);
	}

	page = PDF_open_pdi_page(p, form, 1, "");
	if (page == -1) {
	    printf("Error: %s\n", PDF_get_errmsg(p));
	    return(2);
	}

	boldfont = PDF_load_font(p, "Helvetica-Bold", 0, "host", "");
	regularfont = PDF_load_font(p, "Helvetica", 0, "host", "");
	leading = fontsize + 2;

	/* Establish coordinates with the origin in the upper left corner. */
	PDF_set_parameter(p, "topdown", "true");

	PDF_begin_page(p, pagewidth, pageheight);	/* A4 page */

	PDF_fit_pdi_page(p, page, 0, pageheight, "");
	PDF_close_pdi_page(p, page);

	PDF_setfont(p, regularfont, fontsize);

	/* Print the address */
	y = 170;
	PDF_set_value(p, "leading", leading);

	PDF_show_xy(p, "John Q. Doe", col1, y);
	PDF_continue_text(p, "255 Customer Lane");
	PDF_continue_text(p, "Suite B");
	PDF_continue_text(p, "12345 User Town");
	PDF_continue_text(p, "Everland");

	/* Print the header and date */

	PDF_setfont(p, boldfont, fontsize);
	y = 300;
	PDF_show_xy(p, "INVOICE",	col1, y);

	time(&timer);
	ltime = *localtime(&timer);
	sprintf(buf, "%s %d, %d",
		    months[ltime.tm_mon], ltime.tm_mday, ltime.tm_year + 1900);
	PDF_fit_textline(p, buf, 0, col5, y, "position {100 0}");

	/* Print the invoice header line */
	PDF_setfont(p, boldfont, fontsize);

	/* "position {0 0}" is left-aligned, "position {100 0}" right-aligned */
	y = 370;
	PDF_fit_textline(p, "ITEM",		0, col1, y, "position {0 0}");
	PDF_fit_textline(p, "DESCRIPTION",	0, col2, y, "position {0 0}");
	PDF_fit_textline(p, "QUANTITY",		0, col3, y, "position {100 0}");
	PDF_fit_textline(p, "PRICE",		0, col4, y, "position {100 0}");
	PDF_fit_textline(p, "AMOUNT",		0, col5, y, "position {100 0}");

	/* Print the article list */

	PDF_setfont(p, regularfont, fontsize);
	y += 2*leading;
	total = 0;

	for (i = 0; i < (int) ARTICLECOUNT; i++) {
	    sprintf(buf, "%d", i+1);
	    PDF_show_xy(p, buf, col1, y);

	    PDF_show_xy(p, data[i].name, col2, y);

	    sprintf(buf, "%d", data[i].quantity);
	    PDF_fit_textline(p, buf, 0, col3, y, "position {100 0}");

	    sprintf(buf, "%.2f", data[i].price);
	    PDF_fit_textline(p, buf, 0, col4, y, "position {100 0}");

	    sum = data[i].price * data[i].quantity;
	    sprintf(buf, "%.2f", sum);
	    PDF_fit_textline(p, buf, 0, col5, y, "position {100 0}");

	    y += leading;
	    total += sum;
	}

	y += leading;
	PDF_setfont(p, boldfont, fontsize);
	sprintf(buf, "%.2f", total);
	PDF_fit_textline(p, buf, 0, col5, y, "position {100 0}");

	/* Print the closing text */

	y += 5*leading;
	PDF_setfont(p, regularfont, fontsize);
	PDF_set_value(p, "leading", leading);
	PDF_show_boxed(p, closingtext,
	    col1, y + 4*leading, col5-col1, 4*leading, "justify", "");

	PDF_end_page(p);
	PDF_close(p);
	PDF_close_pdi(p, form);
    }

    PDF_CATCH(p) {
        printf("PDFlib exception occurred in invoice sample:\n");
        printf("[%d] %s: %s\n",
	    PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
        PDF_delete(p);
        return(2);
    }

    PDF_delete(p);

    return 0;
}
