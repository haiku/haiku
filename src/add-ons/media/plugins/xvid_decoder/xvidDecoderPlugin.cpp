#include <stdio.h>
#include <string.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include "xvidDecoderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define OUTPUT_BUFFER_SIZE	(8 * 1024)
#define DECODE_BUFFER_SIZE	(32 * 1024)

inline int translate_colorspace(enum color_space cs) {
	switch (cs) {
	case B_RGB32:    return XVID_CSP_RGB32;
	case B_RGBA32:   return XVID_CSP_RGBA;
	case B_RGB24:    return XVID_CSP_RGB24;
	case B_RGB16:    return XVID_CSP_RGB565;
	case B_RGB15:    return XVID_CSP_RGB555;
	case B_RGBA15:   return XVID_CSP_RGB555;
	case B_YCbCr444: return XVID_CSP_YUY2;
	case B_YUV422:   return XVID_CSP_UYVY; // might be XVID_CSP_YVYU instead
	case B_YUV9:     return XVID_CSP_YV12; // could be wrong
	case B_YUV12:    return XVID_CSP_I420; // could be wrong
	default:
		TRACE("xvidDecoder translate_colorspace failed\n");
		return XVID_CSP_NULL;
	}
}


xvidDecoder::xvidDecoder()
{
	memset(&fXvidDecoderParams, 0, sizeof(fXvidDecoderParams));
	fResidualBytes = 0;
	fResidualBuffer = 0;
	fDecodeBuffer = new uint8 [DECODE_BUFFER_SIZE];
	fFrameSize = 0;
}


xvidDecoder::~xvidDecoder()
{
	if (fXvidDecoderParams.handle) {
		xvid_decore(fXvidDecoderParams.handle,XVID_DEC_DESTROY,NULL,NULL);
	}
	delete [] fDecodeBuffer;
}


status_t
xvidDecoder::Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	int xerr;
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (initdone) {
		TRACE("xvidDecoder::Setup called twice\n");
		return B_ERROR;
	}
	ioDecodedFormat->type = B_MEDIA_RAW_VIDEO;
	ioDecodedFormat->u.raw_video = ioEncodedFormat->u.encoded_video.output;
	fXvidDecoderParams.width = ioDecodedFormat->u.raw_video.display.line_width;
	fXvidDecoderParams.height = ioDecodedFormat->u.raw_video.display.line_count;

	fColorSpace = translate_colorspace(ioDecodedFormat->u.raw_video.display.format);
	if (fColorSpace == XVID_CSP_NULL) {
		TRACE("xvidDecoder: translate_colorspace(...) failed\n");
		return B_ERROR;
	}

	xerr = xvid_decore(NULL, XVID_DEC_CREATE, &fXvidDecoderParams, NULL);
	if (xerr) {
		TRACE("xvidDecoder: xvid_decore(...XVID_DEC_CREATE...) failed\n");
		return B_ERROR;
	}
	
/*	ioDecodedFormat->u.raw_video.field_rate = ?
	ioDecodedFormat->u.raw_video.interlace = 1;
	ioDecodedFormat->u.raw_video.first_active = 0;
	ioDecodedFormat->u.raw_video.last_active = ioDecodedFormat->display.line_count - 1;
	ioDecodedFormat->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	ioDecodedFormat->u.raw_video.pixel_width_aspect = 1;
	ioDecodedFormat->u.raw_video.pixel_height_aspect = 1;
	ioDecodedFormat->u.raw_video.display. ? =*/
	fBytesPerRow = ioDecodedFormat->u.raw_video.display.bytes_per_row;
	fFrameSize = ioDecodedFormat->u.raw_video.display.line_count * fBytesPerRow;
	
	return B_OK;
}


status_t
xvidDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	int xerr;
	static BLocker locker;
	BAutolock lock(locker);
	if (fXvidDecoderParams.handle != 0) {
		xerr = xvid_decore(fXvidDecoderParams.handle,XVID_DEC_DESTROY,NULL,NULL);
		if (xerr) {
			TRACE("xvidDecoder: xvid_decore(...XVID_DEC_CREATE...) failed\n");
			return B_ERROR;
		}
	}
	xerr = xvid_decore(NULL,XVID_DEC_CREATE,&fXvidDecoderParams,NULL);
	if (xerr) {
		TRACE("xvidDecoder: xvid_decore(...XVID_DEC_CREATE...) failed\n");
		return B_ERROR;
	}
	fResidualBytes = 0;
	return B_OK;
}


status_t
xvidDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = OUTPUT_BUFFER_SIZE;
	
	while (out_bytes_needed > 0) {
		if (fResidualBytes) {
			int32 bytes = min_c(fResidualBytes, out_bytes_needed);
			memcpy(out_buffer, fResidualBuffer, bytes);
			fResidualBuffer += bytes;
			fResidualBytes -= bytes;
			out_buffer += bytes;
			out_bytes_needed -= bytes;
			continue;
		}
		
		void *chunkBuffer;
		int32 chunkSize;
		if (B_OK != GetNextChunk(&chunkBuffer, &chunkSize, mediaHeader)) {
			TRACE("xvidDecoder::Decode: GetNextChunk failed\n");
			return B_ERROR;
		}

		XVID_DEC_FRAME xframe;
		xframe.bitstream = chunkBuffer;
		xframe.length = chunkSize;
		xframe.image = out_buffer;
		xframe.stride = fBytesPerRow;
		xframe.colorspace = fColorSpace;
		
		int xerr;
		xerr = xvid_decore(fXvidDecoderParams.handle,XVID_DEC_DECODE,&xframe,NULL);
		if (xerr) {
			TRACE("xvidDecoder::Decode: xvid_decore returned an error\n");
			return B_ERROR;
		}
		
		//printf("xvidDecoder::Decode: decoded %d bytes into %d bytes\n",chunkSize, outsize);
		
		fResidualBuffer = static_cast<uint8 *>(xframe.bitstream);
		fResidualBytes = xframe.length;
	}

	*frameCount = OUTPUT_BUFFER_SIZE / fFrameSize;

	return B_OK;
}


Decoder *
xvidDecoderPlugin::NewDecoder()
{
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (!initdone) {
		int xerr;
		XVID_INIT_PARAM xinit;
		xerr = xvid_init(NULL, 0, &xinit, NULL);
		if (xerr) {
			TRACE("xvidDecoder: xvid_init(...) failed\n");
			return NULL;
		}
		if (xinit.api_version != API_VERSION) {
			TRACE("xvidDecoder: xvid API_VERSION mismatch\n");
			return NULL;
		}
	}
	initdone = true;
	return new xvidDecoder;
}


status_t
xvidDecoderPlugin::RegisterPlugin()
{
	PublishDecoder("videocodec/mpeg4", "mp4", "MPEG 4 video decoder, based on xvid");
	return B_OK;
}


MediaPlugin *instantiate_plugin()
{
	return new xvidDecoderPlugin;
}
