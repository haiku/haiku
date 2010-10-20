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
			c = (long)(y2 & 0x0FF | ((u& 0x0FF00)) | ((y2 & 0x0FF00) << 8)
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
			a = (long)(y1 & 0x0FF | ((u& 0x0FF) << 8) | ((y1 & 0x0FF00) << 8)
				| ((v & 0x0FF) << 24));
			b = (long)(((y1 & 0x0FF0000) >> 16) | ((u& 0x0FF00))
				| ((y1 & 0x0FF000000) >> 8) | ((v & 0x0FF00) << 16));
			c = (long)(y2 & 0x0FF | ((u& 0x0FF0000) >> 8)
				| ((y2 & 0x0FF00) << 8) | ((v & 0x0FF0000) << 8));
			d = (long)(((y2 & 0x0FF0000) >> 16) | ((u& 0x0FF000000) >> 16)
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

void
gfx_conv_yuv410p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	gfx_conv_null(in, out, width, height);
}


void
gfx_conv_yuv411p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	gfx_conv_null(in, out, width, height);
}


#define CLIP(a) if (0xffffff00 & (uint32)a) { if (a < 0) a = 0; else a = 255; }

// http://en.wikipedia.org/wiki/YUV
uint32
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
    
	return (uint32)((r << 16) | (g << 8) | b);
}


void
gfx_conv_YCbCr422_RGB32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	
	uint32 *rgbbase = (uint32 *)out->data[0];

	int uv_index;

	for (uint32 i = 0; i < height; i++) {

		uv_index = 0;

		for (uint32 j=0; j < width; j+=2) {
			rgbbase[j] = YUV444TORGBA8888(ybase[j], ubase[uv_index],
				vbase[uv_index]);
			rgbbase[j + 1] = YUV444TORGBA8888(ybase[j + 1], ubase[uv_index],
				vbase[uv_index]);
    	
    		uv_index++;
		}
		
		ybase += in->linesize[0];
		ubase += in->linesize[1];
		vbase += in->linesize[2];
		
		rgbbase += out->linesize[0] / 4;
	}

	if (height & 1) {
		// XXX special case for last line if height not multiple of 2 goes here
		memset((height - 1) * out->linesize[0] + (uint8 *)out->data[0], 0,
			width * 4);
	}

}
