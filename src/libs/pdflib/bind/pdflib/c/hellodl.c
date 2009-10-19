/* $Id: hellodl.c 14574 2005-10-29 16:27:43Z bonefish $
 * PDFlib client: hello example in C with dynamic DLL loading
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "pdflibdl.h"

int
main(void)
{
    PDF *p;
    int font;
    PDFlib_api *PDFlib;

    /* load the PDFlib dynamic library and create a new PDFlib object*/
    if ((PDFlib = PDF_new_dl(&p)) == (PDFlib_api *) NULL)
    {
        printf("Couldn't create PDFlib object (DLL not found?)\n");
        return(2);
    }

    PDF_TRY_DL(PDFlib, p) {
	/* open new PDF file */
	if (PDFlib->PDF_open_file(p, "hellodl.pdf") == -1) {
	    printf("Error: %s\n", PDFlib->PDF_get_errmsg(p));
	    return(2);
	}

	/* This line is required to avoid problems on Japanese systems */
	PDFlib->PDF_set_parameter(p, "hypertextencoding", "host");

	PDFlib->PDF_set_info(p, "Creator", "hello.c");
	PDFlib->PDF_set_info(p, "Author", "Thomas Merz");
	PDFlib->PDF_set_info(p, "Title", "Hello, world (C DLL)!");

	PDFlib->PDF_begin_page(p, a4_width, a4_height);	/* start a new page */

	/* Change "host" encoding to "winansi" or whatever you need! */
	font = PDFlib->PDF_load_font(p, "Helvetica-Bold", 0, "host", "");

	PDFlib->PDF_setfont(p, font, 24);
	PDFlib->PDF_set_text_pos(p, 50, 700);
	PDFlib->PDF_show(p, "Hello, world!");
	PDFlib->PDF_continue_text(p, "(says C DLL)");
	PDFlib->PDF_end_page(p);		/* close page */

	PDFlib->PDF_close(p);			/* close PDF document */
    }

    PDF_CATCH_DL(PDFlib, p) {
        printf("PDFlib exception occurred in hellodl sample:\n");
        printf("[%d] %s: %s\n",
	PDFlib->PDF_get_errnum(p), PDFlib->PDF_get_apiname(p),
	PDFlib->PDF_get_errmsg(p));
	PDF_delete_dl(PDFlib, p);
        return(2);
    }

    /* delete the PDFlib object and unload the library */
    PDF_delete_dl(PDFlib, p);

    return 0;
}
