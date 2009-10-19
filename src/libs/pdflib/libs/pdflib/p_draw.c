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

/* $Id: p_draw.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib drawing routines
 *
 */

#include "p_intern.h"

/* Path segment operators */

void
pdf_begin_path(PDF *p)
{
    if (PDF_GET_STATE(p) == pdf_state_path)
	return;

    pdf_end_text(p);
    p->contents = c_path;
    PDF_PUSH_STATE(p, "pdf_begin_path", pdf_state_path);
}

void
pdf_end_path(PDF *p)
{
    p->contents = c_page;
    PDF_POP_STATE(p, "pdf_end_path");

    p->gstate[p->sl].x = (float) 0.0;
    p->gstate[p->sl].y = (float) 0.0;
}

/* ----------------- Basic functions for API functions --------------*/

void
pdf__moveto(PDF *p, float x, float y)
{
    p->gstate[p->sl].startx = p->gstate[p->sl].x = x;
    p->gstate[p->sl].starty = p->gstate[p->sl].y = y;

    pdf_begin_path(p);
    pdc_printf(p->out, "%f %f m\n", x, y);
}

void
pdf__rmoveto(PDF *p, float x, float y)
{
    float x_0 = p->gstate[p->sl].x;
    float y_0 = p->gstate[p->sl].y;

    pdf__moveto(p, x_0 + x, y_0 + y);
}

void
pdf__lineto(PDF *p, float x, float y)
{
    pdc_printf(p->out, "%f %f l\n", x, y);

    p->gstate[p->sl].x = x;
    p->gstate[p->sl].y = y;
}

void
pdf__rlineto(PDF *p, float x, float y)
{
    float x_0 = p->gstate[p->sl].x;
    float y_0 = p->gstate[p->sl].y;

    pdf__lineto(p, x_0 + x, y_0 + y);
}

void
pdf__curveto(PDF *p,
    float x_1, float y_1,
    float x_2, float y_2,
    float x_3, float y_3)
{
    if (x_2 == x_3 && y_2 == y_3)  /* second c.p. coincides with final point */
        pdc_printf(p->out, "%f %f %f %f y\n", x_1, y_1, x_3, y_3);

    else                        /* general case with four distinct points */
        pdc_printf(p->out, "%f %f %f %f %f %f c\n",
                    x_1, y_1, x_2, y_2, x_3, y_3);

    p->gstate[p->sl].x = x_3;
    p->gstate[p->sl].y = y_3;
}

void
pdf__rcurveto(PDF *p,
    float x_1, float y_1,
    float x_2, float y_2,
    float x_3, float y_3)
{
    float x_0 = p->gstate[p->sl].x;
    float y_0 = p->gstate[p->sl].y;

    pdf__curveto(p, x_0 + x_1, y_0 + y_1,
                    x_0 + x_2, y_0 + y_2,
                    x_0 + x_3, y_0 + y_3);
}

void
pdf__rrcurveto(PDF *p,
    float x_1, float y_1,
    float x_2, float y_2,
    float x_3, float y_3)
{
    pdf__rcurveto(p, x_1, y_1,
                     x_1 + x_2, y_1 + y_2,
                     x_1 + x_2 + x_3, y_1 + y_2 + y_3);
}

void
pdf__hvcurveto(PDF *p, float x_1, float x_2, float y_2, float y_3)
{
    pdf__rrcurveto(p, x_1, 0, x_2, y_2, 0, y_3);
}

void
pdf__vhcurveto(PDF *p, float y_1, float x_2, float y_2, float x_3)
{
    pdf__rrcurveto(p, 0, y_1, x_2, y_2, x_3, 0);
}

void
pdf__rect(PDF *p, float x, float y, float width, float height)
{
    p->gstate[p->sl].startx = p->gstate[p->sl].x = x;
    p->gstate[p->sl].starty = p->gstate[p->sl].y = y;

    pdf_begin_path(p);
    pdc_printf(p->out, "%f %f %f %f re\n", x, y, width, p->ydirection * height);
}

/* 4/3 * (1-cos 45°)/sin 45° = 4/3 * sqrt(2) - 1 */
#define ARC_MAGIC       ((float) 0.552284749)

