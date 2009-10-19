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

/* $Id: p_template.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib template routines
 *
 */

#include "p_intern.h"
#include "p_image.h"
#include "p_font.h"

/* Start a new template definition. */
PDFLIB_API int PDFLIB_CALL
PDF_begin_template(PDF *p, float width, float height)
{
    static const char fn[] = "PDF_begin_template";
    pdf_image *image;
    int im = -1;

    if (!pdf_enter_api(p, fn, pdf_state_document, "(p[%p], %g, %g)",
	(void *) p, width, height))
    {
        PDF_RETURN_HANDLE(p, im)
    }

    if (width <= 0)
	pdc_error(p->pdc, PDC_E_ILLARG_POSITIVE, "width", 0, 0, 0);

    if (height <= 0)
	pdc_error(p->pdc, PDC_E_ILLARG_POSITIVE, "height", 0, 0, 0);

    for (im = 0; im < p->images_capacity; im++)
	if (!p->images[im].in_use)		/* found free slot */
	    break;

    if (im == p->images_capacity)
	pdf_grow_images(p);

    image		= &p->images[im];
    image->in_use	= pdc_true;		/* mark slot as used */
    image->no		= pdf_new_xobject(p, form_xobject, PDC_NEW_ID);
    image->width	= width;
    image->height	= height;

    p->height		= height;
    p->width		= width;
    p->thumb_id		= PDC_BAD_ID;
    PDF_PUSH_STATE(p, fn, pdf_state_template);
    p->templ		= im;		/* remember the current template id */
    p->next_content	= 0;
    p->contents 	= c_page;
    p->sl		= 0;

    pdf_init_tstate(p);
    pdf_init_gstate(p);
    pdf_init_cstate(p);

    pdc_begin_dict(p->out);				/* form xobject dict*/
    pdc_puts(p->out, "/Type/XObject\n");
    pdc_puts(p->out, "/Subtype/Form\n");

    /* contrary to the PDF reference /FormType and /Matrix are required! */
    pdc_printf(p->out, "/FormType 1\n");
    pdc_printf(p->out, "/Matrix[1 0 0 1 0 0]\n");

    p->res_id = pdc_alloc_id(p->out);
    pdc_printf(p->out, "/Resources %ld 0 R\n", p->res_id);

    pdc_printf(p->out, "/BBox[0 0 %f %f]\n", p->width, p->height);

    p->length_id = pdc_alloc_id(p->out);
    pdc_printf(p->out, "/Length %ld 0 R\n", p->length_id);

    if (pdc_get_compresslevel(p->out))
	pdc_puts(p->out, "/Filter/FlateDecode\n");

    pdc_end_dict(p->out);				/* form xobject dict*/
    pdc_begin_pdfstream(p->out);

    p->next_content++;

    /* top-down y-coordinates */
    pdf_set_topdownsystem(p, height);

    PDF_RETURN_HANDLE(p, im)
}

/* Finish the template definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_template(PDF *p)
{
    static const char fn[] = "PDF_end_template";

    if (!pdf_enter_api(p, fn, pdf_state_template, "(p[%p])\n", (void *) p))
	return;

    /* check whether pdf__save() and pdf__restore() calls are balanced */
    if (p->sl > 0)
	pdc_error(p->pdc, PDF_E_GSTATE_UNMATCHEDSAVE, 0, 0, 0, 0);

    pdf_end_text(p);
    p->contents = c_none;

    pdc_end_pdfstream(p->out);
    pdc_end_obj(p->out);			/* form xobject */

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
