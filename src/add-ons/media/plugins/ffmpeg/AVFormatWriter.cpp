/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2018, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */

#include "AVFormatWriter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <new>

#include <Application.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <MediaIO.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Roster.h>

extern "C" {
	#include "avformat.h"
}

#include "DemuxerTable.h"
#include "EncoderTable.h"
#include "gfx_util.h"


//#define TRACE_AVFORMAT_WRITER
#ifdef TRACE_AVFORMAT_WRITER
#	define TRACE printf
#	define TRACE_IO(a...)
#	define TRACE_PACKET printf
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#	define TRACE_PACKET(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


static const size_t kIOBufferSize = 64 * 1024;
	// TODO: This could depend on the BMediaFile creation flags, IIRC,
	// they allow to specify a buffering mode.

typedef AVCodecID CodecID;

// #pragma mark - AVFormatWriter::StreamCookie


class AVFormatWriter::StreamCookie {
public:
								StreamCookie(AVFormatContext* context,
									BLocker* streamLock);
	virtual						~StreamCookie();

			status_t			Init(media_format* format,
									const media_codec_info* codecInfo);

			status_t			WriteChunk(const void* chunkBuffer,
									size_t chunkSize,
									media_encode_info* encodeInfo);

			status_t			AddTrackInfo(uint32 code, const void* data,
									size_t size, uint32 flags);

private:
			AVFormatContext*	fFormatContext;
			AVStream*			fStream;
			AVPacket			fPacket;
			// Since different threads may write to the target,
			// we need to protect the file position and I/O by a lock.
			BLocker*			fStreamLock;
};



AVFormatWriter::StreamCookie::StreamCookie(AVFormatContext* context,
		BLocker* streamLock)
	:
	fFormatContext(context),
	fStream(NULL),
	fStreamLock(streamLock)
{
	av_init_packet(&fPacket);
}


AVFormatWriter::StreamCookie::~StreamCookie()
{
}


