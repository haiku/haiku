/*
 * Copyright (c) 2004, Marcus Overhagen
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
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "RawFormats.h"
#include "matroska_reader.h"
#include "matroska_codecs.h"
#include "matroska_util.h"

#define TRACE_MKV_READER
#ifdef TRACE_MKV_READER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


enum {
	TRACK_TYPE_VIDEO = 1,
	TRACK_TYPE_AUDIO = 2,
	TRACK_TYPE_TEXT = 17,
};


struct mkv_cookie
{
	unsigned	stream;
	char *		buffer;
	unsigned	buffer_size;
	
	const TrackInfo	*track_info;

	float		frame_rate;
	int64		frame_count;
	bigtime_t 	duration;
	media_format format;
	
	MatroskaFile *file;

	bool		audio;

	// video only:
	uint32		line_count;
};


mkvReader::mkvReader()
 :	fFileCache(0)
 ,	fFile(0)
 ,	fFileInfo(0)
{
	TRACE("mkvReader::mkvReader\n");
}


mkvReader::~mkvReader()
{
	mkv_Close(fFile);
	free(fFileCache);
}

      
const char *
mkvReader::Copyright()
{
	return "Matroska reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
mkvReader::Sniff(int32 *streamCount)
{
	TRACE("mkvReader::Sniff\n");

	char err_msg[100];
	
	fFileCache = CreateFileCache(Source());
	if (!fFileCache) {
		TRACE("mkvReader::Sniff: failed to create file cache\n");
		return B_ERROR;
	}
	
	fFile = mkv_Open(fFileCache, err_msg, sizeof(err_msg));
	if (!fFile) {
		TRACE("mkvReader::Sniff: open failed, error: %s\n", err_msg);
		return B_ERROR;
	}

	fFileInfo = mkv_GetFileInfo(fFile);
	
	*streamCount = mkv_GetNumTracks(fFile);
	
	TRACE("mkvReader::Sniff: file has %ld streams\n", *streamCount);
	TRACE("mkvReader::Sniff: TimecodeScale %Ld\n", fFileInfo->TimecodeScale);
	TRACE("mkvReader::Sniff: Duration %Ld\n", fFileInfo->Duration);
	
	return B_OK;
}

void
mkvReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "video/x-matroska"); // "audio/x-matroska"
	strcpy(mff->file_extension, "mkv");
	strcpy(mff->short_name,  "Matroska");
	strcpy(mff->pretty_name, "Matroska file format");
}

status_t
mkvReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	mkv_cookie *cookie = new mkv_cookie;
	*_cookie = cookie;

	TRACE("mkvReader::AllocateCookie: stream %ld\n", streamNumber); 
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;
	cookie->track_info = mkv_GetTrackInfo(fFile, streamNumber);
	cookie->file = 0;
	
//	TRACE("Number: %d\n", cookie->track_info->Number); 
//	TRACE("Type: %d\n", cookie->track_info->Type); 
//	TRACE("TrackOverlay: %d\n", cookie->track_info->TrackOverlay); 
//	TRACE("UID: %Ld\n", cookie->track_info->UID); 
//	TRACE("MinCache: %Ld\n", cookie->track_info->MinCache); 
//	TRACE("MaxCache: %Ld\n", cookie->track_info->MaxCache); 
	TRACE("DefaultDuration: %Ld\n", cookie->track_info->DefaultDuration); 
	TRACE("TimecodeScale: %.6f\n", cookie->track_info->TimecodeScale);
	TRACE("Name: %s\n", cookie->track_info->Name); 
	TRACE("Language: %s\n", cookie->track_info->Language); 
	TRACE("CodecID: %s\n", cookie->track_info->CodecID); 
	
	status_t status;
	switch (cookie->track_info->Type) {
		case TRACK_TYPE_VIDEO:
			status = SetupVideoCookie(cookie);
			break;
		case TRACK_TYPE_AUDIO:
			status = SetupAudioCookie(cookie);
			break;
		case TRACK_TYPE_TEXT:
			status = SetupTextCookie(cookie);
			break;
		default:
			TRACE("unknown track type %d\n", cookie->track_info->Type);
			status = B_ERROR;
			break;
	}

	if (status != B_OK)
		delete cookie;

	return status;
}


status_t
mkvReader::SetupVideoCookie(mkv_cookie *cookie)
{
	char err_msg[100];
	cookie->file = mkv_Open(fFileCache, err_msg, sizeof(err_msg));
	if (!cookie->file) {
		TRACE("mkvReader::SetupVideoCookie: open failed, error: %s\n", err_msg);
		return B_ERROR;
	}
	mkv_SetTrackMask(cookie->file, ~(1 << cookie->stream));
	
	cookie->audio = false;

	TRACE("video StereoMode: %d\n", cookie->track_info->Video.StereoMode); 
	TRACE("video DisplayUnit: %d\n", cookie->track_info->Video.DisplayUnit); 
	TRACE("video AspectRatioType: %d\n", cookie->track_info->Video.AspectRatioType); 
	TRACE("video PixelWidth: %d\n", cookie->track_info->Video.PixelWidth); 
	TRACE("video PixelHeight: %d\n", cookie->track_info->Video.PixelHeight); 
	TRACE("video DisplayWidth: %d\n", cookie->track_info->Video.DisplayWidth); 
	TRACE("video DisplayHeight: %d\n", cookie->track_info->Video.DisplayHeight); 
	TRACE("video ColourSpace: %d\n", cookie->track_info->Video.ColourSpace); 
	TRACE("video GammaValue: %.4f\n", cookie->track_info->Video.GammaValue); 

	TRACE("video Interlaced: %d\n", cookie->track_info->Video.Interlaced);
	
	cookie->frame_rate = get_frame_rate(cookie->track_info->DefaultDuration);
	cookie->duration = get_duration_in_us(fFileInfo->Duration);
	cookie->frame_count = get_frame_count_by_default_duration(fFileInfo->Duration, cookie->track_info->DefaultDuration);

	cookie->line_count = cookie->track_info->Video.PixelHeight;

	TRACE("mkvReader::Sniff: TimecodeScale %Ld\n", fFileInfo->TimecodeScale);
	TRACE("mkvReader::Sniff: Duration %Ld\n", fFileInfo->Duration);

	TRACE("frame_rate: %.6f\n", cookie->frame_rate); 
	TRACE("duration: %Ld (%.6f)\n", cookie->duration, cookie->duration / 1E6); 
	TRACE("frame_count: %Ld\n", cookie->frame_count); 

	status_t res;
	res = GetVideoFormat(&cookie->format, cookie->track_info->CodecID, cookie->track_info->CodecPrivate, cookie->track_info->CodecPrivateSize);
	if (res != B_OK) {
		TRACE("mkvReader::SetupVideoCookie: codec not recognized\n");
		return B_ERROR;
	}
	
	uint16 width_aspect_ratio;
	uint16 height_aspect_ratio;
	get_pixel_aspect_ratio(&width_aspect_ratio, &height_aspect_ratio,
						   cookie->track_info->Video.PixelWidth,
						   cookie->track_info->Video.PixelHeight,
						   cookie->track_info->Video.DisplayWidth,
						   cookie->track_info->Video.DisplayHeight);

//	cookie->format.u.encoded_video.max_bit_rate = 
//	cookie->format.u.encoded_video.avg_bit_rate = 
	cookie->format.u.encoded_video.output.field_rate = cookie->frame_rate;
	cookie->format.u.encoded_video.output.interlace = 1; // 1: progressive
	cookie->format.u.encoded_video.output.first_active = 0;
	cookie->format.u.encoded_video.output.last_active = cookie->line_count - 1;
	cookie->format.u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	cookie->format.u.encoded_video.output.pixel_width_aspect = width_aspect_ratio;
	cookie->format.u.encoded_video.output.pixel_height_aspect = height_aspect_ratio;
	// cookie->format.u.encoded_video.output.display.format = 0;
	cookie->format.u.encoded_video.output.display.line_width = cookie->track_info->Video.PixelWidth;
	cookie->format.u.encoded_video.output.display.line_count = cookie->track_info->Video.PixelHeight;
	cookie->format.u.encoded_video.output.display.bytes_per_row = 0;
	cookie->format.u.encoded_video.output.display.pixel_offset = 0;
	cookie->format.u.encoded_video.output.display.line_offset = 0;
	cookie->format.u.encoded_video.output.display.flags = 0;
	
	return B_OK;
}


status_t
mkvReader::SetupAudioCookie(mkv_cookie *cookie)
{
	char err_msg[100];
	cookie->file = mkv_Open(fFileCache, err_msg, sizeof(err_msg));
	if (!cookie->file) {
		TRACE("mkvReader::SetupVideoCookie: open failed, error: %s\n", err_msg);
		return B_ERROR;
	}
	mkv_SetTrackMask(cookie->file, ~(1 << cookie->stream));

	cookie->audio = true;

	TRACE("audio SamplingFreq: %.3f\n", cookie->track_info->Audio.SamplingFreq); 
	TRACE("audio OutputSamplingFreq: %.3f\n", cookie->track_info->Audio.OutputSamplingFreq); 
	TRACE("audio Channels: %d\n", cookie->track_info->Audio.Channels); 
	TRACE("audio BitDepth: %d\n", cookie->track_info->Audio.BitDepth);

	TRACE("CodecID: %s\n", cookie->track_info->CodecID); 

	cookie->frame_rate = cookie->track_info->Audio.SamplingFreq;
	cookie->duration = get_duration_in_us(fFileInfo->Duration);
	cookie->frame_count = get_frame_count_by_frame_rate(fFileInfo->Duration, cookie->track_info->Audio.SamplingFreq);

	TRACE("frame_rate: %.6f\n", cookie->frame_rate); 
	TRACE("duration: %Ld (%.6f)\n", cookie->duration, cookie->duration / 1E6); 
	TRACE("frame_count: %Ld\n", cookie->frame_count);
	
	status_t res;
	res = GetAudioFormat(&cookie->format, cookie->track_info->CodecID, cookie->track_info->CodecPrivate, cookie->track_info->CodecPrivateSize);
	if (res != B_OK) {
		TRACE("mkvReader::SetupAudioCookie: codec not recognized\n");
		return B_ERROR;
	}

	cookie->format.u.encoded_audio.bit_rate = cookie->track_info->Audio.BitDepth * cookie->track_info->Audio.Channels * cookie->track_info->Audio.SamplingFreq;
	cookie->format.u.encoded_audio.output.frame_rate = cookie->track_info->Audio.SamplingFreq;
	cookie->format.u.encoded_audio.output.channel_count = cookie->track_info->Audio.Channels;

	return B_OK;
}


status_t
mkvReader::SetupTextCookie(mkv_cookie *cookie)
{
	TRACE("text\n");
	return B_ERROR;
}


status_t
mkvReader::FreeCookie(void *_cookie)
{
	mkv_cookie *cookie = (mkv_cookie *)_cookie;
	
	mkv_Close(cookie->file);

	delete [] cookie->buffer;

	delete cookie;
	return B_OK;
}


status_t
mkvReader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	mkv_cookie *cookie = (mkv_cookie *)_cookie;
	*frameCount = cookie->frame_count;
	*duration = cookie->duration;
	*format = cookie->format;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
mkvReader::Seek(void *_cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	mkv_cookie *cookie = (mkv_cookie *)_cookie;

	TRACE("mkvReader::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		*time = bigtime_t(*frame * (1000000.0 / cookie->frame_rate));
	}
	
	if (seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
		mkv_Seek(cookie->file, *time * 1000, MKVF_SEEK_TO_PREV_KEYFRAME);
	} else if (seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) {
		mkv_Seek(cookie->file, *time * 1000, 0);
		mkv_SkipToKeyframe(cookie->file);
	} else {
		mkv_Seek(cookie->file, *time * 1000, 0);
	}

	int64 timecode;
	timecode = mkv_GetLowestQTimecode(cookie->file);
	TRACE("timecode %Ld\n", timecode);

	return B_OK;
}


status_t
mkvReader::GetNextChunk(void *_cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	mkv_cookie *cookie = (mkv_cookie *)_cookie;
	
	int res;
	unsigned int track;
	ulonglong StartTime; /* in ns */
	ulonglong EndTime; /* in ns */
	ulonglong FilePos; /* in bytes from start of file */
	unsigned int FrameSize; /* in bytes */
	unsigned int FrameFlags;
	
	if (!cookie->file)
		return B_MEDIA_NO_HANDLER;

	res = mkv_ReadFrame(cookie->file, 0, &track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags);
	if (res < 0)
		return B_ERROR;
	
	if (cookie->buffer_size < FrameSize) {
		cookie->buffer = (char *)realloc(cookie->buffer, FrameSize);
		cookie->buffer_size = FrameSize;
	}
	
	res = mkv_ReadData(cookie->file, FilePos, cookie->buffer, FrameSize);
	if (res < 0)
		return B_ERROR;
		
	*chunkBuffer = cookie->buffer;
	*chunkSize = FrameSize;
	
	bool keyframe = false;	
	
	if (cookie->audio) {
		mediaHeader->start_time = StartTime / 1000;
		mediaHeader->type = B_MEDIA_ENCODED_AUDIO;
		mediaHeader->u.encoded_audio.buffer_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
	} else {
		mediaHeader->start_time = StartTime / 1000;
		mediaHeader->type = B_MEDIA_ENCODED_VIDEO;
		mediaHeader->u.encoded_video.field_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		mediaHeader->u.encoded_video.first_active_line = 0;
		mediaHeader->u.encoded_video.line_count = cookie->line_count;
	}
	
	return B_OK;
}


Reader *
mkvReaderPlugin::NewReader()
{
	return new mkvReader;
}


MediaPlugin *instantiate_plugin()
{
	return new mkvReaderPlugin;
}
