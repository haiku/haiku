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

/* $Id: p_shading.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib routines for smooth shading
 *
 */

#include "p_intern.h"
#include "p_font.h"

typedef enum
{
    shnone	= 0,
    axial	= 2,
    radial	= 3
} pdf_shadingtype_e;

struct pdf_shading_s {
    pdc_id	obj_id;			/* object id of this shading */
    pdc_bool	used_on_current_page;	/* this shading used on current page */
};

void
pdf_init_shadings(PDF *p)
{
    int i;

    p->shadings_number = 0;
    p->shadings_capacity = SHADINGS_CHUNKSIZE;

    p->shadings = (pdf_shading *) pdc_malloc(p->pdc,
	sizeof(pdf_shading) * p->shadings_capacity, "pdf_init_shadings");

    for (i = 0; i < p->shadings_capacity; i++) {
	p->shadings[i].used_on_current_page = pdc_false;
	p->shadings[i].obj_id = PDC_BAD_ID;
    }
}

static void
pdf_grow_shadings(PDF *p)
{
    int i;

    p->shadings = (pdf_shading *) pdc_realloc(p->pdc, p->shadings,
	sizeof(pdf_shading) * 2 * p->shadings_capacity, "pdf_grow_shadings");

    for (i = p->shadings_capacity; i < 2 * p->shadings_capacity; i++) {
	p->shadings[i].used_on_current_page = pdc_false;
	p->shadings[i].obj_id = PDC_BAD_ID;
    }

    p->shadings_capacity *= 2;
}

void
pdf_write_page_shadings(PDF *p)
{
    int i, total = 0;

    for (i = 0; i < p->shadings_number; i++)
	if (p->shadings[i].used_on_current_page)
	    total++;

    if (total > 0) {
	pdc_puts(p->out, "/Shading");

	pdc_begin_dict(p->out);			/* Shading */

	for (i = 0; i < p->shadings_number; i++) {
	    if (p->shadings[i].used_on_current_page) {
		p->shadings[i].used_on_current_page = pdc_false; /* reset */
		pdc_printf(p->out, "/Sh%d %ld 0 R\n", i, p->shadings[i].obj_id);
	    }
	}

	pdc_end_dict(p->out);			/* Shading */
    }
}

void
pdf_cleanup_shadings(PDF *p)
{
    if (p->shadings) {
	pdc_free(p->pdc, p->shadings);
	p->shadings = NULL;
    }
}

static const pdc_defopt pdf_shading_pattern_options[] =
{
    {"gstate", pdc_gstatehandle, 0, 1, 1, 0, 0, NULL},
    PDC_OPT_TERMINATE
};

PDFLIB_API int PDFLIB_CALL
PDF_shading_pattern(PDF *p, int shading, const char *optlist)
{
    static const char fn[] = "PDF_shading_pattern";
    pdc_resopt *results;
    pdc_clientdata data;
    int gstate = -1;
    int retval = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page | pdf_state_font),
	"(p[%p], %d, \"%s\")", (void *) p, shading, optlist)) {
        PDF_RETURN_HANDLE(p, retval)
    }

    if (p->compatibility == PDC_1_3)
	pdc_error(p->pdc, PDF_E_SHADING13, 0, 0, 0, 0);

    PDF_INPUT_HANDLE(p, shading)
    pdf_check_handle(p, shading, pdc_shadinghandle);

    if (optlist && strlen(optlist)) {
        data.maxgstate = p->extgstates_number - 1;
        data.hastobepos = p->hastobepos;

	results = pdc_parse_optionlist(p->pdc,
            optlist, pdf_shading_pattern_options, &data, pdc_true);

	(void) pdc_get_optvalues(p->pdc, "gstate", results, &gstate, NULL);

	pdc_cleanup_optionlist(p->pdc, results);
    }

    if (p->pattern_number == p->pattern_capacity)
	pdf_grow_pattern(p);

    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_end_contents_section(p);

    							/* Pattern object */
    p->pattern[p->pattern_number].obj_id = pdc_begin_obj(p->out, PDC_NEW_ID);

    /* Shadings don't have a painttype, but this signals to the
     * code which writes the pattern usage that no color values
     * will be required when setting the pattern color space.
     */
    p->pattern[p->pattern_number].painttype = 1;

    pdc_begin_dict(p->out);				/* Pattern dict*/

    pdc_puts(p->out, "/PatternType 2\n");		/* shading pattern */

    pdc_printf(p->out, "/Shading %ld 0 R\n", p->shadings[shading].obj_id);

    p->shadings[shading].used_on_current_page = pdc_true;

    if (gstate != -1) {
	pdc_printf(p->out, "/ExtGState %ld 0 R\n",
	    pdf_get_gstate_id(p, gstate));
    }

    pdc_end_dict(p->out);				/* Pattern dict*/
    pdc_end_obj(p->out);				/* Pattern object */

    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_begin_contents_section(p);

    retval = p->pattern_number;
    p->pattern_number++;
    PDF_RETURN_HANDLE(p, retval)
}

