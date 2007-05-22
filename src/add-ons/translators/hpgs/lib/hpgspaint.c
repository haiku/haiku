/***********************************************************************
 *                                                                     *
 * $Id: hpgspaint.c 375 2007-01-24 16:22:49Z softadm $
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
 * The implementation of the public API for the pixel painter.         *
 *                                                                     *
 ***********************************************************************/

#include<hpgspaint.h>
#include<math.h>
#include<assert.h>
#include<string.h>
#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include <malloc.h>
#else
#include <alloca.h>
#endif

//#define HPGS_PAINT_DEBUG_ROP3

/*! \defgroup paint_device Paint device.

  This module contains the workhorse for rendering a scenery to pixel
  graphics, \c hpgs_paint_device.

  Most notably, you can call \c hpgs_new_paint_device in order to
  create a new paint device and perform the usual operations
  \c hpgs_moveto, \c hpgs_lineto, ... on it.

  Details about the implementation are explained in the documentation
  of \c hpgs_paint_device_st and \c hpgs_paint_clipper_st and the
  hpyerlinks therein.
*/

/* static paint device methods for the hpgs_device vtable. */
static  int pdv_moveto (hpgs_device *_this, const hpgs_point *p)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (hpgs_paint_path_moveto(pdv->path,p))
    return hpgs_set_error(hpgs_i18n("moveto error."));
  
  return 0;
}

static  int pdv_lineto (hpgs_device *_this, const hpgs_point *p)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (hpgs_paint_path_lineto(pdv->path,p))
    return hpgs_set_error(hpgs_i18n("lineto error."));
  
  return 0;
}

static  int pdv_curveto (hpgs_device *_this, const hpgs_point *p1,
			 const hpgs_point *p2, const hpgs_point *p3 )
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (hpgs_paint_path_curveto(pdv->path,p1,p2,p3))
    return hpgs_set_error(hpgs_i18n("curveto error."));

  return 0;
}

static  int pdv_newpath     (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  hpgs_paint_path_truncate(pdv->path);
  return 0;
}

static  int pdv_closepath   (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (hpgs_paint_path_closepath(pdv->path))
    return hpgs_set_error(hpgs_i18n("closepath error."));

  return 0;
}

static  int pdv_stroke      (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;
  int ret = 0;

  if (pdv->path->n_points > 1 ||
      (pdv->path->n_points == 1 && (pdv->path->points[0].flags&HPGS_POINT_DOT))
      )
    {
      if (!pdv->overscan &&
          pdv->gstate->linewidth < 1.5 * pdv->path_clipper->yfac)
	{
          const hpgs_paint_clipper *clip = pdv->clippers[pdv->current_clipper];

	  if (hpgs_paint_clipper_thin_cut(pdv->path_clipper,
					  pdv->path,pdv->gstate))
	    return hpgs_set_error(hpgs_i18n("Out of memory cutting thin line."));
    
	  if (hpgs_paint_clipper_emit(pdv->image,
				      pdv->path_clipper,clip,
				      &pdv->color,1,1))
	    {
	      if (hpgs_have_error())
		return hpgs_error_ctxt(hpgs_i18n("stroke: Image error"));

	      return hpgs_set_error(hpgs_i18n("stroke: Error emitting scanlines."));
	    }

	  hpgs_paint_clipper_clear(pdv->path_clipper);
	}
      else
	{
	  hpgs_paint_path *p = hpgs_paint_path_stroke_path(pdv->path,
							   pdv->gstate);

	  if (!p)
	    return hpgs_set_error(hpgs_i18n("Out of memory creating stroke path."));

	  ret = hpgs_paint_device_fill(pdv,p,HPGS_TRUE,HPGS_TRUE);
	
	  hpgs_paint_path_destroy(p);
	}
    }
    
  hpgs_paint_path_truncate(pdv->path);

  if (ret)
    return hpgs_set_error(hpgs_i18n("Error filling stroke path."));

  return 0;
}

static  int pdv_fill        (hpgs_device *_this, hpgs_bool winding)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  int ret = hpgs_paint_device_fill(pdv,pdv->path,winding,HPGS_FALSE);

  hpgs_paint_path_truncate(pdv->path);

  return ret;
}

static  int pdv_clip        (hpgs_device *_this, hpgs_bool winding)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  int ret = hpgs_paint_device_clip(pdv,pdv->path,winding);

  hpgs_paint_path_truncate(pdv->path);

  return ret;
}

