#include "gfx_util.h"

#include <strings.h>
#include <stdio.h>

extern "C" {
#include <libavutil/pixdesc.h>
}

#include "CpuCapabilities.h"
#include "gfx_conv_c.h"
#include "gfx_conv_mmx.h"


// ref docs
// http://www.joemaller.com/fcp/fxscript_yuv_color.shtml


#if DEBUG
  #define TRACE(a...) printf(a)
#else
  #define TRACE(a...)
#endif

#if LIBAVCODEC_VERSION_INT < ((54 << 16) | (50 << 8))
#define AVPixelFormat PixelFormat 
#define AV_PIX_FMT_NONE PIX_FMT_NONE
#define AV_PIX_FMT_YUV410P PIX_FMT_YUV410P
#define AV_PIX_FMT_YUV411P PIX_FMT_YUV411P
#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#define AV_PIX_FMT_YUVJ420P PIX_FMT_YUVJ420P
#define AV_PIX_FMT_YUV422P PIX_FMT_YUV422P
#define AV_PIX_FMT_YUVJ422P PIX_FMT_YUVJ422P
#define AV_PIX_FMT_YUYV422 PIX_FMT_YUYV422
#define AV_PIX_FMT_YUV420P10LE PIX_FMT_YUV420P10LE
#define AV_PIX_FMT_YUV444P PIX_FMT_YUV444P
#define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
#define AV_PIX_FMT_BGR24 PIX_FMT_BGR24
#define AV_PIX_FMT_RGB565 PIX_FMT_RGB565
#define AV_PIX_FMT_RGB555 PIX_FMT_RGB555
#define AV_PIX_FMT_GRAY8 PIX_FMT_GRAY8
#define AV_PIX_FMT_MONOBLACK PIX_FMT_MONOBLACK
#define AV_PIX_FMT_PAL8 PIX_FMT_PAL8
#define AV_PIX_FMT_BGR32 PIX_FMT_BGR32
#define AV_PIX_FMT_BGR565 PIX_FMT_BGR565
#define AV_PIX_FMT_BGR555 PIX_FMT_BGR555
#define AV_PIX_FMT_RGB32 PIX_FMT_RGB32
#define AV_PIX_FMT_GBRP PIX_FMT_GBRP
#endif


