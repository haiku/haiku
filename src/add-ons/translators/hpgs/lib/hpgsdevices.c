/***********************************************************************
 *                                                                     *
 * $Id: hpgsdevices.c 373 2007-01-24 13:43:04Z softadm $
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
 * The implementations of the simple plotsize and eps devices.         *
 *                                                                     *
 ***********************************************************************/

#include <hpgsdevices.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#ifdef WIN32
#include<windows.h>
#else
#include <dlfcn.h>
#endif
#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include<malloc.h>
#include<io.h>
#else
#include<alloca.h>
#include<unistd.h>
#endif

//#define HPGS_EPS_DEBUG_ROP3

/*! \defgroup device Basic vector graphics devices.

   This group contains the definitions for the abstract vector graphics device
   \c hpgs_device as well as the implementations of the two very basic
   vector devices \c hpgs_plotsize_device ans \c hpgs_eps_device.
 */

/*!
   Returns the name of the device for use with RTTI, runtime type information.
 */
const char *hpgs_device_rtti(hpgs_device *_this)
{
  return _this->vtable->rtti;
}

/*! Sets the raster operation for the given device.
    Raster operations and source/pattern transparency are described in

    PCL 5 Comparison Guide, Edition 2, 6/2003, Hewlett Packard
    (May be downloaded as bpl13206.pdf from http://www.hp.com)

    The function returns -1, if an invalid raster operation is specified.
    If the device is not capable of raster operations, the function succeeds
    anyways.
*/
int hpgs_setrop3 (hpgs_device *_this, int rop,
                  hpgs_bool src_transparency, hpgs_bool pattern_transparency)
{
  if (!_this->vtable->setrop3) return 0;

  return _this->vtable->setrop3(_this,rop,
                                src_transparency,pattern_transparency); 
}

/*! Sets the patter ncolor applied using the raster operation specified
    in \c hpgs_setrop3.
*/
int hpgs_setpatcol (hpgs_device *_this, const hpgs_color *rgb)
{
  if (!_this->vtable->setpatcol) return 0;

  return _this->vtable->setpatcol(_this,rgb);
}

/*! Draw an image to the device. 
    The arguments \c ll, \c lr and \c ur are the 
    lower left, lower right and upper right corner of the drawn image
    in world coordinates.

    The function returns 0 on success.
    -1 is returned upon failure.
*/
int hpgs_drawimage(hpgs_device *_this, const hpgs_image *img,
                   const hpgs_point *ll, const hpgs_point *lr,
                   const hpgs_point *ur)
{
  if (!_this->vtable->drawimage) return 0;

  return _this->vtable->drawimage(_this,img,ll,lr,ur); 
}

/*! Report a HPGL PS command to the device. If the function returns 2,
    the interpretation of the file is interrupted immediately without error.

    For devices with the \c HPGS_DEVICE_CAP_MULTISIZE capability, this function
    may be called immediately after a showpage command.
*/
int hpgs_setplotsize (hpgs_device *_this, const hpgs_bbox *bb)
{
  if (!_this->vtable->setplotsize)
    return 0;

  return _this->vtable->setplotsize(_this,bb);
}

/*! Gets the plotsize of the given page number \c i or the overall bounding box,
    if \c i is zero.

    The function returns 0 on success.

    If the function returns 1, the overall bounding box is returned,
    because the plotsize of page \c i is not known.

    -1 is returned, if the funtion is unimplemented for the given device.
*/
int hpgs_getplotsize (hpgs_device *_this, int i, hpgs_bbox *bb)
{
  if (!_this->vtable->getplotsize)
    return -1;

  return _this->vtable->getplotsize(_this,i,bb);
}

/*! Finishes the output of a page to the device. If the function returns 2,
    the interpretation of the file is interrupted immediately without error.
    This is the case for device, which are not capable of displaying multiple
    pages.

    The integer argument is the number of the page begin finished.
    This argument is intended as a hint for devices, which write a file
    for each page.

    If this argument is less than or equal to 0, this is the only page
    written to the device. In this case, devices which write a file for
    each page may omit a page counter from the filename of the written
    file.
*/
int hpgs_showpage (hpgs_device *_this, int i)
{
  if (!_this->vtable->showpage)
    return 0;

  return _this->vtable->showpage(_this,i);
}

/*! Finishes the output of a document to the device.
    Implementations of device should discard all output,
    which has been undertaken since the past showpage call.
*/
int hpgs_device_finish (hpgs_device *_this)
{
  if (!_this->vtable->finish)
    return 0;

  return _this->vtable->finish(_this);
}


/*! Destroys the given device and frees all allocated resources by this device.
*/
void hpgs_device_destroy (hpgs_device *_this)
{
  _this->vtable->destroy(_this);
  free(_this);
}

static int eps_expand_media_sizes(hpgs_eps_device *eps)
{
  int n = eps->media_sizes_alloc_size*2;
  hpgs_ps_media_size *nnms = realloc(eps->media_sizes,sizeof(hpgs_bbox)*n);

  if (!nnms)
    return hpgs_set_error(hpgs_i18n("Out of memory expanding page size stack."));
  
  eps->media_sizes=nnms;
  eps->media_sizes_alloc_size=n;
  return 0;
}

#define HPGS_MEDIA_SIZE_HASH(w,h) (size_t)((419430343*((size_t)(w)))^((size_t)(h)))

static int eps_get_papersize (hpgs_eps_device *eps,
                              char *name, size_t name_len,
                              double w, double h)
{
  int paper_width = (int)ceil(w);
  int paper_height = (int)ceil(h);
  size_t hash = HPGS_MEDIA_SIZE_HASH(paper_width,paper_height);
  int i0 = 0;
  int i1 = eps->n_media_sizes;

  // binary search for hash;
  while (i1>i0)
    {
      int i = i0+(i1-i0)/2;

      if (eps->media_sizes[i].hash < hash)
        i0 = i+1;
      else
        i1 = i;
    }

  i1 = 0;

  // search all media sizes with equal hash
  while (i0 < eps->n_media_sizes &&
         eps->media_sizes[i0].hash == hash)
    {
      if (eps->media_sizes[i0].width == paper_width &&
          eps->media_sizes[i0].height == paper_height  )
        { i1 = 1; break; }
      
      ++i0;
    }
          
  if (i1)
    {
      ++eps->media_sizes[i0].usage;
    }
  else
    {
      // no page size found -> make a new one
      if (eps->n_media_sizes>=eps->media_sizes_alloc_size &&
          eps_expand_media_sizes(eps))
        return -1;

      if (i0 < eps->n_media_sizes)
        memmove (eps->media_sizes+i0+1,eps->media_sizes+i0,
                 sizeof(hpgs_ps_media_size) * (eps->n_media_sizes-i0));

      eps->media_sizes[i0].width = paper_width;
      eps->media_sizes[i0].height = paper_height;
      eps->media_sizes[i0].name  = 0;
      eps->media_sizes[i0].usage = 1;
      eps->media_sizes[i0].hash  = hash;
    }

  // fill in media size.
  if (eps->media_sizes[i0].name)
    {
      strcpy(name,eps->media_sizes[i0].name);
    }
  else
    {
#ifdef WIN32
      _snprintf(name,name_len,"Custom%dx%d",
                paper_width,paper_height);
#else
      snprintf(name,name_len,"Custom%dx%d",
               paper_width,paper_height);
#endif
    }
  return 0;
}


