/***********************************************************************
 *                                                                     *
 * $Id: hpgsmatrix.c 298 2006-03-05 18:18:03Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * The implementation of the public API for transformation matrices.   *
 *                                                                     *
 ***********************************************************************/

#include<hpgs.h>

/*!
   Sets the given matrix to the identity matrix.
 */
void hpgs_matrix_set_identity(hpgs_matrix *m)
{
  m->myx = m->mxy = m->dy = m->dx = 0.0;
  m->myy = m->mxx = 1.0;
}

/*!
   Transforms the given point \c p with the matrix \c m.
   The result is stored in \c res.
   The pointers \c p and \c res may point to the same memory location.
 */
void hpgs_matrix_xform(hpgs_point *res,
                       const hpgs_matrix *m, const hpgs_point *p)
{
  double x = m->dx + m->mxx * p->x + m->mxy * p->y;
  double y = m->dy + m->myx * p->x + m->myy * p->y;
  res->x = x;
  res->y = y;
}

/*!
   Transforms the given point \c p with the inverse of the matrix \c m.
   The result is stored in \c res.
   The pointers \c p and \c res may point to the same memory location.
 */
void hpgs_matrix_ixform(hpgs_point *res,
                        const hpgs_point *p, const hpgs_matrix *m)
{
  double x0 = (p->x - m->dx);
  double y0 = (p->y - m->dy);

  double det = m->mxx * m->myy - m->mxy * m->myx;

  res->x = (m->myy * x0 - m->mxy * y0)/det;
  res->y = (m->mxx * y0 - m->myx * x0)/det;
}

/*!
   Transforms the given bounding box \c bb with the matrix \c m.
   The result is the enclosing bounding box of the transformed 
   rectangle of the bounding box and is stored in \c res.
   The pointers \c bb and \c res may point to the same memory location.
 */
void hpgs_matrix_xform_bbox(hpgs_bbox *res,
                            const hpgs_matrix *m, const hpgs_bbox *bb)
{
  hpgs_point ll = { bb->llx, bb->lly };
  hpgs_point ur = { bb->urx, bb->ury };

  hpgs_point lr = { bb->llx, bb->ury };
  hpgs_point ul = { bb->urx, bb->lly };

  hpgs_matrix_xform(&ll,m,&ll);
  hpgs_matrix_xform(&ur,m,&ur);

  hpgs_matrix_xform(&lr,m,&lr);
  hpgs_matrix_xform(&ul,m,&ul);

  res->llx = HPGS_MIN(HPGS_MIN(ll.x,ur.x),HPGS_MIN(lr.x,ul.x));
  res->lly = HPGS_MIN(HPGS_MIN(ll.y,ur.y),HPGS_MIN(lr.y,ul.y));

  res->urx = HPGS_MAX(HPGS_MAX(ll.x,ur.x),HPGS_MAX(lr.x,ul.x));
  res->ury = HPGS_MAX(HPGS_MAX(ll.y,ur.y),HPGS_MAX(lr.y,ul.y));
}

/*!
   Transforms the given bounding box \c bb with the the inverse of
   the matrix \c m.
   The result is the enclosing bounding box of the transformed 
   rectangle of the bounding box and is stored in \c res.
   The pointers \c bb and \c res may point to the same memory location.
 */
void hpgs_matrix_ixform_bbox(hpgs_bbox *res,
                             const hpgs_bbox *bb, const hpgs_matrix *m)
{
  hpgs_point ll = { bb->llx, bb->lly };
  hpgs_point ur = { bb->urx, bb->ury };

  hpgs_point lr = { bb->llx, bb->ury };
  hpgs_point ul = { bb->urx, bb->lly };

  hpgs_matrix_ixform(&ll,&ll,m);
  hpgs_matrix_ixform(&ur,&ur,m);

  hpgs_matrix_ixform(&lr,&lr,m);
  hpgs_matrix_ixform(&ul,&ul,m);

  res->llx = HPGS_MIN(HPGS_MIN(ll.x,ur.x),HPGS_MIN(lr.x,ul.x));
  res->lly = HPGS_MIN(HPGS_MIN(ll.y,ur.y),HPGS_MIN(lr.y,ul.y));

  res->urx = HPGS_MAX(HPGS_MAX(ll.x,ur.x),HPGS_MAX(lr.x,ul.x));
  res->ury = HPGS_MAX(HPGS_MAX(ll.y,ur.y),HPGS_MAX(lr.y,ul.y));
}

