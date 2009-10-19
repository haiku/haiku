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

/* $Id: p_color.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib color routines
 *
 */

#define P_COLOR_C

#include "p_intern.h"
#include "p_font.h"




/* Avoid wrong error messages due to rounding artifacts.
 * This doesn't do any harm since we truncate to 5 decimal places anyway
 * when producing PDF output.
 */
#define EPSILON	((float) (1.0 + PDF_SMALLREAL))

static void pdf_check_color_values(PDF *p, pdf_colorspacetype type,
    float c1, float c2, float c3, float c4);

int
pdf_color_components(PDF *p, int slot)
{
    pdf_colorspace *cs;
    int components = 0;

    cs = &p->colorspaces[slot];

    switch (cs->type) {
	case DeviceGray:
	case Indexed:
	components = 1;
	break;

	case DeviceRGB:
	components = 3;
	break;

	case DeviceCMYK:
	components = 4;
	break;

	case PatternCS:
	if (cs->val.pattern.base == pdc_undef)
	    components = 0;	/* PaintType 1: colored pattern */
	else
	    components = pdf_color_components(p, cs->val.pattern.base);

	default:
	    pdc_error(p->pdc, PDF_E_INT_BADCS,
		pdc_errprintf(p->pdc, "%d", cs->type), 0, 0, 0);
    }

    return components;
} /* pdf_color_components */

void
pdf_write_color_values(PDF *p, pdf_color *color, pdf_drawmode drawmode)
{
    pdf_colorspace *cs = &p->colorspaces[color->cs];

    switch (cs->type) {
	case DeviceGray:
	    pdc_printf(p->out, "%f", color->val.gray);

	    if (drawmode == pdf_fill)
		pdc_puts(p->out, " g\n");
	    else if (drawmode == pdf_stroke)
		pdc_puts(p->out, " G\n");
	    break;

	case DeviceRGB:
	    pdc_printf(p->out, "%f %f %f",
		    color->val.rgb.r, color->val.rgb.g, color->val.rgb.b);

	    if (drawmode == pdf_fill)
		pdc_puts(p->out, " rg\n");
	    else if (drawmode == pdf_stroke)
		pdc_puts(p->out, " RG\n");
	    break;

	case DeviceCMYK:
	    pdc_printf(p->out, "%f %f %f %f",
		    color->val.cmyk.c, color->val.cmyk.m,
		    color->val.cmyk.y, color->val.cmyk.k);

	    if (drawmode == pdf_fill)
		pdc_puts(p->out, " k\n");
	    else if (drawmode == pdf_stroke)
		pdc_puts(p->out, " K\n");
	    break;


	case PatternCS:
	    if (drawmode == pdf_fill) {
		if (p->pattern[color->val.pattern].painttype == 1) {
		    pdc_puts(p->out, "/Pattern cs");
		} else if (p->pattern[color->val.pattern].painttype == 2) {
		    pdc_printf(p->out, "/CS%d cs ", color->cs);
		    pdf_write_color_values(p,
			&p->cstate[p->sl].fill, pdf_none);
		}
		pdc_printf(p->out, "/P%d scn\n", color->val.pattern);

	    } else if (drawmode == pdf_stroke) {
		if (p->pattern[color->val.pattern].painttype == 1) {
		    pdc_puts(p->out, "/Pattern CS");
		} else if (p->pattern[color->val.pattern].painttype == 2) {
		    pdc_printf(p->out, "/CS%d CS ", color->cs);
		    pdf_write_color_values(p,
			&p->cstate[p->sl].stroke, pdf_none);
		}
		pdc_printf(p->out, "/P%d SCN\n", color->val.pattern);
	    }

	    p->pattern[color->val.pattern].used_on_current_page = pdc_true;
	    break;


	case Indexed: /* LATER */
	default:
	    pdc_error(p->pdc, PDF_E_INT_BADCS,
		pdc_errprintf(p->pdc, "%d",
		    p->colorspaces[color->cs].type), 0, 0, 0);
    }
} /* pdf_write_color_values */

