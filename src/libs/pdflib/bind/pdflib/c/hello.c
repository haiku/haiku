/* $Id: hello.c 14574 2005-10-29 16:27:43Z bonefish $
 * PDFlib client: hello example in C
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "pdflib.h"

int
main(void)
{
    PDF *p;
    int font;

    /* create a new PDFlib object */
    if ((p = PDF_new()) == (PDF *) 0)
    {
        printf("Couldn't create PDFlib object (out of memory)!\n");
        return(2);
    }

    PDF_TRY(p) {
	/* open new PDF file */
	if (PDF_open_file(p, "hello.pdf") == -1) {
	    printf("Error: %s\n", PDF_get_errmsg(p));
	    return(2);
	}

	/* This line is required to avoid problems on Japanese systems */
	PDF_set_parameter(p, "hypertextencoding", "host");

	PDF_set_info(p, "Creator", "hello.c");
	PDF_set_info(p, "Author", "Thomas Merz");
	PDF_set_info(p, "Title", "Hello, world (C)!");

	PDF_begin_page(p, a4_width, a4_height);	/* start a new page     */

        /* Change "host" encoding to "winansi" or whatever you need! */
        font = PDF_load_font(p, "Helvetica-Bold", 0, "host", "");

        PDF_setfont(p, font, 24);
        PDF_set_text_pos(p, 50, 700);
        PDF_show(p, "Hello, world!");
        PDF_continue_text(p, "(says C)");
        PDF_end_page(p);                        /* close page           */

	PDF_close(p);				/* close PDF document	*/
    }

    PDF_CATCH(p) {
        printf("PDFlib exception occurred in hello sample:\n");
        printf("[%d] %s: %s\n",
	    PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
        PDF_delete(p);
        return(2);
    }

    PDF_delete(p);				/* delete the PDFlib object */

    return 0;
}
