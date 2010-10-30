/*
 * "$Id: print-image-gimp.c,v 1.2 2004/06/22 18:52:15 rleigh Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>

#include "print_gimp.h"

#include "print-intl.h"


/*
 * "Image" ADT
 *
 * This file defines an abstract data type called "Image".  An Image wraps
 * a Gimp drawable (or some other application-level image representation)
 * for presentation to the low-level printer drivers (which do CMYK
 * separation, dithering and weaving).  The Image ADT has the ability
 * to perform any combination of flips and rotations on the image,
 * and then deliver individual rows to the driver code.
 *
 * Stuff which might be useful to do in this layer:
 *
 * - Scaling, optionally with interpolation/filtering.
 *
 * - Colour-adjustment.
 *
 * - Multiple-image composition.
 *
 * Also useful might be to break off a thin application-dependent
 * sublayer leaving this layer (which does the interesting stuff)
 * application-independent.
 */


/* Concrete type to represent image */
typedef struct
{
  GimpDrawable *drawable;
  GimpPixelRgn  rgn;

  /*
   * Transformations we can impose on the image.  The transformations
   * are considered to be performed in the order given here.
   */

  /* 1: Transpose the x and y axes (flip image over its leading diagonal) */
  int columns;		/* Set if returning columns instead of rows. */

  /* 2: Translate (ox,oy) to the origin */
  int ox, oy;		/* Origin of image */

  /* 3: Flip vertically about the x axis */
  int increment;	/* +1 or -1 for offset of row n+1 from row n. */

  /* 4: Crop to width w, height h */
  int w, h;		/* Width and height of output image */

  /* 5: Flip horizontally about the vertical centre-line of the image */
  int mirror;		/* Set if mirroring rows end-for-end. */

  gint32 image_ID;
  gint32 ncolors;
  gint32 real_bpp;
  GimpImageBaseType base_type;
  guchar *cmap;
  guchar *alpha_table;
  guchar *tmp;
  gint last_printed_percent;
  gint initialized;
} Gimp_Image_t;

static const char *Image_get_appname(stp_image_t *image);
static void Image_conclude(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image,
					unsigned char *data,
					size_t byte_limit, int row);
static int Image_height(stp_image_t *image);
static int Image_width(stp_image_t *image);
static void Image_reset(stp_image_t *image);
static void Image_init(stp_image_t *image);

static void Image_transpose(stpui_image_t *image);
static void Image_hflip(stpui_image_t *image);
static void Image_vflip(stpui_image_t *image);
static void Image_rotate_ccw(stpui_image_t *image);
static void Image_rotate_cw(stpui_image_t *image);
static void Image_rotate_180(stpui_image_t *image);
static void Image_crop(stpui_image_t *image, int, int, int, int);

static stpui_image_t theImage =
{
  {
    Image_init,
    Image_reset,
    Image_width,
    Image_height,
    Image_get_row,
    Image_get_appname,
    Image_conclude,
    NULL,
  },
  Image_transpose,
  Image_hflip,
  Image_vflip,
  Image_rotate_ccw,
  Image_rotate_cw,
  Image_rotate_180,
  Image_crop
};

static void
compute_alpha_table(Gimp_Image_t *image)
{
  unsigned val, alpha;
  image->alpha_table = stp_malloc(65536 * sizeof(unsigned char));
  for (val = 0; val < 256; val++)
    for (alpha = 0; alpha < 256; alpha++)
      image->alpha_table[(val * 256) + alpha] =
	val * alpha / 255 + 255 - alpha;
}

static inline unsigned char
alpha_lookup(Gimp_Image_t *image, int val, int alpha)
{
  return image->alpha_table[(val * 256) + alpha];
}

stpui_image_t *
Image_GimpDrawable_new(GimpDrawable *drawable, gint32 image_ID)
{
  Gimp_Image_t *im = stp_malloc(sizeof(Gimp_Image_t));
  memset(im, 0, sizeof(Gimp_Image_t));
  im->drawable = drawable;
  gimp_pixel_rgn_init(&(im->rgn), drawable, 0, 0,
                      drawable->width, drawable->height, FALSE, FALSE);
  im->image_ID = image_ID;
  im->base_type = gimp_image_base_type(image_ID);
  im->initialized = 0;
  theImage.im.rep = im;
  theImage.im.reset(&(theImage.im));
  switch (im->base_type)
    {
    case GIMP_INDEXED:
      im->cmap = gimp_image_get_cmap(image_ID, &(im->ncolors));
      im->real_bpp = 3;
      break;
    case GIMP_GRAY:
      im->real_bpp = 1;
      break;
    case GIMP_RGB:
      im->real_bpp = 3;
      break;
    }
  if (im->drawable->bpp == 2 || im->drawable->bpp == 4)
    compute_alpha_table(im);
  return &theImage;
}

static void
Image_init(stp_image_t *image)
{
  /* Nothing to do. */
}

static void
Image_reset(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  im->columns = FALSE;
  im->ox = 0;
  im->oy = 0;
  im->increment = 1;
  im->w = im->drawable->width;
  im->h = im->drawable->height;
  im->mirror = FALSE;
}

static int
Image_width(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  return im->w;
}

static int
Image_height(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  return im->h;
}

