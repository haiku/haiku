/*
 * Copyright (c) 2005, David McPaul based on avi_reader copyright (c) 2004 Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mp4_reader.h"
#include "RawFormats.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include <StopWatch.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_MP4_READER
#ifdef TRACE_MP4_READER
#	define TRACE printf
#else
#	define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

struct mp4_cookie {
	unsigned	stream;
	char *		buffer;
	unsigned	buffer_size;

	int64		frame_count;
	bigtime_t 	duration;
	media_format format;

	bool		audio;

	// audio only:
	uint32		bytes_per_sec_rate;
	uint32		bytes_per_sec_scale;

	// video only:
	uint32		line_count;
	
	// Common
	uint32		frame_pos;
	uint32		chunk_index;
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
	uint32		frame_size;
};

mp4Reader::mp4Reader()
 :	theFileReader(0)
{
	TRACE("mp4Reader::mp4Reader\n");
}


mp4Reader::~mp4Reader()
{
 	delete theFileReader;
}


const char *
mp4Reader::Copyright()
{
	return "mp4_reader & libMP4, " B_UTF8_COPYRIGHT " by David McPaul";
}
	

status_t
mp4Reader::Sniff(int32 *streamCount)
{
	TRACE("mp4Reader::Sniff\n");
	
	BPositionIO *pos_io_source;

	pos_io_source = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!pos_io_source) {
		TRACE("mp4Reader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}
	
	if (!MP4FileReader::IsSupported(pos_io_source)) {
		TRACE("mp4Reader::Sniff: unsupported file type\n");
		return B_ERROR;
	}
	
	TRACE("mp4Reader::Sniff: this stream seems to be supported\n");
	
	theFileReader = new MP4FileReader(pos_io_source);
	if (B_OK != theFileReader->ParseFile()) {
		ERROR("mp4Reader::Sniff: error parsing file\n");
		return B_ERROR;
	}
	
	*streamCount = theFileReader->GetStreamCount();
	return B_OK;
}


void
mp4Reader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities = media_file_format::B_READABLE
		| media_file_format::B_KNOWS_ENCODED_VIDEO
		| media_file_format::B_KNOWS_ENCODED_AUDIO
		| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_QUICKTIME_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "video/quicktime");
	strcpy(mff->file_extension, "mp4");
	strcpy(mff->short_name,  "MP4");
	strcpy(mff->pretty_name, "MPEG-4 (MP4) file format");
}

status_t
mp4Reader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	uint32 codecID = 0;

	size_t size;
	const void *data;

	mp4_cookie *cookie = new mp4_cookie;
	*_cookie = cookie;
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;
	cookie->frame_pos = 0;
	cookie->chunk_index = 1;

	BMediaFormats formats;
	media_format *format = &cookie->format;
	media_format_description description;

	printf("Allocate cookie for stream %ld\n",streamNumber);

	if (theFileReader->IsActive(cookie->stream) == false) {
		ERROR("mp4Reader::AllocateCookie: stream %d is not active\n", cookie->stream);
		delete cookie;
		return B_ERROR;
	}
	
	const mp4_stream_header *stream_header;
	stream_header = theFileReader->StreamFormat(cookie->stream);
	if (!stream_header) {
		ERROR("mp4Reader::AllocateCookie: stream %d has no header\n", cookie->stream);
		delete cookie;
		return B_ERROR;
	}
	
	TRACE("mp4Reader::AllocateCookie: stream %ld (%s)\n", streamNumber, theFileReader->IsAudio(cookie->stream) ? "audio" : theFileReader->IsVideo(cookie->stream)  ? "video" : "unknown");

	if (theFileReader->IsAudio(cookie->stream)) {
		const AudioMetaData *audio_format = theFileReader->AudioFormat(cookie->stream);
		if (!audio_format) {
			ERROR("mp4Reader::AllocateCookie: audio stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}

		// frame_count is actually sample_count for audio - makes media_player happy but david sad
		cookie->frame_count = theFileReader->GetFrameCount(cookie->stream) * audio_format->FrameSize;
		cookie->duration = theFileReader->GetAudioDuration(cookie->stream);
		cookie->frame_size = audio_format->FrameSize;

		cookie->audio = true;

		if (stream_header->scale && stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate *
				audio_format->SampleSize * audio_format->NoOfChannels / 8;
			cookie->bytes_per_sec_scale = stream_header->scale;
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = stream_header->scale;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using both)\n",
				cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
			TRACE("samples_per_sec_rate %ld, samples_per_sec_scale %ld (using both)\n",
				cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else if (stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate * audio_format->SampleSize
				* audio_format->NoOfChannels / 8;
			cookie->bytes_per_sec_scale = 1;
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using rate)\n",
				cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
			TRACE("samples_per_sec_rate %ld, samples_per_sec_scale %ld (using rate)\n",
				cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else {
			cookie->bytes_per_sec_rate = 128000;
			cookie->bytes_per_sec_scale = 8;
			cookie->frames_per_sec_rate = 16000;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using fallback)\n",
				cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		}

		if (audio_format->compression == AUDIO_NONE
			|| audio_format->compression == AUDIO_RAW
			|| audio_format->compression == AUDIO_TWOS1
			|| audio_format->compression == AUDIO_TWOS2) {
			
			codecID = B_BENDIAN_TO_HOST_INT32(audio_format->compression);
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (B_OK != formats.GetFormatFor(description, format))
				format->type = B_MEDIA_RAW_AUDIO;

			format->u.raw_audio.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
			format->u.raw_audio.channel_count = audio_format->NoOfChannels;

			format->u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;

			if (audio_format->SampleSize <= 8)
				format->u.raw_audio.format = B_AUDIO_FORMAT_UINT8;
			else if (audio_format->SampleSize <= 16)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
			else if (audio_format->SampleSize <= 24)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT24;
			else if (audio_format->SampleSize <= 32)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT32;
			else {
				ERROR("mp4Reader::AllocateCookie: unhandled bits per sample %d\n", audio_format->SampleSize);
				return B_ERROR;
			}

			if (audio_format->compression == AUDIO_TWOS1) {
				if (audio_format->SampleSize <= 8) {
					format->u.raw_audio.format = B_AUDIO_FORMAT_INT8;
				} else if (audio_format->SampleSize <= 16) {
					format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
					format->u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;
				}
			}
			if (audio_format->compression == AUDIO_TWOS2) {
				if (audio_format->SampleSize <= 8) {
					format->u.raw_audio.format = B_AUDIO_FORMAT_INT8;
				} else if (audio_format->SampleSize <= 16) {
					format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
					format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
				}
			}

			format->u.raw_audio.buffer_size = stream_header->suggested_buffer_size;
		} else {
			TRACE("codecid %4s codecsubtype %4s\n", &audio_format->compression, &audio_format->codecSubType);
		
			description.family = B_QUICKTIME_FORMAT_FAMILY;
			
			if (audio_format->codecSubType != 0) {
				codecID = audio_format->codecSubType;
			} else {
				codecID = audio_format->compression;
			}
			description.u.quicktime.codec = codecID;
			if (B_OK != formats.GetFormatFor(description, format)) {
				format->type = B_MEDIA_ENCODED_AUDIO;
			}
			
			switch (description.u.quicktime.codec) {
				case AUDIO_MS_PCM02:
					TRACE("MS PCM02\n");
					format->u.raw_audio.format |= B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE;
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					break;
				case AUDIO_INTEL_PCM17:
					TRACE("INTEL PCM\n");
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					break;
				case AUDIO_MPEG3_CBR:
				case '.mp3':
					TRACE("MP3\n");
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					format->u.encoded_audio.output.buffer_size = audio_format->BufferSize;
					
					format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
					format->u.encoded_audio.output.byte_order =	B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;

					TRACE("Audio NoOfChannels %d, SampleSize %d, SampleRate %f, FrameSize %ld\n",audio_format->NoOfChannels, audio_format->SampleSize, audio_format->SampleRate, audio_format->FrameSize);

					TRACE("Audio frame_rate %f, channel_count %ld, format %ld, buffer_size %ld, frame_size %ld, bit_rate %f\n",
						format->u.encoded_audio.output.frame_rate, format->u.encoded_audio.output.channel_count, format->u.encoded_audio.output.format,format->u.encoded_audio.output.buffer_size, format->u.encoded_audio.frame_size, format->u.encoded_audio.bit_rate);

					TRACE("Track %d MP4 Audio FrameCount %ld Duration %Ld\n",cookie->stream,theFileReader->GetFrameCount(cookie->stream),cookie->duration);
					break;
				case 'mp4a':
					codecID = 'aac ';
				case 'aac ':
				case 'alac':
					TRACE("AAC audio or ALAC audio\n");
		
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
					format->u.encoded_audio.output.byte_order =	B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;

					// AAC is 1024 samples per frame
					// HE-AAC is 2048 samples per frame
					// ALAC is 4096 samples per frame

					format->u.encoded_audio.frame_size = audio_format->FrameSize;
					format->u.encoded_audio.output.buffer_size = audio_format->BufferSize;
			
					// Average BitRate = (TotalBytes * 8 * (SampleRate / FrameSize)) / TotalFrames
					// Setting a bitrate seems to cause more problems than it solves
					
					format->u.encoded_audio.bit_rate = audio_format->NoOfChannels * 64000;	// Should be 64000 * channelcount

					TRACE("Audio NoOfChannels %d, SampleSize %d, SampleRate %f, FrameSize %ld\n",audio_format->NoOfChannels, audio_format->SampleSize, audio_format->SampleRate, audio_format->FrameSize);

					TRACE("Audio frame_rate %f, channel_count %ld, format %ld, buffer_size %ld, frame_size %ld, bit_rate %f\n",
						format->u.encoded_audio.output.frame_rate, format->u.encoded_audio.output.channel_count, format->u.encoded_audio.output.format,format->u.encoded_audio.output.buffer_size, format->u.encoded_audio.frame_size, format->u.encoded_audio.bit_rate);

					TRACE("Track %d MP4 Audio FrameCount %ld Duration %Ld\n",cookie->stream,theFileReader->GetFrameCount(cookie->stream),cookie->duration);
			
					break;
				default:
					char cc1,cc2,cc3,cc4;

					cc1 = (char)((audio_format->compression >> 24) & 0xff);
					cc2 = (char)((audio_format->compression >> 16) & 0xff);
					cc3 = (char)((audio_format->compression >> 8) & 0xff);
					cc4 = (char)((audio_format->compression >> 0) & 0xff);

					TRACE("compression %c%c%c%c\n", cc1,cc2,cc3,cc4);
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					format->u.encoded_audio.output.buffer_size = audio_format->BufferSize;
					break;
			}
		}

		// Set the DecoderConfig
		size = audio_format->DecoderConfigSize;
		data = audio_format->theDecoderConfig;
		if (size > 0) {
			TRACE("Audio Decoder Config Found Size is %ld\n",size);
			if (format->SetMetaData(data, size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				delete cookie;
				return B_ERROR;
			}
		}

#ifdef TRACE_MP4_READER
		if (data) {
			uint8 *p = (uint8 *)data;
				TRACE("extra_data: %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					size, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
		}
#endif
	
		if (codecID != 0) {
			// Put the codeid in the user data in case someone wants it
			format->user_data_type = B_CODEC_TYPE_INFO;
			*(uint32 *)format->user_data = codecID; format->user_data[4] = 0;
		}

		return B_OK;
	}

	if (theFileReader->IsVideo(cookie->stream)) {
		const VideoMetaData *video_format = theFileReader->VideoFormat(cookie->stream);
		if (!video_format) {
			ERROR("mp4Reader::AllocateCookie: video stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		codecID = B_BENDIAN_TO_HOST_INT32(video_format->compression);

		cookie->audio = false;
		cookie->line_count = theFileReader->MovMainHeader()->height;
		cookie->frame_size = 1;
		
		if (stream_header->scale && stream_header->rate) {
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = stream_header->scale;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using both)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else if (theFileReader->MovMainHeader()->micro_sec_per_frame) {
			cookie->frames_per_sec_rate = 1000000;
			cookie->frames_per_sec_scale = theFileReader->MovMainHeader()->micro_sec_per_frame;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using micro_sec_per_frame)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else {
			cookie->frames_per_sec_rate = 25;
			cookie->frames_per_sec_scale = 1;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using fallback)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		}

		cookie->frame_count = stream_header->length;
		cookie->duration = (cookie->frame_count * (int64)cookie->frames_per_sec_scale * 1000000LL) / cookie->frames_per_sec_rate;
		
		TRACE("frame_count %Ld\n", cookie->frame_count);
		TRACE("duration %.6f (%Ld)\n", cookie->duration / 1E6, cookie->duration);

		char cc1,cc2,cc3,cc4;

		cc1 = (char)((video_format->compression >> 24) & 0xff);
		cc2 = (char)((video_format->compression >> 16) & 0xff);
		cc3 = (char)((video_format->compression >> 8) & 0xff);
		cc4 = (char)((video_format->compression >> 0) & 0xff);

		TRACE("compression %c%c%c%c\n", cc1,cc2,cc3,cc4);

		description.family = B_QUICKTIME_FORMAT_FAMILY;
		if (stream_header->fourcc_handler == 'ekaf' || stream_header->fourcc_handler == 0) // 'fake' or 0 fourcc => used compression id
			description.u.quicktime.codec = video_format->compression;
		else
			description.u.quicktime.codec = video_format->compression;
		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_VIDEO;
			
//		format->u.encoded_video.max_bit_rate = 8 * theFileReader->MovMainHeader()->max_bytes_per_sec;
//		format->u.encoded_video.avg_bit_rate = format->u.encoded_video.max_bit_rate / 2; // XXX fix this
		format->u.encoded_video.output.field_rate = cookie->frames_per_sec_rate / (float)cookie->frames_per_sec_scale;

		format->u.encoded_video.avg_bit_rate = 1;
		format->u.encoded_video.max_bit_rate = 1;

		format->u.encoded_video.frame_size = video_format->width * video_format->height * video_format->planes / 8;
		format->u.encoded_video.output.display.bytes_per_row = video_format->planes / 8 * video_format->width;
		// align to 2 bytes
		format->u.encoded_video.output.display.bytes_per_row +=	format->u.encoded_video.output.display.bytes_per_row & 1;

		switch (video_format->planes) {
			case 16:
				format->u.encoded_video.output.display.format = B_RGB15_BIG;
				break;
			case 24:
				format->u.encoded_video.output.display.format = B_RGB24_BIG;
				break;
			case 32:
				format->u.encoded_video.output.display.format = B_RGB32_BIG;
				break;
			default:
				format->u.encoded_video.output.display.format = B_NO_COLOR_SPACE;
				format->u.encoded_video.frame_size = video_format->width * video_format->height * 8 / 8;
		}

		format->u.encoded_video.output.display.line_width = video_format->width;
		format->u.encoded_video.output.display.line_count = video_format->height;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0;
		format->u.encoded_video.output.interlace = 1; // 1: progressive
		format->u.encoded_video.output.first_active = 0;
		format->u.encoded_video.output.last_active = format->u.encoded_video.output.display.line_count - 1;
		format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format->u.encoded_video.output.pixel_width_aspect = 1;
		format->u.encoded_video.output.pixel_height_aspect = 1;
		
		TRACE("max_bit_rate %.3f\n", format->u.encoded_video.max_bit_rate);
		TRACE("field_rate   %.3f\n", format->u.encoded_video.output.field_rate);

		// Set the Decoder Config
		size = video_format->DecoderConfigSize;
		data = video_format->theDecoderConfig;
		if (size > 0) {
			TRACE("Video Decoder Config Found Size is %ld\n",size);
			if (format->SetMetaData(data, size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				delete cookie;
				return B_ERROR;
			}

#ifdef TRACE_MP4_READER
			if (data) {
				uint8 *p = (uint8 *)data;
				TRACE("extra_data: %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					size, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
			}
#endif
		}

		if (codecID != 0) {
			// Put the codeid in the user data in case someone wants it
			format->user_data_type = B_CODEC_TYPE_INFO;
			*(uint32 *)format->user_data = codecID; format->user_data[4] = 0;
		}

		return B_OK;
	}

	delete cookie;
	return B_ERROR;
}


status_t
mp4Reader::FreeCookie(void *_cookie)
{
	mp4_cookie *cookie = (mp4_cookie *)_cookie;

	delete [] cookie->buffer;

	delete cookie;
	return B_OK;
}


status_t
mp4Reader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
	media_format *format, const void **infoBuffer, size_t *infoSize)
{
	mp4_cookie *cookie = (mp4_cookie *)_cookie;

	if (cookie) {
		*frameCount = cookie->frame_count;
		*duration = cookie->duration;
		*format = cookie->format;

		// Copy metadata to infoBuffer
		if (theFileReader->IsVideo(cookie->stream)) {
			const VideoMetaData *video_format = theFileReader->VideoFormat(cookie->stream);
			*infoBuffer = video_format->theDecoderConfig;
			*infoSize = video_format->DecoderConfigSize;
		} else if (theFileReader->IsAudio(cookie->stream)) {
			const AudioMetaData *audio_format = theFileReader->AudioFormat(cookie->stream);
			*infoBuffer = audio_format->theDecoderConfig;
			*infoSize = audio_format->DecoderConfigSize;
		} else {
			ERROR("No stream Info for stream %d\n",cookie->stream);
		}
		
		TRACE("GetStreamInfo (%d) fc %Ld duration %Ld extra %ld\n", cookie->stream, *frameCount, *duration, *infoSize);
	}
	return B_OK;
}


status_t
mp4Reader::Seek(void *cookie, uint32 flags, int64 *frame, bigtime_t *time)
{
	// seek to the requested time or frame
	mp4_cookie *mp4cookie = (mp4_cookie *)cookie;

	if (flags & B_MEDIA_SEEK_TO_TIME) {
		mp4cookie->frame_pos = theFileReader->GetFrameForTime(mp4cookie->stream, *time);
		*frame = theFileReader->GetSampleForTime(mp4cookie->stream, *time);
	}
	
	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		// Convert Sample to Frame
		mp4cookie->frame_pos = theFileReader->GetFrameForSample(mp4cookie->stream, *frame);
		*time = theFileReader->GetTimeForSample(mp4cookie->stream, *frame);
	}

	TRACE("mp4Reader::Seek: stream %d %s%s%s%s, time %Ld, frame %Ld\n", mp4cookie->stream,
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	return B_OK;
}

status_t
mp4Reader::FindKeyFrame(void* cookie, uint32 flags,
							int64* frame, bigtime_t* time)
{
	// Find the nearest keyframe to the given time or frame.
	// frame is really sample for audio.

	mp4_cookie *mp4cookie = (mp4_cookie *)cookie;
	bool keyframe = false;

	TRACE("mp4Reader::FindKeyFrame: input(stream %d %s%s%s%s, time %Ld, frame %Ld)\n", mp4cookie->stream,
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		if (mp4cookie->audio) {
			// Convert Sample to Frame for Audio
			*frame = theFileReader->GetFrameForSample(mp4cookie->stream, *frame);
		}
		*time = theFileReader->GetTimeForFrame(mp4cookie->stream, *frame);
	}

	if (flags & B_MEDIA_SEEK_TO_TIME) {
		*frame = theFileReader->GetFrameForTime(mp4cookie->stream, *time);
	}

	TRACE("mp4Reader::FindKeyFrame: calc(stream %d %s%s%s%s, time %Ld, frame %Ld)\n", mp4cookie->stream,
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	// Audio does not have keyframes?  Or all audio frames are keyframes?
	if (mp4cookie->audio == false) {
		while (*frame > 0 && *frame <= mp4cookie->frame_count) {
			keyframe = theFileReader->IsKeyFrame(mp4cookie->stream, *frame);
			
			if (keyframe)
				break;
			
			if (flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
				(*frame)--;
			} else {
				(*frame)++;
			}
		}
		
		// We consider frame 0 to be a keyframe but that is likely wrong
		if (!keyframe && *frame > 0) {
			TRACE("mp4Reader::FindKeyFrame: Did NOT find keyframe at frame %Ld\n", *frame);
			return B_LAST_BUFFER_ERROR;
		}
	}

	*time = theFileReader->GetTimeForFrame(mp4cookie->stream, *frame);
	if (mp4cookie->audio) {
		*frame = theFileReader->GetSampleForTime(mp4cookie->stream, *time);
	}
	
	TRACE("mp4Reader::FindKeyFrame: Found keyframe at frame %Ld time %Ld\n" ,*frame, *time);

	return B_OK;
}

status_t
mp4Reader::GetNextChunk(void *_cookie, const void **chunkBuffer,
	size_t *chunkSize, media_header *mediaHeader)
{
	mp4_cookie *cookie = (mp4_cookie *)_cookie;
	off_t start;
	uint32 size;
	bool keyframe;
	uint32 frameCount;

	if (theFileReader->GetBufferForFrame(cookie->stream, cookie->frame_pos, &start, &size, &keyframe, &(mediaHeader->start_time), &frameCount)  == false) {
		TRACE("LAST BUFFER : %d (%ld)\n",cookie->stream, cookie->frame_pos);
		return B_LAST_BUFFER_ERROR;
	}

	if (cookie->buffer_size < size) {
		delete [] cookie->buffer;
		cookie->buffer_size = (size + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
	}
	
//	mediaHeader->start_time = bigtime_t(double(cookie->frame_pos * cookie->frame_size) * 1000000.0 * double(cookie->frames_per_sec_scale)) / cookie->frames_per_sec_rate;

	if (cookie->audio) {
		TRACE("Audio");
		mediaHeader->type = B_MEDIA_ENCODED_AUDIO;
		mediaHeader->u.encoded_audio.buffer_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
	} else {
		TRACE("Video");
		mediaHeader->type = B_MEDIA_ENCODED_VIDEO;
		mediaHeader->u.encoded_video.field_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		mediaHeader->u.encoded_video.first_active_line = 0;
		mediaHeader->u.encoded_video.line_count = cookie->line_count;
		mediaHeader->u.encoded_video.field_number = 0;
		mediaHeader->u.encoded_video.field_sequence = cookie->frame_pos;
	}
	TRACE(" stream %d: frame_pos %ld start time %.6f file pos %lld Size %ld key frame %s frameCount %ld\n", cookie->stream, cookie->frame_pos, mediaHeader->start_time / 1000000.0, start, size, keyframe ? "true" : "false", frameCount);

	cookie->frame_pos += frameCount;
	
	*chunkBuffer = cookie->buffer;
	*chunkSize = size;

	ssize_t bytesRead = theFileReader->Source()->ReadAt(start, cookie->buffer, size);
	if (bytesRead < B_OK)
		return bytesRead;

	if (bytesRead < (ssize_t)size) {
		ERROR("Not enough bytes read asked for %ld got %ld\n", size, bytesRead);
		return B_LAST_BUFFER_ERROR;
	}

	return B_OK;
}


Reader *
mp4ReaderPlugin::NewReader()
{
	return new mp4Reader;
}


MediaPlugin *instantiate_plugin()
{
	return new mp4ReaderPlugin;
}