status_t
AVFormatWriter::StreamCookie::Init(media_format* format,
	const media_codec_info* codecInfo)
{
	TRACE("AVFormatWriter::StreamCookie::Init()\n");

	BAutolock _(fStreamLock);

	fPacket.stream_index = fFormatContext->nb_streams;
	fStream = avformat_new_stream(fFormatContext, NULL);

	if (fStream == NULL) {
		TRACE("  failed to add new stream\n");
		return B_ERROR;
	}

	fStream->id = fPacket.stream_index;

//	TRACE("  fStream->codecpar: %p\n", fStream->codecpar);
	// TODO: This is a hack for now! Use avcodec_find_encoder_by_name()
	// or something similar...
	fStream->codecpar->codec_id = (CodecID)codecInfo->sub_id;
	if (fStream->codecpar->codec_id == AV_CODEC_ID_NONE)
		fStream->codecpar->codec_id = raw_audio_codec_id_for(*format);

	// Setup the stream according to the media format...
	if (format->type == B_MEDIA_RAW_VIDEO) {
		fStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		fStream->time_base.den = (int)format->u.raw_video.field_rate;
		fStream->time_base.num = 1;

		// video size
		fStream->codecpar->width = format->u.raw_video.display.line_width;
		fStream->codecpar->height = format->u.raw_video.display.line_count;
		// pixel aspect ratio
		fStream->sample_aspect_ratio.num
			= format->u.raw_video.pixel_width_aspect;
		fStream->sample_aspect_ratio.den
			= format->u.raw_video.pixel_height_aspect;
		if (fStream->sample_aspect_ratio.num == 0
			|| fStream->sample_aspect_ratio.den == 0) {
			av_reduce(&fStream->sample_aspect_ratio.num,
				&fStream->sample_aspect_ratio.den, fStream->codecpar->width,
				fStream->codecpar->height, 255);
		}

		fStream->codecpar->sample_aspect_ratio = fStream->sample_aspect_ratio;

		// Use the last supported pixel format of the AVCodec, which we hope
		// is the one with the best quality (true for all currently supported
		// encoders).
//		AVCodec* codec = fStream->codecpar->codec;
//		for (int i = 0; codec->pix_fmts[i] != PIX_FMT_NONE; i++)
//			fStream->codecpar->pix_fmt = codec->pix_fmts[i];
		fStream->codecpar->format = AV_PIX_FMT_YUV420P;

	} else if (format->type == B_MEDIA_RAW_AUDIO) {
		fStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;

		// frame rate
		fStream->codecpar->sample_rate = (int)format->u.raw_audio.frame_rate;

		// channels
		fStream->codecpar->channels = format->u.raw_audio.channel_count;

		// set fStream to the audio format we want to use. This is only a hint
		// (each encoder has a different set of accepted formats)
		switch (format->u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				fStream->codecpar->format = AV_SAMPLE_FMT_FLT;
				break;
			case media_raw_audio_format::B_AUDIO_DOUBLE:
				fStream->codecpar->format = AV_SAMPLE_FMT_DBL;
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				fStream->codecpar->format = AV_SAMPLE_FMT_S32;
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				fStream->codecpar->format = AV_SAMPLE_FMT_S16;
				break;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				fStream->codecpar->format = AV_SAMPLE_FMT_U8;
				break;

			case media_raw_audio_format::B_AUDIO_CHAR:
			default:
				return B_MEDIA_BAD_FORMAT;
				break;
		}

		// Now negociate the actual format with the encoder
		// First check if the requested format is acceptable
		AVCodec* codec = avcodec_find_encoder(fStream->codecpar->codec_id);

		if (codec == NULL)
			return B_MEDIA_BAD_FORMAT;

		const enum AVSampleFormat *p = codec->sample_fmts;
		for (; *p != -1; p++) {
			if (*p == fStream->codecpar->format)
				break;
		}
		// If not, force one of the acceptable ones
		if (*p == -1) {
			fStream->codecpar->format = codec->sample_fmts[0];

			// And finally set the format struct to the accepted format. It is
			// then up to the caller to make sure we get data matching that
			// format.
			switch (fStream->codecpar->format) {
				case AV_SAMPLE_FMT_FLT:
					format->u.raw_audio.format
						= media_raw_audio_format::B_AUDIO_FLOAT;
					break;
				case AV_SAMPLE_FMT_DBL:
					format->u.raw_audio.format
						= media_raw_audio_format::B_AUDIO_DOUBLE;
					break;
				case AV_SAMPLE_FMT_S32:
					format->u.raw_audio.format
						= media_raw_audio_format::B_AUDIO_INT;
					break;
				case AV_SAMPLE_FMT_S16:
					format->u.raw_audio.format
						= media_raw_audio_format::B_AUDIO_SHORT;
					break;
				case AV_SAMPLE_FMT_U8:
					format->u.raw_audio.format
						= media_raw_audio_format::B_AUDIO_UCHAR;
					break;
				default:
					return B_MEDIA_BAD_FORMAT;
					break;
			}
		}

		if (format->u.raw_audio.channel_mask == 0) {
			// guess the channel mask...
			switch (format->u.raw_audio.channel_count) {
				default:
				case 2:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
					break;
				case 1:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_MONO;
					break;
				case 3:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_SURROUND;
					break;
				case 4:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_QUAD;
					break;
				case 5:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_5POINT0;
					break;
				case 6:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_5POINT1;
					break;
				case 8:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_7POINT1;
					break;
				case 10:
					fStream->codecpar->channel_layout = AV_CH_LAYOUT_7POINT1_WIDE;
					break;
			}
		} else {
			// The bits match 1:1 for media_multi_channels and FFmpeg defines.
			fStream->codecpar->channel_layout = format->u.raw_audio.channel_mask;
		}
	}

	TRACE("  stream->time_base: (%d/%d), codec->time_base: (%d/%d))\n",
		fStream->time_base.num, fStream->time_base.den,
		fStream->codec->time_base.num, fStream->codec->time_base.den);

#if 0
	// Write the AVCodecContext pointer to the user data section of the
	// media_format. For some encoders, it seems to be necessary to use
	// the AVCodecContext of the AVStream in order to successfully encode
	// anything and write valid media files. For example some codecs need
	// to store meta data or global data in the container.
	app_info appInfo;
	if (be_app->GetAppInfo(&appInfo) == B_OK) {
		uchar* userData = format->user_data;
		*(uint32*)userData = 'ffmp';
		userData += sizeof(uint32);
		*(team_id*)userData = appInfo.team;
		userData += sizeof(team_id);
		*(AVCodecContext**)userData = fStream->codec;
	}
#endif

	return B_OK;
}


