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

/* $Id: pc_geom.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Various geometry routines
 *
 */

#include "pc_util.h"
#include "pc_geom.h"


/* ---------------------- matrix functions ----------------------------- */

pdc_bool
pdc_is_identity_matrix(pdc_matrix *m)
{
    return (m->a == (float) 1 &&
            m->b == (float) 0 &&
            m->c == (float) 0 &&
            m->d == (float) 1 &&
            m->e == (float) 0 &&
            m->f == (float) 0);
}

/* translation matrix */
void
pdc_translation_matrix(float tx, float ty, pdc_matrix *M)
{
    M->a = (float) 1;
    M->b = (float) 0;
    M->c = (float) 0;
    M->d = (float) 1;
    M->e = tx;
    M->f = ty;
}

/* scale matrix */
void
pdc_scale_matrix(float sx, float sy, pdc_matrix *M)
{
    M->a = sx;
    M->b = (float) 0;
    M->c = (float) 0;
    M->d = sy;
    M->e = (float) 0;
    M->f = (float) 0;
}

/* rotation matrix */
void
pdc_rotation_matrix(float alpha, pdc_matrix *M)
{
    double phi, c, s;

    phi = (alpha * PDC_M_PI) / 180.0;
    c = cos(phi);
    s = sin(phi);

    M->a = (float) c;
    M->b = (float) s;
    M->c = (float) -s;
    M->d = (float) c;
    M->e = (float) 0;
    M->f = (float) 0;
}

/* skew matrix */
void
pdc_skew_matrix(float alpha, float beta, pdc_matrix *M)
{
    M->a = (float) 1;
    M->b = (float) tan(alpha * PDC_M_PI / 180.0);
    M->c = (float) tan(beta * PDC_M_PI / 180.0);
    M->d = (float) 1;
    M->e = (float) 0;
    M->f = (float) 0;
}

/* left-multiply M to N and store the result in N */
void
pdc_multiply_matrix(const pdc_matrix *M, pdc_matrix *N)
{
    pdc_matrix result;

    result.a = M->a * N->a + M->b * N->c;
    result.b = M->a * N->b + M->b * N->d;
    result.c = M->c * N->a + M->d * N->c;
    result.d = M->c * N->b + M->d * N->d;

    result.e = M->e * N->a + M->f * N->c + N->e;
    result.f = M->e * N->b + M->f * N->d + N->f;

    N->a = result.a;
    N->b = result.b;
    N->c = result.c;
    N->d = result.d;
    N->e = result.e;
    N->f = result.f;
}

/* invert M and store the result in N */
void
pdc_invert_matrix(pdc_core *pdc, pdc_matrix *N, pdc_matrix *M)
{
    float det = M->a * M->d - M->b * M->c;

    if (fabs(det) < (float) PDC_SMALLREAL)
        pdc_error(pdc, PDC_E_INT_INVMATRIX,
            pdc_errprintf(pdc, "%f %f %f %f %f %f",
            M->a, M->b, M->c, M->d, M->e, M->f),
            0, 0, 0);

    N->a = M->d/det;
    N->b = -M->b/det;
    N->c = -M->c/det;
    N->d = M->a/det;
    N->e = -(M->e * N->a + M->f * N->c);
    N->f = -(M->e * N->b + M->f * N->d);
}

/* transform point */
void
pdc_transform_point(const pdc_matrix *M, float x, float y, float *tx, float *ty)
{
    *tx = M->a * x + M->c * y + M->e;
    *ty = M->b * x + M->d * y + M->f;
}

/* ---------------------- utility functions ----------------------------- */

