/***********************************************************************
 *                                                                     *
 * $Id: hpgsimage.c 386 2007-03-18 18:33:10Z softadm $
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
 * The implementation of the public API for the pixel image.           *
 *                                                                     *
 ***********************************************************************/

#include <hpgsimage.h>
#include <math.h>
#include <errno.h>
#if (defined(__BEOS__) || defined(__HAIKU__))
#include <png.h>
#define png_infopp_NULL (png_infopp)NULL
#else
#include <libpng12/png.h>
#endif

/*! \defgroup image The public pixel image API.

    The structures and functions in this group handle the
    creation and manipulation of rectangular pixel containers.
*/

/* image methods. */
static  int pim_get_pixel_8 (const hpgs_image *_this,
			     int x, int y, hpgs_paint_color *c, double *alpha)
{
  const hpgs_png_image *pim = (const hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  c->r = row[x];
  c->g = row[x];
  c->b = row[x];
  c->index = -1;

  if (alpha)
    *alpha = 1.0;

  return 0;
}

static  int pim_put_pixel_8 (hpgs_image *_this,
			     int x, int y, const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int gray =
    (11 * (int)c->r + 16 * (int)c->g + 5 * (int)c->b)/32;

  if (alpha >= 1.0)
    row[x] = (unsigned char)(gray);
  else if (alpha > 0.0)
    row[x] = (unsigned char)(gray * alpha + row[x] * (1.0-alpha));

  return 0;
}

static  int pim_put_chunk_8 (hpgs_image *_this,
			     int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int gray =
    (11 * (int)c->r + 16 * (int)c->g + 5 * (int)c->b)/32;

  memset(row+x1,gray,x2-x1+1);

  return 0;
}

static  int pim_rop3_pixel_8 (hpgs_image *_this,
                              int x, int y,
                              const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int c_gray = (11 * (int)c->r + 16 * (int)c->g + 5 * (int)c->b)/32;
  int p_gray = (11 * (int)pim->pattern_color.r + 16 * (int)pim->pattern_color.g + 5 * (int)pim->pattern_color.b)/32;

  if (alpha >= 1.0)
    pim->rop3(&row[x],(unsigned char)(c_gray),(unsigned char)(p_gray));
  else if (alpha > 0.0)
    {
      unsigned char gray = row[x];
      pim->rop3(&gray,(unsigned char)(c_gray),(unsigned char)(p_gray));
      row[x] = (unsigned char)(gray * alpha + row[x] * (1.0-alpha));
    }

  return 0;
}

static  int pim_rop3_chunk_8 (hpgs_image *_this,
                              int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int c_gray = (11 * (int)c->r + 16 * (int)c->g + 5 * (int)c->b)/32;
  int p_gray = (11 * (int)pim->pattern_color.r + 16 * (int)pim->pattern_color.g + 5 * (int)pim->pattern_color.b)/32;

  int x;

  for (x=x1;x<=x2;++x)
    pim->rop3(&row[x],(unsigned char)(c_gray),(unsigned char)(p_gray));

  return 0;
}

static  int pim_get_pixel_16 (const hpgs_image *_this,
			      int x, int y, hpgs_paint_color *c, double *alpha)
{
  const hpgs_png_image *pim = (const hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  c->r = row[2*x];
  c->g = row[2*x];
  c->b = row[2*x];

  if (alpha)
    *alpha = row[2*x+1] / 255.0;
  

  return 0;
}

static  int pim_put_pixel_16 (hpgs_image *_this,
			      int x, int y, const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  unsigned gray =
    (11 * (unsigned)c->r + 16 * (unsigned)c->g + 5 * (unsigned)c->b)/32;

  if (alpha >= 1.0)
    {
      row[2*x] = (unsigned char)(gray);
      row[2*x+1] = 0xff;
    }
  else if (alpha > 0.0)
    {
      double a0 = row[2*x+1] / 255.0;
      row[2*x] = (unsigned char)(gray * alpha + row[2*x] * (1.0-alpha));
      row[2*x+1] = (unsigned char)(255.0 * (alpha + a0 * (1.0-alpha)));
    }

  return 0;
}

static  int pim_put_chunk_16 (hpgs_image *_this,
			      int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  unsigned gray =
    (11 * (unsigned)c->r + 16 * (unsigned)c->g + 5 * (unsigned)c->b)/32;

  int i;

  for (i=2*x1;i<=2*x2;++i)
    {
      row[i] = (unsigned char)(gray);
      ++i;
      row[i] = 0xff;
    }
  return 0;
}

static  int pim_rop3_pixel_16 (hpgs_image *_this,
                               int x, int y,
                               const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int c_gray = (11 * (int)c->r + 16 * (int)c->g + 5 * (int)c->b)/32;
  int p_gray = (11 * (int)pim->pattern_color.r + 16 * (int)pim->pattern_color.g + 5 * (int)pim->pattern_color.b)/32;

  if (alpha >= 1.0)
    {
      pim->rop3(&row[2*x],(unsigned char)(c_gray),(unsigned char)(p_gray));
      row[2*x+1] = 0xff;
    }
  else if (alpha > 0.0)
    {
      unsigned char gray = row[2*x];
      double a0 = row[2*x+1] / 255.0;
      pim->rop3(&gray,(unsigned char)(c_gray),(unsigned char)(p_gray));
      row[2*x] = (unsigned char)(gray * alpha + row[2*x] * (1.0-alpha));
      row[2*x+1] = (unsigned char)(255.0 * (alpha + a0 * (1.0-alpha)));
    }

  return 0;
}

static  int pim_rop3_chunk_16 (hpgs_image *_this,
                               int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int c_gray = (11 * (int)c->r + 16 * (int)c->g + 5 * (int)c->b)/32;
  int p_gray = (11 * (int)pim->pattern_color.r + 16 * (int)pim->pattern_color.g + 5 * (int)pim->pattern_color.b)/32;

  int i;

  for (i=2*x1;i<=2*x2;++i)
    {
      pim->rop3(&row[i],(unsigned char)(c_gray),(unsigned char)(p_gray));
      ++i;
      row[i] = 0xff;
    }

  return 0;
}

static int pim_get_pixel_24 (const hpgs_image *_this,
			      int x, int y, hpgs_paint_color *c, double *alpha)
{
  const hpgs_png_image *pim = (const hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  c->r = row[3*x  ];
  c->g = row[3*x+1];
  c->b = row[3*x+2];

  if (alpha)
    *alpha = 1.0;
 
  return 0;
}

static  int pim_put_pixel_24 (hpgs_image *_this,
			     int x, int y, const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  if (alpha >= 1.0)
    {
      row[3*x  ] = c->r;
      row[3*x+1] = c->g;
      row[3*x+2] = c->b;
    }
  else if (alpha > 0.0)
    {
      row[3*x  ] = (unsigned char)(c->r*alpha + row[3*x  ]*(1.0-alpha));
      row[3*x+1] = (unsigned char)(c->g*alpha + row[3*x+1]*(1.0-alpha));
      row[3*x+2] = (unsigned char)(c->b*alpha + row[3*x+2]*(1.0-alpha));
    }
  return 0;
}

static  int pim_put_chunk_24 (hpgs_image *_this,
			      int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int i;

  for (i=3*x1;i<=3*x2;++i)
    {
      row[i] = c->r;
      ++i;
      row[i] = c->g;
      ++i;
      row[i] = c->b;
    }
  return 0;
}

static  int pim_rop3_pixel_24 (hpgs_image *_this,
                               int x, int y,
                               const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  if (alpha >= 1.0)
    {
      pim->rop3(&row[3*x  ],c->r,pim->pattern_color.r);
      pim->rop3(&row[3*x+1],c->g,pim->pattern_color.g);
      pim->rop3(&row[3*x+2],c->b,pim->pattern_color.b);
    }
  else if (alpha > 0.0)
    {
      unsigned char v = row[3*x];
      pim->rop3(&v,c->r,pim->pattern_color.r);
      row[3*x  ] = (unsigned char)(v * alpha + row[3*x  ] * (1.0-alpha));

      v = row[3*x+1];
      pim->rop3(&v,c->g,pim->pattern_color.g);
      row[3*x+1] = (unsigned char)(v * alpha + row[3*x+1] * (1.0-alpha));

      v = row[3*x+2];
      pim->rop3(&v,c->b,pim->pattern_color.b);
      row[3*x+2] = (unsigned char)(v * alpha + row[3*x+2] * (1.0-alpha));
    }

  return 0;
}

static  int pim_rop3_chunk_24 (hpgs_image *_this,
                               int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int i;

  for (i=3*x1;i<=3*x2;++i)
    {
      pim->rop3(&row[i],c->r,pim->pattern_color.r);
      ++i;
      pim->rop3(&row[i],c->g,pim->pattern_color.g);
      ++i;
      pim->rop3(&row[i],c->b,pim->pattern_color.b);
    }

  return 0;
}

static int pim_get_pixel_32 (const hpgs_image *_this,
                             int x, int y, hpgs_paint_color *c, double *alpha)
{
  const hpgs_png_image *pim = (const hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  c->r = row[4*x  ];
  c->g = row[4*x+1];
  c->b = row[4*x+2];

  if (alpha)
    *alpha = row[4*x+3] / 255.0;
 
  return 0;
}

static  int pim_put_pixel_32 (hpgs_image *_this,
			      int x, int y, const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  if (alpha >= 1.0)
    {
      row[4*x  ] = c->r;
      row[4*x+1] = c->g;
      row[4*x+2] = c->b;
      row[4*x+3] = 0xff;
    }
  else if (alpha > 0.0)
    {
      double a0 = row[4*x+3] / 255.0;
      row[4*x  ] = (unsigned char)(c->r*alpha+row[4*x  ]*(1.0-alpha));
      row[4*x+1] = (unsigned char)(c->g*alpha+row[4*x+1]*(1.0-alpha));
      row[4*x+2] = (unsigned char)(c->b*alpha+row[4*x+2]*(1.0-alpha));
      row[4*x+3] = (unsigned char)(255.0*(alpha + a0 * (1.0-alpha)));
    }

  return 0;
}

static  int pim_put_chunk_32 (hpgs_image *_this,
			      int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int i;

  for (i=4*x1;i<=4*x2;++i)
    {
      row[i] = c->r;
      ++i;
      row[i] = c->g;
      ++i;
      row[i] = c->b;
      ++i;
      row[i] = 0xff;
    }
  return 0;
}

static  int pim_rop3_pixel_32 (hpgs_image *_this,
                               int x, int y,
                               const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  if (alpha >= 1.0)
    {
      pim->rop3(&row[4*x  ],c->r,pim->pattern_color.r);
      pim->rop3(&row[4*x+1],c->g,pim->pattern_color.g);
      pim->rop3(&row[4*x+2],c->b,pim->pattern_color.b);
      row[4*x+3] = 0xff;
    }
  else if (alpha > 0.0)
    {
      double a0 = row[4*x+3] / 255.0;
      unsigned char v;

      v=row[4*x  ];
      pim->rop3(&v,c->r,pim->pattern_color.r);
      row[4*x  ] = (unsigned char)(v*alpha+row[4*x  ]*(1.0-alpha));

      v=row[4*x+1];
      pim->rop3(&v,c->g,pim->pattern_color.g);
      row[4*x+1] = (unsigned char)(v*alpha+row[4*x+1]*(1.0-alpha));

      v=row[4*x+2];
      pim->rop3(&v,c->b,pim->pattern_color.b);
      row[4*x+2] = (unsigned char)(v*alpha+row[4*x+2]*(1.0-alpha));

      row[4*x+3] = (unsigned char)(255.0*(alpha + a0 * (1.0-alpha)));
    }

  return 0;
}

static  int pim_rop3_chunk_32 (hpgs_image *_this,
                               int x1, int x2, int y,
                               const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  int i;

  for (i=4*x1;i<=4*x2;++i)
    {
      row[i] = c->r;
      ++i;
      row[i] = c->g;
      ++i;
      row[i] = c->b;
      ++i;
      row[i] = 0xff;
    }
  return 0;
}

static  int pim_put_pixel_256 (hpgs_image *_this,
			       int x, int y,
			       const hpgs_paint_color *c, double alpha)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  if (alpha >= 0.5)
    row[x] = c->index;

  return 0;
}

static  int pim_get_pixel_256 (const hpgs_image *_this,
			       int x, int y,
			       hpgs_paint_color *c, double *alpha)
{
  const hpgs_png_image *pim = (const hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  c->index = row[x];

  c->r = _this->palette[c->index].r;
  c->g = _this->palette[c->index].g;
  c->b = _this->palette[c->index].b;

  if (alpha)
    *alpha=1.0;

  return 0;
}

static  int pim_put_chunk_256 (hpgs_image *_this,
			       int x1, int x2, int y, const hpgs_paint_color *c)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  unsigned char *row = pim->data + pim->bytes_per_row * y;

  memset(row+x1,c->index,x2-x1+1);

  return 0;
}

static int pim_setrop3 (hpgs_image *_this, hpgs_rop3_func_t rop3)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  pim->rop3 = rop3;
  return 0;
}

static int pim_setpatcol (hpgs_image *_this, const hpgs_palette_color *p)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  pim->pattern_color = *p;
  return 0;
}

static int pim_resize(hpgs_image *_this, int w, int h)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;
  int bytes_per_row,bpp,fill,i,j;
  unsigned char *data;

  if (!pim->data) return -1;

  if (w == _this->width && h == _this->height)
    return 0;

  bpp = (pim->depth / 8);

  bytes_per_row = ((w * bpp + 3) / 4) * 4;

  if (pim->color_type == PNG_COLOR_TYPE_PALETTE)
    // fill indexed images with color 1 (white)
    fill = 0x01;
  else
    // fill direct color with white.
    fill = 0xff;

  if (w != _this->width)
    {
      data = (unsigned char *)malloc(bytes_per_row * h);

      if (!data)
        return -1;

      for (i=0; i < h && i <_this->height; ++i)
        {
          unsigned char *orow = pim->data+pim->bytes_per_row * i;
          unsigned char *row = data+bytes_per_row * i;
          
          if (w <= _this->width)
            memcpy(row,orow,w * bpp);
          else
            {
              memcpy(row,orow,_this->width * bpp);
              memset(row+_this->width * bpp,fill,(w-_this->width) * bpp);
              
              // set alpha value to zero.
              if (pim->depth == 16 || pim->depth == 32)
                for (j=_this->width * bpp + bpp -1; j < bytes_per_row; j += bpp)
                  row[j] = 0x00;
            }
        }
      
      free(pim->data);
      pim->data = data;
    }
  else
    {
      data = (unsigned char *)realloc(pim->data,bytes_per_row * h);
      if (!data) return -1;
      pim->data = data;
    }

  for (i = _this->height; i < h; ++i)
    {
      unsigned char *row = data+bytes_per_row * i;

      memset(row,fill,w * bpp);
    }

  pim->bytes_per_row = bytes_per_row;
  _this->width = w;
  _this->height = h;
  return 0;
}