static int eps_startpage (hpgs_eps_device *eps)
{
  if (eps->n_pages < 0)
    {
      // EPS page file start
      eps->out = hpgs_new_file_ostream (eps->filename);

      if (!eps->out)
        return hpgs_set_error(hpgs_i18n("Error opening file %s: %s"),
                              eps->filename,strerror(errno));

      if (hpgs_ostream_printf (eps->out,
                               "%%!PS-Adobe-3.0 EPSF-3.0\n"
                               "%%%%Creator: hpgs-%s\n"
                               "%%%%Title: %s\n"
                               "%%%%BoundingBox: %d %d %d %d\n"
                               "%%%%HiResBoundingBox: %g %g %g %g\n"
                               "/hpgsdict 20 dict def\n"
                               "hpgsdict begin\n"
                               "/B{bind def} bind def\n"
                               "/r{setrgbcolor}B/w{setlinewidth}B/j{setlinejoin}B/J{setlinecap}B/M{setmiterlimit}B\n"
                               "/d{setdash}B/n{newpath}B/h{closepath}B/m{moveto}B/l{lineto}B/c{curveto}B\n"
                               "/s{stroke}B/f{fill}B/g{eofill}B/x{clip}B/y{eoclip}B/cs{gsave}B\n"
                               "/cr{currentrgbcolor currentlinewidth currentlinecap currentlinejoin currentmiterlimit currentdash grestore setdash setmiterlimit setlinejoin setlinecap setlinewidth setrgbcolor}B\n"
                               "end\n"
                               "%%EndComments\n"
                               "%%EndProlog\n"
                               "hpgsdict begin\n"
                               "gsave\n",
                               HPGS_VERSION,
                               eps->filename ? eps->filename : "(stdout)",
                               (int)floor(eps->page_bb.llx),(int)floor(eps->page_bb.lly),
                               (int)ceil(eps->page_bb.urx),(int)ceil(eps->page_bb.ury),
                               eps->page_bb.llx,eps->page_bb.lly,
                               eps->page_bb.urx,eps->page_bb.ury ) < 0)
        {
          hpgs_ostream_close(eps->out);
          eps->out = 0;
          return hpgs_set_error(hpgs_i18n("Error writing header of file %s: %s"),
                                eps->filename?eps->filename:"(stdout)",
                                strerror(errno));
        }
      eps->page_setup = HPGS_TRUE;
    }
  else
    {
      hpgs_bbox bb;
      char paper_name[32];
      double paper_width = eps->page_bb.urx;
      double paper_height = eps->page_bb.ury;

      hpgs_bool landscape = paper_width > paper_height;

      if (eps_get_papersize(eps,paper_name,sizeof(paper_name),
                            landscape ? paper_height : paper_width,
                            landscape ? paper_width  : paper_height))
        return -1;

      if (landscape)
        {
          if (hpgs_ostream_printf (eps->out,
                                   "%%%%Page: %d %d\n"
                                   "%%%%PageMedia: %s\n"
                                   "%%%%PageOrientation: Landscape\n"
                                   "hpgsdict begin\n"
                                   "gsave\n"
                                   "%.8f %.8f translate\n"
                                   "90 rotate\n",
                                   eps->n_pages+1,eps->n_pages+1,paper_name,
                                   paper_height,0.0 ) < 0)
            return hpgs_set_error(hpgs_i18n("Error writing header of file %s: %s"),
                                  eps->filename?eps->filename:"(stdout)",
                                  strerror(errno));


          // caclulate the bounding box on the page.
          bb.llx = 0.0;
          bb.lly = 0.0;
          bb.urx = paper_height;
          bb.ury = paper_width;
        }
      else
        {
          if (hpgs_ostream_printf (eps->out,
                                   "%%%%Page: %d %d\n"
                                   "%%%%PageMedia: %s\n"
                                   "%%%%PageOrientation: Portrait\n"
                                   "gsave\n",
                                   eps->n_pages+1,eps->n_pages+1,paper_name ) < 0)
            return hpgs_set_error(hpgs_i18n("Error writing header of file %s: %s"),
                                  eps->filename?eps->filename:"(stdout)",
                                  strerror(errno));

          // calculate the bounding box on the page.
          bb = eps->page_bb;
        }

      // expand document bounding box.
      if (eps->n_pages)
        hpgs_bbox_expand(&eps->doc_bb,&bb);
      else
        eps->doc_bb = bb;

      ++eps->n_pages;
      eps->page_setup = HPGS_TRUE;
    }

  return 0;
}


static  int eps_moveto (hpgs_device *_this, const hpgs_point *p)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%g %g m\n",p->x,p->y) < 0)
    return hpgs_set_error("moveto: %s",strerror(errno));
  return 0;
}

static  int eps_lineto (hpgs_device *_this, const hpgs_point *p)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%g %g l\n",p->x,p->y) < 0)
    return hpgs_set_error("lineto: %s",strerror(errno));
  return 0;
}

static  int eps_curveto (hpgs_device *_this, const hpgs_point *p1,
			 const hpgs_point *p2, const hpgs_point *p3 )
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%g %g %g %g %g %g c\n",
		 p1->x,p1->y,p2->x,p2->y,p3->x,p3->y) < 0)
    return hpgs_set_error("curveto: %s",strerror(errno));
  return 0;
}

static  int eps_newpath     (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"n\n") < 0)
    return hpgs_set_error("newpath: %s",strerror(errno));
  return 0;
}

static  int eps_closepath   (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"h\n") < 0)
    return hpgs_set_error("closepath: %s",strerror(errno));
  return 0;
}

static  int eps_stroke      (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"s\n") < 0)
    return hpgs_set_error("stroke: %s",strerror(errno));
  return 0;
}