static pdc_bool
pdf_color_equal(PDF *p, pdf_color *c1, pdf_color *c2)
{
    pdf_colorspace *cs1;

    cs1 = &p->colorspaces[c1->cs];

    if (c1->cs != c2->cs)
	return pdc_false;

    switch (cs1->type) {
	case DeviceGray:
	    return (c1->val.gray == c2->val.gray);
	    break;

	case DeviceRGB:
	    return (c1->val.rgb.r == c2->val.rgb.r &&
		    c1->val.rgb.g == c2->val.rgb.g &&
		    c1->val.rgb.b == c2->val.rgb.b);
	    break;

	case DeviceCMYK:
	    return (c1->val.cmyk.c == c2->val.cmyk.c &&
		    c1->val.cmyk.m == c2->val.cmyk.m &&
		    c1->val.cmyk.y == c2->val.cmyk.y &&
		    c1->val.cmyk.k == c2->val.cmyk.k);
	    break;

	case Indexed:
	    return (c1->val.idx == c2->val.idx);
	    break;

	case PatternCS:
	    return (c1->val.pattern == c2->val.pattern);
	    break;


	default:
	    break;
    }

    return pdc_true;
} /* pdf_color_equal */

static pdc_bool
pdf_colorspace_equal(PDF *p, pdf_colorspace *cs1, pdf_colorspace *cs2)
{
    if (cs1->type != cs2->type)
	return pdc_false;

    switch (cs1->type) {
	case DeviceGray:
	case DeviceRGB:
	case DeviceCMYK:
	return pdc_true;
	break;


	case Indexed:
	return ((cs1->val.indexed.base == cs2->val.indexed.base) &&
		(cs1->val.indexed.palette_size==cs2->val.indexed.palette_size)&&
		(cs1->val.indexed.colormap_id != PDC_BAD_ID &&
		 cs1->val.indexed.colormap_id == cs2->val.indexed.colormap_id));
	break;

	case PatternCS:
	return (cs1->val.pattern.base == cs2->val.pattern.base);
	break;


	default:
	    pdc_error(p->pdc, PDF_E_INT_BADCS,
		pdc_errprintf(p->pdc, "%d", cs1->type), 0, 0, 0);
    }

    return pdc_false;
} /* pdf_colorspace_equal */

static void
pdf_grow_colorspaces(PDF *p)
{
    int i;

    p->colorspaces = (pdf_colorspace *) pdc_realloc(p->pdc, p->colorspaces,
	sizeof(pdf_colorspace) * 2 * p->colorspaces_capacity,
	"pdf_grow_colorspaces");

    for (i = p->colorspaces_capacity; i < 2 * p->colorspaces_capacity; i++) {
	p->colorspaces[i].used_on_current_page = pdc_false;
    }

    p->colorspaces_capacity *= 2;
}

int
pdf_add_colorspace(PDF *p, pdf_colorspace *cs, pdc_bool inuse)
{
    pdf_colorspace *cs_new;
    static const char fn[] = "pdf_add_colorspace";
    int slot;

    for (slot = 0; slot < p->colorspaces_number; slot++)
    {
	if (pdf_colorspace_equal(p, &p->colorspaces[slot], cs))
	{
	    if (inuse)
		p->colorspaces[slot].used_on_current_page = pdc_true;
	    return slot;
	}
    }

    slot = p->colorspaces_number;

    if (p->colorspaces_number >= p->colorspaces_capacity)
	pdf_grow_colorspaces(p);

    cs_new = &p->colorspaces[slot];

    cs_new->type = cs->type;

    /* don't allocate id for simple color spaces, since we don't write these */
    if (PDF_SIMPLE_COLORSPACE(cs)) {
	cs_new->obj_id = PDC_BAD_ID;
	cs_new->used_on_current_page = pdc_false;

    } else {
	cs_new->obj_id = pdc_alloc_id(p->out);
	cs_new->used_on_current_page = inuse;
    }

    switch (cs_new->type) {
	case DeviceGray:
	case DeviceRGB:
	case DeviceCMYK:
	break;


	case Indexed:
        cs_new->val.indexed.base = cs->val.indexed.base;
        cs_new->val.indexed.palette_size = cs->val.indexed.palette_size;
        cs_new->val.indexed.colormap_id = pdc_alloc_id(p->out);
        cs_new->val.indexed.colormap =
	    (pdf_colormap *) pdc_malloc(p->pdc, sizeof(pdf_colormap), fn);
        memcpy(cs_new->val.indexed.colormap, cs->val.indexed.colormap,
	    sizeof(pdf_colormap));
        cs_new->val.indexed.colormap_done = pdc_false;
	break;

	case PatternCS:
        cs_new->val.pattern.base = cs->val.pattern.base;
	break;


	default:
	    pdc_error(p->pdc, PDF_E_INT_BADCS,
                pdc_errprintf(p->pdc, "%d", cs_new->type), 0, 0, 0);
    }

    p->colorspaces_number++;

    return slot;
} /* pdf_add_colorspace */

