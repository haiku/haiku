#include <stdio.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include <vector>
#include "theoraCodecPlugin.h"
#include "OggTheoraFormats.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)


static media_format
theora_decoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	init_theora_media_raw_video_format(&format.u.raw_video);
	return format;
}


/*
 * TheoraDecoder
 */


TheoraDecoder::TheoraDecoder()
{
	TRACE("TheoraDecoder::TheoraDecoder\n");
	theora_info_init(&fInfo);
	theora_comment_init(&fComment);
	fStartTime = 0;
	memset(&fOutput, sizeof(fOutput), 0);
}


TheoraDecoder::~TheoraDecoder()
{
	TRACE("TheoraDecoder::~TheoraDecoder\n");
	theora_info_clear(&fInfo);
	theora_comment_clear(&fComment);
}


void
TheoraDecoder::GetCodecInfo(media_codec_info *info)
{
	strncpy(info->short_name, "theora-libtheora", sizeof(info->short_name));
	strncpy(info->pretty_name, "theora decoder[libtheora], by Andrew Bachmann", sizeof(info->pretty_name));
}


status_t
TheoraDecoder::Setup(media_format *inputFormat,
				  const void *infoBuffer, int32 infoSize)
{
	TRACE("TheoraDecoder::Setup\n");
	if (!format_is_compatible(theora_encoded_media_format(),*inputFormat)) {
		return B_MEDIA_BAD_FORMAT;
	}
	// grab header packets from meta data
	if (inputFormat->MetaDataSize() != sizeof(std::vector<ogg_packet>)) {
		TRACE("TheoraDecoder::Setup not called with ogg_packet<vector> meta data: not theora\n");
		return B_ERROR;
	}
	std::vector<ogg_packet> * packets = (std::vector<ogg_packet> *)inputFormat->MetaData();
	if (packets->size() != 3) {
		TRACE("TheoraDecoder::Setup not called with three ogg_packets: not theora\n");
		return B_ERROR;
	}
	// parse header packet
	if (theora_decode_header(&fInfo,&fComment,&(*packets)[0]) != 0) {
		TRACE("TheoraDecoder::Setup: theora_synthesis_headerin failed: not theora header\n");
		return B_ERROR;
	}
	// parse comment packet
	if (theora_decode_header(&fInfo,&fComment,&(*packets)[1]) != 0) {
		TRACE("theoraDecoder::Setup: theora_synthesis_headerin failed: not theora comment\n");
		return B_ERROR;
	}
	// parse codec setup packet
	if (theora_decode_header(&fInfo,&fComment,&(*packets)[2]) != 0) {
		TRACE("theoraDecoder::Setup: theora_synthesis_headerin failed: not theora codec setup\n");
		return B_ERROR;
	}
	// initialize decoder
	theora_decode_init(&fState,&fInfo);
	// setup default output
	media_format requested_format = theora_decoded_media_format();
	((media_raw_video_format)requested_format.u.raw_video) = inputFormat->u.encoded_video.output;
	return NegotiateOutputFormat(&requested_format);
}


status_t
TheoraDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	TRACE("TheoraDecoder::NegotiateOutputFormat\n");
	// BMediaTrack::DecodedFormat
	// Pass in ioFormat the format that you want (with wildcards
	// as applicable). The codec will find and return in ioFormat 
	// its best matching format.
	//
	// BMediaDecoder::SetOutputFormat
	// sets the format the decoder should output. On return, 
	// the outputFormat is changed to match the actual format
	// that will be output; this can be different if you 
	// specified any wildcards.
	//
	// Be R5 behavior seems to be that we can never fail.  If we
	// don't support the requested format, just return one we do.
	media_format format = theora_decoded_media_format();
	if (fInfo.fps_denominator != 0) {
		format.u.raw_video.field_rate =
			(double)fInfo.fps_numerator / (double)fInfo.fps_denominator;
	}
	format.u.raw_video.first_active = fInfo.offset_y;
	format.u.raw_video.last_active = fInfo.offset_y + fInfo.frame_height;
	format.u.raw_video.pixel_width_aspect = fInfo.aspect_numerator;
	format.u.raw_video.pixel_height_aspect = fInfo.aspect_denominator;
	format.u.raw_video.display.line_width = fInfo.frame_width;
	format.u.raw_video.display.line_count = fInfo.frame_height;
	if (!format_is_compatible(format,*ioDecodedFormat)) {
		*ioDecodedFormat = format;
	}
	ioDecodedFormat->SpecializeTo(&format);
	// setup output variables
	fOutput = ioDecodedFormat->u.raw_video;
	return B_OK;
}


