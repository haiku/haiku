/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "Scale.h"
#include <malloc.h>
#include <Bitmap.h>

typedef struct {
	long x_i;
	float p;
	float p1;
} row_values;

static void
scale_bilinear_8(const BBitmap *src, BBitmap *dest,
	const float xFactor, const float yFactor,
	volatile bool *running)
{
	register long drows, dcols, srows, scols;
	register unsigned char *spixptr, *dpixptr, *spix1, *spix2, *dpix;
	register unsigned long slb, dlb;
	register long i, j;
	float p, q, p1, q1;
	float xfac, yfac;
	float xfac_inv, yfac_inv;
	float src_y_f;
	long src_y_i, src_x_i;
	register unsigned char a, b, c, d;
	row_values *r, *rptr;
	float x_f;
	float result;
	
	// Get values from image
	dpix = dpixptr = (unsigned char *)dest->Bits();
	spixptr = (unsigned char *)src->Bits();
	srows = src->Bounds().IntegerHeight()+1;
	scols = src->Bounds().IntegerWidth()+1;
	drows = dest->Bounds().IntegerHeight()+1;
	dcols = dest->Bounds().IntegerWidth()+1;
	slb = src->BytesPerRow();
	dlb = dest->BytesPerRow();
	
	// Compute scale factors and inverse scale factors
	xfac = xFactor;
	yfac = yFactor;
	
	if (xfac < 0.0)
		xfac = (float) dcols / (float) scols;
	if (yfac < 0.0)
		yfac = (float) drows / (float) srows;
	
	xfac_inv = 1.0 / xfac;
	yfac_inv = 1.0 / yfac;
	
	
	// Allocate buffer for storing the values of ma and m1
	rptr = r = (row_values *)malloc (sizeof(row_values) * dcols);
	
	// Fill up the buffer once, to be used for each row
	for (i=0; i<dcols; i++)
	{
		x_f = i*xfac_inv;
		rptr->x_i = (long)x_f;
		rptr->p = x_f - (float) rptr->x_i;
		rptr->p1 = 1.0 - rptr->p;
		rptr++;
	}
	
	// Perform the scaling by inverse mapping from dest to source
	// That is, for each point in the destination, find the 
	// corresponding point in the source.
	for (i=0; *running && i<drows-1; i++)
	{
		src_y_f = i * yfac_inv;
		src_y_i = (long) src_y_f;
		
		q = src_y_f - (float)src_y_i;
		q1 = 1.0 - q;
		spix1 = spixptr + src_y_i*slb;
		spix2 = spixptr + (src_y_i+1)*slb;
		rptr = r;
		
		for (j =0; j<dcols-1; j++,rptr++)
		{
			src_x_i = rptr->x_i;
			p = rptr->p;
			p1 = rptr->p1;
			
			// Get the four corner pixels
			a = *(spix1 + src_x_i);
			b = *(spix1 + src_x_i + 1);
			c = *(spix2 + src_x_i);
			d = *(spix2 + src_x_i + 1);
			
			// Compute the interpolated pixel value
			result = (((float)8*p1 + (float)b*p)*q1
				+ ((float)c*p1 + (float)d*p)*q);
				
			*dpix++ = (unsigned char) result;			
		}		
		
		// Advance to the next destination line
		dpix = (dpixptr += dlb);
	}

	free(r);
}

