/***********************************************************************
 *                                                                     *
 * $Id: hpgsimage.h 270 2006-01-29 21:12:23Z softadm $
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
 * Private declarations for images.                                    *
 *                                                                     *
 ***********************************************************************/

#ifndef __HPGS_IMAGE_H__
#define __HPGS_IMAGE_H__

#include <hpgs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief An concrete pixel image.

    This structure has a public alias \c hpgs_png_image and
    implements \c hpgs_image. The storage format is similar
    to the format used by \c libpng, so the image may be written
    to png file using \c hpgs_png_image_write.
*/

/*! @addtogroup image
 *  @{
 */
struct hpgs_png_image_st {
  hpgs_image inherited;

  int depth;         //!< The bit depth of the image.
  int color_type;    //!< The color type.
  int bytes_per_row; //!< The number of bytes per row of the pixel data.
  int compression;   //!< The compression used for writing the image in the range from 0 to 9.

  unsigned char *data; //!< The pixel data.

  hpgs_rop3_func_t rop3; //!< The ROP3 raster operation.
  hpgs_palette_color pattern_color; //!< The color of the ROP3 pattern.

  unsigned res_x; //!< The number of pixels per meter in x-direction, or 0 if unknown.
  unsigned res_y; //!< The number of pixels per meter in y-direction, or 0 if unknown.
};

/*! @} */ /* end of group reader */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // ! __HPGS_IMAGE_H__
