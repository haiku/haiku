#include <strings.h>
#include <stdio.h>
#include "gfx_conv_c.h"

#define OPTIMIZED 1

void gfx_conv_null_c(AVFrame *in, AVFrame *out, int width, int height)
{
	printf("[%d, %d, %d] -> [%d, %d, %d]\n", in->linesize[0], in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1], out->linesize[2]);
	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
}


#if !OPTIMIZED
// this one is for SVQ1 :^)
// july 2002, mmu_man
void gfx_conv_yuv410p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	int i, j;
	unsigned char *po, *pi, *pi2, *pi3;
//	printf("[%ld, %ld, %ld] -> [%ld, %ld, %ld]\n", in->linesize[0], in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1], out->linesize[2]);
//	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
	po = out->data[0];
	pi = in->data[0];
	pi2 = in->data[1];
	pi3 = in->data[2];
	for(i=0; i<height; i++) {
		for(j=0;j<out->linesize[0];j++)
//			*(((long *)po)+j) = (long)(*(pi+j) + (*(pi2+j) << 8) + (*(pi+j+3) << 16) + (*(pi3+j) << 24));
			*(((long *)po)+j) = (long)(*(pi+2*j) + (*(pi2+(j >> 1)) << 8) + (*(pi+2*j+1) << 16) + (*(pi3+j) << 24));
		po += out->linesize[0];
		pi += in->linesize[0];
		if(i%4 == 1)
			pi2 += in->linesize[1];
		else if (i%4 == 3)
			pi3 += in->linesize[2];
	}
}

#else

void gfx_conv_yuv410p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	int i;
//	bool toggle=false;
	unsigned long *po, *po_eol;
	register unsigned long *p;
	unsigned long *pi;
	unsigned short *pi2, *pi3;
	register unsigned long y1, y2;
	register unsigned short u, v;
	register unsigned long a, b, c, d;
//	printf("[%ld, %ld, %ld] -> [%ld, %ld, %ld]\n", in->linesize[0], in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1], out->linesize[2]);
//	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
	po = (unsigned long *)out->data[0];
	pi = (unsigned long *)in->data[0];
	pi2 = (unsigned short *)in->data[1];
	pi3 = (unsigned short *)in->data[2];
	for(i=0; i<height; i++) {
		for(p=po, po_eol = (unsigned long *)(((char *)po)+out->linesize[0]); p<po_eol;) {
//			*(((long *)po)+j) = (long)(*(pi+j) + (*(pi2+j) << 8) + (*(pi+j+3) << 16) + (*(pi3+j) << 24));
			y1 = *pi++;
			y2 = *pi++;
			u = *pi2;
			v = *pi3;
			a = (long)(y1 & 0x0FF | ((u& 0x0FF) << 8) | ((y1 & 0x0FF00) << 8) | ((v & 0x0FF) << 24));
			b = (long)(((y1 & 0x0FF0000) >> 16) | ((u& 0x0FF) << 8) | ((y1 & 0x0FF000000) >> 8) | ((v & 0x0FF) << 24));
			c = (long)(y2 & 0x0FF | ((u& 0x0FF00)) | ((y2 & 0x0FF00) << 8) | ((v & 0x0FF00) << 16));
			d = (long)(((y2 & 0x0FF0000) >> 16) | ((u& 0x0FF00)) | ((y2 & 0x0FF000000) >> 8) | ((v & 0x0FF00) << 16));
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
		pi2 = (unsigned short *)(in->data[1] + ((i+2)/4) * in->linesize[1]);
		pi3 = (unsigned short *)(in->data[2] + ((i+3)/4) * in->linesize[2]);
	}
}
#endif // OPTIMIZED

// this one is for cyuv
// may 2003, mmu_man
// XXX: FIXME (== yuv410p atm)
void gfx_conv_yuv411p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	gfx_conv_yuv410p_ycbcr422_c(in, out, width, height);
}