static  int eps_fill        (hpgs_device *_this, hpgs_bool winding)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,winding ? "f\n" : "g\n") < 0)
    return hpgs_set_error("fill: %s",strerror(errno));
  return 0;
}

static  int eps_clip        (hpgs_device *_this, hpgs_bool winding)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,winding ? "x\n" : "y\n") < 0)
    return hpgs_set_error("clip: %s",strerror(errno));
  return 0;
}

static  int eps_clipsave       (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"cs\n") < 0)
    return hpgs_set_error("clipsave: %s",strerror(errno));
  return 0;
}

static  int eps_cliprestore    (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"cr\n") < 0)
    return hpgs_set_error("cliprestore: %s",strerror(errno));
  return 0;
}

static  int eps_setrgbcolor (hpgs_device *_this, const hpgs_color *rgb)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%g %g %g r\n",rgb->r,rgb->g,rgb->b) < 0)
    return hpgs_set_error("setrgbcolor: %s",strerror(errno));

  eps->color = *rgb;
  return 0;
}

static  int eps_setdash (hpgs_device *_this, const float *segs,
			 unsigned n, double d)
{
  unsigned i;
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"[ ")<0) return 1;

  for (i=0;i<n;++i)
    if (hpgs_ostream_printf(eps->out,"%f ",(double)segs[i])<0)
      return hpgs_set_error("setdash: %s",strerror(errno));

  if (hpgs_ostream_printf(eps->out,"] %g d\n",d)<0)
    return hpgs_set_error("setdash: %s",strerror(errno));
  return 0;
}

static  int eps_setlinewidth(hpgs_device *_this, double lw)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%g w\n",lw) < 0)
    return hpgs_set_error("setlinewidth: %s",strerror(errno));
  return 0;
}

static  int eps_setlinecap  (hpgs_device *_this, hpgs_line_cap lc)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%d J\n",(int)lc) < 0)
    return hpgs_set_error("setlinecap: %s",strerror(errno));
  return 0;
}

static  int eps_setlinejoin (hpgs_device *_this, hpgs_line_join lj)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%d j\n",(int)lj) < 0)
    return hpgs_set_error("setlinejoin: %s",strerror(errno));
  return 0;
}

static  int eps_setmiterlimit (hpgs_device *_this, double l)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  if (hpgs_ostream_printf(eps->out,"%g M\n",l) < 0)
    return hpgs_set_error("setmiterlimit: %s",strerror(errno));
  return 0;
}

static  int eps_setrop3 (hpgs_device *_this, int rop,
                         hpgs_bool src_transparency, hpgs_bool pattern_transparency)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;
  hpgs_xrop3_func_t rop3;

  if (!eps->rop3) return 0;

#ifdef HPGS_EPS_DEBUG_ROP3
  hpgs_log("setrop3: rop,src_trans,pat_trans=%d,%d,%d.\n",
          rop,src_transparency,pattern_transparency);
#endif
  rop3 = hpgs_xrop3_func(rop,src_transparency,pattern_transparency);

  if (!rop3)
    return hpgs_set_error("setrop3: Invalid ROP3 %d specified",rop);

  eps->rop3 = rop3;
  return 0;
}

static  int eps_setpatcol(hpgs_device *_this, const hpgs_color *rgb)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  eps->pattern_color.r = (unsigned char)(rgb->r * 255.0);
  eps->pattern_color.g = (unsigned char)(rgb->g * 255.0);
  eps->pattern_color.b = (unsigned char)(rgb->b * 255.0);

#ifdef HPGS_EPS_DEBUG_ROP3
  hpgs_log("setpatcol: patcol=%g,%g,%g.\n",rgb->r,rgb->g,rgb->b);
#endif

  return 0;
}

static int eps_drawimage(hpgs_device *_this, const hpgs_image *img,
                         const hpgs_point *ll, const hpgs_point *lr,
                         const hpgs_point *ur)
{
  static const char *hex_4_bit = "0123456789abcdef";  
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;
  int i,j;
  hpgs_point ul;
  int ret = -1;

  if (!img)
    return hpgs_set_error("drawimage: Null image specified.");

  if (!eps->page_setup && eps_startpage(eps)) return -1;

  ul.x = ll->x + (ur->x - lr->x);
  ul.y = ll->y + (ur->y - lr->y);

  hpgs_palette_color *data = (hpgs_palette_color *)
    malloc(img->width*img->height*sizeof(hpgs_palette_color));

  int have_clip = 0;

  // data aqusition.
  if (eps->rop3)
    {
      hpgs_color rgb = eps->color;

      switch (hpgs_image_rop3_clip(_this,data,img,
                                   ll,lr,ur,
                                   &eps->pattern_color,eps->rop3))
        {
        case 0:
          // invisible.
          ret = 0;
          goto cleanup;
        case 1:
          // clipped.
          have_clip = 1;
        case 2:
          // totally visible.
          break;
        case 3:
          // optimized to a single color.
          eps->color = rgb;
          if (hpgs_ostream_printf(eps->out,"%g %g %g r\n",
                                  eps->color.r,eps->color.g,eps->color.b) >= 0)
            ret = 0;
          goto cleanup;
        default:
          goto cleanup;
        }
    }
  else
    {
      hpgs_palette_color *d = data;

      for (j=0; j<img->height;++j)
        {
          hpgs_paint_color c;
          
          for (i=0; i<img->width;++i,++d)
            {
              hpgs_image_get_pixel(img,i,j,&c,(double *)0/*alpha*/);
              d->r = c.r;
              d->g = c.g;
              d->b = c.b;
            }
        }
    }

  if (!have_clip)
    hpgs_ostream_printf(eps->out,
                        "gsave\n");

  if (hpgs_ostream_printf(eps->out,
              "[ %g %g %g %g %g %g ] concat\n"
              "%d %d 8\n"
              "[ %d 0 0 %d 0 0 ]\n"
              "{ currentfile %d string readhexstring pop }\n"
              "dup dup true 3 colorimage\n",
              lr->x-ll->x,lr->y-ll->y,lr->x-ur->x,lr->y-ur->y,ul.x,ul.y,
              img->width,img->height,
              img->width,img->height,img->width) < 0)
    { hpgs_set_error("drawimage: %s",strerror(errno)); goto cleanup; }
  
  // write data
  for (j=0; j<img->height; ++j)
    {
      hpgs_palette_color *scanline = data + j * img->width;

      for (i=0; i<img->width; i++)
        {
          hpgs_ostream_putc(hex_4_bit[scanline[i].r>>4],eps->out);
          hpgs_ostream_putc(hex_4_bit[scanline[i].r&0xf],eps->out);
        }
      for (i=0; i<img->width; i++)
        {
          hpgs_ostream_putc(hex_4_bit[scanline[i].g>>4],eps->out);
          hpgs_ostream_putc(hex_4_bit[scanline[i].g&0xf],eps->out);
        }
      for (i=0; i<img->width; i++)
        {
          hpgs_ostream_putc(hex_4_bit[scanline[i].b>>4],eps->out);
          hpgs_ostream_putc(hex_4_bit[scanline[i].b&0xf],eps->out);
        }
    }

  if (hpgs_ostream_printf(eps->out,"\ngrestore\n")<0)
    { hpgs_set_error("drawimage: %s",strerror(errno)); goto cleanup; }

  ret = 0;

 cleanup:
  free(data);

  return ret;
}

