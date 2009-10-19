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

/* $Id: p_kerning.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib kerning routines
 *
 */

#include "p_intern.h"
#include "p_font.h"
#include "p_truetype.h"


PDFLIB_API float PDFLIB_CALL
PDF_get_kern_amount( PDF *p, int font, int firstchar, int secondchar)
{
    static const char fn[] = "PDF_get_kern_amount";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content | pdf_state_path),
	"(p[%p], %d, %d, %d)", (void *) p, font, firstchar, secondchar))
        return (float) 0;

    pdc_warning(p->pdc, PDF_E_UNSUPP_KERNING, 0, 0, 0, 0);

    return (float) 0;
}

