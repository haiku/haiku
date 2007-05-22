/***********************************************************************
 *                                                                     *
 * $Id: hpgsplugin.h 325 2006-04-22 20:01:11Z softadm $
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
 * The extension classes located in my device plugin.                  *
 *                                                                     *
 ***********************************************************************/

#ifndef	__HPGS_PLUGIN_H
#define	__HPGS_PLUGIN_H

#include<hpgs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HPGS_SHARED
# ifdef WIN32
#  ifdef __GNUC__
#   ifdef HPGS_BUILD_PLUGIN
#    define HPGS_PLUGIN_API __attribute__((dllexport))
#   else
#    define HPGS_PLUGIN_API __attribute__((dllimport))
#   endif
#  else
#   ifdef HPGS_BUILD_PLUGIN
#    define HPGS_PLUGIN_API __declspec(dllexport)
#   else
#    define HPGS_PLUGIN_API __declspec(dllimport)
#   endif
#  endif
# else
#  define HPGS_PLUGIN_API
# endif
#else
# define HPGS_PLUGIN_API
#endif


HPGS_PLUGIN_API int hpgs_plugin_new_device(hpgs_device **device,
                                           void **asset_ctxt,
                                           hpgs_reader_asset_func_t *page_asset_func,
                                           void **frame_asset_ctxt,
                                           hpgs_reader_asset_func_t *frame_asset_func,
                                           const char *dev_name,
                                           const char *out_fn,
                                           const hpgs_bbox *bb,
                                           double xres, double yres,
                                           hpgs_bool do_rop3,
                                           int argc, const char *argv[]);

HPGS_PLUGIN_API void hpgs_plugin_version(int * major, int *minor);

HPGS_PLUGIN_API void hpgs_plugin_init();
HPGS_PLUGIN_API void hpgs_plugin_cleanup();

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif /* ! __HPGS_H */
