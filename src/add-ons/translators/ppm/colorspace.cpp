/*	colorspace.cpp	*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/


#include <Debug.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colorspace.h"


/* expands to BGRA in the output buffer from <whatever> in the input buffer */
int
expand_data(color_space from_space, unsigned char* in_data, int rowbytes,
	unsigned char* out_buf)
{
	ASSERT(in_data != out_buf);

	/*	We don't do YUV and friends yet.	*/
	/*	It's important to replicate the most significant component bits to LSB
	 * when going 15->24	*/
	unsigned char* in_out = out_buf;
	switch (from_space) {
		case B_RGB32:
			while (rowbytes > 3) {
				out_buf[0] = in_data[0];
				out_buf[1] = in_data[1];
				out_buf[2] = in_data[2];
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 4;
				rowbytes -= 4;
			}
			break;
		case B_RGBA32:
			memcpy(out_buf, in_data, rowbytes);
			break;
		case B_RGB24:
			while (rowbytes > 2) {
				out_buf[0] = in_data[0];
				out_buf[1] = in_data[1];
				out_buf[2] = in_data[2];
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 3;
				rowbytes -= 3;
			}
			break;
		case B_RGB15:
			while (rowbytes > 1) {
				uint16 val = in_data[0] + (in_data[1] << 8);
				out_buf[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				out_buf[1] = ((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				out_buf[2] = ((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 2;
				rowbytes -= 2;
			}
			break;
		case B_RGBA15:
			while (rowbytes > 1) {
				uint16 val = in_data[0] + (in_data[1] << 8);
				out_buf[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				out_buf[1] = ((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				out_buf[2] = ((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
				out_buf[3] = (val & 0x8000) ? 255 : 0;
				out_buf += 4;
				in_data += 2;
				rowbytes -= 2;
			}
			break;
		case B_RGB16:
			while (rowbytes > 1) {
				uint16 val = in_data[0] + (in_data[1] << 8);
				out_buf[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				out_buf[1] = ((val & 0x7e0) >> 3) | ((val & 0x7e0) >> 9);
				out_buf[2] = ((val & 0xf800) >> 8) | ((val & 0xf800) >> 13);
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 2;
				rowbytes -= 2;
			}
			break;
		case B_RGB32_BIG:
			while (rowbytes > 3) {
				out_buf[0] = in_data[3];
				out_buf[1] = in_data[2];
				out_buf[2] = in_data[1];
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 4;
				rowbytes -= 4;
			}
			break;
		case B_RGBA32_BIG:
			while (rowbytes > 3) {
				out_buf[0] = in_data[3];
				out_buf[1] = in_data[2];
				out_buf[2] = in_data[1];
				out_buf[3] = in_data[0];
				out_buf += 4;
				in_data += 4;
				rowbytes -= 4;
			}
			break;
		case B_RGB24_BIG:
			while (rowbytes > 2) {
				out_buf[0] = in_data[2];
				out_buf[1] = in_data[1];
				out_buf[2] = in_data[0];
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 3;
				rowbytes -= 3;
			}
			break;
		case B_RGB15_BIG:
			while (rowbytes > 1) {
				uint16 val = in_data[1] + (in_data[0] << 8);
				out_buf[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				out_buf[1] = ((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				out_buf[2] = ((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 2;
				rowbytes -= 2;
			}
			break;
		case B_RGBA15_BIG:
			while (rowbytes > 1) {
				uint16 val = in_data[1] + (in_data[0] << 8);
				out_buf[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				out_buf[1] = ((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				out_buf[2] = ((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
				out_buf[3] = (val & 0x8000) ? 255 : 0;
				out_buf += 4;
				in_data += 2;
				rowbytes -= 2;
			}
			break;
		case B_RGB16_BIG:
			while (rowbytes > 1) {
				uint16 val = in_data[1] + (in_data[0] << 8);
				out_buf[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				out_buf[1] = ((val & 0x7e0) >> 3) | ((val & 0x7e0) >> 9);
				out_buf[2] = ((val & 0xf800) >> 8) | ((val & 0xf800) >> 13);
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 2;
				rowbytes -= 2;
			}
			break;
		case B_CMAP8: {
			const color_map* map = system_colors();
			while (rowbytes > 0) {
				rgb_color c = map->color_list[in_data[0]];
				out_buf[0] = c.blue;
				out_buf[1] = c.green;
				out_buf[2] = c.red;
				out_buf[3] = c.alpha;
				out_buf += 4;
				in_data += 1;
				rowbytes -= 1;
			}
		} break;
		case B_GRAY8:
			while (rowbytes > 0) {
				unsigned char ch = *in_data;
				out_buf[0] = ch;
				out_buf[1] = ch;
				out_buf[2] = ch;
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 1;
				rowbytes -= 1;
			}
			break;
		case B_GRAY1:
			while (rowbytes > 0) { /* expansion 1->32 is pretty good :-) */
				unsigned char c1 = *in_data;
				for (int b = 128; b; b = b >> 1) {
					unsigned char ch;
					if (c1 & b) {
						ch = 0;
					} else {
						ch = 255;
					}
					out_buf[0] = ch;
					out_buf[1] = ch;
					out_buf[2] = ch;
					out_buf[3] = 255;
					out_buf += 4;
				}
				in_data += 1;
				rowbytes -= 1;
			}
			break;
		case B_CMY24: /*	We do the "clean" inversion which doesn't correct
						 for printing ink deficiencies.	*/
			while (rowbytes > 2) {
				out_buf[0] = 255 - in_data[2];
				out_buf[1] = 255 - in_data[1];
				out_buf[2] = 255 - in_data[0];
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 3;
				rowbytes -= 3;
			}
			break;
		case B_CMY32:
			while (rowbytes > 3) {
				out_buf[0] = 255 - in_data[2];
				out_buf[1] = 255 - in_data[1];
				out_buf[2] = 255 - in_data[0];
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 4;
				rowbytes -= 4;
			}
			break;
		case B_CMYA32:
			while (rowbytes > 3) {
				out_buf[0] = 255 - in_data[2];
				out_buf[1] = 255 - in_data[1];
				out_buf[2] = 255 - in_data[0];
				out_buf[3] = in_data[3];
				out_buf += 4;
				in_data += 4;
				rowbytes -= 4;
			}
			break;
		case B_CMYK32: /*	We assume uniform gray removal, and no
						  under-color-removal.	*/
			while (rowbytes > 3) {
				int comp = 255 - in_data[2] - in_data[3];
				out_buf[0] = comp < 0 ? 0 : comp;
				comp = 255 - in_data[1] - in_data[3];
				out_buf[1] = comp < 0 ? 0 : comp;
				comp = 255 - in_data[0] - in_data[3];
				out_buf[2] = comp < 0 ? 0 : comp;
				out_buf[3] = 255;
				out_buf += 4;
				in_data += 4;
				rowbytes -= 4;
			}
			break;
		default:
			break;
	}
	return out_buf - in_out;
}


int
collapse_data(unsigned char* in_buf, int num_bytes, color_space out_space,
	unsigned char* out_buf)
{
	ASSERT(in_buf != out_buf);

	unsigned char* in_out = out_buf;
	/*	We could increase perceived image quality of down conversions by
	 * implementing	*/
	/*	dithering. However, someone might want to operate on the images after
	 */
	/*	conversion, in which case dithering would be un-good. Besides, good
	 * error	*/
	/*	diffusion requires more than one scan line to propagate errors to.	*/
	switch (out_space) {
		case B_RGB32:
			memcpy(out_buf, in_buf, num_bytes);
			break;
		case B_RGBA32:
			memcpy(out_buf, in_buf, num_bytes);
			break;
		case B_RGB24:
			while (num_bytes > 3) {
				out_buf[0] = in_buf[0];
				out_buf[1] = in_buf[1];
				out_buf[2] = in_buf[2];
				out_buf += 3;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGB16:
			while (num_bytes > 3) {
				uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 3) & 0x7e0)
					| ((in_buf[2] << 8) & 0xf800);
				out_buf[0] = val;
				out_buf[1] = val >> 8;
				out_buf += 2;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGB15:
			while (num_bytes > 3) {
				uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 2) & 0x3e0)
					| ((in_buf[2] << 7) & 0x7c00) | 0x8000;
				out_buf[0] = val;
				out_buf[1] = val >> 8;
				out_buf += 2;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGBA15:
			while (num_bytes > 3) {
				uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 2) & 0x3e0)
					| ((in_buf[2] << 7) & 0x7c00);
				if (in_buf[3] > 127) {
					val = val | 0x8000;
				}
				out_buf[0] = val;
				out_buf[1] = val >> 8;
				out_buf += 2;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_CMAP8: {
			const color_map* map = system_colors();
			while (num_bytes > 3) {
				if (in_buf[3] < 128) {
					out_buf[0] = B_TRANSPARENT_8_BIT;
				} else {
					uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 2) & 0x3e0)
						| ((in_buf[2] << 7) & 0x7c00);
					out_buf[0] = map->index_map[val];
				}
				out_buf += 1;
				in_buf += 4;
				num_bytes -= 4;
			}
		} break;
		case B_GRAY8:
			while (num_bytes > 3) { /*	There are better algorithms than Y =
									   .25B+.50G+.25R	*/
				/*	but hardly faster -- and it's still better than (B+G+R)/3 !
				 */
				out_buf[0] = (in_buf[0] + in_buf[1] * 2 + in_buf[2]) >> 2;
				out_buf += 1;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_GRAY1: {
			uchar ob = 0;
			int cnt = 0;
			uchar c = 0;
			while (num_bytes > 3) {
				if (cnt == 8) {
					out_buf[0] = ob;
					out_buf += 1;
					cnt = 0;
					ob = 0;
				}
				c = ((in_buf[0] + in_buf[1] * 2 + in_buf[2]) & 0x200)
					>> (2 + cnt);
				ob = ob | c;
				cnt++;
				in_buf += 4;
				num_bytes -= 4;
			}
			if (cnt > 0) {
				out_buf[0] = ob;
				out_buf += 1;
			}
		} break;
		/* big endian version, when the encoding is not endianess independant */
		case B_RGB32_BIG:
			while (num_bytes > 3) {
				out_buf[3] = in_buf[0];
				out_buf[2] = in_buf[1];
				out_buf[1] = in_buf[2];
				out_buf += 4;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGBA32_BIG:
			while (num_bytes > 3) {
				out_buf[3] = in_buf[0];
				out_buf[2] = in_buf[1];
				out_buf[1] = in_buf[2];
				out_buf[0] = in_buf[3];
				out_buf += 4;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGB24_BIG:
			while (num_bytes > 3) {
				out_buf[2] = in_buf[0];
				out_buf[1] = in_buf[1];
				out_buf[0] = in_buf[2];
				out_buf += 3;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGB16_BIG:
			while (num_bytes > 3) {
				uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 3) & 0x7e0)
					| ((in_buf[2] << 8) & 0xf800);
				out_buf[0] = val >> 8;
				out_buf[1] = val;
				out_buf += 2;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGB15_BIG:
			while (num_bytes > 3) {
				uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 2) & 0x3e0)
					| ((in_buf[2] << 7) & 0x7c00) | 0x8000;
				out_buf[0] = val >> 8;
				out_buf[1] = val;
				out_buf += 2;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_RGBA15_BIG:
			while (num_bytes > 3) {
				uint16 val = (in_buf[0] >> 3) | ((in_buf[1] << 2) & 0x3e0)
					| ((in_buf[2] << 7) & 0x7c00);
				if (in_buf[3] > 127) {
					val = val | 0x8000;
				}
				out_buf[0] = val >> 8;
				out_buf[1] = val;
				out_buf += 2;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_CMY24:
			while (num_bytes > 3) {
				out_buf[0] = 255 - in_buf[2];
				out_buf[1] = 255 - in_buf[1];
				out_buf[2] = 255 - in_buf[0];
				out_buf += 3;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_CMY32:
			while (num_bytes > 3) {
				out_buf[0] = 255 - in_buf[2];
				out_buf[1] = 255 - in_buf[1];
				out_buf[2] = 255 - in_buf[0];
				out_buf += 4;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_CMYA32:
			while (num_bytes > 3) {
				out_buf[0] = 255 - in_buf[2];
				out_buf[1] = 255 - in_buf[1];
				out_buf[2] = 255 - in_buf[0];
				out_buf[3] = in_buf[3];
				out_buf += 4;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		case B_CMYK32:
			while (num_bytes > 3) { /*	We do direct gray removal	*/
				int c = 255 - in_buf[2];
				int m = 255 - in_buf[1];
				int y = 255 - in_buf[0];
				int k = (c > m) ? ((y > c) ? y : c) : ((y > m) ? y : m);
				out_buf[0] = c - k;
				out_buf[1] = m - k;
				out_buf[2] = y - k;
				out_buf[3] = k;
				out_buf += 4;
				in_buf += 4;
				num_bytes -= 4;
			}
			break;
		default:
			break;
	}
	return out_buf - in_out;
}


#if DEBUG_DATA
static void
print_data(unsigned char* ptr, int n)
{
	while (n-- > 0) {
		printf("%02x ", *(ptr++));
	}
	printf("\n");
}
#endif


status_t
convert_space(color_space in_space, color_space out_space,
	unsigned char* in_data, int rowbytes, unsigned char* out_data)
{
	ASSERT(in_data != out_data);

	/*	Instead of coding each transformation separately, which would create
	 */
	/*	a very large number of conversion functions, we write one function to
	 */
	/*	convert to RGBA32, and another function to convert from RGBA32, and	*/
	/*	put them together to get a manageable program, at a slight expense in
	 */
	/*	conversion speed.	*/

	int n;
#if DEBUG_DATA
	printf("convert_space(%x, %x, %x)\n", in_space, out_space, rowbytes);
	printf("raw data: ");
	print_data(in_data, rowbytes);
#endif
	/*	If we convert from a format to itself, well...	*/
	if (in_space == out_space) {
		memcpy(out_data, in_data, rowbytes);
		return B_OK;
	}
	/*	When the input format is RGBA32, we don't need the first conversion.
	 */
	if (in_space == B_RGBA32) {
		n = collapse_data(in_data, rowbytes, out_space, out_data);
#if DEBUG_DATA
		printf("collapsed data: ");
		print_data(out_data, n);
#endif
		return B_OK;
	}
	/*	When the output format is RGBA32, we don't need any second conversion.
	 */
	if (out_space == B_RGB32 || out_space == B_RGBA32) {
		n = expand_data(in_space, in_data, rowbytes, out_data);
#if DEBUG_DATA
		printf("expanded data: ");
		print_data(out_data, n);
#endif
		return B_OK;
	}
	/*	Figure out byte expansion rate -- usually isn't more than 4	*/
	int mul = 4;
	if (in_space == B_GRAY1) {
		mul = 32;
	}
	unsigned char* buf = (unsigned char*) malloc(rowbytes * mul);
	if (buf == NULL) {
		/* oops! */
		return B_NO_MEMORY;
	}
	n = expand_data(in_space, in_data, rowbytes, buf);
#if DEBUG_DATA
	printf("expanded data: ");
	print_data(out_data, n);
#endif
	n = collapse_data(buf, n, out_space, out_data);
#if DEBUG_DATA
	printf("collapsed data: ");
	print_data(out_data, n);
#endif
	free(buf);
	return B_OK;
}


/*	Figure out what the rowbytes is for a given width in a given color space.
 */
/*	Rowbytes is bytes per pixel times width, rounded up to nearest multiple
 * of 4.	*/
int
calc_rowbytes(color_space space, int width)
{
	int v = width * 4;
	switch (space) {
		default:
			/* 4 is fine */
			break;
		case B_RGB24:
		case B_CMY24:
		case B_RGB24_BIG:
			v = width * 3;
			break;
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB16_BIG:
			v = width * 2;
			break;
		case B_CMAP8:
		case B_GRAY8:
			v = width;
			break;
		case B_GRAY1:
			v = (width + 7) / 8; /* whole bytes only, please */
			break;
	}
	v = (v + 3) & ~3;
	return v;
}
