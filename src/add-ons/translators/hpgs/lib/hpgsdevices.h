/***********************************************************************
 *                                                                     *
 * $Id: hpgsdevices.h 298 2006-03-05 18:18:03Z softadm $
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
 * The private interfaces for the basic devices                        *
 *                                                                     *
 ***********************************************************************/

#ifndef	__HPGS_DEVICES_H
#define	__HPGS_DEVICES_H

#include<hpgs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \file hpgsdevices.h

   \brief The private interfaces for basic vector devices.

   A header file, which declares the structures and functions internally
   used for implementing the very basic implementations of \c hpgs_device.
*/

/*! @addtogroup device
 *  @{
 */

#define HPGS_PLOTSIZE_MAX_CLIP_DEPTH 16

/*! \brief A vector graphics device for plotsize calculating.

    This structure implements a \c hpgs_device and is used to calculate
    the bounding box of a vector graphics scenery.
 */
struct hpgs_plotsize_device_st {
  hpgs_device inherited; //!< The base device structure.

  hpgs_bool ignore_ps; //!< Do we ignore a PS command?
  int clip_depth;      //!< The depth of the current clip path stack.

  hpgs_point moveto;   //!< The position of the last moveto.
  int deferred_moveto; //!< Do we have an unregistered moveto pending?

  hpgs_bool do_linewidth; //!< Do we account for the current linewidth?
  double linewidth;       //!< Current linewidth.

  hpgs_bbox clip_bbs[HPGS_PLOTSIZE_MAX_CLIP_DEPTH]; /*! The bounding boxes of the clip paths. */

  hpgs_bbox path_bb; /*! The bounding box of the current path. */
  hpgs_bbox page_bb; /*! The bounding box of the current page. */
  hpgs_bbox global_bb; /*! The currently calculated overall bounding box. */

  /*@{ */
  /*! A stack of page bounding boxes.
   */
  hpgs_bbox *page_bbs; /*! The currently calculated overall bounding box. */
  int n_page_bbs;
  int page_bbs_alloc_size;
  /*@} */
};

typedef struct hpgs_ps_media_size_st hpgs_ps_media_size;

/*! \brief A structure for storing a paper size.

    This structure stores a PostScipt paper size.
 */
struct hpgs_ps_media_size_st
{
  int width;  //!< The width of the paper in pt.
  int height; //!< The height of the paper in pt.

  const char *name; //!< The name of this paper size, if it is a std paper size.
  size_t usage; //!< The usage count of this paper size.

  size_t hash; //!< A hash value for storing media sizes in a sorted vector.
};

/*! \brief A vector graphics device for drawing to an eps file.

    This structure implements a \c hpgs_device and is used to write
    a scenery to an eps file.
 */
struct hpgs_eps_device_st {
  hpgs_device inherited; //!< The base device structure.

  hpgs_bbox doc_bb;
  /*! The document bounding box used for multipage postscript files. */

  hpgs_bbox page_bb;
  /*! The bounding box of the current page. */

  int n_pages;
  /*! The number of pages. -1...we are writing individual eps files. */

  char *filename; //!< The output filename.

  hpgs_ostream *out; //!< The current page stream.
  hpgs_bool page_setup; //!< Is the current page set up?

  /*@{ */
  /*! A stack of media sizes for multipage PostScript files.
      The media sizes are sorted by their hash value.
   */
  hpgs_ps_media_size *media_sizes; /*! The currently calculated overall bounding box. */
  int n_media_sizes;
  int media_sizes_alloc_size;
  /*@} */

  hpgs_xrop3_func_t rop3; //!< The ROP3 transfer raster operation.
  hpgs_palette_color pattern_color; //!< The color of the ROP3 pattern.

  hpgs_color color; //!< The current output color.
};

HPGS_INTERNAL_API void hpgs_cleanup_plugin_devices();

/*! @} */ /* end of group device */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif /* ! __HPGS_DEVICES_H */
