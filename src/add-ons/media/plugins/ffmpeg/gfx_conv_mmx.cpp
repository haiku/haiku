#include "gfx_conv_mmx.h"
#include "gfx_conv_c.h"

extern "C" void _Convert_YUV420P_RGBA32_SSE2(void *fromYPtr, void *fromUPtr, void *fromVPtr, void *toPtr, int width);
extern "C" void _Convert_YUV422_RGBA32_SSE2(void *fromYPtr, void *toPtr, int width);

void gfx_conv_null_mmx(AVFrame *in, AVFrame *out, int width, int height) {
	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
}

void gfx_conv_yuv410p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
//	img_convert((AVPicture *)out,PIX_FMT_YUV422P,(const AVPicture *)in,PIX_FMT_YUV410P,width,height);
}

void gfx_conv_yuv411p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
//	img_convert((AVPicture *)out,PIX_FMT_YUV422P,(const AVPicture *)in,PIX_FMT_YUV411P,width,height);
}

void gfx_conv_yuv420p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
//	img_convert((AVPicture *)out,PIX_FMT_YUV422P,(const AVPicture *)in,PIX_FMT_YUV420P,width,height);
}

void gfx_conv_yuv410p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
//	img_convert((AVPicture *)out,PIX_FMT_RGB32,(const AVPicture *)in,PIX_FMT_YUV410P,width,height);
}

void gfx_conv_yuv411p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
//	img_convert((AVPicture *)out,PIX_FMT_RGB32,(const AVPicture *)in,PIX_FMT_YUV411P,width,height);
}

void gfx_conv_yuv420p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
//	img_convert((AVPicture *)out,PIX_FMT_RGB32,(const AVPicture *)in,PIX_FMT_YUV420P,width,height);
}

// Planar YUV420
void gfx_conv_yuv420p_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height)
{
	// width must be divisibile by 8 and height divisible by 2
	if (width % 8 == 0 && height % 2 == 0) {
	
		uint8 *ybase = (uint8 *)in->data[0];
		uint8 *ubase = (uint8 *)in->data[1];
		uint8 *vbase = (uint8 *)in->data[2];
		uint8 *rgbbase = (uint8 *)out->data[0];
	
		for (int i=0;i<height;i+=2) {
			_Convert_YUV420P_RGBA32_SSE2(ybase, ubase, vbase, rgbbase, width);	// First Y row
			ybase += in->linesize[0];
			rgbbase += out->linesize[0];
		
			_Convert_YUV420P_RGBA32_SSE2(ybase, ubase, vbase, rgbbase, width);	// Second Y row but same u and v row
			ybase += in->linesize[0];
			ubase += in->linesize[1];
			vbase += in->linesize[2];
			rgbbase += out->linesize[0];
		}
	} else {
		gfx_conv_YCbCr420p_RGB32_c(in, out, width, height);
	}
}

// Packed YUV422
void gfx_conv_yuv422p_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height)
{
	// width must be divisibile by 8 
	if (width % 8 == 0) {
		uint8 *ybase = (uint8 *)in->data[0];
		uint8 *rgbbase = (uint8 *)out->data[0];

		for (int i = 0; i <= height; i++) {
			_Convert_YUV422_RGBA32_SSE2(ybase, rgbbase, width);
			ybase += in->linesize[0];
			rgbbase += out->linesize[0];
		}
	} else {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
	}
}