static  int eps_showpage    (hpgs_device *_this, int i)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->out) return 0;

  if (hpgs_ostream_printf(eps->out,"grestore\nshowpage\nend\n") < 0)
    return hpgs_set_error("showpage: %s",strerror(errno));

  // write an eps file, if we are not in multipage mode.
  if (eps->n_pages<0)
    { 
      if (hpgs_ostream_printf(eps->out,"%%%%EOF\n")<0 ||
          hpgs_ostream_close(eps->out) < 0)
        return hpgs_set_error("showpage: %s",strerror(errno));

      eps->out = 0;

      if (i > 0 && eps->filename)
        {
          int l = strlen(eps->filename);
          char *fn = hpgs_alloca(l+20);
          char *dot  = strrchr(eps->filename,'.');
          int pos = dot ? dot-eps->filename : l;

#ifdef WIN32
          _snprintf(fn,l+20,"%.*s%4.4d%s",
                    pos,eps->filename,i,eps->filename+pos);
#else
          snprintf(fn,l+20,"%.*s%4.4d%s",
                   pos,eps->filename,i,eps->filename+pos);
#endif
          l = rename(eps->filename,fn);

          if (l<0)
            return hpgs_set_error(hpgs_i18n("Error moving file %s to %.*s%4.4d%s.eps: %s"),
                                  eps->filename,
                                  pos,eps->filename,i,eps->filename+pos,
                                  strerror(errno));
        }
    }

  eps->page_setup = HPGS_FALSE;

  return 0;
}

static int eps_setplotsize (hpgs_device *_this, const hpgs_bbox *bb)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  eps->page_bb = *bb;

  return 0;
}

static  int eps_capabilities (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  int ret =
    HPGS_DEVICE_CAP_VECTOR |
    HPGS_DEVICE_CAP_MULTIPAGE |
    HPGS_DEVICE_CAP_MULTISIZE |
    HPGS_DEVICE_CAP_DRAWIMAGE;

  if (eps->n_pages >= 0)
    ret |= HPGS_DEVICE_CAP_PAGECOLLATION;

  if (eps->rop3)
    ret |= HPGS_DEVICE_CAP_ROP3;

  return ret;
}

static  int eps_finish (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;

  if (!eps->out) return 0;

  if (eps->n_pages < 0)
    {
      // discard trailing output in EPS mode.
      if (hpgs_ostream_close(eps->out) < 0)
        {
          eps->out = 0;
          return hpgs_set_error("finish: %s",strerror(errno));
        }

      eps->out = 0;
  
      if (eps->filename && unlink(eps->filename)<0)
        return hpgs_set_error(hpgs_i18n("Error removing file %s: %s"),
                              eps->filename,strerror(errno));
    }
  else
    {
      // now really write the postscript file
      int i,i_media=0;
      hpgs_istream *buffer;

      hpgs_ostream *out = hpgs_new_file_ostream (eps->filename);

      if (!out)
        return hpgs_set_error(hpgs_i18n("Error opening file %s: %s"),
                              eps->filename,strerror(errno));

      if (hpgs_ostream_flush(eps->out) <0 ||
          (buffer =hpgs_ostream_getbuf(eps->out)) == 0)
        return hpgs_set_error(hpgs_i18n("Error getting page stream data"));
     
      hpgs_ostream_printf (out,
                           "%%!PS-Adobe-3.0\n"
                           "%%%%Creator: hpgs-%s\n"
                           "%%%%Title: %s\n"
                           "%%%%Pages: %d\n"
                           "%%%%BoundingBox: %d %d %d %d\n"
                           "%%%%HiResBoundingBox: %.3f %.3f %.3f %.3f\n"
                           "%%%%DocumentMedia:",
                           HPGS_VERSION,
                           eps->filename ? eps->filename : "(stdout)",
                           eps->n_pages,
                           (int)floor(eps->doc_bb.llx),(int)floor(eps->doc_bb.lly),
                           (int)ceil(eps->doc_bb.urx),(int)ceil(eps->doc_bb.ury),
                           eps->doc_bb.llx,eps->doc_bb.lly,
                           eps->doc_bb.urx,eps->doc_bb.ury);

      // write paper sizes.
      for (i=0;i<eps->n_media_sizes;++i)
        if (eps->media_sizes[i].usage)
          {
            if (eps->media_sizes[i].name)
              hpgs_ostream_printf (out,"%s %s %d %d 75 white()\n",
                                   i_media ? "%%+" : "",
                                   eps->media_sizes[i].name,
                                   eps->media_sizes[i].width,
                                   eps->media_sizes[i].height );
            else
              hpgs_ostream_printf (out,"%s Custom%dx%d %d %d 75 white()\n",
                                   i_media ? "%%+" : "",
                                   eps->media_sizes[i].width,
                                   eps->media_sizes[i].height,
                                   eps->media_sizes[i].width,
                                   eps->media_sizes[i].height );
            
            ++i_media;
          }

      hpgs_ostream_printf (out,
                           "%%%%EndComments\n"
                           "/hpgsdict 20 dict def\n"
                           "hpgsdict begin\n"
                           "/B{bind def} bind def\n"
                           "/r{setrgbcolor}B/w{setlinewidth}B/j{setlinejoin}B/J{setlinecap}B/M{setmiterlimit}B\n"
                           "/d{setdash}B/n{newpath}B/h{closepath}B/m{moveto}B/l{lineto}B/c{curveto}B\n"
                           "/s{stroke}B/f{fill}B/g{eofill}B/x{clip}B/y{eoclip}B/cs{gsave}B\n"
                           "/cr{currentrgbcolor currentlinewidth currentlinecap currentlinejoin currentmiterlimit currentdash grestore setdash setmiterlimit setlinejoin setlinecap setlinewidth setrgbcolor}B\n"
                           "end\n"
                           "%%%%EndProlog\n");

      hpgs_copy_streams (out,buffer);
      hpgs_istream_close(buffer);

      hpgs_ostream_printf (out,"%%%%EOF\n");

      if (hpgs_ostream_iserror(out))
        {
          if (eps->filename)
            hpgs_ostream_close(out);
          return hpgs_set_error(hpgs_i18n("Error writing file %s: %s"),
                                eps->filename?eps->filename:"(stdout)",
                                strerror(errno));
        }

      hpgs_ostream_close(out);
    }

  return 0;
}

