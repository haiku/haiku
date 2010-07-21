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


#include "asf_reader.h"
#include "RawFormats.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include <StopWatch.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRACE_ASF_READER
#ifdef TRACE_ASF_READER
#	define TRACE printf
#else
#	define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

struct asf_cookie {
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
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
	uint32		frame_size;
};

asfReader::asfReader()
 :	theFileReader(0)
{
	TRACE("asfReader::asfReader\n");
}


asfReader::~asfReader()
{
 	delete theFileReader;
}


const char *
asfReader::Copyright()
{
	return "asf_reader " B_UTF8_COPYRIGHT " by David McPaul";
}
	

status_t
asfReader::Sniff(int32 *streamCount)
{
	TRACE("asfReader::Sniff\n");
	
	BPositionIO *pos_io_source;

	pos_io_source = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!pos_io_source) {
		TRACE("asfReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}
	
	if (!ASFFileReader::IsSupported(pos_io_source)) {
		TRACE("asfReader::Sniff: unsupported file type\n");
		return B_ERROR;
	}
	
	TRACE("asfReader::Sniff: this stream seems to be supported\n");
	
	theFileReader = new ASFFileReader(pos_io_source);
	if (B_OK != theFileReader->ParseFile()) {
		ERROR("asfReader::Sniff: error parsing file\n");
		return B_ERROR;
	}
	
	*streamCount = theFileReader->getStreamCount();
	
	TRACE("asfReader detected %ld streams\n",*streamCount);
	return B_OK;
}


void
asfReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities = media_file_format::B_READABLE
		| media_file_format::B_KNOWS_ENCODED_VIDEO
		| media_file_format::B_KNOWS_ENCODED_AUDIO
		| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "video/asf");
	strcpy(mff->file_extension, "asf");
	strcpy(mff->short_name,  "ASF");
	strcpy(mff->pretty_name, "Microsoft (ASF) file format");
}

