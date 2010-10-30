/*
 * "$Id: print-image-thumbnail.c,v 1.1 2004/09/17 18:38:14 rleigh Exp $"
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
#include <sys/types.h>
#include <string.h>

#include <gutenprintui2/gutenprintui.h>
#include "gutenprintui-internal.h"

/* Concrete type to represent image */
typedef struct
{
  const guchar *data;
  gint w, h, bpp;
  gint32 real_bpp;
} thumbnail_image_t;

static const char *Thumbnail_get_appname(stp_image_t *image);
static void Thumbnail_conclude(stp_image_t *image);
static stp_image_status_t Thumbnail_get_row(stp_image_t *image,
					    unsigned char *data,
					    size_t byte_limit, int row);
static int Thumbnail_height(stp_image_t *image);
static int Thumbnail_width(stp_image_t *image);
static void Thumbnail_reset(stp_image_t *image);
static void Thumbnail_init(stp_image_t *image);

static stp_image_t theImage =
{
  Thumbnail_init,
  Thumbnail_reset,
  Thumbnail_width,
  Thumbnail_height,
  Thumbnail_get_row,
  Thumbnail_get_appname,
  Thumbnail_conclude,
  NULL
};

stp_image_t *
stpui_image_thumbnail_new(const guchar *data, gint w, gint h, gint bpp)
{
  thumbnail_image_t *im;
  if (! theImage.rep)
    theImage.rep = stp_malloc(sizeof(thumbnail_image_t));
  im = (thumbnail_image_t *) (theImage.rep);
  memset(im, 0, sizeof(thumbnail_image_t));
  im->data = data;
  im->w = w;
  im->h = h;
  im->bpp = bpp;

  theImage.reset(&theImage);
  return &theImage;
}

static int
Thumbnail_width(stp_image_t *image)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  return im->w;
}

static int
Thumbnail_height(stp_image_t *image)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  return im->h;
}

static stp_image_status_t
Thumbnail_get_row(stp_image_t *image, unsigned char *data,
		  size_t byte_limit, int row)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  const guchar *where = im->data + (row * im->w * im->bpp);
  memcpy(data, where, im->w * im->bpp);
  return STP_IMAGE_STATUS_OK;
}

static void
Thumbnail_init(stp_image_t *image)
{
  /* Nothing to do. */
}

static void
Thumbnail_reset(stp_image_t *image)
{
}

static void
Thumbnail_conclude(stp_image_t *image)
{
}

static const char *
Thumbnail_get_appname(stp_image_t *image)
{
  static char pluginname[] = "Thumbnail V" VERSION " - " RELEASE_DATE;
  return pluginname;
}

/*
 * End of "$Id: print-image-thumbnail.c,v 1.1 2004/09/17 18:38:14 rleigh Exp $".
 */