void
pdf_init_cstate(PDF *p)
{
    pdf_cstate *cstate;

    cstate = &p->cstate[p->sl];

    cstate->fill.cs		= DeviceGray;
    cstate->fill.val.gray	= (float) 0.0;

    cstate->stroke.cs		= DeviceGray;
    cstate->stroke.val.gray	= (float) 0.0;
}

void
pdf_set_color_values(PDF *p, pdf_color *c, pdf_drawmode drawmode)
{
    pdf_color *current;
    pdf_colorspace *cs;

    cs = &p->colorspaces[c->cs];

    if (drawmode == pdf_stroke || drawmode == pdf_fillstroke) {
	current = &p->cstate[p->sl].stroke;

	if (!pdf_color_equal(p, current, c) || PDF_FORCE_OUTPUT())
	{
	    if (PDF_GET_STATE(p) != pdf_state_document)
		pdf_write_color_values(p, c, pdf_stroke);

	    *current = *c;
	}
    }
    if (drawmode == pdf_fill || drawmode == pdf_fillstroke) {
	current = &p->cstate[p->sl].fill;

	if (!pdf_color_equal(p, current, c) || PDF_FORCE_OUTPUT())
	{
	    if (PDF_GET_STATE(p) != pdf_state_document)
		pdf_write_color_values(p, c, pdf_fill);

	    *current = *c;
	}
    }

    /* simple colorspaces don't get written */
    if (!PDF_SIMPLE_COLORSPACE(cs))
	cs->used_on_current_page = pdc_true;

} /* pdf_set_color_values */

PDFLIB_API void PDFLIB_CALL
PDF_setgray_fill(PDF *p, float g)
{
    static const char fn[] = "PDF_setgray_fill";
    pdf_color c;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, g))
	return;

    pdf_check_color_values(p, DeviceGray, g, (float) 0, (float) 0, (float) 0);
    c.cs = DeviceGray;
    c.val.gray = g;
    pdf_set_color_values(p, &c, pdf_fill);
}

PDFLIB_API void PDFLIB_CALL
PDF_setgray_stroke(PDF *p, float g)
{
    static const char fn[] = "PDF_setgray_stroke";
    pdf_color c;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, g))
	return;

    pdf_check_color_values(p, DeviceGray, g, (float) 0, (float) 0, (float) 0);
    c.cs = DeviceGray;
    c.val.gray = g;
    pdf_set_color_values(p, &c, pdf_stroke);
}

PDFLIB_API void PDFLIB_CALL
PDF_setgray(PDF *p, float g)
{
    static const char fn[] = "PDF_setgray";
    pdf_color c;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, g))
	return;

    pdf_check_color_values(p, DeviceGray, g, (float) 0, (float) 0, (float) 0);
    c.cs = DeviceGray;
    c.val.gray = g;
    pdf_set_color_values(p, &c, pdf_fillstroke);
}

/* RGB functions */

PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor_fill(PDF *p, float r, float g, float b)
{
    static const char fn[] = "PDF_setrgbcolor_fill";
    pdf_color c;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g, %g)\n",
	(void *) p, r, g, b))
	return;

    pdf_check_color_values(p, DeviceRGB, r, g, b, 0);
    c.cs = DeviceRGB;
    c.val.rgb.r = r;
    c.val.rgb.g = g;
    c.val.rgb.b = b;
    pdf_set_color_values(p, &c, pdf_fill);
}

PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor_stroke(PDF *p, float r, float g, float b)
{
    static const char fn[] = "PDF_setrgbcolor_stroke";
    pdf_color c;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g, %g)\n",
	(void *) p, r, g, b))
	return;

    pdf_check_color_values(p, DeviceRGB, r, g, b, 0);
    c.cs = DeviceRGB;
    c.val.rgb.r = r;
    c.val.rgb.g = g;
    c.val.rgb.b = b;
    pdf_set_color_values(p, &c, pdf_stroke);
}

PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor(PDF *p, float r, float g, float b)
{
    static const char fn[] = "PDF_setrgbcolor";
    pdf_color c;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g, %g)\n",
	(void *) p, r, g, b))
	return;

    pdf_check_color_values(p, DeviceRGB, r, g, b, 0);
    c.cs = DeviceRGB;
    c.val.rgb.r = r;
    c.val.rgb.g = g;
    c.val.rgb.b = b;
    pdf_set_color_values(p, &c, pdf_fillstroke);
}

void
pdf_init_colorspaces(PDF *p)
{
    int i, slot;
    pdf_colorspace cs;


    p->colorspaces_number = 0;
    p->colorspaces_capacity = COLORSPACES_CHUNKSIZE;

    p->colorspaces = (pdf_colorspace *)
	pdc_malloc(p->pdc, sizeof(pdf_colorspace) * p->colorspaces_capacity,
	    "pdf_init_colorspaces");

    for (i = 0; i < p->colorspaces_capacity; i++) {
	p->colorspaces[i].used_on_current_page = pdc_false;
    }

    /*
     * Initialize a few slots with simple color spaces for easier use.
     * These can be used without further ado: the slot number is identical
     * to the enum value due to the ordering below.
     */

    cs.type = DeviceGray;
    slot = pdf_add_colorspace(p, &cs, pdc_false);
    cs.type = DeviceRGB;
    slot = pdf_add_colorspace(p, &cs, pdc_false);
    cs.type = DeviceCMYK;
    slot = pdf_add_colorspace(p, &cs, pdc_false);

} /* pdf_init_colorspaces */

void
pdf_write_page_colorspaces(PDF *p)
{
    int i, total = 0;

    for (i = 0; i < p->colorspaces_number; i++)
	if (p->colorspaces[i].used_on_current_page)
	    total++;

    if (total > 0
       ) {
	pdc_puts(p->out, "/ColorSpace");

	pdc_begin_dict(p->out);			/* color space names */

	for (i = 0; i < p->colorspaces_number; i++) {
	    pdf_colorspace *cs = &p->colorspaces[i];

	    if (cs->used_on_current_page) {
		cs->used_on_current_page = pdc_false; /* reset */

		/* don't write simple color spaces as resource */
		if (!PDF_SIMPLE_COLORSPACE(cs))
		    pdc_printf(p->out, "/CS%d %ld 0 R\n", i, cs->obj_id);
	    }
	}


	pdc_end_dict(p->out);			/* color space names */
    }
} /* pdf_write_page_colorspaces */

