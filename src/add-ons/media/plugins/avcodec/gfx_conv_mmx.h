#ifndef _GFX_CONV_MMX_H
#define _GFX_CONV_MMX_H

// BeOS and libavcodec bitmap formats
#include <GraphicsDefs.h>
#include "libavcodec/avcodec.h"

bool IsMmxCpu();

void gfx_conv_null_mmx(AVFrame *in, AVFrame *out, int width, int height);

void gfx_conv_yuv410p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv411p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv420p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height);

void gfx_conv_yuv410p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv411p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height);
void gfx_conv_yuv420p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height);

#endif
