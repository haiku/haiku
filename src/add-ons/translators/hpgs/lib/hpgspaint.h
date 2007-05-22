/***********************************************************************
 *                                                                     *
 * $Id: hpgspaint.h 270 2006-01-29 21:12:23Z softadm $
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
 * The declarations for the pixel painter.                             *
 *                                                                     *
 ***********************************************************************/

#ifndef	__HPGS_PAINT_H
#define	__HPGS_PAINT_H

#include<hpgs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \file hpgspaint.h

   \brief Internals of the pixel renderer.

   A header file, which declares the structures and functions internally
   used by \c hpgs_paint_device.
*/

typedef struct hpgs_scanline_point_st hpgs_scanline_point;
typedef struct hpgs_paint_scanline_st hpgs_paint_scanline;
typedef struct hpgs_paint_clipper_st  hpgs_paint_clipper;

/*! @addtogroup paint_device
 *  @{
 */

#define HPGS_PAINT_MAX_CLIP_DEPTH 16

/*! \brief The pixel rendering vector graphics device.

    This structure has a public alias \c hpgs_paint_device and
    represents a \c hpgs_device which draws to a given \c hpgs_image.
    It is capable of drawing polgonal data with optional antialiasing.
    Image may be drawn using a linear interpolation of the pixel values
    or uninterpolated.

    The following figure outlines the coordinate setup of a paint device.

   \image html clipper.png
   \image latex clipper.eps "Coordinate setup of hpgs_paint_device"

   Further details on antialiasing and scanline setup are described in the
   documentation of \c hpgs_paint_clipper_st.
 */
struct hpgs_paint_device_st
{
  hpgs_device inherited; /*!< The base device structure. */

  hpgs_image *image; /*!< The image to which we draw. */

  char *filename; /*!< The image filename without an eventual extension. */

  hpgs_bbox page_bb;/*! The bounding box in world coordinates, which is mapped onto
                      the image. */

  double xres;  /*!< The resolution of the image in x direction. */
  double yres;  /*!< The resolution of the image in y direction. */

  hpgs_bool overscan;
  /*!< Specifies, whether we use antialiasing for rendering the image. */
  double thin_alpha; /*!< The minimal alpha value for antialiased thin lines. */

  hpgs_paint_path *path; /*!< The current path, which is under contruction. */

  hpgs_paint_clipper *path_clipper; /*!< The mapping of the current path onto the defined scanlines. */

 
  hpgs_paint_clipper *clippers[HPGS_PAINT_MAX_CLIP_DEPTH];
  /*!< A stack of mappings of the clip path' of onto the defined scanlines. */

  
  int current_clipper; /*!< The position of the current clip path in \c clippers. */
  int clip_depth;
  /*!< The current depth of the clip stack, i.e. the number of time
       \c hpgs_clip or \c hpgs_eoclip has been called. */
  
  hpgs_gstate *gstate; /*!< The graphics state. */

  hpgs_paint_color color; /*!< The current color transformed to a device color. */
  hpgs_palette_color patcol; /*!< The current pattern color transformed to a device color. */

  int image_interpolation; /*!< A flag holding whether we do image interpolation. */
};

HPGS_INTERNAL_API int hpgs_paint_device_fill(hpgs_paint_device *pdv,
                                             hpgs_paint_path *path,
                                             hpgs_bool winding,
                                             hpgs_bool stroke);

HPGS_INTERNAL_API int hpgs_paint_device_clip(hpgs_paint_device *pdv,
                                             hpgs_paint_path *path,
                                             hpgs_bool winding);

HPGS_INTERNAL_API int hpgs_paint_device_drawimage(hpgs_paint_device *pdv,
                                                  const hpgs_image *img,
                                                  const hpgs_point *ll, const hpgs_point *lr,
                                                  const hpgs_point *ur);

/*! @} */ /* end of group paint_device */

/*! @addtogroup path
 *  @{
 */

#define HPGS_BEZIER_NSEGS 16

typedef struct hpgs_bezier_length_st     hpgs_bezier_length;

/*! \brief A structure for holding intermediate values for length
           calculations of a bezier spline.

    This structure has a public alias \c hpgs_bezier_length and holds
    internals of a bezier spline used during length calculations.
 */

struct hpgs_bezier_length_st
{
  double l[HPGS_BEZIER_NSEGS+1]; /*!< The length of the spline from the beginning to the parameter -0.5+i/HPGS_BEZIER_NSEGS. */
  double dl[HPGS_BEZIER_NSEGS+1]; /*!< The derivative of the length of the spline at the parameter -0.5+i/HPGS_BEZIER_NSEGS. */
};

HPGS_INTERNAL_API void hpgs_bezier_length_init(hpgs_bezier_length *b,
                                               const hpgs_paint_path *path, int i);
