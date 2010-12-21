#include "gfx_conv_mmx.h"
#include "gfx_conv_c.h"

// Packed
extern "C" void _Convert_YUV422_RGBA32_SSE(void *fromYPtr, void *toPtr,
	int width);
extern "C" void _Convert_YUV422_RGBA32_SSE2(void *fromYPtr, void *toPtr,
	int width);
extern "C" void _Convert_YUV422_RGBA32_SSSE3(void *fromYPtr, void *toPtr,
	int width);

// Planar
extern "C" void _Convert_YUV420P_RGBA32_SSE(void *fromYPtr, void *fromUPtr,
	void *fromVPtr, void *toPtr, int width);
extern "C" void _Convert_YUV420P_RGBA32_SSE2(void *fromYPtr, void *fromUPtr,
	void *fromVPtr, void *toPtr, int width);
extern "C" void _Convert_YUV420P_RGBA32_SSSE3(void *fromYPtr, void *fromUPtr,
	void *fromVPtr, void *toPtr, int width);


// Planar YUV420 means 2 Y lines share a UV line
void
gfx_conv_yuv420p_rgba32_sse(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 16 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 16 != 0) {
		gfx_conv_YCbCr420p_RGB32_c(in, out, width, height);
		return;
	}
	
	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	uint8 *rgbbase = (uint8 *)out->data[0];
	
	int yBaseInc = in->linesize[0];
	int uBaseInc = in->linesize[1];
	int vBaseInc = in->linesize[2];
	int rgbBaseInc = out->linesize[0];
	
	for (int i=0;i<height;i+=2) {
		// First Y row
		_Convert_YUV420P_RGBA32_SSE(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		rgbbase += rgbBaseInc;

		// Second Y row but same u and v row
		_Convert_YUV420P_RGBA32_SSE(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		ubase += uBaseInc;
		vbase += vBaseInc;
		rgbbase += rgbBaseInc;
	}
}

// Planar YUV420 means 2 Y lines share a UV line
void
gfx_conv_yuv420p_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height)
{	
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr420p_RGB32_c(in, out, width, height);
		return;
	}

	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	uint8 *rgbbase = (uint8 *)out->data[0];
	
	int yBaseInc = in->linesize[0];
	int uBaseInc = in->linesize[1];
	int vBaseInc = in->linesize[2];
	int rgbBaseInc = out->linesize[0];
	
	for (int i=0;i<height;i+=2) {
		// First Y row
		_Convert_YUV420P_RGBA32_SSE2(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		rgbbase += rgbBaseInc;

		// Second Y row but same u and v row
		_Convert_YUV420P_RGBA32_SSE2(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		ubase += uBaseInc;
		vbase += vBaseInc;
		rgbbase += rgbBaseInc;
	}
}

// Planar YUV420 means 2 Y lines share a UV line
void
gfx_conv_yuv420p_rgba32_ssse3(AVFrame *in, AVFrame *out, int width, int height)
{	
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr420p_RGB32_c(in, out, width, height);
		return;
	}

	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	uint8 *rgbbase = (uint8 *)out->data[0];
	
	int yBaseInc = in->linesize[0];
	int uBaseInc = in->linesize[1];
	int vBaseInc = in->linesize[2];
	int rgbBaseInc = out->linesize[0];
	
	for (int i=0;i<height;i+=2) {
		// First Y row
		_Convert_YUV420P_RGBA32_SSSE3(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		rgbbase += rgbBaseInc;

		// Second Y row but same u and v row
		_Convert_YUV420P_RGBA32_SSSE3(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		ubase += uBaseInc;
		vbase += vBaseInc;
		rgbbase += rgbBaseInc;
	}
}

// Planar YUV422 means each Y line has it's own UV line
void
gfx_conv_yuv422p_rgba32_sse(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
		return;
	}

	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	uint8 *rgbbase = (uint8 *)out->data[0];
	
	int yBaseInc = in->linesize[0];
	int uBaseInc = in->linesize[1];
	int vBaseInc = in->linesize[2];
	int rgbBaseInc = out->linesize[0];
	
	for (int i=0;i<height;i++) {
		_Convert_YUV420P_RGBA32_SSE(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		ubase += uBaseInc;
		vbase += vBaseInc;
		rgbbase += rgbBaseInc;
	}
}

// Planar YUV422 means each Y line has it's own UV line
void
gfx_conv_yuv422p_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
		return;
	}

	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	uint8 *rgbbase = (uint8 *)out->data[0];
	
	int yBaseInc = in->linesize[0];
	int uBaseInc = in->linesize[1];
	int vBaseInc = in->linesize[2];
	int rgbBaseInc = out->linesize[0];
	
	for (int i=0;i<height;i++) {
		_Convert_YUV420P_RGBA32_SSE2(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		ubase += uBaseInc;
		vbase += vBaseInc;
		rgbbase += rgbBaseInc;
	}
}

// Planar YUV422 means each Y line has it's own UV line
void
gfx_conv_yuv422p_rgba32_ssse3(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
		return;
	}

	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *ubase = (uint8 *)in->data[1];
	uint8 *vbase = (uint8 *)in->data[2];
	uint8 *rgbbase = (uint8 *)out->data[0];
	
	int yBaseInc = in->linesize[0];
	int uBaseInc = in->linesize[1];
	int vBaseInc = in->linesize[2];
	int rgbBaseInc = out->linesize[0];
	
	for (int i=0;i<height;i++) {
		_Convert_YUV420P_RGBA32_SSSE3(ybase, ubase, vbase, rgbbase, width);
		ybase += yBaseInc;
		ubase += uBaseInc;
		vbase += vBaseInc;
		rgbbase += rgbBaseInc;
	}
}

// Packed YUV422 (YUYV)
void
gfx_conv_yuv422_rgba32_sse(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 16 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 16 != 0) {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
		return;
	}
		
	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *rgbbase = (uint8 *)out->data[0];

	for (int i = 0; i <= height; i++) {
		_Convert_YUV422_RGBA32_SSE(ybase, rgbbase, width);
		ybase += in->linesize[0];
		rgbbase += out->linesize[0];
	}
}

// Packed YUV422 (YUYV)
void
gfx_conv_yuv422_rgba32_sse2(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
		return;
	}
		
	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *rgbbase = (uint8 *)out->data[0];

	for (int i = 0; i <= height; i++) {
		_Convert_YUV422_RGBA32_SSE2(ybase, rgbbase, width);
		ybase += in->linesize[0];
		rgbbase += out->linesize[0];
	}
}

// Packed YUV422 (YUYV)
void
gfx_conv_yuv422_rgba32_ssse3(AVFrame *in, AVFrame *out, int width, int height)
{
	// in and out buffers must be aligned to 32 bytes,
	// in should be as ffmpeg allocates it
	if ((off_t)out->data[0] % 32 != 0) {
		gfx_conv_YCbCr422_RGB32_c(in, out, width, height);
		return;
	}
		
	uint8 *ybase = (uint8 *)in->data[0];
	uint8 *rgbbase = (uint8 *)out->data[0];

	for (int i = 0; i <= height; i++) {
		_Convert_YUV422_RGBA32_SSSE3(ybase, rgbbase, width);
		ybase += in->linesize[0];
		rgbbase += out->linesize[0];
	}
}