status_t
AVFormatWriter::StreamCookie::WriteChunk(const void* chunkBuffer,
	size_t chunkSize, media_encode_info* encodeInfo)
{
	TRACE_PACKET("AVFormatWriter::StreamCookie[%d]::WriteChunk(%p, %ld, "
		"start_time: %lld)\n", fStream->index, chunkBuffer, chunkSize,
		encodeInfo->start_time);

	BAutolock _(fStreamLock);

	fPacket.data = const_cast<uint8_t*>((const uint8_t*)chunkBuffer);
	fPacket.size = chunkSize;
	fPacket.stream_index = fStream->index;

	fPacket.pts = int64_t((double)encodeInfo->start_time
		* fStream->time_base.den / (1000000.0 * fStream->time_base.num)
		+ 0.5);

	fPacket.dts = fPacket.pts;

	fPacket.flags = 0;
	if ((encodeInfo->flags & B_MEDIA_KEY_FRAME) != 0)
		fPacket.flags |= AV_PKT_FLAG_KEY;

	TRACE_PACKET("  PTS: %lld (stream->time_base: (%d/%d), "
		"codec->time_base: (%d/%d))\n", fPacket.pts,
		fStream->time_base.num, fStream->time_base.den,
		fStream->codec->time_base.num, fStream->codec->time_base.den);

#if 0
	// TODO: Eventually, we need to write interleaved packets, but
	// maybe we are only supposed to use this if we have actually
	// more than one stream. For the moment, this crashes in AVPacket
	// shuffling inside libavformat. Maybe if we want to use this, we
	// need to allocate a separate AVPacket and copy the chunk buffer.
	int result = av_interleaved_write_frame(fFormatContext, &fPacket);
	if (result < 0)
		TRACE("  av_interleaved_write_frame(): %d\n", result);
#else
	int result = av_write_frame(fFormatContext, &fPacket);
	if (result < 0)
		TRACE("  av_write_frame(): %d\n", result);
#endif

	return result == 0 ? B_OK : B_ERROR;
}


status_t
AVFormatWriter::StreamCookie::AddTrackInfo(uint32 code,
	const void* data, size_t size, uint32 flags)
{
	TRACE("AVFormatWriter::StreamCookie::AddTrackInfo(%lu, %p, %ld, %lu)\n",
		code, data, size, flags);

	BAutolock _(fStreamLock);

	return B_NOT_SUPPORTED;
}


// #pragma mark - AVFormatWriter


AVFormatWriter::AVFormatWriter()
	:
	fFormatContext(avformat_alloc_context()),
	fCodecOpened(false),
	fHeaderError(-1),
	fIOContext(NULL),
	fStreamLock("stream lock")
{
	TRACE("AVFormatWriter::AVFormatWriter\n");
}


AVFormatWriter::~AVFormatWriter()
{
	TRACE("AVFormatWriter::~AVFormatWriter\n");

	// Free the streams and close the AVCodecContexts
	for (unsigned i = 0; i < fFormatContext->nb_streams; i++) {
		av_freep(&fFormatContext->streams[i]->codecpar);
		av_freep(&fFormatContext->streams[i]);
	}

	avformat_free_context(fFormatContext);
	av_free(fIOContext->buffer);
	av_free(fIOContext);
}


// #pragma mark -


status_t
AVFormatWriter::Init(const media_file_format* fileFormat)
{
	TRACE("AVFormatWriter::Init()\n");

	uint8* buffer = static_cast<uint8*>(av_malloc(kIOBufferSize));
	if (buffer == NULL)
		return B_NO_MEMORY;

	// Allocate I/O context and initialize it with buffer
	// and hook functions, pass ourself as cookie.
	fIOContext = avio_alloc_context(buffer, kIOBufferSize, 1, this,
			0, _Write, _Seek);
	if (fIOContext == NULL) {
		TRACE("av_alloc_put_byte() failed!\n");
		return B_ERROR;
	}

	// Setup I/O hooks. This seems to be enough.
	fFormatContext->pb = fIOContext;

	// Set the AVOutputFormat according to fileFormat...
	fFormatContext->oformat = av_guess_format(fileFormat->short_name,
		fileFormat->file_extension, fileFormat->mime_type);
	if (fFormatContext->oformat == NULL) {
		TRACE("  failed to find AVOuputFormat for %s\n",
			fileFormat->short_name);
		return B_NOT_SUPPORTED;
	}

	TRACE("  found AVOuputFormat for %s: %s\n", fileFormat->short_name,
		fFormatContext->oformat->name);

	return B_OK;
}