PDFLIB_API void PDFLIB_CALL
PDF_shfill(PDF *p, int shading)
{
    static const char fn[] = "PDF_shfill";
    int legal_states;

    if (PDF_GET_STATE(p) == pdf_state_glyph && !pdf_get_t3colorized(p))
        legal_states = pdf_state_page | pdf_state_pattern | pdf_state_template;

    else if (PDF_GET_STATE(p) == pdf_state_pattern &&
	p->pattern[p->pattern_number-1].painttype == 2)
        legal_states = pdf_state_page | pdf_state_glyph | pdf_state_template;

    else
        legal_states = pdf_state_content;

    if (!pdf_enter_api(p, fn, (pdf_state) legal_states,
	"(p[%p], %d)\n", (void *) p, shading))
	return;

    if (p->compatibility == PDC_1_3)
	pdc_error(p->pdc, PDF_E_SHADING13, 0, 0, 0, 0);

    PDF_INPUT_HANDLE(p, shading)
    pdf_check_handle(p, shading, pdc_shadinghandle);

    if (PDF_GET_STATE(p) == pdf_state_path)
	pdf_end_path(p);

    pdf_end_text(p);

    pdc_printf(p->out, "/Sh%d sh\n", shading);

    p->shadings[shading].used_on_current_page = pdc_true;
}

static const pdc_defopt pdf_shading_options[] =
{
    {"N", pdc_floatlist, PDC_OPT_NOZERO, 1, 1, 0, PDC_FLOAT_MAX, NULL},
    {"r0", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0, PDC_FLOAT_MAX, NULL},
    {"r1", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0, PDC_FLOAT_MAX, NULL},
    {"extend0", pdc_booleanlist, PDC_OPT_NONE, 0, 1, 0, 1, NULL},
    {"extend1", pdc_booleanlist, PDC_OPT_NONE, 0, 1, 0, 0, NULL},
    {"antialias", pdc_booleanlist, PDC_OPT_NONE, 0, 1, 0, 0, NULL},
    PDC_OPT_TERMINATE
};