//! This function will try to find the best colorspaces for both the ff-codec
// and the Media Kit sides.
gfx_convert_func
resolve_colorspace(color_space colorSpace, AVPixelFormat pixelFormat, int width,
	int height)
{
	CPUCapabilities cpu;

	switch (colorSpace) {
		case B_RGB32:
			// Planar Formats
			if (pixelFormat == AV_PIX_FMT_YUV410P) {
				TRACE("resolve_colorspace: gfx_conv_yuv410p_rgb32_c\n");
				return gfx_conv_yuv410p_rgb32_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUV411P) {
				TRACE("resolve_colorspace: gfx_conv_yuv411p_rgb32_c\n");
				return gfx_conv_yuv411p_rgb32_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUV420P
				|| pixelFormat == AV_PIX_FMT_YUVJ420P) {
#ifndef __x86_64__
				if (cpu.HasSSSE3() && width % 8 == 0 && height % 2 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv420p_rgba32_ssse3\n");
					return gfx_conv_yuv420p_rgba32_ssse3;
				} else if (cpu.HasSSE2() && width % 8 == 0 && height % 2 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv420p_rgba32_sse2\n");
					return gfx_conv_yuv420p_rgba32_sse2;
				} else if (cpu.HasSSE1() && width % 4 == 0
					&& height % 2 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv420p_rgba32_sse\n");
					return gfx_conv_yuv420p_rgba32_sse;
				}
#endif
				TRACE("resolve_colorspace: gfx_conv_YCbCr420p_RGB32_c\n");
				return gfx_conv_YCbCr420p_RGB32_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUV422P
				|| pixelFormat == AV_PIX_FMT_YUVJ422P) {
#ifndef __x86_64__
				if (cpu.HasSSSE3() && width % 8 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv422p_RGB32_ssse3\n");
					return gfx_conv_yuv422p_rgba32_ssse3;
				} else if (cpu.HasSSE2() && width % 8 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv422p_RGB32_sse2\n");
					return gfx_conv_yuv422p_rgba32_sse2;
				} else if (cpu.HasSSE1() && width % 4 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv422p_RGB32_sse\n");
					return gfx_conv_yuv422p_rgba32_sse;
				}
#endif
				TRACE("resolve_colorspace: gfx_conv_YCbCr422p_RGB32_c\n");
				return gfx_conv_YCbCr422_RGB32_c;
			}

			if (pixelFormat == AV_PIX_FMT_GBRP) {
				return gfx_conv_GBRP_RGB32_c;
			}

			// Packed Formats
			if (pixelFormat == AV_PIX_FMT_YUYV422) {
#ifndef __x86_64__
				if (cpu.HasSSSE3() && width % 8 == 0) {
					return gfx_conv_yuv422_rgba32_ssse3;
				} else if (cpu.HasSSE2() && width % 8 == 0) {
					return gfx_conv_yuv422_rgba32_sse2;
				} else if (cpu.HasSSE1() && width % 4 == 0
					&& height % 2 == 0) {
					return gfx_conv_yuv422_rgba32_sse;
				}
#endif
				return gfx_conv_YCbCr422_RGB32_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUV420P10LE)
				return gfx_conv_yuv420p10le_rgb32_c;

			TRACE("resolve_colorspace: %s => B_RGB32: NULL\n",
				pixfmt_to_string(pixelFormat));
			return NULL;

		case B_RGB24_BIG:
			TRACE("resolve_colorspace: %s => B_RGB24_BIG: NULL\n",
				pixfmt_to_string(pixelFormat));
			return NULL;

		case B_RGB24:
			TRACE("resolve_colorspace: %s => B_RGB24: NULL\n",
				pixfmt_to_string(pixelFormat));
			return NULL;

		case B_YCbCr422:
			if (pixelFormat == AV_PIX_FMT_YUV410P) {
				TRACE("resolve_colorspace: gfx_conv_yuv410p_ycbcr422_c\n");
				return gfx_conv_yuv410p_ycbcr422_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUV411P) {
				TRACE("resolve_colorspace: gfx_conv_yuv411p_ycbcr422_c\n");
				return gfx_conv_yuv411p_ycbcr422_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUV420P
				|| pixelFormat == AV_PIX_FMT_YUVJ420P) {
				TRACE("resolve_colorspace: gfx_conv_yuv420p_ycbcr422_c\n");
				return gfx_conv_yuv420p_ycbcr422_c;
			}

			if (pixelFormat == AV_PIX_FMT_YUYV422) {
				TRACE("resolve_colorspace: PIX_FMT_YUV422 => B_YCbCr422: "
					"gfx_conv_null\n");
				return gfx_conv_null;
			}

			TRACE("resolve_colorspace: %s => B_YCbCr422: NULL\n",
				pixfmt_to_string(pixelFormat));
			return NULL;

		default:
			TRACE("resolve_colorspace: default: NULL!!!\n");
			return NULL;
	}
}


const char*
pixfmt_to_string(int pixFormat)
{
	const char* name = av_get_pix_fmt_name((enum AVPixelFormat)pixFormat);
	if (name == NULL)
		return "(unknown)";
	return name;
}


