/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */

#include "AVFormatReader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <new>

#include <ByteOrder.h>
#include <DataIO.h>
#include <MediaDefs.h>
#include <MediaFormats.h>

#include "gfx_util.h"

//#include "RawFormats.h"


#define TRACE_AVFORMAT_READER
#ifdef TRACE_AVFORMAT_READER
#	define TRACE printf
#	define TRACE_IO(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


static const size_t kIOBufferSize = 64 * 1024;
	// TODO: This could depend on the BMediaFile creation flags, IIRC,
	// the allow to specify a buffering mode.


AVFormatReader::AVFormatReader()
	:
	fContext(NULL),
	fIOBuffer(NULL)
{
	TRACE("AVFormatReader::AVFormatReader\n");
	memset(&fFormatParameters, 0, sizeof(fFormatParameters));
}


AVFormatReader::~AVFormatReader()
{
	TRACE("AVFormatReader::~AVFormatReader\n");

	av_free(fContext);
	free(fIOBuffer);
}


// #pragma mark -


const char*
AVFormatReader::Copyright()
{
	// TODO: Return copyright of the file instead!
	return "Copyright 2009, Stephan Aßmus";
}

	
status_t
AVFormatReader::Sniff(int32* streamCount)
{
	TRACE("AVFormatReader::Sniff\n");

	free(fIOBuffer);
	fIOBuffer = (uint8*)malloc(kIOBufferSize);

	size_t probeSize = 1024;
	AVProbeData probeData;
	probeData.filename = "";
	probeData.buf = fIOBuffer;
	probeData.buf_size = probeSize;

	// Read a bit of the input...
	if (_ReadPacket(Source(), fIOBuffer, probeSize) != (ssize_t)probeSize)
		return B_IO_ERROR;
	// ...and seek back to the beginning of the file.
	_Seek(Source(), 0, SEEK_SET);

	// Probe the input format
	AVInputFormat* inputFormat = av_probe_input_format(&probeData, 1);

	if (inputFormat == NULL) {
		TRACE("AVFormatReader::Sniff() - av_probe_input_format() failed!\n");
		return B_ERROR;
	}

	TRACE("AVFormatReader::Sniff() - av_probe_input_format(): %s\n",
		inputFormat->name);

	// Init I/O context with buffer and hook functions
	if (init_put_byte(&fIOContext, fIOBuffer, kIOBufferSize, 0, Source(),
		_ReadPacket, 0, _Seek) != 0) {
		TRACE("AVFormatReader::Sniff() - init_put_byte() failed!\n");
		return B_ERROR;
	}

	if (av_open_input_stream(&fContext, &fIOContext, "", inputFormat,
		&fFormatParameters) < 0) {
		TRACE("AVFormatReader::Sniff() - av_open_input_stream() failed!\n");
		return B_ERROR;
	}

	// Retrieve stream information
	if (av_find_stream_info(fContext) < 0) {
		TRACE("AVFormatReader::Sniff() - av_find_stream_info() failed!\n");
		return B_ERROR;
	}

	TRACE("AVFormatReader::Sniff() - av_find_stream_info() success!\n");

	// Dump information about stream onto standard error
	dump_format(fContext, 0, "", 0);

	if (streamCount != NULL)
		*streamCount = fContext->nb_streams;

//	return B_OK;
return B_ERROR; // For now...
}


void
AVFormatReader::GetFileFormatInfo(media_file_format* mff)
{
	TRACE("AVFormatReader::GetFileFormatInfo\n");

	if (fContext == NULL || fContext->iformat == NULL) {
		TRACE("  no context or AVInputFormat!\n");
		return;
	}

	mff->capabilities = media_file_format::B_READABLE
		| media_file_format::B_KNOWS_ENCODED_VIDEO
		| media_file_format::B_KNOWS_ENCODED_AUDIO
		| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "");
		// TODO: Would be nice to be able to provide this, maybe by extending
		// the FFmpeg code itself (all demuxers, see AVInputFormat struct).

	if (fContext->iformat->extensions != NULL)
		strcpy(mff->file_extension, fContext->iformat->extensions);
	else {
		TRACE("  no file extensions for AVInputFormat.\n");
		strcpy(mff->file_extension, "");
	}

	if (fContext->iformat->name != NULL)
		strcpy(mff->short_name,  fContext->iformat->name);
	else {
		TRACE("  no short name for AVInputFormat.\n");
		strcpy(mff->short_name,  "");
	}

	if (fContext->iformat->long_name != NULL)
		strcpy(mff->pretty_name, fContext->iformat->long_name);
	else {
		TRACE("  no long name for AVInputFormat.\n");
		strcpy(mff->pretty_name, "");
	}
}


