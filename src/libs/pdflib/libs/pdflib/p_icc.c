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

/* $Id: p_icc.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib ICC handling routines
 *
 * This software is based in part on the work of Graeme W. Gill
 *
 */

#include "p_intern.h"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


PDFLIB_API int PDFLIB_CALL
PDF_load_iccprofile(PDF *p, const char *profilename, int reserved,
                    const char *optlist)
{
    static const char fn[] = "PDF_load_iccprofile";
    int slot = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content),
	"(p[%p], \"%s\", %d, \"%s\")",
        (void *) p, profilename, reserved, optlist))
    {
        PDF_RETURN_HANDLE(p, slot)
    }

    pdc_warning(p->pdc, PDF_E_UNSUPP_ICC, 0, 0, 0, 0);

    PDF_RETURN_HANDLE(p, slot)
}