static int pim_set_resolution(hpgs_image *_this, double x_dpi, double y_dpi)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;
 
  pim->res_x = (unsigned)floor(x_dpi/0.0254 + 0.5);
  pim->res_y = (unsigned)floor(y_dpi/0.0254 + 0.5);

  return 0;
}

static  void pim_destroy (hpgs_image *_this)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  if (pim->data) free(pim->data);
}

static  int pim_clear (hpgs_image *_this)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  if (pim->color_type == PNG_COLOR_TYPE_PALETTE)
    // fill indexed images with color 1 (white)
    memset(pim->data,0x01,pim->bytes_per_row * _this->height);
  else
    {
      // fill direct color with white.
      memset(pim->data,0xff,pim->bytes_per_row * _this->height);
      
      // set alpha to zero.
      if (pim->depth == 16 || pim->depth == 32)
        {
          int bpp = pim->depth/8;
          int i,j;

          for (i=0; i < _this->height; ++i)
            {
              unsigned char *row = pim->data+pim->bytes_per_row * i;

              for (j=bpp-1; j < pim->bytes_per_row; j += bpp)
                row[j] = 0x00;
            }
        }
    }

  return 0;
}

/* This is taken from ciro/test/write_png.c */
static void
unpremultiply_data (png_structp png, png_row_infop row_info, png_bytep data)
{
    int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        unsigned char *b = &data[i];
        unsigned int pixel;
        unsigned char alpha;

	memcpy (&pixel, b, sizeof (unsigned int));
	alpha = (pixel & 0xff000000) >> 24;
        if (alpha == 0) {
	    b[0] = b[1] = b[2] = b[3] = 0;
	} else {
            b[0] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
            b[1] = (((pixel & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
            b[2] = (((pixel & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
	    b[3] = alpha;
	}
    }
}

static void png_error_function (png_structp png_ptr, png_const_charp str)
{
  hpgs_set_error("%s",str);
  
  longjmp(png_jmpbuf(png_ptr),1);
}

static void png_warn_function (png_structp png_ptr, png_const_charp str)
{
  hpgs_log(hpgs_i18n("libpng warning: %s.\n"),str);
}

static  int pim_write(hpgs_image *_this,
                      const char *filename)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;
  png_structp png_ptr=0;
  png_infop info_ptr=0;
  png_color_16 white;
  png_text text;
  FILE *fp=0;
  int i;
  unsigned char *row;

  fp = filename ? fopen(filename, "wb") : stdout;

  if (fp == NULL)
    return hpgs_set_error(hpgs_i18n("Error opening file %s for writing: %s."),
                          filename?filename:"(stdout)",strerror(errno));


  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				    NULL,png_error_function,png_warn_function);

  if (png_ptr == NULL)
    return hpgs_set_error(hpgs_i18n("libpng error creating write structure for file %s."),
                          filename);

  info_ptr = png_create_info_struct(png_ptr);

  if (info_ptr == NULL)
     {
       png_destroy_write_struct(&png_ptr, png_infopp_NULL);
       return hpgs_set_error(hpgs_i18n("libpng error creating info structure for file %s."),
                             filename);
     }

  if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      fclose(fp);
      return hpgs_error_ctxt(hpgs_i18n("libpng error writing file %s"),filename);
    }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, pim->inherited.width, pim->inherited.height,
	       8,pim->color_type,PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  if (pim->inherited.palette)
    png_set_PLTE(png_ptr,info_ptr,
                 (png_colorp)pim->inherited.palette,
                 pim->inherited.palette_ncolors);

  png_set_compression_level(png_ptr,pim->compression);

  
  memset(&white,0,sizeof(white));
  
  switch (pim->color_type)
    {
    case PNG_COLOR_TYPE_PALETTE:
      white.index = 1;
      break;

    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      white.gray = 0xff;
      break;

    default:
      white.red = 0xff;
      white.blue = 0xff;
      white.green = 0xff;
    }

  png_set_bKGD (png_ptr, info_ptr, &white);

  if (pim->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_write_user_transform_fn (png_ptr, unpremultiply_data);

#ifdef PNG_TEXT_SUPPORTED
  text.compression = PNG_TEXT_COMPRESSION_NONE;
  text.key         = "Software";
  text.text        = "hpgs v" HPGS_VERSION;
  text.text_length = strlen(text.text);

#ifdef PNG_iTXt_SUPPORTED
   text.itxt_length = 0;
   text.lang        = NULL;
   text.lang_key    = NULL;
#endif
  png_set_text(png_ptr,info_ptr,&text,1);
#endif

#ifdef PNG_pHYs_SUPPORTED
  if (pim->res_x && pim->res_y)
    png_set_pHYs (png_ptr,info_ptr,pim->res_x,pim->res_y,PNG_RESOLUTION_METER);
#endif

  png_write_info(png_ptr, info_ptr);
  
  row = pim->data;

  for (i = 0; i< pim->inherited.height; ++i)
    {
      png_write_row(png_ptr, row);
      row += pim->bytes_per_row;
    }
  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  if (filename)
    fclose(fp);

  return 0;
}

static int pim_get_data(hpgs_image *_this,
                        unsigned char **data, int *stride, int *depth)
{
  hpgs_png_image *pim = (hpgs_png_image *)_this;

  *data   = pim->data;
  *stride = pim->bytes_per_row;
  *depth  = pim->depth;
  return 0;
}

static hpgs_image_vtable pim_vtable_8 =
  {
    pim_get_pixel_8,
    pim_put_pixel_8,
    pim_put_chunk_8,
    pim_put_pixel_8,
    pim_put_chunk_8,
    0 /* pim_setrop3 */,
    0 /* pim_setpatcol */,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_8_rop =
  {
    pim_get_pixel_8,
    pim_put_pixel_8,
    pim_put_chunk_8,
    pim_rop3_pixel_8,
    pim_rop3_chunk_8,
    pim_setrop3,
    pim_setpatcol,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_16 =
  {
    pim_get_pixel_16,
    pim_put_pixel_16,
    pim_put_chunk_16,
    pim_put_pixel_16,
    pim_put_chunk_16,
    0 /* pim_setrop3 */,
    0 /* pim_setpatcol */,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_16_rop =
  {
    pim_get_pixel_16,
    pim_put_pixel_16,
    pim_put_chunk_16,
    pim_rop3_pixel_16,
    pim_rop3_chunk_16,
    pim_setrop3,
    pim_setpatcol,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_24 =
  {
    pim_get_pixel_24,
    pim_put_pixel_24,
    pim_put_chunk_24,
    pim_put_pixel_24,
    pim_put_chunk_24,
    0 /* pim_setrop3 */,
    0 /* pim_setpatcol */,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_24_rop =
  {
    pim_get_pixel_24,
    pim_put_pixel_24,
    pim_put_chunk_24,
    pim_rop3_pixel_24,
    pim_rop3_chunk_24,
    pim_setrop3,
    pim_setpatcol,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_32 =
  {
    pim_get_pixel_32,
    pim_put_pixel_32,
    pim_put_chunk_32,
    pim_put_pixel_32,
    pim_put_chunk_32,
    0 /* pim_setrop3 */,
    0 /* pim_setpatcol */,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_32_rop =
  {
    pim_get_pixel_32,
    pim_put_pixel_32,
    pim_put_chunk_32,
    pim_rop3_pixel_32,
    pim_rop3_chunk_32,
    pim_setrop3,
    pim_setpatcol,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

static hpgs_image_vtable pim_vtable_256 =
  {
    pim_get_pixel_256,
    pim_put_pixel_256,
    pim_put_chunk_256,
    pim_put_pixel_256,
    pim_put_chunk_256,
    pim_setrop3,
    pim_setpatcol,
    pim_resize,
    pim_set_resolution,
    pim_clear,
    pim_write,
    pim_get_data,
    pim_destroy
  };

/*! Creates a new \c hpgs_png_image on the heap using the
    given \c width and \c height. The bit depth \c depth of
    the image is limited to 8,16, 24 or 32.
    
    If \c palette is \c HPGS_TRUE the image uses and indexed palette
    and the bit depth is limited to 8.

   If \c palette is \c HPGS_FALSE the folowing depth are supported:
   \li 8 contructs a greyscale image.
   \li 16 contructs a greyscale image with alpha (transparency) plane.
   \li 24 contructs an RGB image.
   \li 16 contructs an RGB image with alpha (transparency) plane.

   If \c do_rop3 is \c HPGS_TRUE the image uses ROP3 raster operations.
   \c do_rop3 is ignored for indexed images. Indexed images do never support
   raster operations.
*/
hpgs_png_image *hpgs_new_png_image(int width, int height,
				   int depth, hpgs_bool palette,
                                   hpgs_bool do_rop3)
{
  hpgs_png_image *ret=(hpgs_png_image *)malloc(sizeof(hpgs_png_image));

  if (ret)
    {
      if (palette)
	depth = 8;
      else
	{
	  depth = ((depth + 7)/8) * 8;
	  if (depth < 8) depth = 8;
	  if (depth > 32) depth = 32;
	}
      
      ret->depth = depth;

      ret->bytes_per_row = ((width * (depth / 8) + 3) / 4) * 4;

      ret->data = (unsigned char *)malloc(ret->bytes_per_row * height);

      ret->compression = 6;

      ret->inherited.width     = width;
      ret->inherited.height    = height;

      ret->pattern_color.r = 0;
      ret->pattern_color.g = 0;
      ret->pattern_color.b = 0;

      if (do_rop3 && !palette)
        ret->rop3 = hpgs_rop3_func(252,HPGS_TRUE,HPGS_TRUE);
      else
        ret->rop3 = 0;

      ret->res_x = 0;
      ret->res_y = 0;

      if (palette)
	{
	  hpgs_paint_color c;

	  ret->color_type       = PNG_COLOR_TYPE_PALETTE;
	  ret->inherited.vtable = &pim_vtable_256;

	  ret->inherited.palette =
	    (hpgs_palette_color *)malloc(sizeof(hpgs_palette_color)*256);
	  ret->inherited.palette_idx =
	    (unsigned *)malloc(sizeof(unsigned)*256);
	  ret->inherited.palette_ncolors = 0;

	  // define black and white a color 0 resp. 1
	  c.r = c.g = c.b = 0;
	  hpgs_image_define_color (&ret->inherited,&c);
	  c.r = c.g = c.b = 255;
	  hpgs_image_define_color (&ret->inherited,&c);
	}
      else
	{
	  switch (depth)
	    {
	    case 8:
	      ret->color_type       = PNG_COLOR_TYPE_GRAY;
	      ret->inherited.vtable = do_rop3 ? &pim_vtable_8_rop : &pim_vtable_8;
	      break;
	    case 16:
	      ret->color_type       = PNG_COLOR_TYPE_GRAY_ALPHA;
	      ret->inherited.vtable = do_rop3 ? &pim_vtable_16_rop : &pim_vtable_16;
	      break;
	    case 24:
	      ret->color_type       = PNG_COLOR_TYPE_RGB;
	      ret->inherited.vtable = do_rop3 ? &pim_vtable_24_rop : &pim_vtable_24;
	      break;
	    default:
	      /* case 32: */
	      ret->color_type       = PNG_COLOR_TYPE_RGB_ALPHA;
	      ret->inherited.vtable = do_rop3 ? &pim_vtable_32_rop : &pim_vtable_32;
	    }
	  ret->inherited.palette = 0;
	  ret->inherited.palette_idx = 0;
	  ret->inherited.palette_ncolors = 0;
	}
      
      if (!ret->data ||
	  (palette && (!ret->inherited.palette || !ret->inherited.palette_idx)))
	{
	  if (ret->data)
	    free(ret->data);

 	  if (ret->inherited.palette)
	    free(ret->inherited.palette);

 	  if (ret->inherited.palette_idx)
	    free(ret->inherited.palette_idx);

	  free(ret);
	  ret = 0;
	} 
      else
        pim_clear(HPGS_BASE_CLASS(ret));
    }

  return ret;
}

/*! Sets the compression ratio used for a png image.
    Returns 0, if the compression is in the interval
    from 1 to 9 inclusive.

    If the compression is invalid, -1 is returned.
*/
int hpgs_png_image_set_compression(hpgs_png_image *pim, int compression)
{
  if (compression < 1 || compression >9)
    return hpgs_set_error(hpgs_i18n("Invalid PNG compression %d specified."),compression);

  pim->compression = compression;
  return 0;
}

/*! Sets the resolution of this image in dpi. Note that for PNG images the
    information in the pHYs chunk is given as an integral number of pixels
    per meter.
*/
int hpgs_image_set_resolution(hpgs_image *_this, double x_dpi, double y_dpi)
{
  if (!_this->vtable->set_resolution)
    return 0;

  return _this->vtable->set_resolution(_this,x_dpi,y_dpi);
}

/*! Gets the pixel buffer of the png image, which is needed
    by some backends. If the image is not a png image -1 is retuned.
    
    If 0 is returned, *data point to the image data and *stride is
    the number of bytes between two consecutive scanlines of the image.
*/
HPGS_API int hpgs_image_get_data(hpgs_image *_this,
                                 unsigned char **data, int *stride, int *depth)
{
  if (!_this->vtable->get_data)
    return hpgs_set_error(hpgs_i18n("Exposure of image data is not implemented."));

  return _this->vtable->get_data(_this,data,stride,depth);
}

/*! Sets the ROP3 raster operation applied by
    \c hpgs_image_put_pixel and \c hpgs_image_put_chunk. */
int hpgs_image_setrop3 (hpgs_image *_this, hpgs_rop3_func_t rop3)
{
  if (!_this->vtable->setrop3)
    return 0;

  return _this->vtable->setrop3(_this,rop3);
}

/*! Sets the color of the pattern applied by the ROP3 operation. */
int hpgs_image_setpatcol(hpgs_image *_this, const hpgs_palette_color *p)
{
  if (!_this->vtable->setpatcol)
    return 0;

  return _this->vtable->setpatcol(_this,p);
}


/*! Resizes the given image \c _this to a new width \c w and new height \c h.
    The pixel data preserved or initialized, if the image grows. */
int hpgs_image_resize (hpgs_image *_this, int w, int h)
{
  if (!_this->vtable->resize)
    return hpgs_set_error(hpgs_i18n("Image resize is not implemented."));

  return _this->vtable->resize (_this,w,h);
}

/*! Clears the given image \c _this. */
int hpgs_image_clear (hpgs_image *_this)
{
  if (!_this->vtable->clear)
    return hpgs_set_error(hpgs_i18n("Image clear is not implemented."));

  return _this->vtable->clear (_this);
}

/*! Writes the given image \c _this to a file with the given name \c filename. */
int hpgs_image_write (hpgs_image *_this, const char *filename)
{
  if (!_this->vtable->write)
    return hpgs_set_error(hpgs_i18n("Image write is not implemented."));

  return _this->vtable->write (_this,filename);
}

/*! Destroys the given image and frees all allocated resources by this image.
*/
void hpgs_image_destroy (hpgs_image *_this)
{
  _this->vtable->destroy(_this);
  if (_this->palette) free(_this->palette);
  if (_this->palette_idx) free(_this->palette_idx);
  free(_this);
}

/*! The helper function for the inline function
    \c hpgs_image_define_color, which is actually called for indexed images.
*/
int hpgs_image_define_color_func (hpgs_image *image, hpgs_paint_color *c)
{
  if (!image->palette) return 0;

  // compactify color for index lookup.
  unsigned rgb =
    ((unsigned)c->r << 24) | ((unsigned)c->g << 16) | ((unsigned)c->b << 8);

  // binary search in the index.
  int i0 = 0;
  int i1 = image->palette_ncolors;
  
  while (i1>i0)
    {
      int i = i0+(i1-i0)/2;

      if ((image->palette_idx[i] & 0xffffff00) < rgb)
	i0 = i+1;
      else
	i1 = i;
    }

  if (i0 < image->palette_ncolors &&
      rgb == (image->palette_idx[i0] & 0xffffff00))
    {
      c->index = image->palette_idx[i0] & 0xff;
      return 0;
    }

  // palette overflow.
  if (image->palette_ncolors >= 256)
    {
      c->index = 0;
      return hpgs_set_error(hpgs_i18n("Number of indexed colors exceeds 256."));
    }

  memmove(image->palette_idx+i0+1,image->palette_idx+i0,
	  (image->palette_ncolors-i0) * sizeof(unsigned));

  image->palette_idx[i0] = rgb | image->palette_ncolors;
  c->index = image->palette_ncolors;
  image->palette[image->palette_ncolors].r = c->r;
  image->palette[image->palette_ncolors].g = c->g;
  image->palette[image->palette_ncolors].b = c->b;
  ++image->palette_ncolors;

  return 0;
}

static int compare_color_idx(const void *a, const void *b)
{
  unsigned ai = (*((const unsigned *)a)) & 0xffffff00;
  unsigned bi = (*((const unsigned *)b)) & 0xffffff00;
  
  if (ai<bi) return -1;
  if (ai>bi) return 1;
  return 0;
}

/*! Sets the palette to an indexed image.
*/
int hpgs_image_set_palette (hpgs_image *image,
                            hpgs_palette_color *p, int np)
{
  int i;

  if (!image->palette)
    return hpgs_set_error(hpgs_i18n("Try to set a palette to a non-indexed image."));

  if (np<1 || np >256)
    return hpgs_set_error(hpgs_i18n("Invalid number %d of palette colors."),np);

  memmove(image->palette,p,sizeof(hpgs_palette_color)*np);
  image->palette_ncolors = np;
  
  for (i=0;i<np;++i)
    image->palette_idx[i] = 
      ((unsigned)p[i].r << 24) | ((unsigned)p[i].g << 16) | ((unsigned)p[i].b << 8) | i;

  // This is by no way time-critical, so we can use qsort here,
  // which is far from being optimal in performance.
  qsort(image->palette_idx,np,sizeof(unsigned),compare_color_idx);
  return 0;
}
