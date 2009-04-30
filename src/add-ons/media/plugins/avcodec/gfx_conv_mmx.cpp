extern "C" {
#include "libavcodec/imgconvert.h"
}

void gfx_conv_null_mmx(AVFrame *in, AVFrame *out, int width, int height) {
	memcpy(out->data[0], in->data[0], height * in->linesize[0]);
}

void gfx_conv_yuv410p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
	img_convert((AVPicture *)out,PIX_FMT_YUV422P,(const AVPicture *)in,PIX_FMT_YUV410P,width,height);
}

void gfx_conv_yuv411p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
	img_convert((AVPicture *)out,PIX_FMT_YUV422P,(const AVPicture *)in,PIX_FMT_YUV411P,width,height);
}

void gfx_conv_yuv420p_ycbcr422_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
	img_convert((AVPicture *)out,PIX_FMT_YUV422P,(const AVPicture *)in,PIX_FMT_YUV420P,width,height);
}

void gfx_conv_yuv410p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
	img_convert((AVPicture *)out,PIX_FMT_RGB32,(const AVPicture *)in,PIX_FMT_YUV410P,width,height);
}

void gfx_conv_yuv411p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
	img_convert((AVPicture *)out,PIX_FMT_RGB32,(const AVPicture *)in,PIX_FMT_YUV411P,width,height);
}

void gfx_conv_yuv420p_rgb32_mmx(AVFrame *in, AVFrame *out, int width, int height)
{
	img_convert((AVPicture *)out,PIX_FMT_RGB32,(const AVPicture *)in,PIX_FMT_YUV420P,width,height);
}

