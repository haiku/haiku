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

	int64		frame_count;
	bigtime_t 	duration;
	media_format format;
/*
	bool		audio;

	// audio only:	
	int64		byte_pos;
	uint32		bytes_per_sec_rate;
	uint32		bytes_per_sec_scale;

	// video only:
	uint32		frame_pos;
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
	uint32		line_count;
*/
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
}


status_t
mkvReader::SetupAudioCookie(mkv_cookie *cookie)
{
	TRACE("audio SamplingFreq: %.3f\n", cookie->track_info->Audio.SamplingFreq); 
	TRACE("audio OutputSamplingFreq: %.3f\n", cookie->track_info->Audio.OutputSamplingFreq); 
	TRACE("audio Channels: %d\n", cookie->track_info->Audio.Channels); 
	TRACE("audio BitDepth: %d\n", cookie->track_info->Audio.BitDepth);
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
mkvReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	TRACE("mkvReader::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	return B_ERROR;
}


status_t
mkvReader::GetNextChunk(void *_cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	mkv_cookie *cookie = (mkv_cookie *)_cookie;

	return B_LAST_BUFFER_ERROR;
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