static  int pdv_clipsave       (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (pdv->clip_depth >= HPGS_PAINT_MAX_CLIP_DEPTH)
    return hpgs_set_error(hpgs_i18n("Maximum clip depth of %d exceeded."),
                          HPGS_PAINT_MAX_CLIP_DEPTH);

  pdv->clippers[pdv->clip_depth] = 0;

  ++pdv->clip_depth;

  return 0;
}

static  int pdv_cliprestore    (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  --pdv->clip_depth;

  if (pdv->clippers[pdv->clip_depth])
    {
      hpgs_paint_clipper_destroy(pdv->clippers[pdv->clip_depth]);
      pdv->clippers[pdv->clip_depth] = 0;
    }

  if (pdv->current_clipper < pdv->clip_depth)
    return 0;

  while (pdv->current_clipper > 0)
    {
      --pdv->current_clipper;
      if (pdv->clippers[pdv->current_clipper])
	return 0;  
    }

  return hpgs_set_error(hpgs_i18n("cliprestore: No valid clipper found."));
}

static  int pdv_setrgbcolor (hpgs_device *_this, const hpgs_color *rgb)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  pdv->gstate->color = *rgb;

  pdv->color.r = (unsigned char)(rgb->r*255.0);
  pdv->color.g = (unsigned char)(rgb->g*255.0);
  pdv->color.b = (unsigned char)(rgb->b*255.0);

  if  (hpgs_image_define_color(pdv->image,&pdv->color))
    return hpgs_error_ctxt("setrgbcolor");

  pdv->gstate->pattern_color = *rgb;

  hpgs_palette_color patcol;
  
  patcol.r = pdv->color.r & (~pdv->patcol.r);
  patcol.g = pdv->color.g & (~pdv->patcol.g);
  patcol.b = pdv->color.b & (~pdv->patcol.b);

#ifdef HPGS_PAINT_DEBUG_ROP3
  hpgs_log("setrgbcol: patcol=%d,%d,%d.\n",patcol.r,patcol.g,patcol.b);
#endif

  return hpgs_image_setpatcol(pdv->image,&patcol);
}

static  int pdv_setdash (hpgs_device *_this, const float *segs,
			 unsigned n, double d)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (hpgs_gstate_setdash(pdv->gstate,segs,n,d))
    return hpgs_set_error(hpgs_i18n("setdash: Out of memory."));

  return 0;
}

static  int pdv_setlinewidth(hpgs_device *_this, double lw)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (lw < pdv->thin_alpha * pdv->path_clipper->yfac)
    pdv->gstate->linewidth = pdv->thin_alpha * pdv->path_clipper->yfac;
  else
    pdv->gstate->linewidth = lw;
  return 0;
}

static  int pdv_setlinecap  (hpgs_device *_this, hpgs_line_cap lc)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  pdv->gstate->line_cap = lc;
  return 0;
}

static  int pdv_setlinejoin (hpgs_device *_this, hpgs_line_join lj)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  pdv->gstate->line_join = lj;
  return 0;
}

static  int pdv_setmiterlimit (hpgs_device *_this, double l)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  pdv->gstate->miterlimit = l;
  return 0;
}

static  int pdv_setrop3 (hpgs_device *_this, int rop,
                         hpgs_bool src_transparency, hpgs_bool pattern_transparency)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;
  hpgs_rop3_func_t rop3;

#ifdef HPGS_PAINT_DEBUG_ROP3
  hpgs_log("setrop3: rop,src_trans,pat_trans=%d,%d,%d.\n",
          rop,src_transparency,pattern_transparency);
#endif
  pdv->gstate->rop3 = rop;
  pdv->gstate->src_transparency = src_transparency;
  pdv->gstate->pattern_transparency = pattern_transparency;

  rop3 = hpgs_rop3_func(rop,src_transparency,pattern_transparency);

  if (!rop3)
    return hpgs_set_error(hpgs_i18n("setrop3: Invalid ROP3 %d specified"),rop);

  return hpgs_image_setrop3(pdv->image,rop3);
}

static  int pdv_setpatcol(hpgs_device *_this, const hpgs_color *rgb)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  pdv->gstate->pattern_color = *rgb;

  pdv->patcol.r = (unsigned char)(rgb->r * 255.0);
  pdv->patcol.g = (unsigned char)(rgb->g * 255.0);
  pdv->patcol.b = (unsigned char)(rgb->b * 255.0);

  hpgs_palette_color patcol;
  
  patcol.r = pdv->color.r & (~pdv->patcol.r);
  patcol.g = pdv->color.g & (~pdv->patcol.g);
  patcol.b = pdv->color.b & (~pdv->patcol.b);