static  void eps_destroy     (hpgs_device *_this)
{
  hpgs_eps_device *eps = (hpgs_eps_device *)_this;
  if (eps->out)
    hpgs_ostream_close(eps->out);  

  if (eps->filename)
    free(eps->filename);
}

static hpgs_device_vtable eps_vtable =
  {
    "hpgs_eps_device",
    eps_moveto,
    eps_lineto,
    eps_curveto,
    eps_newpath,
    eps_closepath,
    eps_stroke,
    eps_fill,
    eps_clip,
    eps_clipsave,
    eps_cliprestore,
    eps_setrgbcolor,
    eps_setdash,
    eps_setlinewidth,
    eps_setlinecap,
    eps_setlinejoin,
    eps_setmiterlimit,
    eps_setrop3,
    eps_setpatcol,
    eps_drawimage,
    eps_setplotsize,
    0 /* eps_getplotsize */,
    eps_showpage,
    eps_finish,
    eps_capabilities,
    eps_destroy
  };

/*! Retrieves the pointer to a new \c hpgs_eps_device on the heap,
    which writes to the file with the gieven \c filename.

    The bounding box in the eps files is passed to this functions.

    If the file cannot be opened or the system is out of memory,
    a null pointer is returned.
*/
hpgs_eps_device *hpgs_new_eps_device(const char *filename,
				     const hpgs_bbox *bb,
                                     hpgs_bool do_rop3)
{
  hpgs_eps_device *ret = (hpgs_eps_device *)malloc(sizeof(hpgs_eps_device));

  if (!ret)
    return 0;

  ret->out = 0;
  ret->n_pages = -1;
  ret->page_setup = HPGS_FALSE;

  if (filename)
    {
      ret->filename = strdup(filename);

      if (!ret->filename)
        {
          free(ret);
          return 0;
        }
    }
  else
    ret->filename = 0;

  ret->pattern_color.r = 0;
  ret->pattern_color.g = 0;
  ret->pattern_color.b = 0;
  
  if (do_rop3)
    ret->rop3 = hpgs_xrop3_func(252,HPGS_TRUE,HPGS_TRUE);
  else
    ret->rop3 = 0;

  ret->doc_bb = *bb;
  ret->page_bb = *bb;

  ret->media_sizes_alloc_size = 0; 
  ret->n_media_sizes = 0; 
  ret->media_sizes = 0; 

  ret->color.r = 0.0;
  ret->color.g = 0.0;
  ret->color.b = 0.0;

  ret->inherited.vtable=&eps_vtable;

  return ret;
}

#define HPGS_STD_MEDIA_SIZE(w,h,n) {w,h,n,0,HPGS_MEDIA_SIZE_HASH(w,h)}

static hpgs_ps_media_size ps_std_media_sizes[]=
  {
    HPGS_STD_MEDIA_SIZE(596 ,843 ,"A4"),
    HPGS_STD_MEDIA_SIZE(843 ,1192,"A3"),
    HPGS_STD_MEDIA_SIZE(1192,1686,"A2"),
    HPGS_STD_MEDIA_SIZE(1686,2384,"A1"),
    HPGS_STD_MEDIA_SIZE(2384,3371,"A0")
  };

/* A helper for qsort */
static int compare_media_hashes(const void *a, const void *b)
{
  const hpgs_ps_media_size *m1 = (const hpgs_ps_media_size *)a;
  const hpgs_ps_media_size *m2 = (const hpgs_ps_media_size *)b;

  if (m1->hash < m2->hash) return -1;
  if (m1->hash > m2->hash) return 1;
  return 0;
}


/*! Retrieves the pointer to a new \c hpgs_eps_device on the heap,
    which writes to a multipage PostScript file with the given \c filename.

    The overall document bounding box for the PostScript file is
    passed as \c bb.

    If \c paper_width and \c paper_height are greater than zero,
    the content of each page is scaled to this fixed paper format.
    Otherwise, the paper size of each page adpats to the page
    bounding box.

    The given \c border is used in order to place the contents on the
    page.

    If the file cannot be opened or the system is out of memory,
    a null pointer is returned.
*/
hpgs_eps_device *hpgs_new_ps_device(const char *filename,
                                    const hpgs_bbox *bb,
                                    hpgs_bool do_rop3)
{
  hpgs_eps_device *ret = (hpgs_eps_device *)malloc(sizeof(hpgs_eps_device));

  if (!ret)
    return 0;

  ret->out = hpgs_new_mem_ostream(1024*1024);

  if (!ret->out)
    {
      free(ret);
      return 0;
    }

  ret->n_pages = 0;
  ret->page_setup = HPGS_FALSE;

  if (filename)
    {
      ret->filename = strdup(filename);

      if (!ret->filename)
        {
          hpgs_ostream_close(ret->out);
          free(ret);
          return 0;
        }
    }
  else
    ret->filename = 0;

  ret->pattern_color.r = 0;
  ret->pattern_color.g = 0;
  ret->pattern_color.b = 0;
  
  if (do_rop3)
    ret->rop3 = hpgs_xrop3_func(252,HPGS_TRUE,HPGS_TRUE);
  else
    ret->rop3 = 0;

  ret->doc_bb = *bb;
  ret->page_bb = *bb;

  ret->media_sizes_alloc_size = 32; 
  ret->media_sizes =
    (hpgs_ps_media_size*)malloc(sizeof(hpgs_ps_media_size)*ret->media_sizes_alloc_size);
      
  if (!ret->media_sizes)
    {
      hpgs_ostream_close(ret->out);
      free(ret->filename);
      free(ret);
      return 0;
    }

  // fill in std media sizes.
  ret->n_media_sizes = sizeof(ps_std_media_sizes)/sizeof(hpgs_ps_media_size);
  memcpy(ret->media_sizes,ps_std_media_sizes,sizeof(ps_std_media_sizes));

  qsort(ret->media_sizes,ret->n_media_sizes,
        sizeof(hpgs_ps_media_size),compare_media_hashes);

  ret->color.r = 0.0;
  ret->color.g = 0.0;
  ret->color.b = 0.0;

  ret->inherited.vtable=&eps_vtable;

  return ret;
}

