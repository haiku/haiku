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


#define TRACE_MP4_READER
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
	off_t		byte_pos;
	uint32		chunk_pos;
	uint32		bytes_per_sec_rate;
	uint32		bytes_per_sec_scale;

	// video only:
	uint32		line_count;
	
	// Common
	uint32		frame_pos;
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
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
	return "MPEG4 & libMP4, " B_UTF8_COPYRIGHT " by David McPaul";
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
	
	*streamCount = theFileReader->getStreamCount();
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
	mp4_cookie *cookie = new mp4_cookie;
	*_cookie = cookie;
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;
	cookie->frame_pos = 0;

	BMediaFormats formats;
	media_format *format = &cookie->format;
	media_format_description description;

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

		cookie->frame_count = theFileReader->getAudioFrameCount(cookie->stream);
		cookie->duration = theFileReader->getAudioDuration(cookie->stream);
		
		cookie->audio = true;
		cookie->byte_pos = 0;
		cookie->chunk_pos = 1;

		if (stream_header->scale && stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate *
				audio_format->SampleSize * audio_format->NoOfChannels / 8;
			cookie->bytes_per_sec_scale = stream_header->scale;
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = stream_header->scale;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using both)\n",
				cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else if (stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate * audio_format->SampleSize
				* audio_format->NoOfChannels / 8;
			cookie->bytes_per_sec_scale = 1;
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using rate)\n",
				cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else if (audio_format->PacketSize) {
			cookie->bytes_per_sec_rate = audio_format->PacketSize;
			cookie->bytes_per_sec_scale = 1;
			cookie->frames_per_sec_rate = audio_format->PacketSize * 8
				/ audio_format->SampleSize / audio_format->NoOfChannels;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using PacketSize)\n",
				cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
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
			description.family = B_QUICKTIME_FORMAT_FAMILY;
			description.u.quicktime.codec = audio_format->compression;
			if (B_OK != formats.GetFormatFor(description, format)) {
				format->type = B_MEDIA_ENCODED_AUDIO;
			}
			
			switch (audio_format->compression) {
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
					TRACE("MP3\n");
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					break;
				case 'mp4a':
					TRACE("AAC Audio (mp4a)\n");
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
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
					break;
			}
		}

/*		if (audio_format->compression == 0x0001) {
			// a raw PCM format
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (B_OK != formats.GetFormatFor(description, format)) 
				format->type = B_MEDIA_RAW_AUDIO;
			format->u.raw_audio.frame_rate = audio_format->SampleRate;
			format->u.raw_audio.channel_count = audio_format->NoOfChannels;
			if (audio_format->bits_per_sample <= 8)
				format->u.raw_audio.format = B_AUDIO_FORMAT_UINT8;
			else if (audio_format->bits_per_sample <= 16)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
			else if (audio_format->bits_per_sample <= 24)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT24;
			else if (audio_format->bits_per_sample <= 32)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT32;
			else {
				ERROR("movReader::AllocateCookie: unhandled bits per sample %d\n", audio_format->bits_per_sample);
				return B_ERROR;
			}
			format->u.raw_audio.format |= B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE;
			format->u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;
			format->u.raw_audio.buffer_size = stream_header->suggested_buffer_size;
		} else {
			// some encoded format
			description.family = B_WAV_FORMAT_FAMILY;
			description.u.wav.codec = audio_format->compression;
			if (B_OK != formats.GetFormatFor(description, format)) 
				format->type = B_MEDIA_ENCODED_AUDIO;
			format->u.encoded_audio.bit_rate = 8 * audio_format->PacketSize;
			TRACE("bit_rate %.3f\n", format->u.encoded_audio.bit_rate);
			format->u.encoded_audio.output.frame_rate = audio_format->SampleRate;
			format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
		}
		*/
		// this doesn't seem to work (it's not even a fourcc)
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = audio_format->compression; format->user_data[4] = 0;
		
		// Set the VOL
		size_t size = audio_format->VOLSize;
		const void *data = audio_format->theVOL;
		format->SetMetaData(data, size);

