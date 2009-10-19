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

/* $Id: p_pattern.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib pattern routines
 *
 */

#include "p_intern.h"
#include "p_image.h"
#include "p_font.h"

void
pdf_init_pattern(PDF *p)
{
    int i;

    p->pattern_number = 0;
    p->pattern_capacity = PATTERN_CHUNKSIZE;

    p->pattern = (pdf_pattern *) pdc_malloc(p->pdc,
	sizeof(pdf_pattern) * p->pattern_capacity, "pdf_init_pattern");

    for (i = 0; i < p->pattern_capacity; i++) {
	p->pattern[i].used_on_current_page = pdc_false;
	p->pattern[i].obj_id = PDC_BAD_ID;
    }
}

void
pdf_grow_pattern(PDF *p)
{
    int i;

    p->pattern = (pdf_pattern *) pdc_realloc(p->pdc, p->pattern,
	sizeof(pdf_pattern) * 2 * p->pattern_capacity, "pdf_grow_pattern");

    for (i = p->pattern_capacity; i < 2 * p->pattern_capacity; i++) {
	p->pattern[i].used_on_current_page = pdc_false;
	p->pattern[i].obj_id = PDC_BAD_ID;
    }

    p->pattern_capacity *= 2;
}

void
pdf_write_page_pattern(PDF *p)
{
    int i, total = 0;

    for (i = 0; i < p->pattern_number; i++)
	if (p->pattern[i].used_on_current_page)
	    total++;

    if (total > 0) {
	pdc_puts(p->out, "/Pattern");

	pdc_begin_dict(p->out);			/* pattern */

	for (i = 0; i < p->pattern_number; i++) {
	    if (p->pattern[i].used_on_current_page) {
		p->pattern[i].used_on_current_page = pdc_false; /* reset */
		pdc_printf(p->out, "/P%d %ld 0 R\n", i, p->pattern[i].obj_id);
	    }
	}

	pdc_end_dict(p->out);			/* pattern */
    }
}

void
pdf_cleanup_pattern(PDF *p)
{
    if (p->pattern) {
	pdc_free(p->pdc, p->pattern);
	p->pattern = NULL;
    }
}

/* Start a new pattern definition. */
PDFLIB_API int PDFLIB_CALL
PDF_begin_pattern(
    PDF *p,
    float width,
    float height,
    float xstep,
    float ystep,
    int painttype)
{
    static const char fn[] = "PDF_begin_pattern";
    int slot = -1;

    if (!pdf_enter_api(p, fn, pdf_state_document,
	"(p[%p], %g, %g, %g, %g, %d)",
	(void *) p, width, height, xstep, ystep, painttype))
    {
        PDF_RETURN_HANDLE(p, slot)
    }

    if (width <= 0)
	pdc_error(p->pdc, PDC_E_ILLARG_POSITIVE, "width", 0, 0, 0);

    if (height <= 0)
	pdc_error(p->pdc, PDC_E_ILLARG_POSITIVE, "height", 0, 0, 0);

    if (painttype != 1 && painttype != 2)
	pdc_error(p->pdc, PDC_E_ILLARG_INT,
	    "painttype", pdc_errprintf(p->pdc, "%d", painttype), 0, 0);

    if (p->pattern_number == p->pattern_capacity)
	pdf_grow_pattern(p);

    p->pattern[p->pattern_number].obj_id = pdc_begin_obj(p->out, PDC_NEW_ID);
    p->pattern[p->pattern_number].painttype = painttype;

    p->height		= height;
    p->width		= width;
    p->thumb_id		= PDC_BAD_ID;
    PDF_PUSH_STATE(p, fn, pdf_state_pattern);
    p->next_content	= 0;
    p->contents 	= c_page;
    p->sl		= 0;

    pdf_init_tstate(p);
    pdf_init_gstate(p);
    pdf_init_cstate(p);

    pdc_begin_dict(p->out);				/* pattern dict*/

    p->res_id = pdc_alloc_id(p->out);

    pdc_puts(p->out, "/PatternType 1\n");		/* tiling pattern */

    /* colored or uncolored pattern */
    pdc_printf(p->out, "/PaintType %d\n", painttype);
    pdc_puts(p->out, "/TilingType 1\n");		/* constant spacing */

    pdc_printf(p->out, "/BBox[0 0 %f %f]\n", p->width, p->height);

    pdc_printf(p->out, "/XStep %f\n", xstep);
    pdc_printf(p->out, "/YStep %f\n", ystep);

    pdc_printf(p->out, "/Resources %ld 0 R\n", p->res_id);

    p->length_id = pdc_alloc_id(p->out);
    pdc_printf(p->out, "/Length %ld 0 R\n", p->length_id);

    if (pdc_get_compresslevel(p->out))
	pdc_puts(p->out, "/Filter/FlateDecode\n");

    pdc_end_dict(p->out);				/* pattern dict*/
    pdc_begin_pdfstream(p->out);

    p->next_content++;

    slot = p->pattern_number;
    p->pattern_number++;

    /* top-down y-coordinates */
    pdf_set_topdownsystem(p, height);

    PDF_RETURN_HANDLE(p, slot)
}

/* Finish the pattern definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_pattern(PDF *p)
{
    static const char fn[] = "PDF_end_pattern";

    if (!pdf_enter_api(p, fn, pdf_state_pattern, "(p[%p])\n", (void *) p))
	return;

    /* check whether pdf__save() and pdf__restore() calls are balanced */
    if (p->sl > 0)
	pdc_error(p->pdc, PDF_E_GSTATE_UNMATCHEDSAVE, 0, 0, 0, 0);

    pdf_end_text(p);
    p->contents = c_none;

    pdc_end_pdfstream(p->out);
    pdc_end_obj(p->out);			/* pattern */

    pdc_put_pdfstreamlength(p->out, p->length_id);

    pdc_begin_obj(p->out, p->res_id);		/* Resource object */
    pdc_begin_dict(p->out);			/* Resource dict */

    pdf_write_page_fonts(p);			/* Font resources */

    pdf_write_page_colorspaces(p);		/* Color space resources */

    pdf_write_page_pattern(p);			/* Pattern resources */

    pdf_write_xobjects(p);			/* XObject resources */

    pdf_write_page_extgstates(p);		/* ExtGState resources */

    pdc_end_dict(p->out);			/* resource dict */
    pdc_end_obj(p->out);			/* resource object */

    PDF_POP_STATE(p, fn);

    if (p->flush & pdf_flush_page)
	pdc_flush_stream(p->out);
}