status_t
asfReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	uint32 codecID = 0;

	size_t size;
	const void *data;

	asf_cookie *cookie = new asf_cookie;
	*_cookie = cookie;
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;
	cookie->frame_pos = 0;

	BMediaFormats formats;
	media_format *format = &cookie->format;
	media_format_description description;

	ASFAudioFormat audioFormat;
	ASFVideoFormat videoFormat;

	if (theFileReader->getVideoFormat(streamNumber,&videoFormat)) {
		TRACE("Stream %ld is Video\n",streamNumber);
		char cc1,cc2,cc3,cc4;

		cc1 = (char)((videoFormat.Compression >> 24) & 0xff);
		cc2 = (char)((videoFormat.Compression >> 16) & 0xff);
		cc3 = (char)((videoFormat.Compression >> 8) & 0xff);
		cc4 = (char)((videoFormat.Compression >> 0) & 0xff);

		TRACE("Compression %c%c%c%c\n", cc1,cc2,cc3,cc4);

		TRACE("Width %ld\n",videoFormat.VideoWidth);
		TRACE("Height %ld\n",videoFormat.VideoHeight);
		TRACE("Planes %d\n",videoFormat.Planes);
		TRACE("BitCount %d\n",videoFormat.BitCount);
		
		codecID = B_BENDIAN_TO_HOST_INT32(videoFormat.Compression);

		cookie->audio = false;
		cookie->line_count = videoFormat.VideoHeight;
		cookie->frame_size = 1;

		cookie->duration = theFileReader->getStreamDuration(streamNumber);
		cookie->frame_count = theFileReader->getFrameCount(streamNumber);
				
		TRACE("frame_count %Ld\n", cookie->frame_count);
		TRACE("duration %.6f (%Ld)\n", cookie->duration / 1E6, cookie->duration);
		TRACE("calculated fps=%Ld\n", cookie->frame_count * 1000000LL / cookie->duration);
		
		// asf does not have a frame rate!  The extended descriptor defines an average time per frame which is generally useless.
		if (videoFormat.FrameScale && videoFormat.FrameRate) {
			cookie->frames_per_sec_rate = videoFormat.FrameRate;
			cookie->frames_per_sec_scale = videoFormat.FrameScale;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using average time per frame)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else {
			cookie->frames_per_sec_rate = cookie->frame_count;
			cookie->frames_per_sec_scale = cookie->duration / 1000000LL;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (duration over frame count)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		}

		description.family = B_AVI_FORMAT_FAMILY;
		description.u.avi.codec = videoFormat.Compression;

		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_VIDEO;
			
// 		Can we just define a set of bitrates instead of a field rate?
//		format->u.encoded_video.max_bit_rate = 8 * theFileReader->MovMainHeader()->max_bytes_per_sec;
//		format->u.encoded_video.avg_bit_rate = format->u.encoded_video.max_bit_rate / 2; // XXX fix this
		format->u.encoded_video.output.field_rate = cookie->frames_per_sec_rate / (float)cookie->frames_per_sec_scale;

		format->u.encoded_video.avg_bit_rate = 1;
		format->u.encoded_video.max_bit_rate = 1;

		format->u.encoded_video.frame_size = videoFormat.VideoWidth * videoFormat.VideoHeight * videoFormat.Planes / 8;
		format->u.encoded_video.output.display.bytes_per_row = videoFormat.Planes / 8 * videoFormat.VideoWidth;
		// align to 2 bytes
		format->u.encoded_video.output.display.bytes_per_row +=	format->u.encoded_video.output.display.bytes_per_row & 1;

		switch (videoFormat.BitCount) {
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
				format->u.encoded_video.frame_size = videoFormat.VideoWidth * videoFormat.VideoHeight * 8 / 8;
		}

		format->u.encoded_video.output.display.line_width = videoFormat.VideoWidth;
		format->u.encoded_video.output.display.line_count = videoFormat.VideoHeight;
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
		size = videoFormat.extraDataSize;
		data = videoFormat.extraData;
		if (size > 0) {
			TRACE("Video Decoder Config Found Size is %ld\n",size);
			if (format->SetMetaData(data, size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				delete cookie;
				return B_ERROR;
			}

#ifdef TRACE_ASF_READER
			if (videoFormat.extraData) {
				uint8 *p = (uint8 *)videoFormat.extraData;
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

		cookie->buffer_size = ((videoFormat.VideoWidth * videoFormat.VideoHeight * 4) + 15) & ~15;		// WRONG Find max input buffer size needed
		cookie->buffer = new char [cookie->buffer_size];

		return B_OK;

	}
	
	if (theFileReader->getAudioFormat(streamNumber,&audioFormat)) {
		TRACE("Stream %ld is Audio\n",streamNumber);
		TRACE("Format 0x%x\n",audioFormat.Compression);
		TRACE("Channels %d\n",audioFormat.NoChannels);
		TRACE("SampleRate %ld\n",audioFormat.SamplesPerSec);
		TRACE("ByteRate %ld\n",audioFormat.AvgBytesPerSec);
		TRACE("BlockAlign %d\n",audioFormat.BlockAlign);
		TRACE("Bits %d\n",audioFormat.BitsPerSample);
		
		//uint32 sampleSize = (audioFormat.NoChannels * audioFormat.BitsPerSample / 8);
		
		cookie->audio = true;
		cookie->duration = theFileReader->getStreamDuration(streamNumber);
		// Calculate sample count using duration
		cookie->frame_count = (cookie->duration * audioFormat.SamplesPerSec) / 1000000LL;
		cookie->frame_pos = 0;
		cookie->frames_per_sec_rate = audioFormat.SamplesPerSec;
		cookie->frames_per_sec_scale = 1;
		cookie->bytes_per_sec_rate = audioFormat.AvgBytesPerSec;
		cookie->bytes_per_sec_scale = 1;

		TRACE("Chunk Count %ld\n", theFileReader->getFrameCount(streamNumber));
		TRACE("audio frame_count %Ld, duration %.6f\n", cookie->frame_count, cookie->duration / 1E6 );

		if (audioFormat.Compression == 0x0001) {
			// a raw PCM format
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (formats.GetFormatFor(description, format) < B_OK)
				format->type = B_MEDIA_RAW_AUDIO;
			format->u.raw_audio.frame_rate = audioFormat.SamplesPerSec;
			format->u.raw_audio.channel_count = audioFormat.NoChannels;
			if (audioFormat.BitsPerSample <= 8)
				format->u.raw_audio.format = B_AUDIO_FORMAT_UINT8;
			else if (audioFormat.BitsPerSample <= 16)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
			else if (audioFormat.BitsPerSample <= 24)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT24;
			else if (audioFormat.BitsPerSample <= 32)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT32;
			else {
				ERROR("asfReader::AllocateCookie: unhandled bits per sample %d\n", audioFormat.BitsPerSample);
				return B_ERROR;
			}
			format->u.raw_audio.format |= B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE;
			format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
			format->u.raw_audio.buffer_size = audioFormat.BlockAlign;
		} else if (audioFormat.Compression == 0xa) {
			// Windows Media Speech
			return B_ERROR;
		} else {
			// some encoded format
			description.family = B_WAV_FORMAT_FAMILY;
			description.u.wav.codec = audioFormat.Compression;
			if (formats.GetFormatFor(description, format) < B_OK)
				format->type = B_MEDIA_ENCODED_AUDIO;
			format->u.encoded_audio.bit_rate = 8 * audioFormat.AvgBytesPerSec;
			format->u.encoded_audio.output.frame_rate = audioFormat.SamplesPerSec;
			format->u.encoded_audio.output.channel_count = audioFormat.NoChannels;
			format->u.encoded_audio.output.buffer_size = audioFormat.BlockAlign;
			cookie->frame_size = audioFormat.BlockAlign == 0 ? 1 : audioFormat.BlockAlign;
			
			TRACE("audio: bit_rate %.3f, frame_rate %.1f, channel_count %lu, frame_size %ld\n",
				  format->u.encoded_audio.bit_rate,
				  format->u.encoded_audio.output.frame_rate,
				  format->u.encoded_audio.output.channel_count,
				  cookie->frame_size);
		}
		
		cookie->buffer_size = (audioFormat.BlockAlign + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
		
		// TODO: this doesn't seem to work (it's not even a fourcc)
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = audioFormat.Compression; format->user_data[4] = 0;
		
		// Set the Decoder Config
		size = audioFormat.extraDataSize;
		data = audioFormat.extraData;
		if (size > 0) {
			TRACE("Audio Decoder Config Found Size is %ld\n",size);
			if (format->SetMetaData(data, size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				delete cookie;
				return B_ERROR;
			}

#ifdef TRACE_ASF_READER
			if (audioFormat.extraData) {
				uint8 *p = (uint8 *)audioFormat.extraData;
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
asfReader::FreeCookie(void *_cookie)
{
	asf_cookie *cookie = (asf_cookie *)_cookie;

	delete [] cookie->buffer;

	delete cookie;
	return B_OK;
}


status_t
asfReader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
	media_format *format, const void **infoBuffer, size_t *infoSize)
{
	asf_cookie *cookie = (asf_cookie *)_cookie;
	ASFAudioFormat audioFormat;
	ASFVideoFormat videoFormat;

	if (cookie) {
		*frameCount = cookie->frame_count;
		*duration = cookie->duration;
		*format = cookie->format;

		*infoSize = 0;

		// Copy metadata to infoBuffer
		if (theFileReader->getVideoFormat(cookie->stream,&videoFormat)) {
			*infoSize = videoFormat.extraDataSize;
			*infoBuffer = videoFormat.extraData;
		} else if (theFileReader->getAudioFormat(cookie->stream,&audioFormat)) {
			*infoSize = audioFormat.extraDataSize;
			*infoBuffer = audioFormat.extraData;
		} else {
			ERROR("No stream Info for stream %d\n",cookie->stream);
		}
		
		TRACE("GetStreamInfo (%d) fc %Ld duration %Ld extra %ld\n",cookie->stream,*frameCount,*duration,*infoSize);
	}
	return B_OK;
}


status_t
asfReader::Seek(void *cookie, uint32 flags, int64 *frame, bigtime_t *time)
{
	// seek to the requested time or frame
	asf_cookie *asfCookie = (asf_cookie *)cookie;

	if (flags & B_MEDIA_SEEK_TO_TIME) {
		// frame = (time * rate) / fps / 1000000LL
		*frame = ((*time * asfCookie->frames_per_sec_rate) / (int64)asfCookie->frames_per_sec_scale) / 1000000LL;
		asfCookie->frame_pos = theFileReader->GetFrameForTime(asfCookie->stream,*time);
	}
	
	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		// time = frame * 1000000LL * fps / rate
		*time = (*frame * 1000000LL * (int64)asfCookie->frames_per_sec_scale) / asfCookie->frames_per_sec_rate;
		asfCookie->frame_pos = theFileReader->GetFrameForTime(asfCookie->stream,*time);
	}

	TRACE("asfReader::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	return B_OK;
}

status_t
asfReader::FindKeyFrame(void* cookie, uint32 flags,
							int64* frame, bigtime_t* time)
{
	// Find the nearest keyframe to the given time or frame.

	asf_cookie *asfCookie = (asf_cookie *)cookie;
	IndexEntry indexEntry;

	if (flags & B_MEDIA_SEEK_TO_TIME) {
		// convert time to frame as we seek by frame
		// frame = (time * rate) / fps / 1000000LL
		*frame = ((*time * asfCookie->frames_per_sec_rate) / (int64)asfCookie->frames_per_sec_scale) / 1000000LL;
	}

	TRACE("asfReader::FindKeyFrame: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	if (asfCookie->audio == false) {
		// Only video has keyframes
		if (flags & B_MEDIA_SEEK_CLOSEST_FORWARD || flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
			indexEntry = theFileReader->GetIndex(asfCookie->stream,*frame);

			while (indexEntry.noPayloads > 0 && indexEntry.keyFrame == false && *frame > 0) {
				if (flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
					(*frame)--;
				} else {
					(*frame)++;
				}
				indexEntry = theFileReader->GetIndex(asfCookie->stream,*frame);
			}

			if (indexEntry.noPayloads == 0) {
				return B_ERROR;
			}
		}
	}
	
	*time = *frame * 1000000LL * (int64)asfCookie->frames_per_sec_scale / asfCookie->frames_per_sec_rate;

	return B_OK;
}

status_t
asfReader::GetNextChunk(void *_cookie, const void **chunkBuffer,
	size_t *chunkSize, media_header *mediaHeader)
{
	asf_cookie *cookie = (asf_cookie *)_cookie;
	uint32 size;
	bool keyframe;

	if (theFileReader->GetNextChunkInfo(cookie->stream, cookie->frame_pos, &(cookie->buffer), &size, &keyframe, &mediaHeader->start_time) == false) {
		TRACE("LAST BUFFER : Stream %d (%ld)\n",cookie->stream, cookie->frame_pos);
		*chunkSize = 0;
		*chunkBuffer = NULL;
		return B_LAST_BUFFER_ERROR;
	}

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
	
	TRACE(" stream %d: frame %ld start time %.6f Size %ld key frame %s\n",cookie->stream, cookie->frame_pos, mediaHeader->start_time / 1000000.0, size, keyframe ? "true" : "false");

	cookie->frame_pos++;
	
	*chunkBuffer = cookie->buffer;
	*chunkSize = size;

	return B_OK;
}


Reader *
asfReaderPlugin::NewReader()
{
	return new asfReader;
}


MediaPlugin *instantiate_plugin()
{
	return new asfReaderPlugin;
}