static void
pdf_short_arc(PDF *p, float x, float y, float r, float alpha, float beta)
{
    float bcp;
    float cos_alpha, cos_beta, sin_alpha, sin_beta;

    alpha = (float) (alpha * PDC_M_PI / 180);
    beta  = (float) (beta * PDC_M_PI / 180);

    /* This formula yields ARC_MAGIC for alpha == 0, beta == 90 degrees */
    bcp = (float) (4.0/3 * (1 - cos((beta - alpha)/2)) / sin((beta - alpha)/2));

    sin_alpha = (float) sin(alpha);
    sin_beta = (float) sin(beta);
    cos_alpha = (float) cos(alpha);
    cos_beta = (float) cos(beta);

    pdf__curveto(p,
                x + r * (cos_alpha - bcp * sin_alpha),          /* p1 */
                y + r * (sin_alpha + bcp * cos_alpha),
                x + r * (cos_beta + bcp * sin_beta),            /* p2 */
                y + r * (sin_beta - bcp * cos_beta),
                x + r * cos_beta,                               /* p3 */
                y + r * sin_beta);
}

static void
pdf_orient_arc(PDF *p, float x, float y, float r, float alpha, float beta,
               float orient)
{
    float rad_a = (float) (alpha * PDC_M_PI / 180);
    float startx = (float) (x + r * cos(rad_a));
    float starty = (float) (y + r * sin(rad_a));

    if (r < 0)
        pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
            "r", pdc_errprintf(p->pdc, "%f", r), 0, 0);

    if (p->contents != c_path)
        pdf__moveto(p, startx, starty);
    else if ((p->gstate[p->sl].x != startx || p->gstate[p->sl].y != starty))
        pdf__lineto(p, startx, starty);

    if (orient > 0)
    {
        while (beta < alpha)
            beta += 360;

        if (alpha == beta)
            return;

        while (beta - alpha > 90)
        {
            pdf_short_arc(p, x, y, r, alpha, alpha + 90);
            alpha += 90;
        }
    }
    else
    {
        while (alpha < beta)
            alpha += 360;

        if (alpha == beta)
            return;

        while (alpha - beta > 90)
        {
            pdf_short_arc(p, x, y, r, alpha, alpha - 90);
            alpha -= 90;
        }
    }

    if (alpha != beta)
        pdf_short_arc(p, x, y, r, alpha, beta);
}

void
pdf__arc(PDF *p, float x, float y, float r, float alpha, float beta)
{
    pdf_orient_arc(p, x, y, r,
                   p->ydirection * alpha, p->ydirection * beta, p->ydirection);
}

void
pdf__arcn(PDF *p, float x, float y, float r, float alpha, float beta)
{
    pdf_orient_arc(p, x, y, r,
                   p->ydirection * alpha, p->ydirection * beta, -p->ydirection);
}

void
pdf__circle(PDF *p, float x, float y, float r)
{
    if (r < 0)
        pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
            "r", pdc_errprintf(p->pdc, "%f", r), 0, 0);

    /*
     * pdf_begin_path() not required since we descend to other
     * path segment functions.
     */

    /* draw four Bezier curves to approximate a circle */
    pdf__moveto(p, x + r, y);
    pdf__curveto(p, x + r, y + r*ARC_MAGIC, x + r*ARC_MAGIC, y + r, x, y + r);
    pdf__curveto(p, x - r*ARC_MAGIC, y + r, x - r, y + r*ARC_MAGIC, x - r, y);
    pdf__curveto(p, x - r, y - r*ARC_MAGIC, x - r*ARC_MAGIC, y - r, x, y - r);
    pdf__curveto(p, x + r*ARC_MAGIC, y - r, x + r, y - r*ARC_MAGIC, x + r, y);

    pdf__closepath(p);
}

void
pdf__closepath(PDF *p)
{
    pdc_puts(p->out, "h\n");

    p->gstate[p->sl].x = p->gstate[p->sl].startx;
    p->gstate[p->sl].y = p->gstate[p->sl].starty;
}

void
pdf__endpath(PDF *p)
{
    pdc_puts(p->out, "n\n");
    pdf_end_path(p);
}

void
pdf__stroke(PDF *p)
{
    pdc_puts(p->out, "S\n");
    pdf_end_path(p);
}

void
pdf__closepath_stroke(PDF *p)
{
    pdc_puts(p->out, "s\n");
    pdf_end_path(p);
}

void
pdf__fill(PDF *p)
{
    if (p->fillrule == pdf_fill_winding)
        pdc_puts(p->out, "f\n");
    else if (p->fillrule == pdf_fill_evenodd)
        pdc_puts(p->out, "f*\n");

    pdf_end_path(p);
}

void
pdf__fill_stroke(PDF *p)
{
    if (p->fillrule == pdf_fill_winding)
        pdc_puts(p->out, "B\n");
    else if (p->fillrule == pdf_fill_evenodd)
        pdc_puts(p->out, "B*\n");

    pdf_end_path(p);
}