// #pragma mark -


status_t
AVFormatReader::AllocateCookie(int32 streamIndex, void** _cookie)
{
	TRACE("AVFormatReader::AllocateCookie(%ld)\n", streamIndex);

	if (fContext == NULL)
		return B_NO_INIT;

	if (streamIndex < 0 || streamIndex >= (int32)fContext->nb_streams)
		return B_BAD_INDEX;

	if (_cookie == NULL)
		return B_BAD_VALUE;

	StreamCookie* cookie = new(std::nothrow) StreamCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	// Get a pointer to the codec context for the stream at sreamIndex.
	AVCodecContext* codecContext = fContext->streams[streamIndex]->codec;
	AVCodec* codec = avcodec_find_decoder(codecContext->codec_id);
	if (codec == NULL || avcodec_open(codecContext, codec) < 0) {
		delete cookie;
		return B_ERROR;
	}

//    codecContext->get_buffer = _GetBuffer;
//    codecContext->release_buffer = _ReleaseBuffer;

	AVStream* stream = fContext->streams[streamIndex];

	cookie->stream = stream;
	cookie->codecContext = codecContext;
	cookie->codec = codec;

	media_format* format = &cookie->format;
	memset(format, 0, sizeof(media_format));

	BMediaFormats formats;
	media_format_description description;

	if (stream->codec->codec_type == CODEC_TYPE_VIDEO) {
		// TODO: Fix this up! Maybe do this for AVI demuxer and MOV
		// demuxer and use B_MISC_FORMAT_FAMILY for all the others?
		description.family = B_AVI_FORMAT_FAMILY;
		description.u.avi.codec = codecContext->codec_tag;
		TRACE("  fourcc '%.4s'\n", (char*)&codecContext->codec_tag);

		if (formats.GetFormatFor(description, format) < B_OK)
			format->type = B_MEDIA_ENCODED_VIDEO;

//		format->require_flags = 
//		format->deny_flags = 

		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32*)format->user_data = codecContext->codec_tag;
		format->user_data[4] = 0;

		// TODO: We don't actually know the bitrate for this stream,
		// only the total bitrate!
		format->u.encoded_video.avg_bit_rate = codecContext->bit_rate; 
		format->u.encoded_video.max_bit_rate = codecContext->bit_rate
			+ codecContext->bit_rate_tolerance;

		format->u.encoded_video.encoding = media_encoded_video_format::B_ANY;

		format->u.encoded_video.frame_size = 1;
//		format->u.encoded_video.forward_history = 0;
//		format->u.encoded_video.backward_history = 0;

		format->u.encoded_video.output.field_rate
			= av_q2d(stream->r_frame_rate);
		format->u.encoded_video.output.interlace = 1;
			// TODO: Fix up for interlaced video
		format->u.encoded_video.output.first_active = 0;
		format->u.encoded_video.output.last_active = 0;
			// TODO: Maybe libavformat actually provides that info somewhere...
		format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;

		// TODO: Implement aspect ratio for real
		format->u.encoded_video.output.pixel_width_aspect
			= 1;//stream->sample_aspect_ratio.num;
		format->u.encoded_video.output.pixel_height_aspect
			= 1;//stream->sample_aspect_ratio.den;

		TRACE("  pixel width/height aspect: %d/%d or %.4f\n",
			stream->sample_aspect_ratio.num,
			stream->sample_aspect_ratio.den,
			av_q2d(stream->sample_aspect_ratio));

		format->u.encoded_video.output.display.format
			= pixfmt_to_colorspace(codecContext->pix_fmt);
		format->u.encoded_video.output.display.line_width = codecContext->width;
		format->u.encoded_video.output.display.line_count = codecContext->height;
		format->u.encoded_video.output.display.bytes_per_row = 0;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0; // TODO

		uint32 encoding = format->Encoding();
		TRACE("  encoding '%.4s'\n", (char*)&encoding);

	} else if (stream->codec->codec_type == CODEC_TYPE_AUDIO) {
		format->type = B_MEDIA_ENCODED_AUDIO;
//		format->require_flags = 
//		format->deny_flags = 

//		format->u.encoded_audio.
	} else {
		return B_NOT_SUPPORTED;
	}

	*_cookie = cookie;

	return B_OK;
}