/*!
  Plotsize device.
*/
static int pls_expand_pages(hpgs_plotsize_device *pls)
{
  int i,n = pls->page_bbs_alloc_size*2;
  hpgs_bbox *nbbs;

  if (pls->n_page_bbs > n) n=pls->n_page_bbs;

  nbbs = realloc(pls->page_bbs,sizeof(hpgs_bbox)*n);

  if (!nbbs)
    return hpgs_set_error(hpgs_i18n("Out of memory expanding page size stack."));
  
  pls->page_bbs=nbbs;

  for (i=pls->page_bbs_alloc_size;i<n;++i)
    hpgs_bbox_null(pls->page_bbs+i);

  pls->page_bbs_alloc_size=n;
  return 0;
}

static  int pls_moveto (hpgs_device *_this, const hpgs_point *p)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  pls->moveto = *p;
  pls->deferred_moveto = 1;

  return 0;
}

static  int pls_lineto (hpgs_device *_this, const hpgs_point *p)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (pls->deferred_moveto)
    {
      hpgs_bbox_add(&pls->path_bb,&pls->moveto);
      pls->deferred_moveto = 0;
    }

  hpgs_bbox_add(&pls->path_bb,p);
  pls->moveto = *p;

  return 0;
}

static void add_bezier(hpgs_plotsize_device *pls,
                       const hpgs_point *p0, const hpgs_point *p1,
                       const hpgs_point *p2, const hpgs_point *p3,
                       int depth )
{
  hpgs_point ll =  { HPGS_MIN(p0->x,p3->x),HPGS_MIN(p0->y,p3->y) };
  hpgs_point ur =  { HPGS_MAX(p0->x,p3->x),HPGS_MAX(p0->y,p3->y) };

  // p1 and p2 inside bbox of p0 and p3.
  if (depth > 2 ||
      p1->x < ll.x || p2->x < ll.x ||
      p1->y < ll.y || p2->y < ll.y ||
      p1->x > ur.x || p2->x > ur.x ||
      p1->y > ur.y || p2->y > ur.y   )
    {
      hpgs_bbox_add(&pls->path_bb,p3);
    }
  else
    {
      // split spline
      hpgs_point p1l = { 0.5 * (p0->x + p1->x), 0.5 * (p0->y + p1->y) };
      hpgs_point p2l = { 0.25 * (p0->x + p2->x) + 0.5 * p1->x, 0.25 * (p0->y + p2->y) + 0.5 * p1->y };
      hpgs_point pm = { (p0->x + p3->x) * 0.125 + 0.375 * (p1->x + p2->x), (p0->y + p3->y) * 0.125 + 0.375 * (p1->y + p2->y)  };
      hpgs_point p1u = { 0.25 * (p1->x + p3->x) + 0.5 * p2->x, 0.25 * (p1->y + p3->y) + 0.5 * p2->y };
      hpgs_point p2u = { 0.5 * (p2->x + p3->x), 0.5 * (p2->y + p3->y) };

      add_bezier(pls,p0,&p1l,&p2l,&pm,depth+1);
      add_bezier(pls,&pm,&p1u,&p2u,p3,depth+1);
    }
}

static  int pls_curveto (hpgs_device *_this, const hpgs_point *p1,
			 const hpgs_point *p2, const hpgs_point *p3 )
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (pls->deferred_moveto)
    {
      hpgs_bbox_add(&pls->path_bb,&pls->moveto);
      pls->deferred_moveto = 0;
    }

  add_bezier(pls,&pls->moveto,p1,p2,p3,0);

  pls->moveto = *p3;

  return 0;
}

static  int pls_addpath (hpgs_device *_this, hpgs_bool do_linewidth)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (do_linewidth)
    {
      hpgs_bbox_addborder(&pls->path_bb,0.5 * pls->linewidth);
    }

  if (pls->clip_depth >= 0)
    {
      hpgs_bbox_intersect(&pls->path_bb,pls->clip_bbs+pls->clip_depth);

      // check for null intersection
      if (hpgs_bbox_isnull(&pls->path_bb))
        goto get_out;
    }

  hpgs_bbox_expand(&pls->page_bb,&pls->path_bb);
  hpgs_bbox_expand(&pls->global_bb,&pls->path_bb);

 get_out:
  hpgs_bbox_null(&pls->path_bb);

  pls->moveto.x = 0.0;
  pls->moveto.y = 0.0;
  pls->deferred_moveto = 0;

  return 0;
}

static  int pls_stroke (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  return pls_addpath (_this,pls->do_linewidth);
}

static  int pls_fill (hpgs_device *_this, hpgs_bool winding)
{
  return pls_addpath (_this,HPGS_FALSE);
}

static  int pls_closepath   (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  pls->deferred_moveto = 1;

  return 0;
}

static  int pls_newpath   (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  hpgs_bbox_null(&pls->path_bb);

  pls->moveto.x = 0.0;
  pls->moveto.y = 0.0;
  pls->deferred_moveto = 0;

  return 0;
}

static  int pls_clip        (hpgs_device *_this, hpgs_bool winding)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  hpgs_bbox_intersect(pls->clip_bbs+pls->clip_depth,&pls->path_bb);

  return 0;
}

static  int pls_clipsave       (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (pls->clip_depth+1 >= HPGS_PLOTSIZE_MAX_CLIP_DEPTH)
    return hpgs_set_error(hpgs_i18n("Maximum clip depth %d exceeded."),
                          HPGS_PLOTSIZE_MAX_CLIP_DEPTH);

  pls->clip_bbs[pls->clip_depth+1] = pls->clip_bbs[pls->clip_depth] ;

  ++pls->clip_depth;

  return 0;
}

static  int pls_cliprestore    (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;
  --pls->clip_depth;

  if (pls->clip_depth < 0)
    return hpgs_set_error("cliprestore: clip stack underflow.");

  return 0;
}

static int pls_setlinewidth(hpgs_device *_this, double lw)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  pls->linewidth = lw;
  return 0;
}

static int pls_drawimage(hpgs_device *_this, const hpgs_image *img,
                         const hpgs_point *ll, const hpgs_point *lr,
                         const hpgs_point *ur)
{
  hpgs_point ul;

  ul.x = ll->x + (ur->x - lr->x);
  ul.y = ll->y + (ur->y - lr->y);

  pls_newpath(_this);
  pls_moveto(_this,ll);
  pls_lineto(_this,lr);
  pls_lineto(_this,ur);
  pls_lineto(_this,&ul);
  pls_addpath(_this,HPGS_FALSE);

  return 0;
}

