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

/* $Id: p_pdi.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDF import routines (require the PDI library)
 *
 */

#define P_PDI_C

#include "p_intern.h"


PDFLIB_API int PDFLIB_CALL
PDF_open_pdi(
    PDF *p,
    const char *filename,
    const char *optlist,
    int reserved)
{
    static const char fn[] = "PDF_open_pdi";
    int retval = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_object | pdf_state_document | pdf_state_page),
	"(p[%p], \"%s\", \"%s\", %d)",
        (void *) p, filename, optlist, reserved))
    {
        PDF_RETURN_HANDLE(p, retval)
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API int PDFLIB_CALL
PDF_open_pdi_callback(
    PDF *p,
    void *opaque,
    size_t filesize,
    size_t (*readproc)(void *opaque, void *buffer, size_t size),
    int (*seekproc)(void *opaque, long offset),
    const char *optlist)
{
    static const char fn[] = "PDF_open_pdi_callback";
    int retval = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_object | pdf_state_document | pdf_state_page),
        "(p[%p], opaque[%p], %ld, readproc[%p], seekproc[%p] \"%s\")",
        (void *)p, opaque, filesize, readproc, seekproc, optlist))
    {
        PDF_RETURN_HANDLE(p, retval)
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API void PDFLIB_CALL
PDF_close_pdi(PDF *p, int doc)
{
    static const char fn[] = "PDF_close_pdi";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_object | pdf_state_document | pdf_state_page),
	"(p[%p], %d)\n", (void *) p, doc))
	return;

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    return;
}

PDFLIB_API int PDFLIB_CALL
PDF_open_pdi_page( PDF *p, int doc, int page, const char *optlist)
{
    static const char fn[] = "PDF_open_pdi_page";
    int retval = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page),
        "(p[%p], %d, %d, \"%s\")", (void *) p, doc, page, optlist))
    {
        PDF_RETURN_HANDLE(p, retval)
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API void PDFLIB_CALL
PDF_fit_pdi_page(PDF *p, int page, float x, float y, const char *optlist)
{
    static const char fn[] = "PDF_fit_pdi_page";

    /* precise scope diagnosis in pdf__place_image */
    if (!pdf_enter_api(p, fn, pdf_state_content,
        "(p[%p], %d, %g, %g, \"%s\")\n", (void *) p, page, x, y, optlist))
        return;

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    return;
}

PDFLIB_API void PDFLIB_CALL
PDF_place_pdi_page(PDF *p, int page, float x, float y, float sx, float sy)
{
    static const char fn[] = "PDF_place_pdi_page";

    if (!pdf_enter_api(p, fn, pdf_state_content,
	"(p[%p], %d, %g, %g, %g, %g)\n", (void *) p, page, x, y, sx, sy))
	return;

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    return;
}

PDFLIB_API void PDFLIB_CALL
PDF_close_pdi_page(PDF *p, int page)
{
    static const char fn[] = "PDF_close_pdi_page";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page),
	"(p[%p], %d)\n", (void *) p, page))
	return;

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    return;
}

PDFLIB_API const char *PDFLIB_CALL
PDF_get_pdi_parameter(
    PDF *p,
    const char *key,
    int doc,
    int page,
    int idx,
    int *len)
{
    static const char fn[] = "PDF_get_pdi_parameter";

    if (!pdf_enter_api(p, fn, pdf_state_all,
	"(p[%p], \"%s\", %d, %d, %d, len[%p])\n",
	(void *) p, key, doc, page, idx, len))
	return (const char *) NULL;

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    return (const char *) NULL;
}

PDFLIB_API float PDFLIB_CALL
PDF_get_pdi_value(PDF *p, const char *key, int doc, int page, int idx)
{
    static const char fn[] = "PDF_get_pdi_value";

    if (!pdf_enter_api(p, fn, pdf_state_all,
	"(p[%p], \"%s\", %d, %d, %d)\n",
	(void *) p, key, doc, page, idx))
	return (float) 0;

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    return (float) 0;
}

PDFLIB_API int PDFLIB_CALL
PDF_process_pdi(PDF *p, int doc, int page, const char *optlist)
{
    static const char fn[] = "PDF_process_pdi";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document),
        "(p[%p], %d, %d, %s)\n", (void *) p, doc, page, optlist))
    {
        PDF_RETURN_BOOLEAN(p, -1);
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_PDI, 0, 0, 0, 0);

    PDF_RETURN_BOOLEAN(p, -1);
}

