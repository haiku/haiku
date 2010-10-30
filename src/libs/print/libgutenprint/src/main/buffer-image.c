/*
 *   Buffer an Image
 *
 *   Copyright 2007 Sascha Sommer <saschasommer@freenet.de>
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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"

struct buffered_image_priv
{
	stp_image_t* image;
	unsigned char** buf;
	unsigned int flags;
};

static void
buffered_image_init(stp_image_t* image)
{
	struct buffered_image_priv *priv = image->rep;
	if(priv->image->init)
		priv->image->init(priv->image);
}


static int
buffered_image_width(stp_image_t * image)
{
	struct buffered_image_priv *priv = image->rep;
	return priv->image->width(priv->image);
}

static int
buffered_image_height(stp_image_t * image)
{
	struct buffered_image_priv *priv = image->rep;
	return priv->image->height(priv->image);
}

static const char *
buffered_image_get_appname(stp_image_t *image)
{
	struct buffered_image_priv *priv = image->rep;
	return priv->image->get_appname(priv->image);
}


static stp_image_status_t
buffered_image_get_row(stp_image_t* image,unsigned char *data, size_t byte_limit, int row)
{
	struct buffered_image_priv *priv = image->rep;
	int width = buffered_image_width(image);
	int height = buffered_image_height(image);
	/* FIXME this will break with padding bytes */
	int bytes_per_pixel = byte_limit / width;
	int inc = bytes_per_pixel;
	unsigned char* src;
	int i;
	/* fill buffer */
	if(!priv->buf){
		priv->buf = stp_zalloc((sizeof(unsigned short*) + 1) * height);
		if(!priv->buf){
			return STP_IMAGE_STATUS_ABORT;
		}
		for(i=0;i<height;i++){
			priv->buf[i] = stp_malloc(byte_limit);
			if(STP_IMAGE_STATUS_OK != priv->image->get_row(priv->image,priv->buf[i],byte_limit,i))
				return STP_IMAGE_STATUS_ABORT;
		}
	}
	if(priv->flags & BUFFER_FLAG_FLIP_Y)
		row = height - row - 1;

	src = priv->buf[row];

	if(priv->flags & BUFFER_FLAG_FLIP_X){
		src += byte_limit - bytes_per_pixel;
		inc = -bytes_per_pixel;
	}

	/* copy data */
	for( i = 0 ; i < width ; i++){
		memcpy(data,src,bytes_per_pixel);
		src += inc;
		data += bytes_per_pixel;
	}
	return STP_IMAGE_STATUS_OK;
}

static void
buffered_image_conclude(stp_image_t * image)
{
	struct buffered_image_priv *priv = image->rep;
	if(priv->buf){
		int i = 0;
		while(priv->buf[i]){
			stp_free(priv->buf[i]);
			++i;
		}
		stp_free(priv->buf);
		priv->buf = NULL;
	}
	if(priv->image->conclude)
		priv->image->conclude(priv->image);

	stp_free(priv);
	stp_free(image);
}

stp_image_t*
stpi_buffer_image(stp_image_t* image, unsigned int flags)
{
	struct buffered_image_priv *priv;
	stp_image_t* buffered_image = stp_zalloc(sizeof(stp_image_t));
	if(!buffered_image){
		return NULL;
	}
	priv = buffered_image->rep = stp_zalloc(sizeof(struct buffered_image_priv));
	if(!priv){
		stp_free(buffered_image);
		return NULL;
	}

	if(image->init)
		buffered_image->init = buffered_image_init;
	buffered_image->width = buffered_image_width;
	buffered_image->height = buffered_image_height;
	buffered_image->get_row = buffered_image_get_row;
	buffered_image->conclude = buffered_image_conclude;
	priv->image = image;
	priv->flags = flags;
	if(image->get_appname)
		buffered_image->get_appname = buffered_image_get_appname;


	return buffered_image;
}

 