void
pdf_write_function_dict(PDF *p, pdf_color *c0, pdf_color *c1, float N)
{
    pdf_colorspace *cs;

    cs = &p->colorspaces[c1->cs];

    pdc_begin_dict(p->out);			/* function dict */

    pdc_puts(p->out, "/FunctionType 2\n");
    pdc_puts(p->out, "/Domain[0 1]\n");
    pdc_printf(p->out, "/N %f\n", N);

    switch (cs->type) {

	case DeviceGray:
	pdc_puts(p->out, "/Range[0 1]\n");
	if (c0->val.gray != 0) pdc_printf(p->out, "/C0[%f]\n", c0->val.gray);
	if (c1->val.gray != 1) pdc_printf(p->out, "/C1[%f]", c1->val.gray);
	break;

	case DeviceRGB:
	pdc_puts(p->out, "/Range[0 1 0 1 0 1]\n");
	pdc_printf(p->out, "/C0[%f %f %f]\n",
		c0->val.rgb.r, c0->val.rgb.g, c0->val.rgb.b);
	pdc_printf(p->out, "/C1[%f %f %f]",
		c1->val.rgb.r, c1->val.rgb.g, c1->val.rgb.b);
	break;

	case DeviceCMYK:
	pdc_puts(p->out, "/Range[0 1 0 1 0 1 0 1]\n");
	pdc_printf(p->out, "/C0[%f %f %f %f]\n",
		c0->val.cmyk.c, c0->val.cmyk.m, c0->val.cmyk.y, c0->val.cmyk.k);
	pdc_printf(p->out, "/C1[%f %f %f %f]",
		c1->val.cmyk.c, c1->val.cmyk.m, c1->val.cmyk.y, c1->val.cmyk.k);
	break;


	default:
	pdc_error(p->pdc, PDF_E_INT_BADCS,
	    pdc_errprintf(p->pdc, "%d", cs->type), 0, 0, 0);
    }

    pdc_end_dict(p->out);		/* function dict */
} /* pdf_write_function_dict */

void
pdf_write_colormap(PDF *p, int slot)
{
    PDF_data_source src;
    pdf_colorspace *cs, *base;
    pdc_id length_id;

    cs   = &p->colorspaces[slot];

    if (cs->type != Indexed || cs->val.indexed.colormap_done == pdc_true)
	return;

    base = &p->colorspaces[cs->val.indexed.base];

    pdc_begin_obj(p->out, cs->val.indexed.colormap_id);	/* colormap object */
    pdc_begin_dict(p->out);

    if (p->debug['a'])
	pdc_puts(p->out, "/Filter/ASCIIHexDecode\n");
    else if (pdc_get_compresslevel(p->out))
	pdc_puts(p->out, "/Filter/FlateDecode\n");

    /* Length of colormap object */
    length_id = pdc_alloc_id(p->out);
    pdc_printf(p->out,"/Length %ld 0 R\n", length_id);
    pdc_end_dict(p->out);

    src.init		= NULL;
    src.fill		= pdf_data_source_buf_fill;
    src.terminate	= NULL;

    src.buffer_start	= (unsigned char *) cs->val.indexed.colormap;

    src.buffer_length = (size_t) (cs->val.indexed.palette_size *
				pdf_color_components(p, cs->val.indexed.base));

    src.bytes_available = 0;
    src.next_byte	= NULL;

    /* Write colormap data */
    if (p->debug['a'])
	pdf_ASCIIHexEncode(p, &src);
    else {
	pdf_copy_stream(p, &src, pdc_true);	/* colormap data */
    }

    pdc_end_obj(p->out);				/* colormap object */

    pdc_put_pdfstreamlength(p->out, length_id);

    /* free the colormap now that it's written */
    pdc_free(p->pdc, cs->val.indexed.colormap);
    cs->val.indexed.colormap = NULL;
    cs->val.indexed.colormap_done = pdc_true;
} /* pdf_write_colormap */

void
pdf_write_colorspace(PDF *p, int slot, pdc_bool direct)
{
    pdf_colorspace *cs;
    int base;

    if (slot < 0 || slot >= p->colorspaces_number)
	pdc_error(p->pdc, PDF_E_INT_BADCS,
	    pdc_errprintf(p->pdc, "%d", slot), 0, 0, 0);

    cs = &p->colorspaces[slot];

    /* we always write simple colorspaces directly */
    if (PDF_SIMPLE_COLORSPACE(cs))
	direct = pdc_true;

    if (!direct) {
	pdc_printf(p->out, " %ld 0 R", cs->obj_id);
	return;
    }

    switch (cs->type) {
	case DeviceGray:
	pdc_printf(p->out, "/DeviceGray");
	break;

	case DeviceRGB:
	pdc_printf(p->out, "/DeviceRGB");
	break;

	case DeviceCMYK:
	pdc_printf(p->out, "/DeviceCMYK");
	break;



	case Indexed:
	base = cs->val.indexed.base;

	pdc_puts(p->out, "[/Indexed");

	pdf_write_colorspace(p, base, pdc_false);
	pdc_puts(p->out, " ");

	pdc_printf(p->out, "%d %ld 0 R", cs->val.indexed.palette_size - 1,
	                cs->val.indexed.colormap_id);
	pdc_puts(p->out, "]");
	break;

	case PatternCS:
	pdc_printf(p->out, "[/Pattern");
	pdf_write_colorspace(p, cs->val.pattern.base, pdc_false);
	pdc_puts(p->out, "]");
	break;

	default:
	pdc_error(p->pdc, PDF_E_INT_BADCS,
	    pdc_errprintf(p->pdc, "%d", cs->type), 0, 0, 0);
    }
} /* pdf_write_colorspace */