static  int pls_showpage (hpgs_device *_this, int i)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (i>0)
    {
      if (pls->page_bb.urx < pls->page_bb.llx)
        pls->page_bb.urx = pls->page_bb.llx = 0.0;

      if (pls->page_bb.ury < pls->page_bb.lly)
        pls->page_bb.ury = pls->page_bb.lly = 0.0;

      if (i > pls->n_page_bbs) pls->n_page_bbs=i;

      if (pls->n_page_bbs >  pls->page_bbs_alloc_size &&
          pls_expand_pages(pls))
        return -1;

      pls->page_bbs[i-1] = pls->page_bb;
    }

  hpgs_bbox_null(&pls->page_bb);

  return 0;
}

static  int pls_finish (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (pls->global_bb.urx < pls->global_bb.llx)
    pls->global_bb.urx = pls->global_bb.llx = 0.0;

  if (pls->global_bb.ury < pls->global_bb.lly)
    pls->global_bb.ury = pls->global_bb.lly = 0.0;

  return 0;
}

static  int pls_capabilities (hpgs_device *_this)
{
  return
    HPGS_DEVICE_CAP_PLOTSIZE |
    HPGS_DEVICE_CAP_MULTIPAGE |
    HPGS_DEVICE_CAP_MULTISIZE |
    HPGS_DEVICE_CAP_NULLIMAGE;  
}

static  void pls_destroy     (hpgs_device *_this)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (pls->page_bbs)
    free (pls->page_bbs);
}

static  int pls_setplotsize (hpgs_device *_this, const hpgs_bbox *bb)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  if (pls->ignore_ps)
    return 0;

  pls->global_bb = *bb;
  pls->page_bb = *bb;

  return 2;
}

static  int pls_getplotsize (hpgs_device *_this, int i, hpgs_bbox *bb)
{
  hpgs_plotsize_device *pls = (hpgs_plotsize_device *)_this;

  int ret = 0;

  if (i<=0 || i > pls->n_page_bbs)
    {
      *bb = pls->global_bb;
      if (i>0) ret = 1;
    }
  else
    *bb = pls->page_bbs[i-1];

  return ret;
}

static hpgs_device_vtable pls_vtable =
  {
    "hpgs_plotsize_device",
    pls_moveto,
    pls_lineto,
    pls_curveto,
    pls_newpath,
    pls_closepath,
    pls_stroke,
    pls_fill,
    pls_clip,
    pls_clipsave,
    pls_cliprestore,
    0 /* pls_setrgbcolor */,
    0 /* pls_setdash */,
    pls_setlinewidth,
    0 /* pls_setlinecap */,
    0 /* pls_setlinejoin */,
    0 /* pls_setmiterlimit */,
    0 /* pls_setrop3 */,
    0 /* pls_setpatcol */,
    pls_drawimage,
    pls_setplotsize,
    pls_getplotsize,
    pls_showpage,
    pls_finish,
    pls_capabilities,
    pls_destroy
  };

/*! Retrieves the pointer to a new \c hpgs_plotsize_device on the heap.

    If \c ignore_ps is \c HPGS_TRUE, a HPGL PS statement is ignored an the
    plotsize is calculated from the vector graphics contents.

    If \c do_linewidth is \c HPGS_TRUE, the current linewidth is
    taken into account in the plotsize calculation.

    If the system is out of memory, a null pointer is returned.
*/
hpgs_plotsize_device *hpgs_new_plotsize_device(hpgs_bool ignore_ps,
                                               hpgs_bool do_linewidth)
{
  hpgs_plotsize_device *ret =
    (hpgs_plotsize_device *)malloc(sizeof(hpgs_plotsize_device));

  if (ret)
    {
      ret->n_page_bbs = 0;
      ret->page_bbs_alloc_size = 32;
      ret->page_bbs = malloc(sizeof(hpgs_bbox)*ret->page_bbs_alloc_size);

      if (!ret->page_bbs)
        {
          free(ret);
          return 0;
        }

      hpgs_bbox_null(&ret->path_bb);
      hpgs_bbox_null(&ret->page_bb);
      hpgs_bbox_null(&ret->global_bb);
      ret->moveto.x = 0.0;
      ret->moveto.y = 0.0;
      ret->deferred_moveto = 0;
      ret->clip_bbs[0].llx = -1.0e20;
      ret->clip_bbs[0].lly = -1.0e20;
      ret->clip_bbs[0].urx = 1.0e20;
      ret->clip_bbs[0].ury = 1.0e20;
      ret->clip_depth = 0;
      ret->linewidth = 1.0;
      ret->ignore_ps = ignore_ps;
      ret->do_linewidth = do_linewidth;
      ret->inherited.vtable=&pls_vtable;
    }

  return ret;
}

/*
  Custom device from plugin.
*/
typedef int (*hpgs_new_device_func_t)(hpgs_device **device,
                                      void **page_asset_ctxt,
                                      hpgs_reader_asset_func_t *page_asset_func,
                                      void **frame_asset_ctxt,
                                      hpgs_reader_asset_func_t *frame_asset_func,
                                      const char *dev_name,
                                      const char *filename,
                                      const hpgs_bbox *bb,
                                      double xres, double yres,
                                      hpgs_bool do_rop3,
                                      int argc, const char *argv[]);

typedef void (*hpgs_plugin_version_func_t)(int *major, int *minor);

typedef void (*hpgs_plugin_init_func_t)();
typedef void (*hpgs_plugin_cleanup_func_t)();

typedef struct hpgs_plugin_ref_st hpgs_plugin_ref;

#define HPGS_PLUGIN_NAME_LEN 8

struct hpgs_plugin_ref_st
{
  char name[HPGS_PLUGIN_NAME_LEN];
#ifdef WIN32
  HMODULE handle;
#else
  void *handle;
#endif
  hpgs_new_device_func_t new_dev_func;
  hpgs_plugin_version_func_t version_func;
  hpgs_plugin_init_func_t init_func;
  hpgs_plugin_init_func_t cleanup_func;
};

static hpgs_plugin_ref plugins[4] =
  { { "",0,0,0}, { "",0,0,0}, { "",0,0,0}, { "",0,0,0} };

