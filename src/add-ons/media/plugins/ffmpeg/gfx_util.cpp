#include "gfx_util.h"

#include <strings.h>
#include <stdio.h>

#include "CpuCapabilities.h"
#include "gfx_conv_c.h"
#include "gfx_conv_mmx.h"


// ref docs
// http://www.joemaller.com/fcp/fxscript_yuv_color.shtml


#if 1
  #define TRACE(a...) printf(a)
#else
  #define TRACE(a...)
#endif


//! This function will try to find the best colorspaces for both the ff-codec
// and the Media Kit sides.
gfx_convert_func
resolve_colorspace(color_space colorSpace, PixelFormat pixelFormat, int width,
	int height)
{
	CPUCapabilities cpu;

	switch (colorSpace) {
		case B_RGB32:
			// Planar Formats
			if (pixelFormat == PIX_FMT_YUV410P) {
				TRACE("resolve_colorspace: gfx_conv_yuv410p_rgb32_c\n");
				return gfx_conv_yuv410p_rgb32_c;
			}

			if (pixelFormat == PIX_FMT_YUV411P) {
				TRACE("resolve_colorspace: gfx_conv_yuv411p_rgb32_c\n");
				return gfx_conv_yuv411p_rgb32_c;
			}

			if (pixelFormat == PIX_FMT_YUV420P 
				|| pixelFormat == PIX_FMT_YUVJ420P) {
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
				} else {
					TRACE("resolve_colorspace: gfx_conv_YCbCr420p_RGB32_c\n");
					return gfx_conv_YCbCr420p_RGB32_c;
				}
			}

			if (pixelFormat == PIX_FMT_YUV422P
				|| pixelFormat == PIX_FMT_YUVJ422P) {
				if (cpu.HasSSSE3() && width % 8 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv422p_RGB32_ssse3\n");
					return gfx_conv_yuv422p_rgba32_ssse3;
				} else if (cpu.HasSSE2() && width % 8 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv422p_RGB32_sse2\n");
					return gfx_conv_yuv422p_rgba32_sse2;
				} else if (cpu.HasSSE1() && width % 4 == 0) {
					TRACE("resolve_colorspace: gfx_conv_yuv422p_RGB32_sse\n");
					return gfx_conv_yuv422p_rgba32_sse;
				} else {
					TRACE("resolve_colorspace: gfx_conv_YCbCr422p_RGB32_c\n");
					return gfx_conv_YCbCr422_RGB32_c;
				}
			}
			
			// Packed Formats
			if (pixelFormat == PIX_FMT_YUYV422) {
				if (cpu.HasSSSE3() && width % 8 == 0) {
					return gfx_conv_yuv422_rgba32_ssse3;
				} else if (cpu.HasSSE2() && width % 8 == 0) {
					return gfx_conv_yuv422_rgba32_sse2;
				} else if (cpu.HasSSE1() && width % 4 == 0
					&& height % 2 == 0) {
					return gfx_conv_yuv422_rgba32_sse;
				} else {
					return gfx_conv_YCbCr422_RGB32_c;
				}
			}
			
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
			if (pixelFormat == PIX_FMT_YUV410P) {
				TRACE("resolve_colorspace: gfx_conv_yuv410p_ycbcr422_c\n");
				return gfx_conv_yuv410p_ycbcr422_c;
			}

			if (pixelFormat == PIX_FMT_YUV411P) {
				TRACE("resolve_colorspace: gfx_conv_yuv411p_ycbcr422_c\n");
				return gfx_conv_yuv411p_ycbcr422_c;
			}

			if (pixelFormat == PIX_FMT_YUV420P
				|| pixelFormat == PIX_FMT_YUVJ420P) {
				TRACE("resolve_colorspace: gfx_conv_yuv420p_ycbcr422_c\n");
				return gfx_conv_yuv420p_ycbcr422_c;
			}

			if (pixelFormat == PIX_FMT_YUYV422) {
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
	switch (pixFormat) {
		case PIX_FMT_NONE:
			return "PIX_FMT_NONE";
	
		case PIX_FMT_YUV420P:
			// planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
			return "PIX_FMT_YUV420P";
	
		case PIX_FMT_YUYV422:
			// packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
			return "PIX_FMT_YUYV422";
	
		case PIX_FMT_RGB24:
			// packed RGB 8:8:8, 24bpp, RGBRGB...
			return "PIX_FMT_RGB24";
	
		case PIX_FMT_BGR24:
			// packed RGB 8:8:8, 24bpp, BGRBGR...
			return "PIX_FMT_BGR24";
	
		case PIX_FMT_YUV422P:
			// planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
			return "PIX_FMT_YUV422P";
	
		case PIX_FMT_YUV444P:
			// planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
			return "PIX_FMT_YUV444P";
	
		case PIX_FMT_RGB32:
			// packed RGB 8:8:8, 32bpp, (msb)8A 8R 8G 8B(lsb), in CPU
			// endianness
			return "PIX_FMT_RGB32";
	
		case PIX_FMT_YUV410P:
			// planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
			return "PIX_FMT_YUV410P";
	
		case PIX_FMT_YUV411P:
			// planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
			return "PIX_FMT_YUV411P";
	
		case PIX_FMT_RGB565:
			// packed RGB 5:6:5, 16bpp, (msb)5R 6G 5B(lsb), in CPU endianness
			return "PIX_FMT_RGB565";
	
		case PIX_FMT_RGB555:
			// packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), in CPU
			// endianness, most significant bit to 0
			return "PIX_FMT_RGB555";
	
		case PIX_FMT_GRAY8:
			// Y, 8bpp
			return "PIX_FMT_GRAY8";
	
		case PIX_FMT_MONOWHITE:
			// Y, 1bpp, 0 is white, 1 is black
			return "PIX_FMT_MONOWHITE";
	
		case PIX_FMT_MONOBLACK:
			// Y, 1bpp, 0 is black, 1 is white
			return "PIX_FMT_MONOBLACK";
	
		case PIX_FMT_PAL8:
			// 8 bit with PIX_FMT_RGB32 palette
			return "PIX_FMT_PAL8";
	
		case PIX_FMT_YUVJ420P:
			// planar YUV 4:2:0, 12bpp, full scale (JPEG)
			return "PIX_FMT_YUVJ420P - YUV420P (Jpeg)";
	
		case PIX_FMT_YUVJ422P:
			// planar YUV 4:2:2, 16bpp, full scale (JPEG)
			return "PIX_FMT_YUVJ422P - YUV422P (Jpeg)";
	
		case PIX_FMT_YUVJ444P:
			// planar YUV 4:4:4, 24bpp, full scale (JPEG)
			return "PIX_FMT_YUVJ444P";
	
		case PIX_FMT_XVMC_MPEG2_MC:
			// XVideo Motion Acceleration via common packet passing
			return "PIX_FMT_XVMC_MPEG2_MC";
	
		case PIX_FMT_XVMC_MPEG2_IDCT:
			return "PIX_FMT_XVMC_MPEG2_IDCT";
		case PIX_FMT_UYVY422:
			// packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
			return "PIX_FMT_UYVY422";
	
		case PIX_FMT_UYYVYY411:
			// packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
			return "PIX_FMT_UYYVYY411";
	
		case PIX_FMT_BGR32:
			// packed RGB 8:8:8, 32bpp, (msb)8A 8B 8G 8R(lsb), in CPU
			// endianness
			return "PIX_FMT_BGR32";
	
		case PIX_FMT_BGR565:
			// packed RGB 5:6:5, 16bpp, (msb)5B 6G 5R(lsb), in CPU endianness
			return "PIX_FMT_BGR565";
	
		case PIX_FMT_BGR555:
			// packed RGB 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), in CPU
			// endianness, most significant bit to 1
			return "PIX_FMT_BGR555";
	
		case PIX_FMT_BGR8:
			// packed RGB 3:3:2, 8bpp, (msb)2B 3G 3R(lsb)
			return "PIX_FMT_BGR8";
	
		case PIX_FMT_BGR4:
			// packed RGB 1:2:1, 4bpp, (msb)1B 2G 1R(lsb)
			return "PIX_FMT_BGR4";
	
		case PIX_FMT_BGR4_BYTE:
			// packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
			return "PIX_FMT_BGR4_BYTE";
	
		case PIX_FMT_RGB8:
			// packed RGB 3:3:2, 8bpp, (msb)2R 3G 3B(lsb)
			return "PIX_FMT_RGB8";
	
		case PIX_FMT_RGB4:
			// packed RGB 1:2:1, 4bpp, (msb)1R 2G 1B(lsb)
			return "PIX_FMT_RGB4";
	
		case PIX_FMT_RGB4_BYTE:
			// packed RGB 1:2:1, 8bpp, (msb)1R 2G 1B(lsb)
			return "PIX_FMT_RGB4_BYTE";
	
		case PIX_FMT_NV12:
			// planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 for UV
			return "PIX_FMT_NV12";
	
		case PIX_FMT_NV21:
			// as above, but U and V bytes are swapped
			return "PIX_FMT_NV21";
	
		case PIX_FMT_RGB32_1:
			// packed RGB 8:8:8, 32bpp, (msb)8R 8G 8B 8A(lsb), in CPU
			// endianness
			return "PIX_FMT_RGB32_1";
	
		case PIX_FMT_BGR32_1:
			// packed RGB 8:8:8, 32bpp, (msb)8B 8G 8R 8A(lsb), in CPU
			// endianness
			return "PIX_FMT_BGR32_1";
	
		case PIX_FMT_GRAY16BE:
			// Y, 16bpp, big-endian
			return "PIX_FMT_GRAY16BE";
	
		case PIX_FMT_GRAY16LE:
			// Y, 16bpp, little-endian
			return "PIX_FMT_GRAY16LE";
	
		case PIX_FMT_YUV440P:
			// planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
			return "PIX_FMT_YUV440P";
	
		case PIX_FMT_YUVJ440P:
			// planar YUV 4:4:0 full scale (JPEG)
			return "PIX_FMT_YUVJ440P - YUV440P (Jpeg)";
	
		case PIX_FMT_YUVA420P:
			// planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A
			// samples)
			return "PIX_FMT_YUVA420P - YUV420P (Alpha)";
	
		case PIX_FMT_VDPAU_H264:
			// H.264 HW decoding with VDPAU, data[0] contains a
			// vdpau_render_state struct which contains the bitstream of the
			// slices as well as various fields extracted from headers
			return "PIX_FMT_VDPAU_H264";
	
		case PIX_FMT_VDPAU_MPEG1:
			// MPEG-1 HW decoding with VDPAU, data[0] contains a
			// vdpau_render_state struct which contains the bitstream of the
			// slices as well as various fields extracted from headers
			return "PIX_FMT_VDPAU_MPEG1";
	
		case PIX_FMT_VDPAU_MPEG2:
			// MPEG-2 HW decoding with VDPAU, data[0] contains a
			// vdpau_render_state struct which contains the bitstream of the
			// slices as well as various fields extracted from headers
			return "PIX_FMT_VDPAU_MPEG2";
	
		case PIX_FMT_VDPAU_WMV3:
			// WMV3 HW decoding with VDPAU, data[0] contains a
			// vdpau_render_state struct which contains the bitstream of the
			// slices as well as various fields extracted from headers
			return "PIX_FMT_VDPAU_WMV3";
	
		case PIX_FMT_VDPAU_VC1:
			// VC-1 HW decoding with VDPAU, data[0] contains a
			// vdpau_render_state struct which contains the bitstream of the
			// slices as well as various fields extracted from headers
			return "PIX_FMT_VDPAU_VC1";
	
		case PIX_FMT_RGB48BE:
			// packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, big-endian
			return "PIX_FMT_RGB48BE";
	
		case PIX_FMT_RGB48LE:
			// packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, little-endian
			return "PIX_FMT_RGB48LE";
	
		case PIX_FMT_VAAPI_MOCO:
			// HW acceleration through VA API at motion compensation
			// entry-point, Picture.data[0] contains a vaapi_render_state
			// struct which contains macroblocks as well as various fields
			// extracted from headers
			return "PIX_FMT_VAAPI_MOCO";
	
		case PIX_FMT_VAAPI_IDCT:
			// HW acceleration through VA API at IDCT entry-point,
			// Picture.data[0] contains a vaapi_render_state struct which
			// contains fields extracted from headers
			return "PIX_FMT_VAAPI_IDCT";
	
		case PIX_FMT_VAAPI_VLD:
			// HW decoding through VA API, Picture.data[0] contains a
			// vaapi_render_state struct which contains the bitstream of the
			// slices as well as various fields extracted from headers
			return "PIX_FMT_VAAPI_VLD";
	
		default:
			return "(unknown)";
	}
}


color_space
pixfmt_to_colorspace(int pixFormat)
{
	switch(pixFormat) {
		default:
			TRACE("No BE API colorspace definition for pixel format "
				"\"%s\".\n", pixfmt_to_string(pixFormat));
			// Supposed to fall through.
		case PIX_FMT_NONE:
			return B_NO_COLOR_SPACE;
	
		// NOTE: See pixfmt_to_colorspace() for what these are.
		case PIX_FMT_YUV420P:
			return B_YUV420;
		case PIX_FMT_YUYV422:
			return B_YUV422;
		case PIX_FMT_RGB24:
			return B_RGB24_BIG;
		case PIX_FMT_BGR24:
			return B_RGB24;
		case PIX_FMT_YUV422P:
			return B_YUV422;
		case PIX_FMT_YUV444P:
			return B_YUV444;
		case PIX_FMT_RGB32:
			return B_RGBA32_BIG;
		case PIX_FMT_YUV410P:
			return B_YUV9;
		case PIX_FMT_YUV411P:
			return B_YUV12;
		case PIX_FMT_RGB565:
			return B_RGB16_BIG;
		case PIX_FMT_RGB555:
			return B_RGB15_BIG;
		case PIX_FMT_GRAY8:
			return B_GRAY8;
		case PIX_FMT_MONOBLACK:
			return B_GRAY1;
		case PIX_FMT_PAL8:
			return B_CMAP8;
		case PIX_FMT_BGR32:
			return B_RGB32;
		case PIX_FMT_BGR565:
			return B_RGB16;
		case PIX_FMT_BGR555:
			return B_RGB15;
	}
}


PixelFormat
colorspace_to_pixfmt(color_space format)
{
	switch(format) {
		default:
		case B_NO_COLOR_SPACE:
			return PIX_FMT_NONE;

		// NOTE: See pixfmt_to_colorspace() for what these are.
		case B_YUV420:
			return PIX_FMT_YUV420P;
		case B_YUV422:
			return PIX_FMT_YUV422P;
		case B_RGB24_BIG:
			return PIX_FMT_RGB24;
		case B_RGB24:
			return PIX_FMT_BGR24;
		case B_YUV444:
			return PIX_FMT_YUV444P;
		case B_RGBA32_BIG:
		case B_RGB32_BIG:
			return PIX_FMT_BGR32;
		case B_YUV9:
			return PIX_FMT_YUV410P;
		case B_YUV12:
			return PIX_FMT_YUV411P;
		// TODO: YCbCr color spaces! These are not the same as YUV!
		case B_RGB16_BIG:
			return PIX_FMT_RGB565;
		case B_RGB15_BIG:
			return PIX_FMT_RGB555;
		case B_GRAY8:
			return PIX_FMT_GRAY8;
		case B_GRAY1:
			return PIX_FMT_MONOBLACK;
		case B_CMAP8:
			return PIX_FMT_PAL8;
		case B_RGBA32:
		case B_RGB32:
			return PIX_FMT_RGB32;
		case B_RGB16:
			return PIX_FMT_BGR565;
		case B_RGB15:
			return PIX_FMT_BGR555;
	}
}


#define BEGIN_TAG "\033[31m"
#define END_TAG "\033[0m"

void
dump_ffframe(AVFrame* frame, const char* name)
{
	const char* picttypes[] = {"no pict type", "intra", "predicted",
		"bidir pre", "s(gmc)-vop"};
	printf(BEGIN_TAG"AVFrame(%s) pts:%-10lld cnum:%-5d dnum:%-5d %s%s, "
		" ]\n"END_TAG,
		name,
		frame->pts,
		frame->coded_picture_number,
		frame->display_picture_number,
//		frame->quality,
		frame->key_frame?"keyframe, ":"",
		picttypes[frame->pict_type]);
//	printf(BEGIN_TAG"\t\tlinesize[] = {%ld, %ld, %ld, %ld}\n"END_TAG,
//		frame->linesize[0], frame->linesize[1], frame->linesize[2],
//		frame->linesize[3]);
}

