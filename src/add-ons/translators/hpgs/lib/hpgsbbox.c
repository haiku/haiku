/***********************************************************************
 *                                                                     *
 * $Id: hpgsbbox.c 298 2006-03-05 18:18:03Z softadm $
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
 * The implementation of the public API for bounding boxes.            *
 *                                                                     *
 ***********************************************************************/

#include<hpgs.h>

/*!
   Returns, whether the two bounding boxes are equal.
 */
hpgs_bool hpgs_bbox_isequal   (const hpgs_bbox *bb1, const hpgs_bbox *bb2)
{
  return
    bb1->llx == bb2->llx &&
    bb1->lly == bb2->lly &&
    bb1->urx == bb2->urx &&
    bb1->ury == bb2->ury;
}

/*!
   Calculates the distance of the two bounding boxes.
 */
void hpgs_bbox_distance  (hpgs_point *d, const hpgs_bbox *bb1, const hpgs_bbox *bb2)
{
  if (hpgs_bbox_isnull(bb1) ||
      hpgs_bbox_isnull(bb2)   )
    {
      d->x = 0.0;
      d->y = 0.0;
      return;
    } 

  if (bb2->llx >= bb1->urx)
    {
      d->x = bb2->llx - bb1->urx;
    }
  else if (bb1->llx >= bb2->urx)
    {
      d->x = bb1->llx - bb2->urx;
    }
  else
    d->x = 0.0;

  if (bb2->lly >= bb1->ury)
    {
      d->y = bb2->lly - bb1->ury;
    }
  else if (bb1->lly >= bb2->ury)
    {
      d->y = bb1->lly - bb2->ury;
    }
  else
    d->y = 0.0;
}

/*!
   Returns, whether the bounding box is null.
   A null bounding box is als empty.
   An empty bounding box may not be null, if
   either of the x or y extend of the bounding box
   is exactly zero.
 */
hpgs_bool hpgs_bbox_isnull  (const hpgs_bbox *bb)
{
  return
    bb->llx > bb->urx ||
    bb->lly > bb->ury;
}

/*!
   Returns, whether the bounding box is empty.
 */
hpgs_bool hpgs_bbox_isempty (const hpgs_bbox *bb)
{
  return
    bb->llx >= bb->urx ||
    bb->lly >= bb->ury;
}

/*!
   Sets this bounding box to an empty bounding box.
 */
void hpgs_bbox_null (hpgs_bbox *bb)
{
  bb->llx = 1.0e20;
  bb->lly = 1.0e20;
  bb->urx = -1.0e20;
  bb->ury = -1.0e20;
}

/*!
   Expands \c bb1 in order to comprise the smallest bounding box
   containing both \c bb1 and \c bb2
 */
void hpgs_bbox_expand (hpgs_bbox *bb1, const hpgs_bbox *bb2)
{
  if (bb2->llx < bb1->llx) bb1->llx = bb2->llx;
  if (bb2->lly < bb1->lly) bb1->lly = bb2->lly;
  if (bb2->urx > bb1->urx) bb1->urx = bb2->urx;
  if (bb2->ury > bb1->ury) bb1->ury = bb2->ury;
}

/*!
   Shrinks \c bb1 in order to comprise the intersection
   of \c bb1 and \c bb2. If the bounding boxes do not intersect,
   \c bb1 is set to a null bounding box.
 */
void hpgs_bbox_intersect (hpgs_bbox *bb1, const hpgs_bbox *bb2)
{
  if (bb2->llx > bb1->llx) bb1->llx=bb2->llx;
  if (bb2->lly > bb1->lly) bb1->lly=bb2->lly;
  if (bb2->urx < bb1->urx) bb1->urx=bb2->urx;
  if (bb2->ury < bb1->ury) bb1->ury=bb2->ury;
}
