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

/* $Id: pc_geom.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib core geometry utilities
 *
 */

#ifndef PC_GEOM_H
#define PC_GEOM_H

/* Unfortunately M_PI causes porting woes, so we use a private name */
#define PDC_M_PI                3.14159265358979323846          /* pi */

/* Conversion factors */
#define PDC_INCH2METER          0.0254
#define PDC_METER2INCH         39.3701

/* same as PDF_SMALLREAL */
#define PDC_SMALLREAL   (0.000015)

typedef struct { float llx, lly, urx, ury; } pdc_rectangle;
typedef struct { float a, b, c, d, e, f; } pdc_matrix;

typedef double pdc_scalar;
typedef struct { pdc_scalar x, y; } pdc_vector;
typedef struct { pdc_vector ll, ur; } pdc_box;

/* methods for fitting rectangle elements into a box */
typedef enum
{
    pdc_nofit = 0,      /* no fit, only positioning */
    pdc_clip,           /* no fit, only positioning with following condition:
                         * - the parts of element beyond the bounds of box
                         *   are clipped */
    pdc_slice,          /* fit into the box with following conditions:
                         * - aspect ratio of element is preserved
                         * - entire box is covered by the element
                         * - the parts of element beyond the bounds of box
                         *   are clipped */
    pdc_meet,           /* fit into the box with following conditions:
                         * - aspect ratio of element is preserved
                         * - entire element is visible in the box */
    pdc_entire,         /* fit into the box with following conditions:
                         * - entire box is covered by the element
                         * - entire element is visible in the box */
    pdc_tauto           /* automatic fitting. If element extends fit box in
                         * length, then element is shrinked, if shrink
                         * factor is greater than a specified value. Otherwise
                         * pdc_meet is applied. */
}
pdc_fitmethod;

pdc_bool pdc_is_identity_matrix(pdc_matrix *m);
void     pdc_translation_matrix(float tx, float ty, pdc_matrix *M);
void     pdc_scale_matrix(float sx, float sy, pdc_matrix *M);
void     pdc_rotation_matrix(float angle, pdc_matrix *M);
void     pdc_skew_matrix(float alpha, float beta, pdc_matrix *M);
void     pdc_multiply_matrix(const pdc_matrix *M, pdc_matrix *N);
void     pdc_invert_matrix(pdc_core *pdc, pdc_matrix *N, pdc_matrix *M);
void     pdc_transform_point(const pdc_matrix *M,
            float x, float y, float *tx, float *ty);

void     pdc_place_element(pdc_fitmethod method,
            pdc_scalar minfscale, const pdc_box *fitbox,
            const pdc_vector *relpos, const pdc_vector *elemsize,
            pdc_box *elembox, pdc_vector *scale);
void     pdc_box2polyline(const pdc_box *box, pdc_vector *polyline);

pdc_bool pdc_rect_isnull(pdc_rectangle *r);
pdc_bool pdc_rect_contains(pdc_rectangle *r1, pdc_rectangle *r2);
void     pdc_rect_copy(pdc_rectangle *r1, pdc_rectangle *r2);
void     pdc_rect_init(pdc_rectangle *r,
            float llx, float lly, float urx, float ury);
void     pdc_rect_transform(const pdc_matrix *M,
            pdc_rectangle *r1, pdc_rectangle *r2);

#endif  /* PC_GEOM_H */