status_t
AVFormatReader::FreeCookie(void *_cookie)
{
	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);

	avcodec_close(cookie->codecContext);
	delete cookie;

	return B_OK;
}


// #pragma mark -


status_t
AVFormatReader::GetStreamInfo(void* _cookie, int64* frameCount,
	bigtime_t* duration, media_format* format, const void** infoBuffer,
	size_t* infoSize)
{
	TRACE("AVFormatReader::GetStreamInfo()\n");

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	AVStream* stream = cookie->stream;

	double frameRate = av_q2d(stream->r_frame_rate);

	TRACE("  frameRate: %.4f\n", frameRate);

	*duration = (bigtime_t)(1000000LL * stream->duration
		* av_q2d(stream->time_base));

	TRACE("  duration: %lld\n", *duration);

	*frameCount = stream->nb_frames;
	if (*frameCount == 0) {
		// TODO: Calculate from duration and frame rate!
		*frameCount = (int64)(*duration * frameRate / 1000000);
	}

	TRACE("  frameCount: %lld\n", *frameCount);

	*format = cookie->format;

	// TODO: Possibly use stream->metadata for this:
	*infoBuffer = 0;
	*infoSize = 0;

	return B_OK;
}


status_t
AVFormatReader::Seek(void* _cookie, uint32 seekTo, int64* frame,
	bigtime_t* time)
{
	return B_ERROR;
}


status_t
AVFormatReader::FindKeyFrame(void* _cookie, uint32 flags, int64* frame,
	bigtime_t* time)
{
	return B_ERROR;
}


status_t
AVFormatReader::GetNextChunk(void* _cookie, const void** chunkBuffer,
	size_t* chunkSize, media_header* mediaHeader)
{
	return B_ERROR;
}


// #pragma mark -


/*static*/ int
AVFormatReader::_ReadPacket(void* cookie, uint8* buffer, int bufferSize)
{
	TRACE_IO("AVFormatReader::_ReadPacket(%p, %p, %d)\n", cookie, buffer,
		bufferSize);

	BDataIO* dataIO = reinterpret_cast<BDataIO*>(cookie);
	ssize_t read = dataIO->Read(buffer, bufferSize);

	TRACE_IO("  read: %ld\n", read);
	return (int)read;
}


/*static*/ off_t
AVFormatReader::_Seek(void* cookie, off_t offset, int whence)
{
	TRACE_IO("AVFormatReader::_Seek(%p, %lld, %d)\n", cookie, offset, whence);

	BDataIO* dataIO = reinterpret_cast<BDataIO*>(cookie);
	BPositionIO* positionIO = dynamic_cast<BPositionIO*>(dataIO);
	if (positionIO == NULL) {
		TRACE("  not a BPositionIO\n");
		return -1;
	}

	// Support for special file size retrieval API without seeking anywhere:
	if (whence == AVSEEK_SIZE) {
		off_t size;
		if (positionIO->GetSize(&size) == B_OK)
			return size;
		return -1;
	}

	off_t position = positionIO->Seek(offset, whence);

	TRACE_IO("  position: %lld\n", position);
	return position;
}