void
pdf__closepath_fill_stroke(PDF *p)
{
    if (p->fillrule == pdf_fill_winding)
        pdc_puts(p->out, "b\n");
    else if (p->fillrule == pdf_fill_evenodd)
        pdc_puts(p->out, "b*\n");

    pdf_end_path(p);
}

void
pdf__clip(PDF *p)
{
    if (p->fillrule == pdf_fill_winding)
        pdc_puts(p->out, "W n\n");
    else if (p->fillrule == pdf_fill_evenodd)
        pdc_puts(p->out, "W* n\n");

    pdf_end_path(p);
}

/* --------------------------- API functions ------------------------------*/

PDFLIB_API void PDFLIB_CALL
PDF_moveto(PDF *p, float x, float y)
{
    static const char fn[] = "PDF_moveto";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_path),
	"(p[%p], %g, %g)\n", (void *) p, x, y))
    {
        pdf__moveto(p, x, y);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_rmoveto(PDF *p, float x, float y)
{
    static const char fn[] = "PDF_rmoveto";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_path),
	"(p[%p], %g, %g)\n", (void *) p, x, y))
    {
        pdf__rmoveto(p, x, y);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_lineto(PDF *p, float x, float y)
{
    static const char fn[] = "PDF_lineto";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p], %g, %g)\n",
	(void *) p, x, y))
    {
        pdf__lineto(p, x, y);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_rlineto(PDF *p, float x, float y)
{
    static const char fn[] = "PDF_rlineto";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p], %g, %g)\n",
        (void *) p, x, y))
    {
        pdf__rlineto(p, x, y);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_curveto(PDF *p,
    float x_1, float y_1, float x_2, float y_2, float x_3, float y_3)
{
    static const char fn[] = "PDF_curveto";

    if (pdf_enter_api(p, fn, pdf_state_path,
	"(p[%p], %g, %g, %g, %g, %g, %g)\n",
	(void *) p, x_1, y_1, x_2, y_2, x_3, y_3))
    {
        pdf__curveto(p, x_1, y_1, x_2, y_2, x_3, y_3);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_rcurveto(PDF *p,
    float x_1, float y_1, float x_2, float y_2, float x_3, float y_3)
{
    static const char fn[] = "PDF_rcurveto";

    if (pdf_enter_api(p, fn, pdf_state_path,
	"(p[%p], %g, %g, %g, %g, %g, %g)\n",
	(void *) p, x_1, y_1, x_2, y_2, x_3, y_3))
    {
        pdf__rcurveto(p, x_1, y_1, x_2, y_2, x_3, y_3);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_rect(PDF *p, float x, float y, float width, float height)
{
    static const char fn[] = "PDF_rect";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_path),
        "(p[%p], %g, %g, %g, %g)\n", (void *) p, x, y, width, height))
    {
        pdf__rect(p, x, y, width, height);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_arc(PDF *p, float x, float y, float r, float alpha, float beta)
{
    static const char fn[] = "PDF_arc";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_path),
	"(p[%p], %g, %g, %g, %g, %g)\n", (void *) p, x, y, r, alpha, beta))
    {
	pdf__arc(p, x, y, r, alpha, beta);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_arcn(PDF *p, float x, float y, float r, float alpha, float beta)
{
    static const char fn[] = "PDF_arcn";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_path),
	"(p[%p], %g, %g, %g, %g, %g)\n", (void *) p, x, y, r, alpha, beta))
    {
	pdf__arcn(p, x, y, r, alpha, beta);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_circle(PDF *p, float x, float y, float r)
{
    static const char fn[] = "PDF_circle";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_path),
	"(p[%p], %g, %g, %g)\n", (void *) p, x, y, r))
    {
        pdf__circle(p, x, y, r);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_closepath(PDF *p)
{
    static const char fn[] = "PDF_closepath";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__closepath(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_endpath(PDF *p)
{
    static const char fn[] = "PDF_endpath";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__endpath(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_stroke(PDF *p)
{
    static const char fn[] = "PDF_stroke";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__stroke(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_closepath_stroke(PDF *p)
{
    static const char fn[] = "PDF_closepath_stroke";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__closepath_stroke(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_fill(PDF *p)
{
    static const char fn[] = "PDF_fill";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__fill(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_fill_stroke(PDF *p)
{
    static const char fn[] = "PDF_fill_stroke";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__fill_stroke(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_closepath_fill_stroke(PDF *p)
{
    static const char fn[] = "PDF_closepath_fill_stroke";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__closepath_fill_stroke(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_clip(PDF *p)
{
    static const char fn[] = "PDF_clip";

    if (pdf_enter_api(p, fn, pdf_state_path, "(p[%p])\n", (void *) p))
    {
        pdf__clip(p);
    }
}
