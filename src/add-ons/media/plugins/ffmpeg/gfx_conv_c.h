#ifndef _GFX_CONV_C_H
#define _GFX_CONV_C_H


#include <GraphicsDefs.h>

extern "C" {
	#include "libavcodec/avcodec.h"
}


void gfx_conv_null(AVFrame *in, AVFrame *out, int width, int height);


void gfx_conv_yuv410p_ycbcr422_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_yuv411p_ycbcr422_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_yuv420p_ycbcr422_c(AVFrame *in, AVFrame *out, int width,
	int height);


void gfx_conv_yuv420p10le_rgb32_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_yuv410p_rgb32_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_yuv411p_rgb32_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_YCbCr420p_RGB32_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_YCbCr422_RGB32_c(AVFrame *in, AVFrame *out, int width,
	int height);
void gfx_conv_GBRP_RGB32_c(AVFrame *in, AVFrame *out, int width,
	int height);


#endif // _GFX_CONV_C_H
