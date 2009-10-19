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

/* $Id: p_gstate.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib routines dealing with the graphics states
 *
 */

#include "p_intern.h"

/* ---------------------- matrix functions ----------------------------- */

void
pdf_concat_raw(PDF *p, pdc_matrix *m)
{
    if (pdc_is_identity_matrix(m))
	return;

    pdf_end_text(p);

    pdc_printf(p->out, "%f %f %f %f %f %f cm\n",
		    m->a, m->b, m->c, m->d, m->e, m->f);

    pdc_multiply_matrix(m, &p->gstate[p->sl].ctm);
}

void
pdf_concat_raw_ob(PDF *p, pdc_matrix *m, pdc_bool blind)
{
    if (!blind)
        pdf_concat_raw(p, m);
    else
        pdc_multiply_matrix(m, &p->gstate[p->sl].ctm);
}

void
pdf_set_topdownsystem(PDF *p, float height)
{
    if (p->ydirection < (float) 0.0)
    {
        pdc_matrix m;
        pdc_translation_matrix(0, height, &m);
        pdf_concat_raw(p, &m);
        pdc_scale_matrix(1, -1, &m);
        pdf_concat_raw(p, &m);
        pdf_set_horiz_scaling(p, 100);
    }
}

/* -------------------- Special graphics state ---------------------------- */

void
pdf_init_gstate(PDF *p)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    gs->ctm.a = (float) 1;
    gs->ctm.b = (float) 0;
    gs->ctm.c = (float) 0;
    gs->ctm.d = (float) 1;
    gs->ctm.e = (float) 0.0;
    gs->ctm.f = (float) 0.0;

    gs->x = (float) 0.0;
    gs->y = (float) 0.0;

    p->fillrule = pdf_fill_winding;

    gs->lwidth = (float) 1;
    gs->lcap = 0;
    gs->ljoin = 0;
    gs->miter = (float) 10;
    gs->flatness = (float) -1;	/* -1 means "has not been set" */
    gs->dashed = pdc_false;
}

void
pdf__save(PDF *p)
{
    if (p->sl == PDF_MAX_SAVE_LEVEL - 1)
	pdc_error(p->pdc, PDF_E_GSTATE_SAVELEVEL,
	    pdc_errprintf(p->pdc, "%d", PDF_MAX_SAVE_LEVEL - 1), 0, 0, 0);

    pdf_end_text(p);

    pdc_puts(p->out, "q\n");

    /* propagate states to next level */
    p->sl++;
    memcpy(&p->gstate[p->sl], &p->gstate[p->sl - 1], sizeof(pdf_gstate));
    memcpy(&p->tstate[p->sl], &p->tstate[p->sl - 1], sizeof(pdf_tstate));
    memcpy(&p->cstate[p->sl], &p->cstate[p->sl - 1], sizeof(pdf_cstate));
}


void
pdf__restore(PDF *p)
{
    if (p->sl == 0)
	pdc_error(p->pdc, PDF_E_GSTATE_RESTORE, 0, 0, 0, 0);

    pdf_end_text(p);

    pdc_puts(p->out, "Q\n");

    p->sl--;
}

PDFLIB_API void PDFLIB_CALL
PDF_save(PDF *p)
{
    static const char fn[] = "PDF_save";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p])\n", (void *) p))
	return;

    pdf__save(p);
}

PDFLIB_API void PDFLIB_CALL
PDF_restore(PDF *p)
{
    static const char fn[] = "PDF_restore";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p])\n", (void *) p))
	return;

    pdf__restore(p);
}

PDFLIB_API void PDFLIB_CALL
PDF_translate(PDF *p, float tx, float ty)
{
    static const char fn[] = "PDF_translate";
    pdc_matrix m;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g)\n",
	(void *) p, tx, ty))
	return;

    if (tx == (float) 0 && ty == (float) 0)
	return;

    pdc_translation_matrix(tx, ty, &m);

    pdf_concat_raw(p, &m);
}

PDFLIB_API void PDFLIB_CALL
PDF_scale(PDF *p, float sx, float sy)
{
    static const char fn[] = "PDF_scale";
    pdc_matrix m;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g)\n",
	(void *) p, sx, sy))
	return;

    if (sx == (float) 0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT, "sx", "0", 0, 0);

    if (sy == (float) 0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT, "sy", "0", 0, 0);

    if (sx == (float) 1 && sy == (float) 1)
	return;

    pdc_scale_matrix(sx, sy, &m);

    pdf_concat_raw(p, &m);
}

