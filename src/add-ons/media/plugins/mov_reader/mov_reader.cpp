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
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <StopWatch.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "RawFormats.h"

#include "mov_reader.h"

//#define TRACE_MOV_READER
#ifdef TRACE_MOV_READER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

struct mov_cookie
{
	unsigned	stream;
	char *		buffer;
	unsigned	buffer_size;

	int64		frame_count;
	bigtime_t 	duration;
	media_format format;

	bool		audio;

	// audio only:
	off_t		byte_pos;
	uint32		bytes_per_sec_rate;
	uint32		bytes_per_sec_scale;

	// video only:
	uint32		line_count;
	
	// Common
	uint32		frame_pos;
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
};


movReader::movReader()
 :	theFileReader(0)
{
	TRACE("movReader::movReader\n");
}

movReader::~movReader()
{
 	delete theFileReader;
}

const char *
movReader::Copyright()
{
	return "mov_reader & libMOV, " B_UTF8_COPYRIGHT " by David McPaul";
}
	
status_t
movReader::Sniff(int32 *streamCount)
{
	TRACE("movReader::Sniff\n");
	
	BPositionIO *pos_io_source;

	pos_io_source = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!pos_io_source) {
		TRACE("movReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}
	
	if (!MOVFileReader::IsSupported(pos_io_source)) {
		TRACE("movReader::Sniff: unsupported file type\n");
		return B_ERROR;
	}
	
	TRACE("movReader::Sniff: this stream seems to be supported\n");
	
	theFileReader = new MOVFileReader(pos_io_source);
	if (B_OK != theFileReader->ParseFile()) {
		ERROR("movReader::Sniff: error parsing file\n");
		return B_ERROR;
	}
	
	*streamCount = theFileReader->getStreamCount();
	return B_OK;
}

void
movReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_QUICKTIME_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "video/quicktime");
	strcpy(mff->file_extension, "mov");
	strcpy(mff->short_name,  "MOV");
	strcpy(mff->pretty_name, "Quicktime (MOV) file format");
}