static stp_image_status_t
Image_get_row(stp_image_t *image, unsigned char *data, size_t byte_limit,
	      int row)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  int last_printed_percent;
  guchar *inter;
  if (!im->initialized)
    {
      gimp_progress_init(_("Printing..."));
      switch (im->base_type)
	{
	case GIMP_INDEXED:
	  im->tmp = stp_malloc(im->drawable->bpp * im->w);
	  break;
	case GIMP_GRAY:
	  if (im->drawable->bpp == 2)
	    im->tmp = stp_malloc(im->drawable->bpp * im->w);
	  break;
	case GIMP_RGB:
	  if (im->drawable->bpp == 4)
	    im->tmp = stp_malloc(im->drawable->bpp * im->w);
	  break;
	}
      im->initialized = 1;
    }    
  if (im->tmp)
    inter = im->tmp;
  else
    inter = data;
  if (im->columns)
    gimp_pixel_rgn_get_col(&(im->rgn), inter,
                           im->oy + row * im->increment, im->ox, im->w);
  else
    gimp_pixel_rgn_get_row(&(im->rgn), inter,
                           im->ox, im->oy + row * im->increment, im->w);
  if (im->cmap)
    {
      int i;
      if (im->alpha_table)
	{
	  for (i = 0; i < im->w; i++)
	    {
	      int j;
	      for (j = 0; j < 3; j++)
		{
		  gint32 tval = im->cmap[(3 * inter[2 * i]) + j];
		  data[(3 * i) + j] = alpha_lookup(im, tval,
						   inter[(2 * i) + 1]);
		}

	    }
	}
      else
	{
	  for (i = 0; i < im->w; i++)
	    {
	      data[(3 * i) + 0] = im->cmap[(3 * inter[i]) + 0];
	      data[(3 * i) + 1] = im->cmap[(3 * inter[i]) + 1];
	      data[(3 * i) + 2] = im->cmap[(3 * inter[i]) + 2];
	    }
	}
    }
  else if (im->alpha_table)
    {
      int i;
      for (i = 0; i < im->w; i++)
	{
	  int j;
	  for (j = 0; j < im->real_bpp; j++)
	    {
	      gint32 tval = inter[(i * im->drawable->bpp) + j];
	      data[(i * im->real_bpp) + j] =
		alpha_lookup(im, tval,
			     inter[((i + 1) * im->drawable->bpp) - 1]);
	    }

	}
    }
  if (im->mirror)
    {
      /* Flip row -- probably inefficiently */
      int f;
      int l;
      int b = im->real_bpp;
      for (f = 0, l = im->w - 1; f < l; f++, l--)
	{
	  int c;
	  unsigned char tmp;
	  for (c = 0; c < b; c++)
	    {
	      tmp = data[f*b+c];
	      data[f*b+c] = data[l*b+c];
	      data[l*b+c] = tmp;
	    }
	}
    }
  last_printed_percent = row * 100 / im->h;
  if (last_printed_percent > im->last_printed_percent)
    {
      gimp_progress_update((double) row / (double) im->h);
      im->last_printed_percent = last_printed_percent;
    }
  return STP_IMAGE_STATUS_OK;
}

static void
Image_transpose(stpui_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  int tmp;

  if (im->mirror) im->ox += im->w - 1;

  im->columns = !im->columns;

  tmp = im->ox;
  im->ox = im->oy;
  im->oy = tmp;

  tmp = im->mirror;
  im->mirror = im->increment < 0;
  im->increment = tmp ? -1 : 1;

  tmp = im->w;
  im->w = im->h;
  im->h = tmp;

  if (im->mirror) im->ox -= im->w - 1;
}

static void
Image_hflip(stpui_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  im->mirror = !im->mirror;
}

static void
Image_vflip(stpui_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  im->oy += (im->h-1) * im->increment;
  im->increment = -im->increment;
}

/*
 * Image_crop:
 *
 * Crop the given number of pixels off the LEFT, TOP, RIGHT and BOTTOM
 * of the image.
 */

static void
Image_crop(stpui_image_t *image, int left, int top, int right, int bottom)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  int xmax = (im->columns ? im->drawable->height : im->drawable->width) - 1;
  int ymax = (im->columns ? im->drawable->width : im->drawable->height) - 1;

  int nx = im->ox + im->mirror ? right : left;
  int ny = im->oy + top * (im->increment);

  int nw = im->w - left - right;
  int nh = im->h - top - bottom;

  int wmax, hmax;

  if (nx < 0)         nx = 0;
  else if (nx > xmax) nx = xmax;

  if (ny < 0)         ny = 0;
  else if (ny > ymax) ny = ymax;

  wmax = xmax - nx + 1;
  hmax = im->increment ? ny + 1 : ymax - ny + 1;

  if (nw < 1)         nw = 1;
  else if (nw > wmax) nw = wmax;

  if (nh < 1)         nh = 1;
  else if (nh > hmax) nh = hmax;

  im->ox = nx;
  im->oy = ny;
  im->w = nw;
  im->h = nh;
}

static void
Image_rotate_ccw(stpui_image_t *image)
{
  Image_transpose(image);
  Image_vflip(image);
}

static void
Image_rotate_cw(stpui_image_t *image)
{
  Image_transpose(image);
  Image_hflip(image);
}

static void
Image_rotate_180(stpui_image_t *image)
{
  Image_vflip(image);
  Image_hflip(image);
}

static void
Image_conclude(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  gimp_progress_update(1);
  if (im->alpha_table)
    stp_free(im->alpha_table);
  if (im->tmp)
    stp_free(im->tmp);
}

static const char *
Image_get_appname(stp_image_t *image)
{
  static char pluginname[] = "Print plug-in V" VERSION " - " RELEASE_DATE
    " for GIMP";
  return pluginname;
}

/*
 * End of "$Id: print-image-gimp.c,v 1.2 2004/06/22 18:52:15 rleigh Exp $".
 */