/*!
   Transforms the given point \c p with the matrix \c m without
   applying the translation part of \c m. This is useful in order
   to transform delta vectors.

   The result is stored in \c res.
   The pointers \c p and \c res may point to the same memory location.
 */
void hpgs_matrix_scale(hpgs_point *res,
                       const hpgs_matrix *m, const hpgs_point *p)
{
  double x = m->mxx * p->x + m->mxy * p->y;
  double y = m->myx * p->x + m->myy * p->y;
  res->x = x;
  res->y = y;
}

/*!
   Transforms the given point \c p with the inverse of the matrix \c m
   without applying the translation part of \c m. This is useful in order
   to transform delta vectors.

   The result is stored in \c res.
   The pointers \c p and \c res may point to the same memory location.
 */
void hpgs_matrix_iscale(hpgs_point *res,
                        const hpgs_point *p, const hpgs_matrix *m)
{
  double x0 = p->x;
  double y0 = p->y;

  double det = m->mxx * m->myy - m->mxy * m->myx;

  res->x = (m->myy * x0 - m->mxy * y0)/det;
  res->y = (m->mxx * y0 - m->myx * x0)/det;
}

/*!
   Concatenates the given matrices \c a and \c b.
   The result is stored in \c res.
   Either of the pointer \c a or \b may point to the same memory location as
   the pointer \c res.
 */
void hpgs_matrix_concat(hpgs_matrix *res,
                        const hpgs_matrix *a, const hpgs_matrix *b)
{
  //
  // |  1   0   0 |   |  1   0   0 |   |                1               0               0 |
  // | dx mxx mxy | x | Dx Mxx Mxy | = | dx+Dx*mxx+Dy*mxy mxx*Mxx+mxy*Myx mxx*Mxy+mxy*Myy |
  // | dy myx myy |   | Dy Myx Myy |   | dy+Dx*myx+Dy*myy myx*Mxx+myy*Myx myx*Mxy+myy*Myy |
  //
  double r0,r1,r2,r3;

  r0 = a->dx+b->dx*a->mxx+b->dy*a->mxy;
  r1 = a->dy+b->dx*a->myx+b->dy*a->myy;

  res->dx = r0;
  res->dy = r1;

  r0 = a->mxx*b->mxx+a->mxy*b->myx;
  r1 = a->mxx*b->mxy+a->mxy*b->myy;
  r2 = a->myx*b->mxx+a->myy*b->myx;
  r3 = a->myx*b->mxy+a->myy*b->myy;

  res->mxx = r0;
  res->mxy = r1;
  res->myx = r2;
  res->myy = r3;
}

/*!
   Inverts the given matrix \c m.
   The result is stored in \c res.
   The pointers \c m and \c res may point to the same memory location.
 */
void hpgs_matrix_invert(hpgs_matrix *res, const hpgs_matrix *m)
{
  double det = m->mxx * m->myy - m->mxy * m->myx;

  double tmp = m->mxx/det;
  res->mxx = m->myy/det;
  res->myy = tmp;

  res->mxy = -m->mxy/det;
  res->myx = -m->myx/det;

  double x0 = -m->dx;
  double y0 = -m->dy;
  res->dx = x0 * res->mxx + y0 * res->mxy;
  res->dy = x0 * res->myx + y0 * res->myy;
}