void
pdf_write_doc_colorspaces(PDF *p)
{
    int i;

    for (i = 0; i < p->colorspaces_number; i++) {
	pdf_colorspace *cs = &p->colorspaces[i];

	/* don't write simple color spaces as resource */
	if (PDF_SIMPLE_COLORSPACE(cs))
	    continue;

	pdc_begin_obj(p->out, cs->obj_id);
	pdf_write_colorspace(p, i, pdc_true);
	pdc_puts(p->out, "\n");
	pdc_end_obj(p->out);			/* color space resource */

	pdf_write_colormap(p, i);		/* write pending colormaps */
    }
}

void
pdf_cleanup_colorspaces(PDF *p)
{
    int i;

    if (!p->colorspaces)
	return;

    for (i = 0; i < p->colorspaces_number; i++) {
	pdf_colorspace *cs = &p->colorspaces[i];;

	switch (cs->type) {
	    case DeviceGray:
	    case DeviceRGB:
	    case DeviceCMYK:
	    case PatternCS:
	    break;

	    case Indexed:
	    if (cs->val.indexed.colormap)
		pdc_free(p->pdc, cs->val.indexed.colormap);
	    break;


	    default:
		pdc_error(p->pdc, PDF_E_INT_BADCS,
		    pdc_errprintf(p->pdc, "%d", cs->type), 0, 0, 0);
	}
    }

    pdc_free(p->pdc, p->colorspaces);
    p->colorspaces = NULL;
}





PDFLIB_API int PDFLIB_CALL
PDF_makespotcolor(PDF *p, const char *spotname, int reserved)
{
    static const char fn[] = "PDF_makespotcolor";
    int slot = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_document),
        "(p[%p], \"%s\", %d)", (void *) p, spotname, reserved))
    {
        PDF_RETURN_HANDLE(p, slot)
    }

    pdc_error(p->pdc, PDF_E_UNSUPP_SPOTCOLOR, 0, 0, 0, 0);

    PDF_RETURN_HANDLE(p, slot)
}