status_t
TheoraDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	TRACE("TheoraDecoder::Seek\n");
	// throw the old samples away!
/*	int samples = theora_synthesis_pcmout(&fDspState,&pcm);
	theora_synthesis_read(&fDspState,samples);
*/
	return B_OK;
}


status_t
TheoraDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
	TRACE("TheoraDecoder::Decode\n");
	debugger("in");
	status_t status = B_OK;

	bool synced = false;

	// get a new packet
	void *chunkBuffer;
	int32 chunkSize;
	media_header mh;
	status = GetNextChunk(&chunkBuffer, &chunkSize, &mh);
	if (status == B_LAST_BUFFER_ERROR) {
		goto done;
	}
	if (status != B_OK) {
		TRACE("TheoraDecoder::Decode: GetNextChunk failed\n");
		return status;
	}
	if (chunkSize != sizeof(ogg_packet)) {
		TRACE("TheoraDecoder::Decode: chunk not ogg_packet-sized\n");
		return B_ERROR;
	}

	// decode the packet
	{
		ogg_packet * packet = static_cast<ogg_packet*>(chunkBuffer);
		// push the packet in and get the decoded yuv output
		theora_decode_packetin(&fState, packet);
		yuv_buffer yuv;
		theora_decode_YUVout(&fState, &yuv);
		// now copy the decoded yuv output to the buffer
		uint8 * out = static_cast<uint8 *>(buffer);
		// the following represents a simple YV12 [YUV12?] -> YUY2 [YCbCr422] transform
		// it possibly (probably) doesn't work when bytes_per_line != line_width
		uint draw_bytes_per_line = yuv.y_width + yuv.uv_width*2;
		uint bytes_per_line = draw_bytes_per_line;
		for (uint line = 0 ; line < fOutput.display.line_count ; line++) {
			char * y = yuv.y;
			char * u = yuv.u;
			char * v = yuv.v;
			for (uint pos = 0 ; pos < draw_bytes_per_line ; pos += 4) {
				out[pos]   = *(y++);
				out[pos+1] = *(u++);
				out[pos+2] = *(y++);
				out[pos+3] = *(v++);
			}
			out += bytes_per_line;
			yuv.y += yuv.y_stride;
			if (line % 2 == 1) {
				yuv.u += yuv.uv_stride;
				yuv.v += yuv.uv_stride;
			}
		}
	}

	if (!synced) {
		if (mh.start_time > 0) {
			mediaHeader->start_time = mh.start_time - (bigtime_t) (1000000LL / fOutput.field_rate);
			synced = true;
		}
	}
done:
	if (!synced) {
		mediaHeader->start_time = fStartTime;
	}
	fStartTime = (bigtime_t) mediaHeader->start_time + (bigtime_t) (1000000LL / fOutput.field_rate);

	*frameCount = 1;

	return status;
}


/*
 * TheoraDecoderPlugin
 */


Decoder *
TheoraDecoderPlugin::NewDecoder(uint index)
{
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (!initdone) {
		initdone = true;
	}
	return new TheoraDecoder;
}

static media_format theora_formats[1];

status_t
TheoraDecoderPlugin::GetSupportedFormats(media_format ** formats, size_t * count)
{
	media_format_description description = theora_description();
	media_format format = theora_encoded_media_format();

	BMediaFormats mediaFormats;
	status_t result = mediaFormats.InitCheck();
	if (result != B_OK) {
		return result;
	}
	result = mediaFormats.MakeFormatFor(&description, 1, &format);
	if (result != B_OK) {
		return result;
	}
	theora_formats[0] = format;

	*formats = theora_formats;
	*count = 1;

	return result;
}


/*
 * instantiate_plugin
 */


MediaPlugin *instantiate_plugin()
{
	return new TheoraDecoderPlugin;
}