#ifdef HPGS_PAINT_DEBUG_ROP3
  hpgs_log("setpatcol: patcol=%d,%d,%d.\n",patcol.r,patcol.g,patcol.b);
#endif

  return hpgs_image_setpatcol(pdv->image,&patcol);
}

static int pdv_drawimage(hpgs_device *_this,
                         const hpgs_image *img,
                         const hpgs_point *ll, const hpgs_point *lr,
                         const hpgs_point *ur)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  if (!img)
    return hpgs_set_error(hpgs_i18n("drawimage: Null image specified."));

  hpgs_palette_color patcol;
  patcol.r = 0;
  patcol.g = 0;
  patcol.b = 0;
  
  if (hpgs_image_setpatcol(pdv->image,&patcol))
    return -1;

  if (hpgs_paint_device_drawimage(pdv,img,ll,lr,ur))
    {
      if (hpgs_have_error())
        return hpgs_error_ctxt(hpgs_i18n("drawimage error"));

      return hpgs_set_error(hpgs_i18n("drawimage error."));
    }

  patcol.r = pdv->color.r & (~pdv->patcol.r);
  patcol.g = pdv->color.g & (~pdv->patcol.g);
  patcol.b = pdv->color.b & (~pdv->patcol.b);

#ifdef HPGS_PAINT_DEBUG_ROP3
  hpgs_log("setpatcol: patcol=%d,%d,%d.\n",patcol.r,patcol.g,patcol.b);
#endif

  return hpgs_image_setpatcol(pdv->image,&patcol);
}

static  int pdv_showpage    (hpgs_device *_this, int i)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  int ret = 0;

  if (i>0 && pdv->filename)
    {
      int l = strlen(pdv->filename);
      char *fn = hpgs_alloca(l+20);
      char * dot  = strrchr(pdv->filename,'.');
      int pos = dot ? dot-pdv->filename : l;

#ifdef WIN32
      _snprintf(fn,l+20,"%.*s%4.4d%s",
                pos,pdv->filename,i,pdv->filename+pos);
#else
      snprintf(fn,l+20,"%.*s%4.4d%s",
               pos,pdv->filename,i,pdv->filename+pos);
#endif
      ret = hpgs_image_write(pdv->image,fn);

      if (ret == 0)
        ret = hpgs_image_clear(pdv->image);
    }
  else if (pdv->filename == 0 || strlen(pdv->filename) > 0)
    ret = hpgs_image_write(pdv->image,pdv->filename);

  return ret;
}

static  int pdv_setplotsize (hpgs_device *_this, const hpgs_bbox *bb)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;
  int i;

  if (hpgs_bbox_isequal(&pdv->page_bb,bb))
    return 0;
  
  int w = (int)ceil((bb->urx-bb->llx)*pdv->xres);
  int h = (int)ceil((bb->ury-bb->lly)*pdv->yres);
  
  if (w<2 || h<2)
    return hpgs_set_error(hpgs_i18n("setplotsize: The given bounding box result in a null image."));

  if (hpgs_image_resize(pdv->image,w,h))
    return hpgs_error_ctxt(hpgs_i18n("setplotsize: error resizing image"));
  

  if (pdv->path_clipper) hpgs_paint_clipper_destroy(pdv->path_clipper);

  pdv->path_clipper =  hpgs_new_paint_clipper(bb,
                                              pdv->image->height,
                                              16,pdv->overscan);
  if (!pdv->path_clipper)
    return hpgs_set_error(hpgs_i18n("setplotsize: Out of memory allocating path clipper."));
  
  for (i=0;i<pdv->clip_depth;++i)
    {
      if (pdv->clippers[i]) hpgs_paint_clipper_destroy(pdv->clippers[i]);
      pdv->clippers[i]=0;
    }
  
  // Initial dimension of clip segments is 4.
  pdv->clippers[0] = hpgs_new_paint_clipper(bb,
                                            pdv->image->height,4,pdv->overscan);
  
  if (!pdv->clippers[0])
    return hpgs_set_error(hpgs_i18n("setplotsize: Out of memory allocating clip clipper."));

  if (hpgs_paint_clipper_reset(pdv->clippers[0],bb->llx,bb->urx))
    return hpgs_set_error(hpgs_i18n("setplotsize: Out of memory initializing clip clipper."));
  
  pdv->clip_depth = 1;
  pdv->current_clipper = 0;
  
  return 0;
}