PDFLIB_API void PDFLIB_CALL
PDF_rotate(PDF *p, float phi)
{
    static const char fn[] = "PDF_rotate";
    pdc_matrix m;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, phi))
	return;

    if (phi == (float) 0)
	return;

    pdc_rotation_matrix(p->ydirection * phi, &m);

    pdf_concat_raw(p, &m);
}

PDFLIB_API void PDFLIB_CALL
PDF_skew(PDF *p, float alpha, float beta)
{
    static const char fn[] = "PDF_skew";
    pdc_matrix m;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g)\n",
	(void *) p, alpha, beta))
	return;

    if (alpha == (float) 0 && beta == (float) 0)
	return;

    if (alpha > (float) 360 || alpha < (float) -360 ||
	alpha == (float) -90 || alpha == (float) -270 ||
	alpha == (float) 90 || alpha == (float) 270) {
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "alpha", pdc_errprintf(p->pdc, "%f", alpha), 0, 0);
    }

    if (beta > (float) 360 || beta < (float) -360 ||
	beta == (float) -90 || beta == (float) -270 ||
	beta == (float) 90 || beta == (float) 270) {
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "beta", pdc_errprintf(p->pdc, "%f", beta), 0, 0);
    }

    pdc_skew_matrix(p->ydirection * alpha, p->ydirection * beta, &m);

    pdf_concat_raw(p, &m);
}

PDFLIB_API void PDFLIB_CALL
PDF_concat(PDF *p, float a, float b, float c, float d, float e, float f)
{
    static const char fn[] = "PDF_concat";
    pdc_matrix m;
    float det = a * d - b * c;

    if (!pdf_enter_api(p, fn, pdf_state_content,
	"(p[%p], %g, %g, %g, %g, %g, %g)\n", (void *) p, a, b, c, d, e, f))
    {
	return;
    }

    if (fabs(det) < (float) PDF_SMALLREAL)
	pdc_error(p->pdc, PDC_E_ILLARG_MATRIX,
	    pdc_errprintf(p->pdc, "%f %f %f %f %f %f", a, b, c, d, e, f),
	    0, 0, 0);

    m.a = (float) a;
    m.b = (float) b;
    m.c = (float) c;
    m.d = (float) d;
    m.e = (float) e;
    m.f = (float) f;

    pdf_concat_raw(p, &m);
}

void
pdf__setmatrix(PDF *p, pdc_matrix *n)
{
    pdc_matrix m;
    float det = n->a * n->d - n->b * n->c;

    if (fabs(det) < (float) PDF_SMALLREAL)
	pdc_error(p->pdc, PDC_E_ILLARG_MATRIX,
	    pdc_errprintf(p->pdc, "%f %f %f %f %f %f",
		n->a, n->b, n->c, n->d, n->e, n->f),
	    0, 0, 0);

    pdc_invert_matrix(p->pdc, &m, &p->gstate[p->sl].ctm);
    pdf_concat_raw(p, &m);
    pdf_concat_raw(p, n);
}

PDFLIB_API void PDFLIB_CALL
PDF_setmatrix(PDF *p, float a, float b, float c, float d, float e, float f)
{
    static const char fn[] = "PDF_setmatrix";
    pdc_matrix m;
    float det = a * d - b * c;

    if (!pdf_enter_api(p, fn, pdf_state_content,
	"(p[%p], %g, %g, %g, %g, %g, %g)\n", (void *) p, a, b, c, d, e, f))
    {
	return;
    }

    if (fabs(det) < (float) PDF_SMALLREAL)
	pdc_error(p->pdc, PDC_E_ILLARG_MATRIX,
	    pdc_errprintf(p->pdc, "%f %f %f %f %f %f", a, b, c, d, e, f),
	    0, 0, 0);

    pdc_invert_matrix(p->pdc, &m, &p->gstate[p->sl].ctm);
    pdf_concat_raw(p, &m);

    m.a = (float) a;
    m.b = (float) b;
    m.c = (float) c;
    m.d = (float) d;
    m.e = (float) e;
    m.f = (float) f;

    pdf_concat_raw(p, &m);
}

/* -------------------- General graphics state ---------------------------- */

