/* $Id: quickreference.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib/PDI client: mini imposition demo
 */
#include <stdio.h>
#include <stdlib.h>

#include "pdflib.h"

int
main(void)
{
    PDF		*p;
    int		manual, page;
    int		font, row, col;
    const	int maxrow = 2;
    const	int maxcol = 2;
    char	optlist[128];
    int		startpage = 1, endpage = 4;
    const float	width = 500, height = 770;
    int		pageno;
    const char *infile = "reference.pdf";

    /* This is where font/image/PDF input files live. Adjust as necessary. */
    char *searchpath = "../data";

    /* create a new PDFlib object */
    if ((p = PDF_new()) == (PDF *) 0)
    {
        printf("Couldn't create PDFlib object (out of memory)!\n");
        return(2);
    }

    PDF_TRY(p) {
        /* open new PDF file */
	if (PDF_open_file(p, "quickreference.pdf") == -1) {
	    printf("Error: %s\n", PDF_get_errmsg(p));
	    return(2);
	}

	PDF_set_parameter(p, "SearchPath", searchpath);

	/* This line is required to avoid problems on Japanese systems */
	PDF_set_parameter(p, "hypertextencoding", "host");

	PDF_set_info(p, "Creator", "quickreference.c");
	PDF_set_info(p, "Author", "Thomas Merz");
	PDF_set_info(p, "Title", "mini imposition demo (C)");

	manual = PDF_open_pdi(p, infile, "", 0);
	if (manual == -1) {
	    printf("Error: %s\n", PDF_get_errmsg(p));
	    return(2);
	}

	row = 0;
	col = 0;

	PDF_set_parameter(p, "topdown", "true");

	for (pageno = startpage; pageno <= endpage; pageno++) {
	    if (row == 0 && col == 0) {
		PDF_begin_page(p, width, height);
		font = PDF_load_font(p, "Helvetica-Bold", 0, "host", "");
		PDF_setfont(p, font, 18);
		PDF_set_text_pos(p, 24, 24);
		PDF_show(p, "PDFlib Quick Reference");
	    }

	    page = PDF_open_pdi_page(p, manual, pageno, "");

	    if (page == -1) {
		printf("Error: %s\n", PDF_get_errmsg(p));
		return(2);
	    }

	    sprintf(optlist, "scale %f", (float) 1/maxrow);
	    PDF_fit_pdi_page(p, page,
		width/maxcol*col, (row + 1) * height/maxrow, optlist);
	    PDF_close_pdi_page(p, page);

	    col++;
	    if (col == maxcol) {
		col = 0;
		row++;
	    }
	    if (row == maxrow) {
		row = 0;
		PDF_end_page(p);
	    }
	}

	/* finish the last partial page */
	if (row != 0 || col != 0)
	    PDF_end_page(p);

	PDF_close(p);
	PDF_close_pdi(p, manual);
    }

    PDF_CATCH(p) {
        printf("PDFlib exception occurred in quickreference sample:\n");
        printf("[%d] %s: %s\n",
	    PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
        PDF_delete(p);
        return(2);
    }

    PDF_delete(p);

    return 0;
}