color_space
pixfmt_to_colorspace(int pixFormat)
{
	switch (pixFormat) {
		default:
			TRACE("No BE API colorspace definition for pixel format "
				"\"%s\".\n", pixfmt_to_string(pixFormat));
			// Supposed to fall through.
		case AV_PIX_FMT_NONE:
			return B_NO_COLOR_SPACE;

		// NOTE: See pixfmt_to_colorspace() for what these are.
		case AV_PIX_FMT_YUV420P:
			return B_YUV420;
		case AV_PIX_FMT_YUYV422:
			return B_YUV422;
		case AV_PIX_FMT_RGB24:
			return B_RGB24_BIG;
		case AV_PIX_FMT_BGR24:
			return B_RGB24;
		case AV_PIX_FMT_YUV422P:
			return B_YUV422;
		case AV_PIX_FMT_YUV444P:
			return B_YUV444;
		case AV_PIX_FMT_RGB32:
			return B_RGBA32_BIG;
		case AV_PIX_FMT_YUV410P:
			return B_YUV9;
		case AV_PIX_FMT_YUV411P:
			return B_YUV12;
		case AV_PIX_FMT_RGB565:
			return B_RGB16_BIG;
		case AV_PIX_FMT_RGB555:
			return B_RGB15_BIG;
		case AV_PIX_FMT_GRAY8:
			return B_GRAY8;
		case AV_PIX_FMT_MONOBLACK:
			return B_GRAY1;
		case AV_PIX_FMT_PAL8:
			return B_CMAP8;
		case AV_PIX_FMT_BGR32:
			return B_RGB32;
		case AV_PIX_FMT_BGR565:
			return B_RGB16;
		case AV_PIX_FMT_BGR555:
			return B_RGB15;
	}
}


AVPixelFormat
colorspace_to_pixfmt(color_space format)
{
	switch(format) {
		default:
		case B_NO_COLOR_SPACE:
			return AV_PIX_FMT_NONE;

		// NOTE: See pixfmt_to_colorspace() for what these are.
		case B_YUV420:
			return AV_PIX_FMT_YUV420P;
		case B_YUV422:
			return AV_PIX_FMT_YUV422P;
		case B_RGB24_BIG:
			return AV_PIX_FMT_RGB24;
		case B_RGB24:
			return AV_PIX_FMT_BGR24;
		case B_YUV444:
			return AV_PIX_FMT_YUV444P;
		case B_RGBA32_BIG:
		case B_RGB32_BIG:
			return AV_PIX_FMT_BGR32;
		case B_YUV9:
			return AV_PIX_FMT_YUV410P;
		case B_YUV12:
			return AV_PIX_FMT_YUV411P;
		// TODO: YCbCr color spaces! These are not the same as YUV!
		case B_RGB16_BIG:
			return AV_PIX_FMT_RGB565;
		case B_RGB15_BIG:
			return AV_PIX_FMT_RGB555;
		case B_GRAY8:
			return AV_PIX_FMT_GRAY8;
		case B_GRAY1:
			return AV_PIX_FMT_MONOBLACK;
		case B_CMAP8:
			return AV_PIX_FMT_PAL8;
		case B_RGBA32:
		case B_RGB32:
			return AV_PIX_FMT_RGB32;
		case B_RGB16:
			return AV_PIX_FMT_BGR565;
		case B_RGB15:
			return AV_PIX_FMT_BGR555;
	}
}


#define BEGIN_TAG "\033[31m"
#define END_TAG "\033[0m"

void
dump_ffframe_audio(AVFrame* frame, const char* name)
{
	printf(BEGIN_TAG"AVFrame(%s) [ pkt_dts:%-10lld #samples:%-5d %s"
		" ]\n"END_TAG,
		name,
		frame->pkt_dts,
		frame->nb_samples,
		av_get_sample_fmt_name(static_cast<AVSampleFormat>(frame->format)));
}


void
dump_ffframe_video(AVFrame* frame, const char* name)
{
	const char* picttypes[] = {"no pict type", "intra", "predicted",
		"bidir pre", "s(gmc)-vop"};
	printf(BEGIN_TAG"AVFrame(%s) [ pkt_dts:%-10lld cnum:%-5d dnum:%-5d %s%s"
		" ]\n"END_TAG,
		name,
		frame->pkt_dts,
		frame->coded_picture_number,
		frame->display_picture_number,
		frame->key_frame?"keyframe, ":"",
		picttypes[frame->pict_type]);
}