static  int pdv_capabilities (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;

  int ret =
    HPGS_DEVICE_CAP_RASTER |
    HPGS_DEVICE_CAP_MULTIPAGE |
    HPGS_DEVICE_CAP_MULTISIZE |
    HPGS_DEVICE_CAP_DRAWIMAGE;

  if (pdv->overscan)
    ret |= HPGS_DEVICE_CAP_ANTIALIAS;

  if (pdv->image->vtable->setrop3)
    ret |= HPGS_DEVICE_CAP_ROP3;

  return ret;
}

static  void pdv_destroy     (hpgs_device *_this)
{
  hpgs_paint_device *pdv = (hpgs_paint_device *)_this;
  int i;

  if (pdv->filename) free(pdv->filename);
  if (pdv->path) hpgs_paint_path_destroy(pdv->path);
  if (pdv->path_clipper) hpgs_paint_clipper_destroy(pdv->path_clipper);
  if (pdv->gstate) hpgs_gstate_destroy(pdv->gstate);
  if (pdv->image)  hpgs_image_destroy(pdv->image);

  for (i=0;i<pdv->clip_depth;++i)
    if (pdv->clippers[i]) hpgs_paint_clipper_destroy(pdv->clippers[i]);
}

static hpgs_device_vtable pdv_vtable =
  {
    "hpgs_paint_device",
    pdv_moveto,
    pdv_lineto,
    pdv_curveto,
    pdv_newpath,
    pdv_closepath,
    pdv_stroke,
    pdv_fill,
    pdv_clip,
    pdv_clipsave,
    pdv_cliprestore,
    pdv_setrgbcolor,
    pdv_setdash,
    pdv_setlinewidth,
    pdv_setlinecap,
    pdv_setlinejoin,
    pdv_setmiterlimit,
    pdv_setrop3,
    pdv_setpatcol,
    pdv_drawimage,
    pdv_setplotsize,
    0 /* pdv_getplotsize */,
    pdv_showpage,
    0 /* pdv_finish */,
    pdv_capabilities,
    pdv_destroy
  };

/*! Creates a new paint device on the heap.
    Use \c hpgs_destroy in order to destroy the returned
    device pointer.

    The bounding box, which is mapped onto the given \c image is passed
    in as well as the \c antialiasing switch.

    A null pointer is returned, if the system is out of memory.
*/
hpgs_paint_device *hpgs_new_paint_device(hpgs_image *image,
                                         const char *filename,
					 const hpgs_bbox *bb,
					 hpgs_bool antialias )
{
  hpgs_paint_device *ret=(hpgs_paint_device *)malloc(sizeof(hpgs_paint_device));

  if (ret)
    {
      ret->image = image;

      ret->path = 0;
      ret->gstate = 0;
      ret->overscan = antialias; 
      ret->thin_alpha = 0.25;

      if (filename)
        {
          ret->filename = strdup(filename);

          if (!ret->filename) goto fatal_error;
        }
      else
        ret->filename = 0;

      memset(ret->clippers,
	     0,HPGS_PAINT_MAX_CLIP_DEPTH * sizeof(hpgs_paint_clipper *));

      ret->path = hpgs_new_paint_path();
      if (!ret->path) goto fatal_error;

      ret->path_clipper =  hpgs_new_paint_clipper(bb,
						  image->height,
						  16,ret->overscan);
      if (!ret->path_clipper) goto fatal_error;

      ret->gstate = hpgs_new_gstate();
      if (!ret->gstate) goto fatal_error;

     // Initial dimesion of clip segments is 4.
      ret->clippers[0] = hpgs_new_paint_clipper(bb,
						image->height,4,ret->overscan);
      if (!ret->clippers[0]) goto fatal_error;
      if (hpgs_paint_clipper_reset(ret->clippers[0],bb->llx,bb->urx))
	goto fatal_error;

      ret->clip_depth = 1;
      ret->current_clipper = 0;

      ret->page_bb = *bb;
      ret->xres = image->width/(bb->urx-bb->llx);
      ret->yres = image->height/(bb->ury-bb->lly);

      if (hpgs_image_set_resolution(image,72.0*ret->xres,72.0*ret->yres))
        goto fatal_error;

      ret->image_interpolation = 0;

      ret->color.r = 0;
      ret->color.g = 0;
      ret->color.b = 0;
      ret->color.index = 0;

      ret->patcol.r = 0;
      ret->patcol.g = 0;
      ret->patcol.b = 0;

      ret->inherited.vtable    = &pdv_vtable;
    }

  return ret;

 fatal_error:
  if (ret->filename) free(ret->filename);
  if (ret->path) hpgs_paint_path_destroy(ret->path);
  if (ret->clippers[0]) hpgs_paint_clipper_destroy(ret->clippers[0]);
  if (ret->gstate) hpgs_gstate_destroy(ret->gstate);
  if (ret->image)  hpgs_image_destroy(ret->image);
  free(ret);
  return 0;
}