static void hpgs_cleanup_plugin(hpgs_plugin_ref *plugin)
{
  if (strlen(plugin->name) == 0)
    return;
       
  if (plugin->cleanup_func) plugin->cleanup_func();

#ifdef WIN32
  if (!FreeLibrary(plugin->handle))
    hpgs_log("hpgs_cleanup_plugin_devices: unable to close plugin %s.\n",
             plugin->name);
#else
  if (dlclose(plugin->handle))
    hpgs_log("hpgs_cleanup_plugin_devices: unable to close plugin %s: %s.\n",
             plugin->name,dlerror());
#endif

  plugin->name[0]      = '\0';
  plugin->handle       = 0;
  plugin->new_dev_func = 0;
  plugin->version_func = 0;
  plugin->init_func    = 0;
  plugin->cleanup_func = 0;
}

void hpgs_cleanup_plugin_devices()
{
  int i;

  for (i=0;i<sizeof(plugins)/sizeof(hpgs_plugin_ref);++i)
    {
      hpgs_cleanup_plugin(&plugins[i]);
    }
}

int hpgs_new_plugin_device( hpgs_device **device,
                            void **page_asset_ctxt,
                            hpgs_reader_asset_func_t *page_asset_func,
                            void **frame_asset_ctxt,
                            hpgs_reader_asset_func_t *frame_asset_func,
                            const char *dev_name,
                            const char *filename,
                            const hpgs_bbox *bb,
                            double xres, double yres,
                            hpgs_bool do_rop3,
                            int argc, const char *argv[])
{
  char *underscore=strchr(dev_name,'_');
  int i,l = underscore ? underscore-dev_name : strlen(dev_name);
  hpgs_plugin_ref *plugin=0;

  if (l >= HPGS_PLUGIN_NAME_LEN) l = HPGS_PLUGIN_NAME_LEN-1;

  for (i=0;i<sizeof(plugins)/sizeof(hpgs_plugin_ref);++i)
    {
      if (strlen(plugins[i].name) == 0)
        break;

      if (strlen(plugins[i].name) == l &&
          strncmp(plugins[i].name,dev_name,l) == 0)
        { plugin = plugins+i; break; }
    }

  if (i>=sizeof(plugins)/sizeof(hpgs_plugin_ref))
    {
      hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: too much plugins registered (max %u).\n"),
                     (unsigned)(sizeof(plugins)/sizeof(hpgs_plugin_ref)));
      return 0;
    }
  
  if (!plugin)
    {
      char plugin_name[1024];
      plugin = plugins+i;

#ifdef WIN32
      _snprintf(plugin_name,sizeof(plugin_name),"%s\\lib\\hpgs\\hpgs%.*splugin.%d.%d.dll",
                hpgs_get_prefix(),
                l,dev_name,
                HPGS_MAJOR_VERSION,HPGS_MINOR_VERSION);
      plugin->handle =LoadLibraryA(plugin_name);

      if (!plugin->handle)
	{
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: unable to open plugin %s.\n"),
                         plugin_name);
	  return 0;
	}
#else

#ifndef HPGS_LIBSFX
#define HPGS_LIBSFX "lib"
#endif
      snprintf(plugin_name,sizeof(plugin_name),"%s/" HPGS_LIBSFX "/hpgs/hpgs%.*splugin.so.%d.%d",
               hpgs_get_prefix(),
               l,dev_name,
               HPGS_MAJOR_VERSION,HPGS_MINOR_VERSION);

      plugin->handle = dlopen(plugin_name,RTLD_LAZY);

      if (!plugin->handle)
	{
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: unable to open plugin %s: %s.\n"),
                         plugin_name,dlerror());
	  return 0;
	}
#endif
      memcpy(plugin->name,dev_name,l);
      plugin->name[l]='\0';

      plugin->version_func=0;
      plugin->init_func=0;
      plugin->cleanup_func=0;
      plugin->new_dev_func=0;
    }


  if (! plugin->version_func)
    {
      int minor = 0;
      int major = 0;
#ifdef WIN32
      plugin->version_func =
	(hpgs_plugin_version_func_t)GetProcAddress(plugin->handle,
                                                   "hpgs_plugin_version");
      if (!plugin->version_func)
	{
          hpgs_cleanup_plugin(plugin);
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: unable to resolve function hpgs_plugin_version.\n"));
	  return 0;
	}
#else
      plugin->version_func =
	(hpgs_plugin_version_func_t)dlsym(plugin->handle,
					  "hpgs_plugin_version");
      if (!plugin->version_func)
	{
          hpgs_cleanup_plugin(plugin);
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: unable to resolve function hpgs_plugin_version: %s.\n"),
                         dlerror());
	  return 0;
	}
#endif

      plugin->version_func(&major,&minor);
      
      if (major != HPGS_MAJOR_VERSION ||
	  minor != HPGS_MINOR_VERSION   )
	{
          hpgs_cleanup_plugin(plugin);
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: Plugin version %d.%d does not match application version %d.%d.\n"),
                         major,minor,HPGS_MAJOR_VERSION,HPGS_MINOR_VERSION);

	  return 0;
	}

#ifdef WIN32
      plugin->init_func =
        (hpgs_plugin_init_func_t)GetProcAddress(plugin->handle,
                                                "hpgs_plugin_init");
      plugin->cleanup_func =
        (hpgs_plugin_cleanup_func_t)GetProcAddress(plugin->handle,
                                                "hpgs_plugin_cleanup");
#else
      plugin->init_func =
        (hpgs_plugin_init_func_t)dlsym(plugin->handle,
                                       "hpgs_plugin_init");
      plugin->cleanup_func =
        (hpgs_plugin_cleanup_func_t)dlsym(plugin->handle,
                                          "hpgs_plugin_cleanup");
#endif

      if (plugin->init_func) plugin->init_func();
    }


  if (!plugin->new_dev_func)
    {
#ifdef WIN32
      plugin->new_dev_func =
	(hpgs_new_device_func_t)GetProcAddress(plugin->handle,
                                               "hpgs_plugin_new_device");
      if (!plugin->new_dev_func)
	{
          hpgs_cleanup_plugin(plugin);
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: unable to resolve function hpgs_plugin_new_device.\n"));
	  return 0;
	}
#else
      plugin->new_dev_func =
	(hpgs_new_device_func_t)dlsym(plugin->handle,
				      "hpgs_plugin_new_device");
      
      if (!plugin->new_dev_func)
	{
          hpgs_cleanup_plugin(plugin);
	  hpgs_set_error(hpgs_i18n("hpgs_new_plugin_device: unable to resolve function hpgs_plugin_new_device: %s.\n"),
                         dlerror());
	  return 0;
	}
#endif
    }

  return plugin->new_dev_func(device,
                              page_asset_ctxt,page_asset_func,
                              frame_asset_ctxt,frame_asset_func,
                              dev_name,filename,bb,xres,yres,do_rop3,argc,argv);
}
