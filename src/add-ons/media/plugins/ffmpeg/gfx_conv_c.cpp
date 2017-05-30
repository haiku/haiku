#include "gfx_conv_c.h"

#include <strings.h>
#include <stdio.h>


void
gfx_conv_null(AVFrame *in, AVFrame *out, int width, int height)
{
	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
}


void
gfx_conv_yuv410p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	int i;
//	bool toggle=false;
	unsigned long *po_eol;
	register unsigned long *p;
	register unsigned long y1;
	register unsigned long y2;
	register unsigned short u;
	register unsigned short v;
	register unsigned long a;
	register unsigned long b;
	register unsigned long c;
	register unsigned long d;
//	printf("[%ld, %ld, %ld] -> [%ld, %ld, %ld]\n", in->linesize[0],
//		in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1],
//		out->linesize[2]);
//	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
	unsigned long *po = (unsigned long *)out->data[0];
	unsigned long *pi = (unsigned long *)in->data[0];
	unsigned short *pi2 = (unsigned short *)in->data[1];
	unsigned short *pi3 = (unsigned short *)in->data[2];
	for (i = 0; i < height; i++) {
		for (p = po,
			po_eol = (unsigned long *)(((char *)po) + out->linesize[0]);
			p < po_eol;) {
//			*(((long *)po) + j) = (long)(*(pi + j) + (*(pi2 + j) << 8) 
//				+ (*(pi + j + 3) << 16) + (*(pi3 + j) << 24));
			y1 = *pi++;
			y2 = *pi++;
			u = *pi2;
			v = *pi3;
			a = (long)((y1 & 0x0FF) | ((u& 0x0FF) << 8) | ((y1 & 0x0FF00) << 8)
				| ((v & 0x0FF) << 24));
			b = (long)(((y1 & 0x0FF0000) >> 16) | ((u& 0x0FF) << 8)
				| ((y1 & 0x0FF000000) >> 8) | ((v & 0x0FF) << 24));
			c = (long)((y2 & 0x0FF) | (u & 0x0FF00) | ((y2 & 0x0FF00) << 8)
				| ((v & 0x0FF00) << 16));
			d = (long)(((y2 & 0x0FF0000) >> 16) | ((u& 0x0FF00))
				| ((y2 & 0x0FF000000) >> 8) | ((v & 0x0FF00) << 16));
//			if (toggle) {
				pi2++;
//			} else {
				pi3++;
//			}
//			toggle = !toggle;

			*(p++) = a;
			*(p++) = b;
			*(p++) = c;
			*(p++) = d;
		}
		po = (unsigned long *)((char *)po + out->linesize[0]);
		pi = (unsigned long *)(in->data[0] + i * in->linesize[0]);
		pi2 = (unsigned short *)(in->data[1] + ((i + 2) / 4)
			* in->linesize[1]);
		pi3 = (unsigned short *)(in->data[2] + ((i + 3) / 4)
			* in->linesize[2]);
	}
}


void
gfx_conv_yuv411p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	// this one is for cyuv
	// TODO: (== yuv410p atm)
	gfx_conv_yuv410p_ycbcr422_c(in, out, width, height);
}


void
gfx_conv_yuv420p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	unsigned long *po_eol;
	register unsigned long *p;
	register unsigned long y1;
	register unsigned long y2;
	register unsigned long u;
	register unsigned long v;
	register unsigned long a;
	register unsigned long b;
	register unsigned long c;
	register unsigned long d;
//	printf("[%ld, %ld, %ld] -> [%ld, %ld, %ld]\n", in->linesize[0],
//		in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1],
//		out->linesize[2]);
//	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
	unsigned long *po = (unsigned long *)out->data[0];
	unsigned long *pi = (unsigned long *)in->data[0];
	unsigned long *pi2 = (unsigned long *)in->data[1];
	unsigned long *pi3 = (unsigned long *)in->data[2];
	for (int i = 0; i < height; i++) {
		for (p = po, po_eol = (unsigned long *)(((char *)po)+out->linesize[0]);
			p < po_eol;) {
//			*(((long *)po) + j) = (long)(*(pi + j) + (*(pi2 + j) << 8)
//				+ (*(pi + j + 3) << 16) + (*(pi3 + j) << 24));
			y1 = *pi++;
			y2 = *pi++;
			u = *pi2++;
			v = *pi3++;
			a = (long)((y1 & 0x0FF) | ((u & 0x0FF) << 8) | ((y1 & 0x0FF00) << 8)
				| ((v & 0x0FF) << 24));
			b = (long)(((y1 & 0x0FF0000) >> 16) | ((u & 0x0FF00))
				| ((y1 & 0x0FF000000) >> 8) | ((v & 0x0FF00) << 16));
			c = (long)((y2 & 0x0FF) | ((u & 0x0FF0000) >> 8)
				| ((y2 & 0x0FF00) << 8) | ((v & 0x0FF0000) << 8));
			d = (long)(((y2 & 0x0FF0000) >> 16) | ((u & 0x0FF000000) >> 16)
				| ((y2 & 0x0FF000000) >> 8) | ((v & 0x0FF000000)));

			*(p++) = a;
			*(p++) = b;
			*(p++) = c;
			*(p++) = d;
		}
		po = (unsigned long *)((char *)po + out->linesize[0]);
		pi = (unsigned long *)(in->data[0] + i * in->linesize[0]);
		pi2 = (unsigned long *)(in->data[1] + ((i + 1) / 2) * in->linesize[1]);
		pi3 = (unsigned long *)(in->data[2] + (i / 2) * in->linesize[2]);
	}
}


