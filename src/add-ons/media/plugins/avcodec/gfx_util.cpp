#include <strings.h>
#include <stdio.h>
#include "gfx_util.h"
#include "gfx_conv_c.h"
#include "gfx_conv_mmx.h"

/*
 * ref docs
 * http://www.joemaller.com/fcp/fxscript_yuv_color.shtml
 */

#if 1
  #define TRACE(a...) printf(a)
#else
  #define TRACE(a...)
#endif

//#define INCLUDE_MMX 	defined(__INTEL__)
#define INCLUDE_MMX 	0

// this function will try to find the best colorspaces for both the ff-codec and 
// the Media Kit sides.
gfx_convert_func resolve_colorspace(color_space colorSpace, PixelFormat pixelFormat)
{
#if INCLUDE_MMX
	bool mmx = IsMmxCpu();
#endif

	switch (colorSpace)
	{
		case B_RGB32:
			if (pixelFormat == PIX_FMT_YUV410P) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: gfx_conv_yuv410p_rgb32_mmx\n");
					return gfx_conv_yuv410p_rgb32_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: gfx_conv_yuv410p_rgb32_c\n");
					return gfx_conv_yuv410p_rgb32_c;
				}
			}
			
			if (pixelFormat == PIX_FMT_YUV411P) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: gfx_conv_yuv411p_rgb32_mmx\n");
					return gfx_conv_yuv411p_rgb32_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: gfx_conv_yuv411p_rgb32_c\n");
					return gfx_conv_yuv411p_rgb32_c;
				}
			}
					
			if (pixelFormat == PIX_FMT_YUV420P) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: gfx_conv_yuv420p_rgb32_mmx\n");
					return gfx_conv_yuv420p_rgb32_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: gfx_conv_yuv420p_rgb32_c\n");
					return gfx_conv_YCbCr420p_RGB32_c;
				}
			}
			
			TRACE("resolve_colorspace: %s => B_RGB32: NULL\n", pixfmt_to_string(pixelFormat));
			return NULL;

		case B_RGB24_BIG:
			TRACE("resolve_colorspace: %s => B_RGB24_BIG: NULL\n", pixfmt_to_string(pixelFormat));
			return NULL;

		case B_RGB24:
			TRACE("resolve_colorspace: %s => B_RGB24: NULL\n", pixfmt_to_string(pixelFormat));
			return NULL;

		case B_YCbCr422:

			if (pixelFormat == PIX_FMT_YUV410P) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: gfx_conv_yuv410p_ycbcr422_mmx\n");
					return gfx_conv_yuv410p_ycbcr422_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: gfx_conv_yuv410p_ycbcr422_c\n");
					return gfx_conv_yuv410p_ycbcr422_c;
				}
			}

			if (pixelFormat == PIX_FMT_YUV411P) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: gfx_conv_yuv411p_ycbcr422_mmx\n");
					return gfx_conv_yuv411p_ycbcr422_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: gfx_conv_yuv411p_ycbcr422_c\n");
					return gfx_conv_yuv411p_ycbcr422_c;
				}
			}
			
			if (pixelFormat == PIX_FMT_YUV420P) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: gfx_conv_yuv420p_ycbcr422_mmx\n");
					return gfx_conv_yuv420p_ycbcr422_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: gfx_conv_yuv420p_ycbcr422_c\n");
					return gfx_conv_yuv420p_ycbcr422_c;
				}
			}
			
			if (pixelFormat == PIX_FMT_YUV422) {
				#if INCLUDE_MMX
				if (mmx) {
					TRACE("resolve_colorspace: PIX_FMT_YUV422 => B_YCbCr422: gfx_conv_null_mmx\n");
					return gfx_conv_null_mmx;
				} else
				#endif
				{
					TRACE("resolve_colorspace: PIX_FMT_YUV422 => B_YCbCr422: gfx_conv_null_c\n");
					return gfx_conv_null_c;
				}
			}
			
			TRACE("resolve_colorspace: %s => B_YCbCr422: NULL\n", pixfmt_to_string(pixelFormat));
			return NULL;
		
		default:
			TRACE("resolve_colorspace: default: NULL !!!\n");
			return NULL;
	}
}

const char *pixfmt_to_string(int p)
{
	switch(p) {
	case PIX_FMT_YUV420P:
		return "PIX_FMT_YUV420P";
	case PIX_FMT_YUV422:
		return "PIX_FMT_YUV422";
	case PIX_FMT_RGB24:
		return "PIX_FMT_RGB24";
	case PIX_FMT_BGR24:
		return "PIX_FMT_BGR24";
	case PIX_FMT_YUV422P:
		return "PIX_FMT_YUV422P";
	case PIX_FMT_YUV444P:
		return "PIX_FMT_YUV444P";
	case PIX_FMT_RGBA32:
		return "PIX_FMT_RGBA32";
	case PIX_FMT_YUV410P:
		return "PIX_FMT_YUV410P";
	case PIX_FMT_YUV411P:
		return "PIX_FMT_YUV411P";
	case PIX_FMT_RGB565:
		return "PIX_FMT_RGB565";
	case PIX_FMT_RGB555:
		return "PIX_FMT_RGB555";
	default:
		return "(unknown)";
	}
}

#define BEGIN_TAG "\033[31m"
#define END_TAG "\033[0m"

void dump_ffframe(AVFrame *frame, const char *name)
{
	const char *picttypes[] = {"no pict type", "intra", "predicted", "bidir pre", "s(gmc)-vop"};
	printf(BEGIN_TAG"AVFrame(%s) pts:%-10lld cnum:%-5d dnum:%-5d %s%s, ]\n"END_TAG, 
		name, 
		frame->pts, 
		frame->coded_picture_number, 
		frame->display_picture_number, 
//		frame->quality, 
		frame->key_frame?"keyframe, ":"", 
		picttypes[frame->pict_type]);
//	printf(BEGIN_TAG"\t\tlinesize[] = {%ld, %ld, %ld, %ld}\n"END_TAG, frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3]);
}