static void
scale_bilinear_32(const BBitmap *src, BBitmap *dest, 
	const float xFactor, const float yFactor,
	volatile bool *running)
{
	register long drows, dcols, srows, scols;
	register unsigned long *spixptr, *spix1, *spix2;
	register unsigned char *dpixptr, *dpix;
	register unsigned long slb, dlb;
	register long i, j;
	float p, q, p1, q1;
	float xfac = xFactor, yfac = yFactor;
	float xfac_inv, yfac_inv;
	float src_y_f;
	long src_y_i, src_x_i;
	register unsigned char *a, *b, *c, *d;
	row_values *r, *rptr;
	float x_f;
	float rred, rgreen, rblue;
	
	srows = src->Bounds().IntegerHeight()+1;
	scols = src->Bounds().IntegerWidth()+1;
	drows = dest->Bounds().IntegerHeight()+1;
	dcols = dest->Bounds().IntegerWidth()+1;

	if (xFactor < 0.0)
		xfac = (float) dcols / (float) scols;
	else
		dcols = (long) ceil(scols * xfac);
	
	if (yFactor < 0.0)
		yfac = (float) drows / (float) srows;
	else
		drows = (long) ceil(srows * yfac);
		


	// Get values from image
	dpixptr = (unsigned char *)dest->Bits();
	dpix = (unsigned char *)dpixptr;
	spixptr = (unsigned long *)src->Bits();
	slb = src->BytesPerRow()/4;
	dlb = dest->BytesPerRow();
	
	
	xfac_inv = 1.0 / xfac;
	yfac_inv = 1.0 / yfac;
	
	
	// Allocate buffer for storing the values of ma and m1
	rptr = r = (row_values *)malloc (sizeof(row_values) * dcols);
	
	// Fill up the buffer once, to be used for each row
	for (i=0; i<dcols; i++)
	{
		x_f = i*xfac_inv;
		rptr->x_i = (long)x_f;
		rptr->p = x_f - (float) rptr->x_i;
		rptr->p1 = 1.0 - rptr->p;
		rptr++;
	}
	
	// Perform the scaling by inverse mapping from dest to source
	// That is, for each point in the destination, find the 
	// corresponding point in the source.
	for (i=0; *running && i<drows-1; i++)
	{
		src_y_f = i * yfac_inv;
		src_y_i = (long) src_y_f;
		
		q = src_y_f - (float)src_y_i;
		q1 = 1.0 - q;
		spix1 = spixptr + src_y_i*slb;
		spix2 = spixptr + (src_y_i+1)*slb;
		rptr = r;
		
		for (j =0; j<dcols-1; j++,rptr++)
		{
			src_x_i = rptr->x_i;
			p = rptr->p;
			p1 = rptr->p1;
			
			// Get the four corner pixels
			a = (unsigned char *)(spix1 + src_x_i);
			b = (unsigned char *)(spix1 + src_x_i + 1);
			c = (unsigned char *)(spix2 + src_x_i);
			d = (unsigned char *)(spix2 + src_x_i + 1);
			
			// Compute the interpolated pixel value
			rblue = (((float)a[0]*p1 + (float)b[0]*p)*q1
				+ ((float)c[0]*p1 + (float)d[0]*p)*q);
			rgreen = (((float)a[1]*p1 + (float)b[1]*p)*q1
				+ ((float)c[1]*p1 + (float)d[1]*p)*q);
			rred = (((float)a[2]*p1 + (float)b[2]*p)*q1
				+ ((float)c[2]*p1 + (float)d[2]*p)*q);
				
			dpix[0] = (unsigned char) rblue;
			dpix[1] = (unsigned char) rgreen;
			dpix[2] = (unsigned char) rred;
			dpix += 4;
		}		
		
		// Advance to the next destination line
		dpix = (dpixptr += dlb);
	}

	free(r);
}

status_t scale(const BBitmap *src, BBitmap *dst, 
	 volatile bool* running,
	 const float xFactor, const float yFactor,
	 scale_method scmethod)
{
	if (src->ColorSpace() != dst->ColorSpace())
		return -1;
		
	
	switch (scmethod)
	{
		case IMG_SCALE_BILINEAR:
		{
			switch (src->ColorSpace())
			{
				case B_COLOR_8_BIT:
				case B_GRAYSCALE_8_BIT:
					scale_bilinear_8(src, dst, xFactor, yFactor, running);
				break;
		
				case B_RGB32:
				case B_RGBA32:
					scale_bilinear_32(src, dst, xFactor, yFactor, running);
				break;
		
				default:	// color space we can't deal with
					return -1;
				break;
			}
		}
	}
	
	return B_NO_ERROR;
}