#define CLIP(a) if (0xffffff00 & (uint32)a) { if (a < 0) a = 0; else a = 255; }

static inline uint32
YUV10TORGBA8888(uint16 y, uint16 u, uint16 v)
{
	int32 c = y - 64;
	int32 d = u - 512;
	int32 e = v - 512;

	int32 r = (298 * c + 409 * e + 512) >> 10;
	int32 g = (298 * c - 100 * d - 208 * e + 512) >> 10;
	int32 b = (298 * c + 516 * d + 512) >> 10;

	CLIP(r);
	CLIP(g);
	CLIP(b);

	return (uint32)((255 << 24) | (r << 16) | (g << 8) | b);
}


void
gfx_conv_yuv420p10le_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	uint16 *yBase = (uint16 *)in->data[0];
	uint16 *uBase = (uint16 *)in->data[1];
	uint16 *vBase = (uint16 *)in->data[2];

	uint32 *rgbBase = (uint32 *)out->data[0];

	int uvIndex;

	for (int32 i = 0; i < height; i++) {
		uvIndex = 0;
		for (int32 j=0; j < width; j += 2) {
			rgbBase[j] = YUV10TORGBA8888(yBase[j], uBase[uvIndex],
				vBase[uvIndex]);
			rgbBase[j + 1] = YUV10TORGBA8888(yBase[j + 1], uBase[uvIndex],
				vBase[uvIndex]);
			uvIndex++;
		}

		// Advance pointers to next line
		yBase += in->linesize[0] / 2;

		if ((i & 1) == 0) {
			// These are the same for 2 lines
			uBase += in->linesize[1] / 2;
			vBase += in->linesize[2] / 2;
		}

		rgbBase += out->linesize[0] / 4;
	}
}


// http://en.wikipedia.org/wiki/YUV
static inline uint32
YUV444TORGBA8888(uint8 y, uint8 u, uint8 v)
{
	int32 c = y - 16;
	int32 d = u - 128;
	int32 e = v - 128;

	int32 r = (298 * c + 409 * e + 128) >> 8;
	int32 g = (298 * c - 100 * d - 208 * e + 128) >> 8;
	int32 b = (298 * c + 516 * d + 128) >> 8;

	CLIP(r);
	CLIP(g);
	CLIP(b);

	return (uint32)((255 << 24) | (r << 16) | (g << 8) | b);
}


void
gfx_conv_yuv410p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	uint8 *yBase = (uint8 *)in->data[0];
	uint8 *uBase = (uint8 *)in->data[1];
	uint8 *vBase = (uint8 *)in->data[2];

	uint32 *rgbBase = (uint32 *)out->data[0];

	int uvIndex;

	for (int32 i = 0; i < height; i++) {
		uvIndex = 0;
		for (int32 j=0; j < width; j+=4) {
			rgbBase[j] = YUV444TORGBA8888(yBase[j], uBase[uvIndex],
				vBase[uvIndex]);
			rgbBase[j + 1] = YUV444TORGBA8888(yBase[j + 1], uBase[uvIndex],
				vBase[uvIndex]);
			rgbBase[j + 2] = YUV444TORGBA8888(yBase[j + 2], uBase[uvIndex],
				vBase[uvIndex]);
			rgbBase[j + 3] = YUV444TORGBA8888(yBase[j + 3], uBase[uvIndex],
				vBase[uvIndex]);
			uvIndex++;
		}

		// Advance pointers to next line
		yBase += in->linesize[0];

		if ((i & 3) == 0) {
			// These are the same for 4 lines
			uBase += in->linesize[1];
			vBase += in->linesize[2];
		}

		rgbBase += out->linesize[0] / 4;
	}
}


void
gfx_conv_yuv411p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	gfx_conv_null(in, out, width, height);
}


void
gfx_conv_YCbCr422_RGB32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	uint8 *yBase = (uint8 *)in->data[0];
	uint8 *uBase = (uint8 *)in->data[1];
	uint8 *vBase = (uint8 *)in->data[2];

	uint32 *rgbBase = (uint32 *)out->data[0];

	int uvIndex;

	for (int32 i = 0; i < height; i++) {

		uvIndex = 0;

		for (int32 j = 0; j < width; j += 2) {
			rgbBase[j] = YUV444TORGBA8888(yBase[j], uBase[uvIndex],
				vBase[uvIndex]);
			rgbBase[j + 1] = YUV444TORGBA8888(yBase[j + 1], uBase[uvIndex],
				vBase[uvIndex]);

			uvIndex++;
		}

		yBase += in->linesize[0];
		uBase += in->linesize[1];
		vBase += in->linesize[2];

		rgbBase += out->linesize[0] / 4;
	}

	if (height & 1) {
		// XXX special case for last line if height not multiple of 2 goes here
		memset((height - 1) * out->linesize[0] + (uint8 *)out->data[0], 0,
			width * 4);
	}

}


void
gfx_conv_GBRP_RGB32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	uint8 *bBase = (uint8 *)in->data[0];
	uint8 *gBase = (uint8 *)in->data[1];
	uint8 *rBase = (uint8 *)in->data[2];

	uint32 *rgbBase = (uint32 *)out->data[0];

	for (int32 i = 0; i < height; i++) {

		for (int32 j = 0; j < width; j ++) {
			rgbBase[j] = gBase[j] | (bBase[j] << 8) | (rBase[j] << 16);
		}

		bBase += in->linesize[0];
		gBase += in->linesize[1];
		rBase += in->linesize[2];

		rgbBase += out->linesize[0] / 4;
	}
}
