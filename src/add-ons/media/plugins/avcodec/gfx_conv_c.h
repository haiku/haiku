#ifndef _GFX_CONV_C_H
#define _GFX_CONV_C_H

// BeOS and libavcodec bitmap formats
#include <GraphicsDefs.h>
#include "libavcodec/avcodec.h"

void gfx_conv_null_c(AVFrame *in, AVFrame *out, int width, int height);

void gfx_conv_yuv410p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv411p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv420p_ycbcr422_c(AVFrame *in, AVFrame *out, int width, int height);

void gfx_conv_yuv410p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv411p_rgb32_c(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_YCbCr420p_RGB32_c(AVFrame *in, AVFrame *out, int width, int height);

#endif
