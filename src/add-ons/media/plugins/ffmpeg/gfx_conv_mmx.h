#ifndef _GFX_CONV_MMX_H
#define _GFX_CONV_MMX_H

// BeOS and libavcodec bitmap formats
#include <GraphicsDefs.h>
#include "libavcodec/avcodec.h"

void gfx_conv_null_mmx(AVFrame *in, AVFrame *out, int width, int height);

// Planar
void gfx_conv_yuv420p_rgba32_sse(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv420p_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv420p_rgba32_ssse3(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv422p_rgba32_sse(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv422p_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv422p_rgba32_ssse3(AVFrame *in, AVFrame *out, int width, int height);

// Packed
void gfx_conv_yuv422_rgba32_sse(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv422_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv422_rgba32_ssse3(AVFrame *in, AVFrame *out, int width, int height);

#endif