/*
 * DOES CRASH
void gfx_conv_yuv420p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	int i, j;
	unsigned char *po, *pi, *pi2, *pi3;
//	printf("[%ld, %ld, %ld] -> [%ld, %ld, %ld]\n", in->linesize[0], in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1], out->linesize[2]);
//	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
	po = out->data[0];
	pi = in->data[0];
	pi2 = in->data[1];
	pi3 = in->data[2];
	for(i=0; i<height; i++) {
		for(j=0;j<out->linesize[0];j++)
//			*(((long *)po)+j) = (long)(*(pi+j) + (*(pi2+j) << 8) + (*(pi+j+3) << 16) + (*(pi3+j) << 24));
			*(((long *)po)+j) = (long)(*(pi+2*j) + (*(pi2+j) << 8) + (*(pi+2*j+1) << 16) + (*(pi3+j) << 24));
		po += out->linesize[0];
		pi += in->linesize[0];
		if(i%2)
			pi2 += in->linesize[1];
		else
			pi3 += in->linesize[2];
	}
}
*/


void gfx_conv_yuv420p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height)
{
	int i;
	unsigned long *po, *po_eol;
	register unsigned long *p;
	unsigned long *pi, *pi2, *pi3;
	register unsigned long y1, y2, u, v;
	register unsigned long a, b, c, d;
//	printf("[%ld, %ld, %ld] -> [%ld, %ld, %ld]\n", in->linesize[0], in->linesize[1], in->linesize[2], out->linesize[0], out->linesize[1], out->linesize[2]);
//	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
	po = (unsigned long *)out->data[0];
	pi = (unsigned long *)in->data[0];
	pi2 = (unsigned long *)in->data[1];
	pi3 = (unsigned long *)in->data[2];
	for(i=0; i<height; i++) {
		for(p=po, po_eol = (unsigned long *)(((char *)po)+out->linesize[0]); p<po_eol;) {
//			*(((long *)po)+j) = (long)(*(pi+j) + (*(pi2+j) << 8) + (*(pi+j+3) << 16) + (*(pi3+j) << 24));
			y1 = *pi++;
			y2 = *pi++;
			u = *pi2++;
			v = *pi3++;
			a = (long)(y1 & 0x0FF | ((u& 0x0FF) << 8) | ((y1 & 0x0FF00) << 8) | ((v & 0x0FF) << 24));
			b = (long)(((y1 & 0x0FF0000) >> 16) | ((u& 0x0FF00)) | ((y1 & 0x0FF000000) >> 8) | ((v & 0x0FF00) << 16));
			c = (long)(y2 & 0x0FF | ((u& 0x0FF0000) >> 8) | ((y2 & 0x0FF00) << 8) | ((v & 0x0FF0000) << 8));
			d = (long)(((y2 & 0x0FF0000) >> 16) | ((u& 0x0FF000000) >> 16) | ((y2 & 0x0FF000000) >> 8) | ((v & 0x0FF000000)));


			*(p++) = a;
			*(p++) = b;
			*(p++) = c;
			*(p++) = d;
		}
		po = (unsigned long *)((char *)po + out->linesize[0]);
		pi = (unsigned long *)(in->data[0] + i * in->linesize[0]);
		pi2 = (unsigned long *)(in->data[1] + ((i+1)/2) * in->linesize[1]);
		pi3 = (unsigned long *)(in->data[2] + ((i)/2) * in->linesize[2]);
	}
}

void gfx_conv_yuv410p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	gfx_conv_null_c(in, out, width, height);
}


void gfx_conv_yuv411p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	gfx_conv_null_c(in, out, width, height);
}