/*! Sets the image interpolation value of the given paint device.
    Currently, the following values are supported:

    \li 0 no image iterpolation
    \li 1 linear image interpolation.

    Other values specifiying square or cubic interpolation may be
    supported in the future. The default value is 0.
*/
void hpgs_paint_device_set_image_interpolation (hpgs_paint_device *pdv, int i)
{
  pdv->image_interpolation = i;
}

/*! Sets the minimal alpha value for thin lines, when antialiasing is
    in effect. The supplied value must be grater than or equal to 0.01 and
    lesser than or equal to 10.0. Other values are ignored.
*/
void hpgs_paint_device_set_thin_alpha (hpgs_paint_device *pdv, double a)
{
  if (a > 0.01 && a <= 10.0)
    pdv->thin_alpha = a;
}

/*! Fills the intersection of the given path with the current clip
    path of the paint device \c pdv using the current graphics state of \c pdv.

    if \c winding is \c HPGS_TRUE, the non-zero winding rule is used for filling,
    otherwise the exclusive-or rule applies.

    Return values:

    \li 0 Sucess.
    \li -1 An error ocurred.
            Call \c hpgs_device_get_error in order to retrieve the error message.

*/
int hpgs_paint_device_fill(hpgs_paint_device *pdv,
			   hpgs_paint_path *path,
			   hpgs_bool winding,
                           hpgs_bool stroke  )
{
  const hpgs_paint_clipper *clip = pdv->clippers[pdv->current_clipper];

  if (hpgs_paint_clipper_cut(pdv->path_clipper,path))
    return hpgs_set_error(hpgs_i18n("fill: Error cutting current path."));

  if (hpgs_paint_clipper_emit(pdv->image,
			      pdv->path_clipper,clip,
			      &pdv->color,winding,stroke))
    {
      if (hpgs_have_error())
	return hpgs_error_ctxt(hpgs_i18n("fill: Image error"));

      return hpgs_set_error(hpgs_i18n("fill: Error emitting scanlines."));
    }
   

  hpgs_paint_clipper_clear(pdv->path_clipper);

  return 0;
}

/*! Sets the intersection of the given path with the current clip
    path of the paint device \c pdv as the current clip path of \c pdv.

    if \c winding is \c HPGS_TRUE, the non-zero winding rule is used for the
    path intersection, otherwise the exclusive-or rule applies.

    Return values:

    \li 0 Sucess.
    \li -1 An error ocurred.
            Call \c hpgs_device_get_error in order to retrieve the error message.

*/
int hpgs_paint_device_clip(hpgs_paint_device *pdv,
			   hpgs_paint_path *path,
			   hpgs_bool winding)
{
  const hpgs_paint_clipper *clip = pdv->clippers[pdv->current_clipper];

  if (hpgs_paint_clipper_cut(pdv->path_clipper,path))
    return hpgs_set_error(hpgs_i18n("clip: Error cutting current path."));

  hpgs_paint_clipper *nclip =
    hpgs_paint_clipper_clip(pdv->path_clipper,clip,winding);

  if (!nclip)
    return hpgs_set_error(hpgs_i18n("clip: Out of memory creating new clipper."));

  // push the new clipper onto the stack of clippers.
  pdv->current_clipper = pdv->clip_depth-1;
      
  if (pdv->clippers[pdv->current_clipper])
    hpgs_paint_clipper_destroy(pdv->clippers[pdv->current_clipper]);
  
  pdv->clippers[pdv->current_clipper] = nclip;

  hpgs_paint_clipper_clear(pdv->path_clipper);

  return 0;
}