/* definitions of dash options */
static const pdc_defopt pdf_dashoptions[] =
{
    {"dasharray", pdc_floatlist, 0, 2, MAX_DASH_LENGTH,
      PDC_FLOAT_PREC, PDC_FLOAT_MAX, NULL},

    {"dashphase", pdc_floatlist, 0, 1, 1, 0.0, PDC_FLOAT_MAX, NULL},

    PDC_OPT_TERMINATE
};

static void
pdf__setdashpattern(PDF *p, float *darray, int length, float phase)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    /* length == 0 or 1 means solid line */
    if (length < 2)
    {
        if (gs->dashed || PDF_FORCE_OUTPUT())
        {
            pdc_puts(p->out, "[] 0 d\n");
            gs->dashed = pdc_false;
        }
    }
    else
    {
        int i;

        pdc_puts(p->out, "[");
        for (i = 0; i < length; i++)
        {
            pdc_printf(p->out, "%f ", darray[i]);
        }
        pdc_printf(p->out, "] %f d\n", phase);
        gs->dashed = pdc_true;
    }
}

void
pdf__setdash(PDF *p, float b, float w)
{
    float darray[2];
    int length = 2;

     /* both zero means solid line */
    if (b == 0.0 && w == 0.0)
    {
        length = 0;
    }
    else
    {
        darray[0] = b;
        darray[1] = w;
    }
    pdf__setdashpattern(p, darray, length, (float) 0.0);
}

void
pdf__setflat(PDF *p, float flat)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    if (flat != gs->flatness || PDF_FORCE_OUTPUT())
    {
	gs->flatness = flat;
	pdc_printf(p->out, "%f i\n", flat);
    }
}

void
pdf__setlinejoin(PDF *p, int join)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    if (join != gs->ljoin || PDF_FORCE_OUTPUT())
    {
	gs->ljoin = join;
	pdc_printf(p->out, "%d j\n", join);
    }
}

void
pdf__setlinecap(PDF *p, int cap)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    if (cap != gs->lcap || PDF_FORCE_OUTPUT())
    {
	gs->lcap = cap;
	pdc_printf(p->out, "%d J\n", cap);
    }
}

void
pdf__setlinewidth(PDF *p, float width)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    if (width != gs->lwidth || PDF_FORCE_OUTPUT())
    {
	gs->lwidth = width;
	pdc_printf(p->out, "%f w\n", width);
    }
}

void
pdf__setmiterlimit(PDF *p, float miter)
{
    pdf_gstate *gs = &p->gstate[p->sl];

    if (miter != gs->miter || PDF_FORCE_OUTPUT())
    {
	gs->miter = miter;
	pdc_printf(p->out, "%f M\n", miter);
    }
}


PDFLIB_API void PDFLIB_CALL
PDF_setdash(PDF *p, float b, float w)
{
    static const char fn[] = "PDF_setdash";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g)\n",
	(void *) p, b, w))
	return;

    if (b < (float) 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "b", pdc_errprintf(p->pdc, "%f", b), 0, 0);

    if (w < (float) 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "w", pdc_errprintf(p->pdc, "%f", w), 0, 0);

    pdf__setdash(p, b, w);
}

PDFLIB_API void PDFLIB_CALL
PDF_setpolydash(PDF *p, float *darray, int length)
{
    static const char fn[] = "PDF_setpolydash";

    int i;

    for (i = 0; i < length; i++)
	pdc_trace(p->pdc, "*(darray+%d) = %g;\n", i, darray[i]);

    if (!pdf_enter_api(p, fn, pdf_state_content,
	"(p[%p], darray[%p], %d)\n", (void *) p, (void *) darray, length))
    {
	return;
    }

    if (length > 1)
    {
        /* sanity checks */
        if (!darray)
            pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "darray", 0, 0, 0);

        if (length < 0 || length > MAX_DASH_LENGTH)
            pdc_error(p->pdc, PDC_E_ILLARG_INT,
                "length", pdc_errprintf(p->pdc, "%d", length), 0, 0);

        for (i = 0; i < length; i++) {
            if (darray[i] < (float) 0.0)
                pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
                    "darray[i]", pdc_errprintf(p->pdc, "%f", darray[i]), 0, 0);
        }
    }

    pdf__setdashpattern(p, darray, length, (float) 0.0);
}