status_t
movReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	uint32 codecID;

	mov_cookie *cookie = new mov_cookie;
	*_cookie = cookie;
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;
	cookie->frame_pos = 0;

	BMediaFormats formats;
	media_format *format = &cookie->format;
	media_format_description description;

	if (theFileReader->IsActive(cookie->stream) == false) {
		ERROR("movReader::AllocateCookie: stream %d is not active\n", cookie->stream);
		delete cookie;
		return B_ERROR;
	}
	
	const mov_stream_header *stream_header;
	stream_header = theFileReader->StreamFormat(cookie->stream);
	if (!stream_header) {
		ERROR("movReader::AllocateCookie: stream %d has no header\n", cookie->stream);
		delete cookie;
		return B_ERROR;
	}
	
	TRACE("movReader::AllocateCookie: stream %ld (%s)\n", streamNumber, theFileReader->IsAudio(cookie->stream) ? "audio" : theFileReader->IsVideo(cookie->stream)  ? "video" : "unknown");

	if (theFileReader->IsAudio(cookie->stream)) {
		const AudioMetaData *audio_format = theFileReader->AudioFormat(cookie->stream);
		if (!audio_format) {
			ERROR("movReader::AllocateCookie: audio stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}

		codecID = B_BENDIAN_TO_HOST_INT32(audio_format->compression);

		cookie->frame_count = theFileReader->getAudioFrameCount(cookie->stream);
		cookie->duration = theFileReader->getAudioDuration(cookie->stream);
		
		cookie->audio = true;
		cookie->byte_pos = 0;

		if (stream_header->scale && stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate * audio_format->SampleSize * audio_format->NoOfChannels / 8;
			cookie->bytes_per_sec_scale = stream_header->scale;
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = stream_header->scale;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using both)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else if (stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate * audio_format->SampleSize * audio_format->NoOfChannels / 8;
			cookie->bytes_per_sec_scale = 1;
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using rate)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else if (audio_format->BufferSize) {
			cookie->bytes_per_sec_rate = audio_format->BufferSize;
			cookie->bytes_per_sec_scale = 1;
			cookie->frames_per_sec_rate = audio_format->BufferSize * 8 / audio_format->SampleSize / audio_format->NoOfChannels;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using PacketSize)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else {
			cookie->bytes_per_sec_rate = 128000;
			cookie->bytes_per_sec_scale = 8;
			cookie->frames_per_sec_rate = 16000;
			cookie->frames_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using fallback)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		}

		if ((audio_format->compression == AUDIO_NONE) ||
			(audio_format->compression == AUDIO_RAW) ||
			(audio_format->compression == AUDIO_TWOS1) ||
			(audio_format->compression == AUDIO_TWOS2)) {
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (B_OK != formats.GetFormatFor(description, format)) {
				format->type = B_MEDIA_RAW_AUDIO;
			}

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
				ERROR("movReader::AllocateCookie: unhandled bits per sample %d\n", audio_format->SampleSize);
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
				ERROR("movReader::AllocateCookie: unhandled bits per sample %d\n", audio_format->SampleSize);
				return B_ERROR;
			}

			format->u.encoded_audio.frame_size = audio_format->FrameSize;
			format->u.encoded_audio.output.buffer_size = audio_format->BufferSize;

			TRACE("compression ");

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
				case AUDIO_IMA4:
					TRACE("IMA4\n");
					format->u.encoded_audio.bit_rate = audio_format->BitRate;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					break;
				default:
					TRACE("OTHER %s\n",(char *)(&codecID));
					format->u.encoded_audio.bit_rate = 8 * cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.frame_rate = cookie->frames_per_sec_rate / cookie->frames_per_sec_scale;
					format->u.encoded_audio.output.channel_count = audio_format->NoOfChannels;
					break;
			}
		}

		TRACE("Audio NoOfChannels %d, SampleSize %d, SampleRate %f, FrameSize %ld\n",audio_format->NoOfChannels, audio_format->SampleSize, audio_format->SampleRate, audio_format->FrameSize);

		TRACE("Audio frame_rate %f, channel_count %ld, format %ld, buffer_size %ld, frame_size %ld, bit_rate %f\n",
				format->u.encoded_audio.output.frame_rate, format->u.encoded_audio.output.channel_count, format->u.encoded_audio.output.format,format->u.encoded_audio.output.buffer_size, format->u.encoded_audio.frame_size, format->u.encoded_audio.bit_rate);
		
		// Some codecs have additional setup data that they need, put it in metadata
		size_t size = audio_format->VOLSize;
		const void *data = audio_format->theVOL;
		if (size > 0) {
			TRACE("VOL SIZE %ld\n", size);
			format->SetMetaData(data, size);
		}
	
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
			ERROR("movReader::AllocateCookie: video stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		codecID = B_BENDIAN_TO_HOST_INT32(video_format->compression);
		
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
		TRACE("compression %s\n", (char *)(&codecID));

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
		format->u.encoded_video.output.display.line_width = theFileReader->MovMainHeader()->width;
		format->u.encoded_video.output.display.line_count = cookie->line_count;
		format->u.encoded_video.output.display.bytes_per_row = 0; // format->u.encoded_video.output.display.line_width * 4;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0;
		
		TRACE("max_bit_rate %.3f\n", format->u.encoded_video.max_bit_rate);
		TRACE("field_rate   %.3f\n", format->u.encoded_video.output.field_rate);

		// Some decoders need additional metadata passed via a special Atom
		size_t size = video_format->VOLSize;
		const void *data = video_format->theVOL;
		if (size > 0) {
			TRACE("VOL SIZE  %ld\n", size);
			format->SetMetaData(data, size);
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
movReader::FreeCookie(void *_cookie)
{
	mov_cookie *cookie = (mov_cookie *)_cookie;

	delete [] cookie->buffer;

	delete cookie;
	return B_OK;
}


status_t
movReader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, const void **infoBuffer, size_t *infoSize)
{
	mov_cookie *cookie = (mov_cookie *)_cookie;

	if (cookie) {
		*frameCount = cookie->frame_count;
		*duration = cookie->duration;
		*format = cookie->format;
	
		// Copy metadata to infoBuffer
		if (theFileReader->IsVideo(cookie->stream)) {
			const VideoMetaData *video_format = theFileReader->VideoFormat(cookie->stream);
			*infoBuffer = video_format->theVOL;
			*infoSize = video_format->VOLSize;
		} else {
			const AudioMetaData *audio_format = theFileReader->AudioFormat(cookie->stream);
			*infoBuffer = audio_format->theVOL;
			*infoSize = audio_format->VOLSize;
		}
	}
	
	return B_OK;
}


status_t
movReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	// Seek to the requested frame
	
	mov_cookie *movcookie = (mov_cookie *)cookie;

	if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		// frame = (time * rate) / fps / 1000000LL
		*frame = ((*time * movcookie->frames_per_sec_rate) / (int64)movcookie->frames_per_sec_scale) / 1000000LL;
		movcookie->frame_pos = *frame;
		TRACE("Time %Ld to Frame %Ld\n",*time, *frame);
	}
	
	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		// time = frame * 1000000LL * fps / rate
		*time = (*frame * 1000000LL * (int64)movcookie->frames_per_sec_scale) / movcookie->frames_per_sec_rate;
		movcookie->frame_pos = *frame;
		TRACE("Frame %Ld to Time %Ld\n", *frame, *time);
	}

	TRACE("movReader::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	return B_OK;
}