PDFLIB_API int PDFLIB_CALL
PDF_shading(
    PDF *p,
    const char *type,
    float x_0, float y_0,
    float x_1, float y_1,
    float c_1, float c_2, float c_3, float c_4,
    const char *optlist)
{
    static const char fn[] = "PDF_shading";
    pdf_shadingtype_e shtype = shnone;
    pdf_color *color0, color1;
    pdf_colorspace *cs;
    pdc_resopt *results;
    float N = (float) 1.0;
    float r_0, r_1;
    pdc_bool extend0 = pdc_false, extend1 = pdc_false, antialias = pdc_false;
    int retval = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page | pdf_state_font),
	"(p[%p], \"%s\", \"%s\")",
	(void *) p, type, optlist))
    {
        PDF_RETURN_HANDLE(p, retval)
    }

    if (p->compatibility == PDC_1_3)
	pdc_error(p->pdc, PDF_E_SHADING13, 0, 0, 0, 0);

    if (!strcmp(type, "axial")) {
	shtype = axial;

    } else if (!strcmp(type, "radial")) {
	shtype = radial;

    } else
	pdc_error(p->pdc, PDC_E_ILLARG_STRING, "type", type, 0, 0);

    color0 = &p->cstate[p->sl].fill;

    color1.cs = color0->cs;

    cs = &p->colorspaces[color0->cs];

    switch (cs->type) {
	case DeviceGray:
	color1.val.gray = c_1;
	break;

	case DeviceRGB:
	color1.val.rgb.r = c_1;
	color1.val.rgb.g = c_2;
	color1.val.rgb.b = c_3;
	break;

	case DeviceCMYK:
	color1.val.cmyk.c = c_1;
	color1.val.cmyk.m = c_2;
	color1.val.cmyk.y = c_3;
	color1.val.cmyk.k = c_4;
	break;



	default:
	pdc_error(p->pdc, PDF_E_INT_BADCS,
	    pdc_errprintf(p->pdc, "%d", color0->cs), 0, 0, 0);
    }

    if (optlist && strlen(optlist)) {
	results = pdc_parse_optionlist(p->pdc,
            optlist, pdf_shading_options, NULL, pdc_true);

	(void) pdc_get_optvalues(p->pdc, "N", results, &N, NULL);

	(void) pdc_get_optvalues(p->pdc, "antialias", results, &antialias,NULL);

	if (shtype == radial) {
	    if (pdc_get_optvalues(p->pdc, "r0", results, &r_0, NULL) != 1)
		pdc_error(p->pdc, PDC_E_OPT_NOTFOUND, "r0", 0, 0, 0);

	    if (pdc_get_optvalues(p->pdc, "r1", results, &r_1, NULL) != 1)
		pdc_error(p->pdc, PDC_E_OPT_NOTFOUND, "r1", 0, 0, 0);
	}

	if (shtype == axial) {
	    if (pdc_get_optvalues(p->pdc, "r0", results, &r_0, NULL) == 1)
		pdc_warning(p->pdc, PDC_E_OPT_IGNORED, "r0", 0, 0, 0);

	    if (pdc_get_optvalues(p->pdc, "r1", results, &r_1, NULL) == 1)
		pdc_warning(p->pdc, PDC_E_OPT_IGNORED, "r1", 0, 0, 0);
	}

	if (shtype == radial || shtype == axial) {
	    pdc_get_optvalues(p->pdc, "extend0", results, &extend0, NULL);
	    pdc_get_optvalues(p->pdc, "extend1", results, &extend1, NULL);
	}

	pdc_cleanup_optionlist(p->pdc, results);
    }

    if (p->shadings_number == p->shadings_capacity)
	pdf_grow_shadings(p);

    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_end_contents_section(p);

    							/* Shading object */
    p->shadings[p->shadings_number].obj_id = pdc_begin_obj(p->out, PDC_NEW_ID);

    pdc_begin_dict(p->out);				/* Shading dict*/

    pdc_printf(p->out, "/ShadingType %d\n", (int) shtype);

    pdc_printf(p->out, "/ColorSpace");
    pdf_write_colorspace(p, color1.cs, pdc_false);
    pdc_puts(p->out, "\n");

    if (antialias)
	pdc_printf(p->out, "/AntiAlias true\n");

    switch (shtype) {
	case axial:	/* Type 2 */
	pdc_printf(p->out, "/Coords[%f %f %f %f]\n", x_0, y_0, x_1, y_1);
	if (extend0 || extend1)
	    pdc_printf(p->out, "/Extend[%s %s]\n",
		extend0 ? "true" : "false", extend1 ? "true" : "false");
	pdc_puts(p->out, "/Function");
	pdf_write_function_dict(p, color0, &color1, N);
	break;

	case radial:	/* Type 3 */
	pdc_printf(p->out, "/Coords[%f %f %f %f %f %f]\n",
	    x_0, y_0, r_0, x_1, y_1, r_1);
	if (extend0 || extend1)
	    pdc_printf(p->out, "/Extend[%s %s]\n",
		extend0 ? "true" : "false", extend1 ? "true" : "false");
	pdc_puts(p->out, "/Function");
	pdf_write_function_dict(p, color0, &color1, N);
	break;

	default:
	break;
    }

    pdc_end_dict(p->out);				/* Shading dict */
    pdc_end_obj(p->out);				/* Shading object */

    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_begin_contents_section(p);

    retval = p->shadings_number;
    p->shadings_number++;
    PDF_RETURN_HANDLE(p, retval)
}