#ifdef TRACE_MP4_READER
		if (data) {
			uint8 *p = (uint8 *)data;
			TRACE("extra_data: %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				size , p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
		}
#endif
	
		return B_OK;
	}

	if (theFileReader->IsVideo(cookie->stream)) {
		const VideoMetaData *video_format = theFileReader->VideoFormat(cookie->stream);
		if (!video_format) {
			ERROR("mp4Reader::AllocateCookie: video stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		cookie->audio = false;
		cookie->line_count = theFileReader->MovMainHeader()->height;
		
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
			
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = description.u.quicktime.codec; format->user_data[4] = 0;
		
		format->u.encoded_video.max_bit_rate = 8 * theFileReader->MovMainHeader()->max_bytes_per_sec;
		format->u.encoded_video.avg_bit_rate = format->u.encoded_video.max_bit_rate / 2; // XXX fix this
		format->u.encoded_video.output.field_rate = cookie->frames_per_sec_rate / (float)cookie->frames_per_sec_scale;
		format->u.encoded_video.output.interlace = 1; // 1: progressive
		format->u.encoded_video.output.first_active = 0;
		format->u.encoded_video.output.last_active = cookie->line_count - 1;
		format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format->u.encoded_video.output.pixel_width_aspect = 1;
		format->u.encoded_video.output.pixel_height_aspect = 1;
		// format->u.encoded_video.output.display.format = 0;
		format->u.encoded_video.output.display.line_width = theFileReader->MovMainHeader()->width;
		format->u.encoded_video.output.display.line_count = cookie->line_count;
		format->u.encoded_video.output.display.bytes_per_row = 0; // format->u.encoded_video.output.display.line_width * 4;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0;
		
		TRACE("max_bit_rate %.3f\n", format->u.encoded_video.max_bit_rate);
		TRACE("field_rate   %.3f\n", format->u.encoded_video.output.field_rate);

		// Set the VOL
		if (video_format->VOLSize > 0) {
			size_t size = video_format->VOLSize;
			const void *data = video_format->theVOL;
			format->SetMetaData(data, size);

#ifdef TRACE_MP4_READER
			if (data) {
				uint8 *p = (uint8 *)data;
				TRACE("extra_data: %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					size, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
			}
#endif
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

	*frameCount = cookie->frame_count;
	*duration = cookie->duration;
	*format = cookie->format;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
mp4Reader::Seek(void *cookie, uint32 seekTo, int64 *frame, bigtime_t *time)
{

// We should seek to nearest keyframe requested
// currently returning B_OK for audio streams causes many problems.

	mp4_cookie *mp4cookie = (mp4_cookie *)cookie;

	if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		// frame = (time * rate) / fps / 1000000LL
		*frame = ((*time * mp4cookie->frames_per_sec_rate) / (int64)mp4cookie->frames_per_sec_scale) / 1000000LL;
		TRACE("Time %Ld to Frame %Ld\n",*time, *frame);
//		movcookie->frame_pos = *frame;
//		return B_ERROR;
	}
	
	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		// time = frame * 1000000LL * fps / rate
		TRACE("Frame %Ld to Time %Ld\n", *frame, *time);
		*time = (*frame * 1000000LL * (int64)mp4cookie->frames_per_sec_scale) / mp4cookie->frames_per_sec_rate;
//		movcookie->frame_pos = *frame;
//		return B_ERROR;
	}

	TRACE("mp4Reader::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	return B_ERROR;
}


status_t
mp4Reader::GetNextChunk(void *_cookie, const void **chunkBuffer,
	size_t *chunkSize, media_header *mediaHeader)
{
	mp4_cookie *cookie = (mp4_cookie *)_cookie;
	int64 start;
	uint32 size;
	bool keyframe;

	if (cookie->audio) {
		// use chunk position
		if (!theFileReader->GetNextChunkInfo(cookie->stream, cookie->chunk_pos,
				&start, &size, &keyframe))
			return B_LAST_BUFFER_ERROR;
	} else {
		// use frame position
		if (!theFileReader->GetNextChunkInfo(cookie->stream, cookie->frame_pos,
				&start, &size, &keyframe))
			return B_LAST_BUFFER_ERROR;
	}

	if (cookie->buffer_size < size) {
		delete [] cookie->buffer;
		cookie->buffer_size = (size + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
	}
	
	if (cookie->audio) {
		TRACE("Audio stream %d: chunk %ld expected start time %lld output size %ld key %d\n",cookie->stream, cookie->chunk_pos, start, size, keyframe);
		mediaHeader->type = B_MEDIA_ENCODED_AUDIO;
		mediaHeader->u.encoded_audio.buffer_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;

		// This will only work with raw audio I think.
		mediaHeader->start_time = (cookie->byte_pos * 1000000LL * cookie->bytes_per_sec_scale) / cookie->bytes_per_sec_rate;
		TRACE("Audio - Frames in Chunk %ld / Actual Start Time %Ld using byte_pos\n",theFileReader->getNoFramesInChunk(cookie->stream,cookie->chunk_pos),mediaHeader->start_time);
		
		// We should find the current frame position (ie first frame in chunk) then compute using fps
		cookie->frame_pos = theFileReader->getFirstFrameInChunk(cookie->stream,cookie->chunk_pos);
		mediaHeader->start_time = (cookie->frame_pos * 1000000LL * (int64)cookie->frames_per_sec_scale) / cookie->frames_per_sec_rate;
		TRACE("Audio - Frames in Chunk %ld / Actual Start Time %Ld using frame_no %ld\n",theFileReader->getNoFramesInChunk(cookie->stream,cookie->chunk_pos),mediaHeader->start_time, cookie->frame_pos);

		cookie->byte_pos += size;
		// frame_pos is chunk No for audio
		cookie->chunk_pos++;
	} else {
		TRACE("Video stream %d: frame %ld start %lld Size %ld key %d\n",cookie->stream, cookie->frame_pos, start, size, keyframe);
		mediaHeader->start_time = (cookie->frame_pos * 1000000LL * (int64)cookie->frames_per_sec_scale) / cookie->frames_per_sec_rate;
		mediaHeader->type = B_MEDIA_ENCODED_VIDEO;
		mediaHeader->u.encoded_video.field_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		mediaHeader->u.encoded_video.first_active_line = 0;
		mediaHeader->u.encoded_video.line_count = cookie->line_count;
	
		cookie->frame_pos++;
	}
	
	TRACE("stream %d: start_time %.6f\n", cookie->stream, mediaHeader->start_time / 1000000.0);

	*chunkBuffer = cookie->buffer;
	*chunkSize = size;

	ssize_t bytesRead = theFileReader->Source()->ReadAt(start, cookie->buffer, size);
	if (bytesRead < B_OK)
		return bytesRead;

	if (bytesRead < (ssize_t)size)
		return B_LAST_BUFFER_ERROR;

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