status_t
movReader::FindKeyFrame(void* cookie, uint32 flags,
							int64* frame, bigtime_t* time)
{
	// Find the nearest keyframe to the given time or frame.

	mov_cookie *movcookie = (mov_cookie *)cookie;

	bool keyframe = false;

	if (flags & B_MEDIA_SEEK_TO_TIME) {
		// convert time to frame as we seek by frame
		// frame = (time * rate) / fps / 1000000LL
		*frame = ((*time * movcookie->frames_per_sec_rate) / (int64)movcookie->frames_per_sec_scale) / 1000000LL;
	}

	TRACE("movReader::FindKeyFrame: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	if (movcookie->audio) {
		// Audio does not have keyframes?  Or all audio frames are keyframes?
		return B_OK;
	} else {
		while (*frame > 0 && *frame <= movcookie->frame_count) {
			keyframe = theFileReader->IsKeyFrame(movcookie->stream, *frame);
			
			if (keyframe)
				break;
			
			if (flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
				(*frame)--;
			} else {
				(*frame)++;
			}
		}
		
		// We consider frame 0 to be a keyframe.
		if (!keyframe && *frame > 0) {
			TRACE("Did NOT find keyframe at frame %Ld\n",*frame);
			return B_LAST_BUFFER_ERROR;
		}
	}

	// convert frame found to time
	// time = frame * 1000000LL * fps / rate
	*time = (*frame * 1000000LL * (int64)movcookie->frames_per_sec_scale) / movcookie->frames_per_sec_rate;

	TRACE("Found keyframe at frame %Ld time %Ld\n",*frame,*time);

	return B_OK;
}

status_t
movReader::GetNextChunk(void *_cookie,
						const void **chunkBuffer, size_t *chunkSize,
						media_header *mediaHeader)
{
	mov_cookie *cookie = (mov_cookie *)_cookie;

	int64 start; uint32 size; bool keyframe; uint32 chunkFrameCount;

	if (!theFileReader->GetNextChunkInfo(cookie->stream, cookie->frame_pos, &start, &size, &keyframe, &chunkFrameCount))
		return B_LAST_BUFFER_ERROR;

	if (cookie->buffer_size < size) {
		delete [] cookie->buffer;
		cookie->buffer_size = (size + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
	}
	
	if (cookie->audio) {
		TRACE("Audio stream %d: chunk %ld expected start %lld Size %ld key %d\n",cookie->stream, cookie->frame_pos, start, size, keyframe);
		mediaHeader->type = B_MEDIA_ENCODED_AUDIO;
		mediaHeader->u.encoded_audio.buffer_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;

		// This will only work with raw audio I think.
		mediaHeader->start_time = (cookie->byte_pos * 1000000LL * cookie->bytes_per_sec_scale) / cookie->bytes_per_sec_rate;
//		TRACE("Audio - Frames in Chunk %ld / Actual Start Time %Ld using byte_pos\n",theFileReader->getNoFramesInChunk(cookie->stream,cookie->frame_pos),mediaHeader->start_time);
		
		// We should find the current frame position (ie first frame in chunk) then compute using fps
//		cookie->frame_pos = theFileReader->getFirstFrameInChunk(cookie->stream,cookie->frame_pos);
		mediaHeader->start_time = (cookie->frame_pos * 1000000LL * (int64)cookie->frames_per_sec_scale) / cookie->frames_per_sec_rate;
//		TRACE("Audio - Frames in Chunk %ld / Actual Start Time %Ld using frame_no %ld\n",theFileReader->getNoFramesInChunk(cookie->stream,cookie->frame_pos),mediaHeader->start_time, cookie->frame_pos);

		cookie->byte_pos += size;
	} else {
		TRACE("Video stream %d: frame %ld start %lld Size %ld key %d\n",cookie->stream, cookie->frame_pos, start, size, keyframe);
		mediaHeader->start_time = (cookie->frame_pos * 1000000LL * (int64)cookie->frames_per_sec_scale) / cookie->frames_per_sec_rate;
		mediaHeader->type = B_MEDIA_ENCODED_VIDEO;
		mediaHeader->u.encoded_video.field_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		mediaHeader->u.encoded_video.first_active_line = 0;
		mediaHeader->u.encoded_video.line_count = cookie->line_count;
	}
	
	cookie->frame_pos += chunkFrameCount;
	TRACE("stream %d: start_time %.6f\n", cookie->stream, mediaHeader->start_time / 1000000.0);

	*chunkBuffer = cookie->buffer;
	*chunkSize = size;
	return (int)size == theFileReader->Source()->ReadAt(start, cookie->buffer, size) ? B_OK : B_LAST_BUFFER_ERROR;
}


Reader *
movReaderPlugin::NewReader()
{
	return new movReader;
}


MediaPlugin *instantiate_plugin()
{
	return new movReaderPlugin;
}
