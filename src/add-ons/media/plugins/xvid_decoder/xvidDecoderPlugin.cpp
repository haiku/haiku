#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include "xvidDecoderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)

// this colorspace must be supported in translate_colorspace
#define DEFAULT_COLORSPACE B_RGB32

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
	fXvidColorSpace = XVID_CSP_NULL;
	fResidualBytes = 0;
	fResidualBuffer = 0;
	fDecodeBuffer = new uint8 [DECODE_BUFFER_SIZE];
	fStartTime = 0;
	fFrameSize = 0;
	fBitRate = 0;
	fOutputBufferSize = 0;
}


xvidDecoder::~xvidDecoder()
{
	if (fXvidDecoderParams.handle) {
		xvid_decore(fXvidDecoderParams.handle,XVID_DEC_DESTROY,NULL,NULL);
	}
	delete [] fDecodeBuffer;
}


status_t
xvidDecoder::Setup(media_format *ioEncodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	int xerr;

	if (ioEncodedFormat->type != B_MEDIA_ENCODED_VIDEO) {
		TRACE("xvidDecoder::Setup not called with encoded video");
		return B_ERROR;
	}

	// save the extractor information for future reference
	fOutput = ioEncodedFormat->u.encoded_video.output;
	fBitRate = ioEncodedFormat->u.encoded_video.avg_bit_rate;
	fFrameSize = ioEncodedFormat->u.encoded_video.frame_size;

	// xvid can not detect the width and height so the extractor better give them to us
	fXvidDecoderParams.width  = fOutput.display.line_width;
	fXvidDecoderParams.height = fOutput.display.line_count;

	xerr = xvid_decore(NULL, XVID_DEC_CREATE, &fXvidDecoderParams, NULL);
	if (xerr) {
		TRACE("xvidDecoder: xvid_decore(...XVID_DEC_CREATE...) failed\n");
		return B_ERROR;
	}
	
	return B_OK;
}

status_t
xvidDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	// BeBook says: The codec will find and return in ioFormat its best matching format
	// => This means, we never return an error, and always change the format values
	//    that we don't support to something more applicable

	ioDecodedFormat->type = B_MEDIA_RAW_VIDEO;
	
	// 1st: the requested colorspace, 2nd: the extractor colorspace, 3rd: default
	fXvidColorSpace = translate_colorspace(ioDecodedFormat->u.raw_video.display.format);
	if (fXvidColorSpace == XVID_CSP_NULL) {
		TRACE("xvidDecoder: translate_colorspace(...) failed: trying extractor colorspace\n");
		fXvidColorSpace = translate_colorspace(fOutput.display.format);
		if (fXvidColorSpace == XVID_CSP_NULL) {
			TRACE("xvidDecoder: translate_colorspace(...) failed: using DEFAULT_COLORSPACE\n");
			fOutput.display.format = DEFAULT_COLORSPACE;
			fXvidColorSpace = translate_colorspace(fOutput.display.format);
			assert(fXvidColorSpace != XVID_CSP_NULL);
		}
	} else {
		fOutput.display.format = ioDecodedFormat->u.raw_video.display.format;
	}
	
	// xvid has nothing more to contribute about the output than what is already specified
	ioDecodedFormat->u.raw_video = fOutput;
	
	// setup rest of the needed variables
	fOutputBufferSize = fOutput.display.line_count * fOutput.display.bytes_per_row;
	
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
	int32	out_bytes_needed = fOutputBufferSize;
	
	mediaHeader->start_time = fStartTime;
	//TRACE("xvidDecoder: Decoding start time %.6f\n", fStartTime / 1000000.0);
	
	while (out_bytes_needed > 0) {
		if (fResidualBytes) {
			int32 bytes = min_c(fResidualBytes, out_bytes_needed);
			memcpy(out_buffer, fResidualBuffer, bytes);
			fResidualBuffer += bytes;
			fResidualBytes -= bytes;
			out_buffer += bytes;
			out_bytes_needed -= bytes;
			
			fStartTime += (bigtime_t) (((1000000LL * (bytes / fFrameSize))
			                             / fOutput.interlace)
			                           / fOutput.field_rate) ;

			//TRACE("xvidDecoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
			continue;
		}
		
		if (B_OK != DecodeNextChunk())
			break;
	}

	*frameCount = (fOutputBufferSize - out_bytes_needed) / fFrameSize;

	// XXX this doesn't guarantee that we always return B_LAST_BUFFER_ERROR bofore returning B_ERROR
	return (out_bytes_needed == 0) ? B_OK : (out_bytes_needed == fOutputBufferSize) ? B_ERROR : B_LAST_BUFFER_ERROR;
}

status_t
xvidDecoder::DecodeNextChunk()
{
	void *chunkBuffer;
	int32 chunkSize;
	media_header mh;
	if (B_OK != GetNextChunk(&chunkBuffer, &chunkSize, &mh)) {
		TRACE("xvidDecoder::Decode: GetNextChunk failed\n");
		return B_ERROR;
	}
	
	fStartTime = mh.start_time;

	//TRACE("xvidDecoder: fStartTime reset to %.6f\n", fStartTime / 1000000.0);

	XVID_DEC_FRAME xframe;
	xframe.bitstream = chunkBuffer;
	xframe.length = chunkSize;
	xframe.image = fDecodeBuffer;
	xframe.stride = fOutput.display.bytes_per_row;
	xframe.colorspace = fXvidColorSpace;
	
	int xerr;
	xerr = xvid_decore(fXvidDecoderParams.handle,XVID_DEC_DECODE,&xframe,NULL);
	if (xerr) {
		TRACE("xvidDecoder::Decode: xvid_decore returned an error\n");
		return B_ERROR;
	}
	
	//printf("xvidDecoder::Decode: decoded %d bytes into %d bytes\n",chunkSize, outsize);
	
	fResidualBuffer = static_cast<uint8 *>(xframe.bitstream);
	fResidualBytes = xframe.length;

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
		initdone = true;
	}
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