status_t
AVFormatWriter::SetMetaData(BMetaData* data)
{
	TRACE("AVFormatWriter::SetCopyright(%s)\n", copyright);

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::CommitHeader()
{
	TRACE("AVFormatWriter::CommitHeader\n");

	if (fFormatContext == NULL)
		return B_NO_INIT;

	if (fCodecOpened)
		return B_NOT_ALLOWED;

	// We need to close the codecs we opened, even in case of failure.
	fCodecOpened = true;

	fHeaderError = avformat_write_header(fFormatContext, NULL);
	if (fHeaderError < 0)
		TRACE("  avformat_write_header(): %d\n", fHeaderError);

	#ifdef TRACE_AVFORMAT_WRITER
	TRACE("  wrote header\n");
	for (unsigned i = 0; i < fFormatContext->nb_streams; i++) {
		AVStream* stream = fFormatContext->streams[i];
		TRACE("  stream[%u] time_base: (%d/%d), codec->time_base: (%d/%d)\n",
			i, stream->time_base.num, stream->time_base.den,
			stream->codec->time_base.num, stream->codec->time_base.den);
	}
	#endif // TRACE_AVFORMAT_WRITER

	return fHeaderError == 0 ? B_OK : B_ERROR;
}


status_t
AVFormatWriter::Flush()
{
	TRACE("AVFormatWriter::Flush\n");

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::Close()
{
	TRACE("AVFormatWriter::Close\n");

	if (fFormatContext == NULL)
		return B_NO_INIT;

	if (!fCodecOpened)
		return B_NOT_ALLOWED;

	// From ffmpeg documentation: [av_write_trailer] may only be called
	// after a successful call to avformat_write_header.
	if (fHeaderError != 0)
		return B_ERROR;

	int result = av_write_trailer(fFormatContext);
	if (result < 0)
		TRACE("  av_write_trailer(): %d\n", result);
	return result == 0 ? B_OK : B_ERROR;
}


status_t
AVFormatWriter::AllocateCookie(void** _cookie, media_format* format,
	const media_codec_info* codecInfo)
{
	TRACE("AVFormatWriter::AllocateCookie()\n");

	if (fCodecOpened)
		return B_NOT_ALLOWED;

	BAutolock _(fStreamLock);

	if (_cookie == NULL)
		return B_BAD_VALUE;

	StreamCookie* cookie = new(std::nothrow) StreamCookie(fFormatContext,
		&fStreamLock);

	status_t ret = cookie->Init(format, codecInfo);
	if (ret != B_OK) {
		delete cookie;
		return ret;
	}

	*_cookie = cookie;
	return B_OK;
}


status_t
AVFormatWriter::FreeCookie(void* _cookie)
{
	BAutolock _(fStreamLock);

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	delete cookie;

	return B_OK;
}


// #pragma mark -


status_t
AVFormatWriter::SetMetaData(void* cookie, BMetaData* data)
{
	TRACE("AVFormatWriter::SetMetaData(%p)\n", cookie);

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::AddTrackInfo(void* _cookie, uint32 code,
	const void* data, size_t size, uint32 flags)
{
	TRACE("AVFormatWriter::AddTrackInfo(%lu, %p, %ld, %lu)\n",
		code, data, size, flags);

	if (fHeaderError != 0)
		return B_ERROR;

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	return cookie->AddTrackInfo(code, data, size, flags);
}


status_t
AVFormatWriter::WriteChunk(void* _cookie, const void* chunkBuffer,
	size_t chunkSize, media_encode_info* encodeInfo)
{
	TRACE_PACKET("AVFormatWriter::WriteChunk(%p, %ld, %p)\n", chunkBuffer,
		chunkSize, encodeInfo);

	if (fHeaderError != 0)
		return B_ERROR;

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	return cookie->WriteChunk(chunkBuffer, chunkSize, encodeInfo);
}


// #pragma mark - I/O hooks


/*static*/ int
AVFormatWriter::_Write(void* cookie, uint8* buffer, int bufferSize)
{
	TRACE_IO("AVFormatWriter::_Write(%p, %p, %d)\n",
		cookie, buffer, bufferSize);

	AVFormatWriter* writer = reinterpret_cast<AVFormatWriter*>(cookie);

	ssize_t written = writer->Target()->Write(buffer, bufferSize);

	TRACE_IO("  written: %ld\n", written);
	return (int)written;

}


/*static*/ off_t
AVFormatWriter::_Seek(void* cookie, off_t offset, int whence)
{
	TRACE_IO("AVFormatWriter::_Seek(%p, %lld, %d)\n",
		cookie, offset, whence);

	AVFormatWriter* writer = reinterpret_cast<AVFormatWriter*>(cookie);

	BMediaIO* mediaIO = dynamic_cast<BMediaIO*>(writer->Target());
	if (mediaIO == NULL)
		return -1;

	// Support for special file size retrieval API without seeking anywhere:
	if (whence == AVSEEK_SIZE) {
		off_t size;
		if (mediaIO->GetSize(&size) == B_OK)
			return size;

		return -1;
	}

	off_t position = mediaIO->Seek(offset, whence);
	TRACE_IO("  position: %lld\n", position);
	if (position < 0)
		return -1;

	return position;
}