void gfx_conv_yuv420p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height)
{
	uint32 poutInc = 2 * out->linesize[0];
	uint32 *poutEven = (uint32 *)out->data[0];
	uint32 *poutOdd = (uint32 *)(out->linesize[0] + (uint8 *)poutEven);
	
	uint32 pi1Inc = in->linesize[0];
	uint32 pi1Inc2 = 2 * pi1Inc;
	uint32 pi2Inc = in->linesize[1];
	uint32 pi3Inc = in->linesize[2];
	
	uint8 *pi1Base = (uint8 *)in->data[0];
	uint8 *pi2Base = (uint8 *)in->data[1];
	uint8 *pi3Base = (uint8 *)in->data[2];

	uint32 runs = height / 2;
	for (uint32 i = 0; i < runs; i++) {

		uint8 *pi1Even = i * pi1Inc2 + pi1Base;
		uint8 *pi1Odd = pi1Inc + pi1Even;
		uint8 *pi2 = i * pi2Inc + pi2Base;
		uint8 *pi3 = i *pi3Inc + pi3Base;

		for (uint32 j = 0; j < (uint32)width; j+= 2) {

			int32 Cr_R, Cr_G, Cb_G, Cb_B;
			register int32 Y0, Y1, R, G, B;

			B = - 128 + *(pi2++);
			R = - 128 + *(pi3++);
			Cr_R = R * 52298;
			Cr_G = R * -26640;
			Cb_G = B * -12845;
			Cb_B = B * 66493;

			G = *(uint16 *)pi1Even;
			pi1Even += 2;
			Y0 = ((G & 0x000000ff) - 16) * 38142;
			Y1 = (((G & 0x0000ff00) >> 8) - 16) * 38142;
			
			R = (Y0 + Cr_R) >> 15;
			if (R < 0) R = 0; else if (R > 255) R = 255;
			G = (Y0 + Cr_G + Cb_G) >> 15;
			if (G < 0) G = 0; else if (G > 255) G = 255;
			B = (Y0 + Cb_B) >> 15;
			if (B < 0) B = 0; else if (B > 255) B = 255;
			poutEven[j] = (R << 16) | (G << 8) | B;
			
			R = (Y1 + Cr_R) >> 15;
			if (R < 0) R = 0; else if (R > 255) R = 255;
			G = (Y1 + Cr_G + Cb_G) >> 15;
			if (G < 0) G = 0; else if (G > 255) G = 255;
			B = (Y1 + Cb_B) >> 15;
			if (B < 0) B = 0; else if (B > 255) B = 255;
			poutEven[j + 1] = (R << 16) | (G << 8) | B;

			G = *(uint16 *)pi1Odd;
			pi1Odd += 2;
			Y0 = ((G & 0x000000ff) - 16) * 38142;
			Y1 = (((G & 0x0000ff00) >> 8) - 16) * 38142;

			R = (Y0 + Cr_R) >> 15;
			if (R < 0) R = 0; else if (R > 255) R = 255;
			G = (Y0 + Cr_G + Cb_G) >> 15;
			if (G < 0) G = 0; else if (G > 255) G = 255;
			B = (Y0 + Cb_B) >> 15;
			if (B < 0) B = 0; else if (B > 255) B = 255;
			poutOdd[j] = (R << 16) | (G << 8) | B;
			
			R = (Y1 + Cr_R) >> 15;
			if (R < 0) R = 0; else if (R > 255) R = 255;
			G = (Y1 + Cr_G + Cb_G) >> 15;
			if (G < 0) G = 0; else if (G > 255) G = 255;
			B = (Y1 + Cb_B) >> 15;
			if (B < 0) B = 0; else if (B > 255) B = 255;
			poutOdd[j + 1] = (R << 16) | (G << 8) | B;
		}
		poutEven = (uint32 *)(poutInc + (uint8 *)poutEven);
		poutOdd = (uint32 *)(poutInc + (uint8 *)poutOdd);
	}
	if (height & 1) {
		// XXX special case for last line if height not multiple of 2 goes here
		memset((height - 1) * out->linesize[0] + (uint8 *)out->data[0], 0, width * 4);
	}
}