HPGS_INTERNAL_API double hpgs_bezier_length_param(const hpgs_bezier_length *b, double l);

/*! @} */ /* end of group path */

/*! @addtogroup scanline
 *  @{
 */

/*! \brief An intersection point of a path with a scanline.

    This structure has a public alias \c hpgs_scanline_point and holds
    the information of an intersection point of a path with a scanline.
 */
struct hpgs_scanline_point_st
{
  double x; 
  /*!< The x coordinate of the intersection point of the path with the scanline. */

  int    order;
  /*!< The order of the intersection.
       An \c order of 1 means an upward intersection, -1 means a downward intersection.
       Horizontal intersection are stored as two distinct intersection points with the same
       x coordinate. If antialiasing is used, the order represents a delta of a broken down
       winding value. A order of 256 means that the scanline is intersected from the bottom
       to the top of a physical image row between the previous and the actual intersection
       point. if The order is smaller than 256, the physical row is not touched in its whole
       extend an a corresponding alpha value less than 1 is generated. */
};

/*! \brief A scanline of the pixel renderer.

    This structure has a public alias \c hpgs_paint_scanline and holds
    the intersections points of a path with a scanline.
 */
struct hpgs_paint_scanline_st
{
  /*@{*/
  /*! A vector of intersection points.
   */
  hpgs_scanline_point *points;
  int points_malloc_size;
  int n_points;
  /*@}*/
};

/*! \brief A collection of scanlines for mapping paths onto images.

    This structure has a public alias \c hpgs_paint_clipper and holds
    intersection points of a path with a rectangular region represented
    by a series of equidistantly distributed scanlines.

    The \c overscan member determines, whether we use antialiasing for
    mappinng the path. The name  of this structure member is historical,
    because up to \c hpgs-0.8.x the antialiasing renderer effectively used
    more than one scanline per physical image row in order to caclulate
    alpha values.

    Nowadays a scanline always represents the middle of the
    corresponding physical row of the image as sketch in the folowing figure,
    which show the generated segment for non-antialiased rendering.

   \image html scanline_0.png
   \image latex scanline_0.eps "scanline setup and pixel filling without antialiasing."

    If \c overscan is non-zero, the segment generator caclulates slopes of the
    trapezoids, which are generated by the path segments cutting the boundaries
    between two physical grid lines as sketched in the following figure.

   \image html scanline_n.png
   \image latex scanline_n.eps "scanline setup and alpha generation with antialiasing."

 */
struct hpgs_paint_clipper_st
{
  /*@{ */
  /*! The vector of scanlines in this clipper.
   */
  hpgs_paint_scanline *scanlines;
  int n_scanlines;
  /*@} */

  hpgs_bbox bb; /*! The bounding box of this clipper in world coordinates. */

  /*@{ */
  /*! The mapping of scanline numbers to world coordinates.
     \c iscan=(y0-y)/yfac and  \c y=y0-iscan*yfac.
   */
  double yfac; // height of a single scanline.
  double y0;   // y for scanline 0
  /*@} */

  hpgs_bool overscan; /*!< Do we use antialiasing ?. */
  int height; /*!< Number of physical pixels of the underlying image. */

  int iscan0; /*!< The first scanline with non-zero intersections. */
  int iscan1; /*!< The last scanline with non-zero intersections. */
};

HPGS_INTERNAL_API hpgs_paint_clipper *hpgs_new_paint_clipper(const hpgs_bbox *bb,
                                                             int height,
                                                             int scanline_msize,
                                                             int overscan);

HPGS_INTERNAL_API hpgs_paint_clipper *hpgs_paint_clipper_clip(const hpgs_paint_clipper *orig,
                                                              const hpgs_paint_clipper *clip,
                                                              hpgs_bool winding);

HPGS_INTERNAL_API int hpgs_paint_clipper_cut(hpgs_paint_clipper *c,
                                             hpgs_paint_path *path);

HPGS_INTERNAL_API int hpgs_paint_clipper_thin_cut(hpgs_paint_clipper *c,
                                                  hpgs_paint_path *path,
                                                  const hpgs_gstate *gstate);

HPGS_INTERNAL_API int hpgs_paint_clipper_emit (hpgs_image *image,
                                               const hpgs_paint_clipper *img,
                                               const hpgs_paint_clipper *clip,
                                               const hpgs_paint_color *c,
                                               hpgs_bool winding,
                                               hpgs_bool stroke);

HPGS_INTERNAL_API int  hpgs_paint_clipper_reset(hpgs_paint_clipper *c, double llx, double urx);
HPGS_INTERNAL_API void hpgs_paint_clipper_clear(hpgs_paint_clipper *c);
HPGS_INTERNAL_API void hpgs_paint_clipper_destroy(hpgs_paint_clipper *c);

/*! @} */ /* end of group scanline */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif /* ! __HPGS_PAINT_H */