void
pdc_place_element(pdc_fitmethod method, pdc_scalar minfscale,
                  const pdc_box *fitbox, const pdc_vector *relpos,
                  const pdc_vector *elemsize, pdc_box *elembox,
                  pdc_vector *scale)
{
    pdc_vector refpos;
    pdc_scalar width, height, det, fscale = 1.0;
    pdc_bool xscaling = pdc_false;

    /* reference position in fitbox */
    width = fitbox->ur.x - fitbox->ll.x;
    height = fitbox->ur.y - fitbox->ll.y;
    refpos.x = fitbox->ll.x + relpos->x * width;
    refpos.y = fitbox->ll.y + relpos->y * height;

    /* first check */
    switch (method)
    {
        case pdc_entire:
        case pdc_slice:
        case pdc_meet:
        case pdc_tauto:
        if (fabs(width) > PDC_FLOAT_PREC && fabs(height) > PDC_FLOAT_PREC)
        {
            if (method != pdc_entire)
            {
                det = elemsize->x * height - elemsize->y * width;
                xscaling = (method == pdc_slice && det <= 0) ||
                            ((method == pdc_meet || method == pdc_tauto) &&
                             det > 0) ? pdc_true : pdc_false;
                if (xscaling)
                    fscale = width / elemsize->x;
                else
                    fscale = height / elemsize->y;
            }

            if (method == pdc_tauto)
            {
                if(fscale >= 1.0)
                {
                    method = pdc_nofit;
                }
                else if (fscale < minfscale)
                {
                    method = pdc_meet;
                }
            }
        }
        else
        {
            method = pdc_nofit;
        }
        break;

        default:
        break;
    }

    /* calculation */
    switch (method)
    {
        /* entire box is covered by entire element */
        case pdc_entire:
        *elembox = *fitbox;
        scale->x = width / elemsize->x;
        scale->y = height / elemsize->y;
        return;

        /* fit into and preserve aspect ratio */
        case pdc_slice:
        case pdc_meet:
        if (xscaling)
            height = fscale * elemsize->y;
        else
            width = fscale * elemsize->x;
        scale->x = fscale;
        scale->y = fscale;
        break;

        /* fit into and doesn't preserve aspect ratio */
        case pdc_tauto:
        if (xscaling)
        {
            height = elemsize->y;
            scale->x = fscale;
            scale->y = 1.0;
        }
        else
        {
            width = elemsize->x;
            scale->x = 1.0;
            scale->y = fscale;
        }
        break;

        /* only positioning */
        case pdc_nofit:
        case pdc_clip:
        width = elemsize->x;
        height = elemsize->y;
        scale->x = 1.0;
        scale->y = 1.0;
        break;
    }

    /* placed element box */
    elembox->ll.x = refpos.x - relpos->x * width;
    elembox->ll.y = refpos.y - relpos->y * height;
    elembox->ur.x = refpos.x + (1.0 - relpos->x) * width;
    elembox->ur.y = refpos.y + (1.0 - relpos->y) * height;
}

void
pdc_box2polyline(const pdc_box *box, pdc_vector *polyline)
{
    /* counter clockwise */
    polyline[0].x = box->ll.x;
    polyline[0].y = box->ll.y;
    polyline[1].x = box->ur.x;
    polyline[1].y = box->ll.y;
    polyline[2].x = box->ur.x;
    polyline[2].y = box->ur.y;
    polyline[3].x = box->ll.x;
    polyline[3].y = box->ur.y;
    polyline[4] = polyline[0];
}

/* --------------------------- rectangles  --------------------------- */
pdc_bool
pdc_rect_isnull(pdc_rectangle *r)
{
    return
        (r->llx == (float) 0 && r->lly == (float) 0 &&
         r->urx == (float) 0 && r->ury == (float) 0);
}

pdc_bool
pdc_rect_contains(pdc_rectangle *r1, pdc_rectangle *r2)
{
    return
        (r1->llx <= r2->llx && r1->lly <= r2->lly &&
         r1->urx >= r2->urx && r1->ury >= r2->ury);
}

void
pdc_rect_copy(pdc_rectangle *r1, pdc_rectangle *r2)
{
    r1->llx = r2->llx;
    r1->lly = r2->lly;
    r1->urx = r2->urx;
    r1->ury = r2->ury;
}

void
pdc_rect_init(pdc_rectangle *r, float llx, float lly, float urx, float ury)
{
    r->llx = llx;
    r->lly = lly;
    r->urx = urx;
    r->ury = ury;
}

void
pdc_rect_transform(const pdc_matrix *M, pdc_rectangle *r1, pdc_rectangle *r2)
{
    float x[4], y[4];
    int i;

    pdc_transform_point(M, r1->llx, r1->lly, &x[0], &y[0]);
    pdc_transform_point(M, r1->llx, r1->ury, &x[1], &y[1]);
    pdc_transform_point(M, r1->urx, r1->ury, &x[2], &y[2]);
    pdc_transform_point(M, r1->urx, r1->lly, &x[3], &y[3]);

    pdc_rect_init(r2, (float) PDC_FLOAT_MAX, (float) PDC_FLOAT_MAX,
                      (float) PDC_FLOAT_MIN, (float) PDC_FLOAT_MIN);

    for (i = 0; i < 4; i++)
    {
        if (x[i] < r2->llx)
            r2->llx = x[i];
        if (y[i] < r2->lly)
            r2->lly = y[i];

        if (x[i] > r2->urx)
            r2->urx = x[i];
        if (y[i] > r2->ury)
            r2->ury = y[i];
    }
}

