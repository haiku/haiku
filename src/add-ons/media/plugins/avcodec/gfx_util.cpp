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
#define INCLUDE_MMX 	1

// this function will try to find the best colorspaces for both the ff-codec and 
// the Media Kit sides.
gfx_convert_func resolve_colorspace(color_space colorSpace, PixelFormat pixelFormat)
{
#if INCLUDE_MMX
	bool mmx = true;
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
					
			if (pixelFormat == PIX_FMT_YUV420P || pixelFormat == PIX_FMT_YUVJ420P) {
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
			
			if (pixelFormat == PIX_FMT_YUV422P || pixelFormat == PIX_FMT_YUVJ422P) {
				return gfx_conv_YCbCr422_RGB32_c;
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
			
			if (pixelFormat == PIX_FMT_YUV420P || pixelFormat == PIX_FMT_YUVJ420P) {
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
			return gfx_conv_null_c;
		
		default:
			TRACE("resolve_colorspace: default: NULL !!!\n");
			return NULL;
	}
}

const char *pixfmt_to_string(int p)
{
	switch(p) {
    case PIX_FMT_NONE: return "PIX_FMT_NONE";
    case PIX_FMT_YUV420P: return "PIX_FMT_YUV420P";   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case PIX_FMT_YUYV422: return "PIX_FMT_YUYV422";   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    case PIX_FMT_RGB24: return "PIX_FMT_RGB24";     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    case PIX_FMT_BGR24: return "PIX_FMT_BGR24";     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    case PIX_FMT_YUV422P: return "PIX_FMT_YUV422P";   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case PIX_FMT_YUV444P: return "PIX_FMT_YUV444P";   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case PIX_FMT_RGB32: return "PIX_FMT_RGB32";     ///< packed RGB 8:8:8, 32bpp, (msb)8A 8R 8G 8B(lsb), in CPU endianness
    case PIX_FMT_YUV410P: return "PIX_FMT_YUV410P";   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    case PIX_FMT_YUV411P: return "PIX_FMT_YUV411P";   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    case PIX_FMT_RGB565: return "PIX_FMT_RGB565";    ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), in CPU endianness
    case PIX_FMT_RGB555: return "PIX_FMT_RGB555";    ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), in CPU endianness, most significant bit to 0
    case PIX_FMT_GRAY8: return "PIX_FMT_GRAY8";     ///<        Y        ,  8bpp
    case PIX_FMT_MONOWHITE: return "PIX_FMT_MONOWHITE"; ///<        Y        ,  1bpp, 0 is white, 1 is black
    case PIX_FMT_MONOBLACK: return "PIX_FMT_MONOBLACK"; ///<        Y        ,  1bpp, 0 is black, 1 is white
    case PIX_FMT_PAL8: return "PIX_FMT_PAL8";      ///< 8 bit with PIX_FMT_RGB32 palette
    case PIX_FMT_YUVJ420P: return "PIX_FMT_YUVJ420P - YUV420P (Jpeg)";  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG)
    case PIX_FMT_YUVJ422P: return "PIX_FMT_YUVJ422P - YUV422P (Jpeg)";  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG)
    case PIX_FMT_YUVJ444P: return "PIX_FMT_YUVJ444P";  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG)
    case PIX_FMT_XVMC_MPEG2_MC: return "PIX_FMT_XVMC_MPEG2_MC";///< XVideo Motion Acceleration via common packet passing
    case PIX_FMT_XVMC_MPEG2_IDCT: return "PIX_FMT_XVMC_MPEG2_IDCT";
    case PIX_FMT_UYVY422: return "PIX_FMT_UYVY422";   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    case PIX_FMT_UYYVYY411: return "PIX_FMT_UYYVYY411"; ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    case PIX_FMT_BGR32: return "PIX_FMT_BGR32";     ///< packed RGB 8:8:8, 32bpp, (msb)8A 8B 8G 8R(lsb), in CPU endianness
    case PIX_FMT_BGR565: return "PIX_FMT_BGR565";    ///< packed RGB 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), in CPU endianness
    case PIX_FMT_BGR555: return "PIX_FMT_BGR555";    ///< packed RGB 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), in CPU endianness, most significant bit to 1
    case PIX_FMT_BGR8: return "PIX_FMT_BGR8";      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    case PIX_FMT_BGR4: return "PIX_FMT_BGR4";      ///< packed RGB 1:2:1,  4bpp, (msb)1B 2G 1R(lsb)
    case PIX_FMT_BGR4_BYTE: return "PIX_FMT_BGR4_BYTE"; ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    case PIX_FMT_RGB8: return "PIX_FMT_RGB8";      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    case PIX_FMT_RGB4: return "PIX_FMT_RGB4";      ///< packed RGB 1:2:1,  4bpp, (msb)1R 2G 1B(lsb)
    case PIX_FMT_RGB4_BYTE: return "PIX_FMT_RGB4_BYTE"; ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
    case PIX_FMT_NV12: return "PIX_FMT_NV12";      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 for UV
    case PIX_FMT_NV21: return "PIX_FMT_NV21";      ///< as above, but U and V bytes are swapped
    case PIX_FMT_RGB32_1: return "PIX_FMT_RGB32_1";   ///< packed RGB 8:8:8, 32bpp, (msb)8R 8G 8B 8A(lsb), in CPU endianness
    case PIX_FMT_BGR32_1: return "PIX_FMT_BGR32_1";   ///< packed RGB 8:8:8, 32bpp, (msb)8B 8G 8R 8A(lsb), in CPU endianness
    case PIX_FMT_GRAY16BE: return "PIX_FMT_GRAY16BE";  ///<        Y        , 16bpp, big-endian
    case PIX_FMT_GRAY16LE: return "PIX_FMT_GRAY16LE";  ///<        Y        , 16bpp, little-endian
    case PIX_FMT_YUV440P: return "PIX_FMT_YUV440P";   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    case PIX_FMT_YUVJ440P: return "PIX_FMT_YUVJ440P - YUV440P (Jpeg)";  ///< planar YUV 4:4:0 full scale (JPEG)
    case PIX_FMT_YUVA420P: return "PIX_FMT_YUVA420P - YUV420P (Alpha)";  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    case PIX_FMT_VDPAU_H264: return "PIX_FMT_VDPAU_H264";///< H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    case PIX_FMT_VDPAU_MPEG1: return "PIX_FMT_VDPAU_MPEG1";///< MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    case PIX_FMT_VDPAU_MPEG2: return "PIX_FMT_VDPAU_MPEG2";///< MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    case PIX_FMT_VDPAU_WMV3: return "PIX_FMT_VDPAU_WMV3";///< WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    case PIX_FMT_VDPAU_VC1: return "PIX_FMT_VDPAU_VC1"; ///< VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    case PIX_FMT_RGB48BE: return "PIX_FMT_RGB48BE";   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, big-endian
    case PIX_FMT_RGB48LE: return "PIX_FMT_RGB48LE";   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, little-endian
    case PIX_FMT_VAAPI_MOCO: return "PIX_FMT_VAAPI_MOCO"; ///< HW acceleration through VA API at motion compensation entry-point, Picture.data[0] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers
    case PIX_FMT_VAAPI_IDCT: return "PIX_FMT_VAAPI_IDCT"; ///< HW acceleration through VA API at IDCT entry-point, Picture.data[0] contains a vaapi_render_state struct which contains fields extracted from headers
    case PIX_FMT_VAAPI_VLD: return "PIX_FMT_VAAPI_VLD";  ///< HW decoding through VA API, Picture.data[0] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
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