static void
pdf_check_color_values(
    PDF *p,
    pdf_colorspacetype type,
    float c1, float c2, float c3, float c4)
{
    switch (type) {
	case DeviceGray:
	if (c1 < 0.0 || c1 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c1", pdc_errprintf(p->pdc, "%f", c1), 0, 0);
	break;

	case DeviceRGB:
	if (c1 < 0.0 || c1 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c1", pdc_errprintf(p->pdc, "%f", c1), 0, 0);
	if (c2 < 0.0 || c2 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c2", pdc_errprintf(p->pdc, "%f", c2), 0, 0);
	if (c3 < 0.0 || c3 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c3", pdc_errprintf(p->pdc, "%f", c3), 0, 0);

	break;

	case DeviceCMYK:
	if (c1 < 0.0 || c1 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c1", pdc_errprintf(p->pdc, "%f", c1), 0, 0);
	if (c2 < 0.0 || c2 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c2", pdc_errprintf(p->pdc, "%f", c2), 0, 0);
	if (c3 < 0.0 || c3 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c3", pdc_errprintf(p->pdc, "%f", c3), 0, 0);

	if (c4 < 0.0 || c4 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c4", pdc_errprintf(p->pdc, "%f", c4), 0, 0);
	break;



	case PatternCS:
        pdf_check_handle(p, (int) c1, pdc_patternhandle);
	break;

	case Separation:
        pdf_check_handle(p, (int) c1, pdc_colorhandle);
	if (c2 < 0.0 || c2 > EPSILON )
	    pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
		"c2", pdc_errprintf(p->pdc, "%f", c2), 0, 0);
	break;

	case Indexed:
	default:
	    break;
    }
} /* pdf_check_color_values */

static const pdc_keyconn pdf_fstype_keylist[] =
{
    {"stroke",     pdf_stroke},
    {"fill",       pdf_fill},
    {"fillstroke", pdf_stroke | pdf_fill},
    {"both",       pdf_stroke | pdf_fill},
    {NULL, 0}
};

void
pdf__setcolor(
    PDF *p,
    const char *fstype,
    const char *colorspace,
    float c1, float c2, float c3, float c4)
{
    pdf_color c;
    pdf_drawmode drawmode = pdf_none;
    pdf_colorspace cs;
    int k;

    k = pdc_get_keycode_ci(fstype, pdf_fstype_keylist);
    if (k == PDC_KEY_NOTFOUND)
        pdc_error(p->pdc, PDC_E_ILLARG_STRING, "fstype", fstype, 0, 0);
    drawmode = (pdf_drawmode) k;

    k = pdc_get_keycode_ci(colorspace, pdf_colorspace_keylist);
    if (k == PDC_KEY_NOTFOUND)
        pdc_error(p->pdc, PDC_E_ILLARG_STRING, "colorspace", colorspace, 0, 0);
    cs.type = (pdf_colorspacetype) k;

    switch (cs.type) {

        case DeviceGray:
	c.cs = cs.type;
	c.val.gray = c1;
	pdf_check_color_values(p, cs.type, c1, c2, c3, c4);
        break;

        case DeviceRGB:
	c.cs = cs.type;
	c.val.rgb.r = c1;
	c.val.rgb.g = c2;
	c.val.rgb.b = c3;
	pdf_check_color_values(p, cs.type, c1, c2, c3, c4);
        break;

        case DeviceCMYK:
	c.cs = cs.type;
	c.val.cmyk.c = c1;
	c.val.cmyk.m = c2;
	c.val.cmyk.y = c3;
	c.val.cmyk.k = c4;
	pdf_check_color_values(p, cs.type, c1, c2, c3, c4);
        break;


        case PatternCS:

        c.val.pattern = (int) c1;
        PDF_INPUT_HANDLE(p, c.val.pattern)
        pdf_check_color_values(p, cs.type, c1, c2, c3, c4);

        if (p->pattern[c.val.pattern].painttype == 1) {
	    cs.val.pattern.base = pdc_undef;
	    c.cs = pdf_add_colorspace(p, &cs, pdc_false);
	} else {
	    cs.val.pattern.base = p->cstate[p->sl].fill.cs;
	    c.cs = pdf_add_colorspace(p, &cs, pdc_true);
	}
        break;


        default:
        pdc_error(p->pdc, PDC_E_OPT_UNSUPP, colorspace, 0, 0, 0);
    }

    pdf_set_color_values(p, &c, drawmode);
}

PDFLIB_API void PDFLIB_CALL
PDF_setcolor(
    PDF *p,
    const char *fstype,
    const char *colorspace,
    float c1, float c2, float c3, float c4)
{
    static const char fn[] = "PDF_setcolor";
    int legal_states;

    if (PDF_GET_STATE(p) == pdf_state_glyph && !pdf_get_t3colorized(p))
        legal_states = pdf_state_page | pdf_state_pattern | pdf_state_template;
    else
        legal_states = pdf_state_content | pdf_state_document;

    if (!pdf_enter_api(p, fn, (pdf_state) legal_states,
        "(p[%p], \"%s\", \"%s\", %g, %g, %g, %g)\n",
        (void *) p, fstype, colorspace, c1, c2, c3, c4))
        return;

    if (!fstype || !*fstype)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "fstype", 0, 0, 0);

    if (!colorspace || !*colorspace)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "colorspace", 0, 0, 0);

    pdf__setcolor(p, fstype, colorspace, c1, c2, c3, c4);
}
