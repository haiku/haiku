/*	colorspace.h	*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(COLORSPACE_H)
#define COLORSPACE_H


/*	This is the main conversion function; it converts data in in_data	*/
/*	with rowbytes amount of pixel data into some other color space in	*/
/*	out_data, with enough memory assumed to be in out_data for the		*/
/*	converted data.	*/
status_t convert_space(color_space in_space, color_space out_space,
	unsigned char* in_data, int rowbytes, unsigned char* out_data);

/*	This function expands rowbytes amount of data from in_data into		*/
/*	RGBA32 data in out_buf, which must be big enough.					*/
int expand_data(color_space from_space, unsigned char* in_data, int rowbytes,
	unsigned char* out_buf);

/*	This function converts num_bytes bytes of RGBA32 data into some new	*/
/*	color space in out_buf, where out_buf must be big enough.			*/
int collapse_data(unsigned char* in_buf, int num_bytes, color_space out_space,
	unsigned char* out_buf);

/*	Given a specific number of pixels in width in the color space space	*/
/*	this function calculates what the row_bytes should be.				*/
int calc_rowbytes(color_space space, int width);

#endif /* COLORSPACE_H */
