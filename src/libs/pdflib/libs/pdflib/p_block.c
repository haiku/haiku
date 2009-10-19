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

/* $Id: p_block.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Block processing routines (require the PDI library)
 *
 */

#define P_BLOCK_C

#include "p_intern.h"
#include "p_font.h"
#include "p_image.h"


PDFLIB_API int PDFLIB_CALL
PDF_fill_textblock(PDF *p,
int page, const char *blockname, const char *text, int len, const char *optlist)
{
    static const char fn[] = "PDF_fill_textblock";

    if (!pdf_enter_api(p, fn,
        (pdf_state) pdf_state_content,
	"(p[%p], %d, \"%s\", \"%s\", %d, \"%s\")",
	(void *) p, page, blockname,
        pdc_strprint(p->pdc, text, len), len, optlist))
    {
	PDF_RETURN_BOOLEAN(p, -1);
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_BLOCK, 0, 0, 0, 0);

    PDF_RETURN_BOOLEAN(p, -1);
}

PDFLIB_API int PDFLIB_CALL
PDF_fill_imageblock(PDF *p,
int page, const char *blockname, int image, const char *optlist)
{
    static const char fn[] = "PDF_fill_imageblock";

    if (!pdf_enter_api(p, fn,
        (pdf_state) pdf_state_content,
	"(p[%p], %d, \"%s\", %d, \"%s\")",
	(void *) p, page, blockname, image, optlist))
    {
	PDF_RETURN_BOOLEAN(p, -1);
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_BLOCK, 0, 0, 0, 0);

    PDF_RETURN_BOOLEAN(p, -1);
}

PDFLIB_API int PDFLIB_CALL
PDF_fill_pdfblock(PDF *p,
int page, const char *blockname, int contents, const char *optlist)
{
    static const char fn[] = "PDF_fill_pdfblock";

    if (!pdf_enter_api(p, fn,
        (pdf_state) pdf_state_content,
	"(p[%p], %d, \"%s\", %d, \"%s\")",
	(void *) p, page, blockname, contents, optlist))
    {
	PDF_RETURN_BOOLEAN(p, -1);
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_BLOCK, 0, 0, 0, 0);

    PDF_RETURN_BOOLEAN(p, -1);
}