PDFLIB_API void PDFLIB_CALL
PDF_setdashpattern(PDF *p, const char *optlist)
{
    static const char fn[] = "PDF_setdashpattern";
    pdc_resopt *results;
    float *darray, phase;
    int length;

    if (!pdf_enter_api(p, fn, pdf_state_content,
        "(p[%p], \"%s\")\n", (void *) p, optlist))
        return;

    /* parsing optlist */
    results = pdc_parse_optionlist(p->pdc, optlist, pdf_dashoptions, NULL,
                                   pdc_true);

    length = pdc_get_optvalues(p->pdc, "dasharray", results,
                               NULL, (void **) &darray);

    phase = (float) 0.0;
    (void) pdc_get_optvalues(p->pdc, "dashphase", results, &phase, NULL);

    pdc_cleanup_optionlist(p->pdc, results);

    pdf__setdashpattern(p, darray, length, phase);

    if (darray)
        pdc_free(p->pdc, darray);
}

PDFLIB_API void PDFLIB_CALL
PDF_setflat(PDF *p, float flat)
{
    static const char fn[] = "PDF_setflat";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, flat))
	return;

    if (flat < 0.0 || flat > 100.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "flat", pdc_errprintf(p->pdc, "%f", flat), 0, 0);

    pdf__setflat(p, flat);
}


PDFLIB_API void PDFLIB_CALL
PDF_setlinejoin(PDF *p, int join)
{
    static const char fn[] = "PDF_setlinejoin";
    const int LAST_JOIN = 2;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %d)\n",
	(void *) p, join))
	return;

    if (join < 0 || join > LAST_JOIN)
	pdc_error(p->pdc, PDC_E_ILLARG_INT,
	    "join", pdc_errprintf(p->pdc, "%d", join), 0, 0);

    pdf__setlinejoin(p, join);
}


PDFLIB_API void PDFLIB_CALL
PDF_setlinecap(PDF *p, int cap)
{
    static const char fn[] = "PDF_setlinecap";
    const int LAST_CAP = 2;

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %d)\n",
	(void *) p, cap))
	return;

    if (cap < 0 || cap > LAST_CAP)
	pdc_error(p->pdc, PDC_E_ILLARG_INT,
	    "cap", pdc_errprintf(p->pdc, "%d", cap), 0, 0);

    pdf__setlinecap(p, cap);
}

PDFLIB_API void PDFLIB_CALL
PDF_setmiterlimit(PDF *p, float miter)
{
    static const char fn[] = "PDF_setmiterlimit";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, miter))
	return;

    if (miter < (float) 1.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "miter", pdc_errprintf(p->pdc, "%f", miter), 0, 0);

    pdf__setmiterlimit(p, miter);
}

PDFLIB_API void PDFLIB_CALL
PDF_setlinewidth(PDF *p, float width)
{
    static const char fn[] = "PDF_setlinewidth";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g)\n",
	(void *) p, width))
	return;

    if (width <= (float) 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "width", pdc_errprintf(p->pdc, "%f", width), 0, 0);

    pdf__setlinewidth(p, width);
}

/* reset all gstate parameters except CTM
*/
void
pdf_reset_gstate(PDF *p)
{
    pdf_gstate *gs = &p->gstate[p->sl];


    pdf__setcolor(p, "fillstroke", "gray",
	(float) 0, (float) 0, (float) 0, (float) 0);


    pdf__setlinewidth(p, 1);
    pdf__setlinecap(p, 0);
    pdf__setlinejoin(p, 0);
    pdf__setmiterlimit(p, 10);
    pdf__setdash(p, 0, 0);

    if (gs->flatness != (float) -1)
	pdf__setflat(p, (float) 1.0);
}

void
pdf__initgraphics(PDF *p)
{
    pdc_matrix inv_ctm;

    pdf_reset_gstate(p);

    pdc_invert_matrix(p->pdc, &inv_ctm, &p->gstate[p->sl].ctm);
    pdf_concat_raw(p, &inv_ctm);

    /* This also resets the CTM which guards against rounding artifacts. */
    pdf_init_gstate(p);
}

PDFLIB_API void PDFLIB_CALL
PDF_initgraphics(PDF *p)
{
    static const char fn[] = "PDF_initgraphics";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p])\n", (void *) p))
	return;

    pdf__initgraphics(p);
}
