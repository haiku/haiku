/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2004 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: pdflibdl.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Function prototypes for dynamically loading the PDFlib DLL at runtime
 *
 */

#ifdef __cplusplus
#define PDF PDF_c
#endif

#include "pdflib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Notes for using the PDFlib DLL loading mechanism:
 *
 * - PDF_TRY_DL()/PDF_CATCH_DL() must be used instead of the standard
 *   exception handling macros.
 * - PDF_new_dl() must be used instead of PDF_boot() and PDF_new()/PDF_new2().
 * - PDF_delete_dl() must be used instead of PDF_delete() and PDF_shutdown().
 * - PDF_get_opaque() must not be used.
 */

/* Load the PDFlib DLL, and fetch pointers to all exported functions. */
PDFLIB_API PDFlib_api * PDFLIB_CALL
PDF_new_dl(PDF **pp);

/* Unload the previously loaded PDFlib DLL  (also calls PDF_shutdown()) */
PDFLIB_API void PDFLIB_CALL
PDF_delete_dl(PDFlib_api *PDFlib, PDF *p);


#define PDF_TRY_DL(PDFlib, p)	\
    if (p) { if (setjmp(PDFlib->pdf_jbuf(p)->jbuf) == 0)

/* Inform the exception machinery that a PDF_TRY() will be left without
   entering the corresponding PDF_CATCH( ) clause. */
#define PDF_EXIT_TRY_DL(PDFlib, p)		PDFlib->pdf_exit_try(p)

/* Catch an exception; must always be paired with PDF_TRY(). */
#define PDF_CATCH_DL(PDFlib, p)		} if (PDFlib->pdf_catch(p))

/* Re-throw an exception to another handler. */
#define PDF_RETHROW_DL(PDFlib, p)		PDFlib->pdf_rethrow(p)

#ifdef __cplusplus
}	/* extern "C" */
#endif
