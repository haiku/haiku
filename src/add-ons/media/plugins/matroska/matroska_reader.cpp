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
	
	int			private_data_size;
	uint8 *		private_data;		// reference
	
	uint8 *		fake_private_data;	// created as needed

	float		frame_rate;
	int64		frame_count;
	bigtime_t 	duration;
	media_format format;
	
	MatroskaFile *file;

	bool		audio;

	// video only:
	uint32		line_count;
};

static uint8 
matroska_samplerate_to_samplerateindex(double samplerate)
{
	if (samplerate <= 7350.0) {
		return 12;
	} else if (samplerate <= 8000.0) {
		return 11;
	} else if (samplerate <= 11025.0) {
		return 10;
	} else if (samplerate <= 12000.0) {
		return 9;
	} else if (samplerate <= 16000.0) {
		return 8;
	} else if (samplerate <= 22050.0) {
		return 7;
	} else if (samplerate <= 24000.0) {
		return 6;
	} else if (samplerate <= 32000.0) {
		return 5;
	} else if (samplerate <= 44100.0) {
		return 4;
	} else if (samplerate <= 48000.0) {
		return 3;
	} else if (samplerate <= 64000.0) {
		return 2;
	} else if (samplerate <= 88200.0) {
		return 1;
	} else if (samplerate <= 96000.0) {
		return 0;
	}
	
	return 15;
}

mkvReader::mkvReader()
 :	fInputStream(0)
 ,	fFile(0)
 ,	fFileInfo(0)
{
	TRACE("mkvReader::mkvReader\n");
}


mkvReader::~mkvReader()
{
	mkv_Close(fFile);
	free(fInputStream);
}

int
mkvReader::CreateFakeAACDecoderConfig(const TrackInfo *track, uint8 **fakeExtraData)
{
	// Try to fake up a valid Decoder Config for the AAC decoder to parse
	TRACE("AAC (%s) requires private data (attempting to fake one)\n",track->CodecID);

    int profile = 1;
    
    if (strstr(track->CodecID,"MAIN")) {
    	profile = 1;
    } else if (strstr(track->CodecID,"LC")) {
    	profile = 2;
    } else if (strstr(track->CodecID,"SSR")) {
    	profile = 3;
	}

    uint8 sampleRateIndex = matroska_samplerate_to_samplerateindex(track->AV.Audio.SamplingFreq);

	TRACE("Profile %d, SampleRate %f, SampleRateIndex %d\n",profile, track->AV.Audio.SamplingFreq, sampleRateIndex);
    
    // profile			5 bits
    // SampleRateIndex	4 bits
    // Channels			4 Bits
    (*fakeExtraData)[0] = (profile << 3) | ((sampleRateIndex & 0x0E) >> 1);
    (*fakeExtraData)[1] = ((sampleRateIndex & 0x01) << 7) | (track->AV.Audio.Channels << 3);
    if (strstr(track->CodecID, "SBR")) {
    	TRACE("Extension SBR Needs more configuration\n");
    	// Sync Extension 0x2b7				11 bits
		// Extension Audio Object Type 0x5	5 bits
		// SBR present flag	0x1    			1 bit
		// SampleRateIndex 					3 bits
		// if SampleRateIndex = 15
		// ExtendedSamplingFrequency		24 bits
    
		sampleRateIndex = matroska_samplerate_to_samplerateindex(track->AV.Audio.OutputSamplingFreq);
        (*fakeExtraData)[2] = 0x56;
        (*fakeExtraData)[3] = 0xE5;
        if (sampleRateIndex != 15) {
	        (*fakeExtraData)[4] = 0x80 | (sampleRateIndex << 3);
   	        return 5;
        }

		uint32 sampleRate = uint32(track->AV.Audio.OutputSamplingFreq);
        
        (*fakeExtraData)[4] = 0x80 | (sampleRateIndex << 3) | ((sampleRate & 0xFFF0 ) >> 4);
        (*fakeExtraData)[5] = (sampleRate & 0x0FFF) << 4;
        (*fakeExtraData)[6] = sampleRate << 12;
        (*fakeExtraData)[7] = sampleRate << 20;
        return 8;
    } else {
    	return 2;
	}
	
	return 0;
}
      
const char *
mkvReader::Copyright()
{
	return "Matroska reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen\nUsing MatroskaParser " B_UTF8_COPYRIGHT " by Mike Matsnev";
}
	
status_t
mkvReader::Sniff(int32 *streamCount)
{
	TRACE("mkvReader::Sniff\n");

	char err_msg[100];
	
	fInputStream = CreateFileCache(Source());
	if (!fInputStream) {
		TRACE("mkvReader::Sniff: failed to create file cache\n");
		return B_ERROR;
	}
	
	fFile = mkv_Open(fInputStream, err_msg, sizeof(err_msg));
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
	cookie->file = mkv_Open(fInputStream, err_msg, sizeof(err_msg));
	if (!cookie->file) {
		TRACE("mkvReader::SetupVideoCookie: open failed, error: %s\n", err_msg);
		return B_ERROR;
	}
	mkv_SetTrackMask(cookie->file, ~(1 << cookie->stream));
	
	cookie->audio = false;

	TRACE("video StereoMode: %d\n", cookie->track_info->AV.Video.StereoMode); 
	TRACE("video DisplayUnit: %d\n", cookie->track_info->AV.Video.DisplayUnit); 
	TRACE("video AspectRatioType: %d\n", cookie->track_info->AV.Video.AspectRatioType); 
	TRACE("video PixelWidth: %d\n", cookie->track_info->AV.Video.PixelWidth); 
	TRACE("video PixelHeight: %d\n", cookie->track_info->AV.Video.PixelHeight); 
	TRACE("video DisplayWidth: %d\n", cookie->track_info->AV.Video.DisplayWidth); 
	TRACE("video DisplayHeight: %d\n", cookie->track_info->AV.Video.DisplayHeight); 
	TRACE("video ColourSpace: %d\n", cookie->track_info->AV.Video.ColourSpace); 
	TRACE("video GammaValue: %.4f\n", cookie->track_info->AV.Video.GammaValue); 

	TRACE("video Interlaced: %d\n", cookie->track_info->AV.Video.Interlaced);
	
	cookie->frame_rate = get_frame_rate(cookie->track_info->DefaultDuration);
	cookie->duration = get_duration_in_us(fFileInfo->Duration);
	cookie->frame_count = get_frame_count_by_default_duration(fFileInfo->Duration, cookie->track_info->DefaultDuration);

	cookie->line_count = cookie->track_info->AV.Video.PixelHeight;

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
	
	cookie->private_data = (uint8 *)cookie->track_info->CodecPrivate;
	cookie->private_data_size = cookie->track_info->CodecPrivateSize;

	uint16 width_aspect_ratio;
	uint16 height_aspect_ratio;
	get_pixel_aspect_ratio(&width_aspect_ratio, &height_aspect_ratio,
						   cookie->track_info->AV.Video.PixelWidth,
						   cookie->track_info->AV.Video.PixelHeight,
						   cookie->track_info->AV.Video.DisplayWidth,
						   cookie->track_info->AV.Video.DisplayHeight);

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
	cookie->format.u.encoded_video.output.display.line_width = cookie->track_info->AV.Video.PixelWidth;
	cookie->format.u.encoded_video.output.display.line_count = cookie->track_info->AV.Video.PixelHeight;
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
	cookie->file = mkv_Open(fInputStream, err_msg, sizeof(err_msg));
	if (!cookie->file) {
		TRACE("mkvReader::SetupVideoCookie: open failed, error: %s\n", err_msg);
		return B_ERROR;
	}
	mkv_SetTrackMask(cookie->file, ~(1 << cookie->stream));

	cookie->audio = true;

	TRACE("audio SamplingFreq: %.3f\n", cookie->track_info->AV.Audio.SamplingFreq); 
	TRACE("audio OutputSamplingFreq: %.3f\n", cookie->track_info->AV.Audio.OutputSamplingFreq); 
	TRACE("audio Channels: %d\n", cookie->track_info->AV.Audio.Channels); 
	TRACE("audio BitDepth: %d\n", cookie->track_info->AV.Audio.BitDepth);

	TRACE("CodecID: %s\n", cookie->track_info->CodecID); 

	cookie->frame_rate = cookie->track_info->AV.Audio.SamplingFreq;
	cookie->duration = get_duration_in_us(fFileInfo->Duration);
	cookie->frame_count = get_frame_count_by_frame_rate(fFileInfo->Duration, cookie->frame_rate);
	
	TRACE("frame_rate: %.6f\n", cookie->frame_rate); 
	TRACE("duration: %Ld (%.6f)\n", cookie->duration, cookie->duration / 1E6); 
	TRACE("frame_count: %Ld\n", cookie->frame_count);
	
	if (GetAudioFormat(&cookie->format, cookie->track_info->CodecID, cookie->track_info->CodecPrivate, cookie->track_info->CodecPrivateSize) != B_OK) {
		TRACE("mkvReader::SetupAudioCookie: codec not recognized\n");
		return B_ERROR;
	}

	cookie->format.u.encoded_audio.output.frame_rate = cookie->frame_rate;
	cookie->format.u.encoded_audio.output.channel_count = cookie->track_info->AV.Audio.Channels;
	cookie->format.u.encoded_audio.bit_rate = cookie->track_info->AV.Audio.BitDepth * cookie->format.u.encoded_audio.output.channel_count * cookie->frame_rate;

	cookie->private_data = (uint8 *)cookie->track_info->CodecPrivate;
	cookie->private_data_size = cookie->track_info->CodecPrivateSize;

	if (IS_CODEC(cookie->track_info->CodecID, "A_AAC")) {
		if (cookie->private_data_size == 0) {
			// we create our own private data and keep a reference for us to dispose.
		    cookie->fake_private_data = new uint8[8];
			cookie->private_data = cookie->fake_private_data;

			cookie->private_data_size = CreateFakeAACDecoderConfig(cookie->track_info, &cookie->private_data);
		}
		
		cookie->format.u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
		cookie->format.u.encoded_audio.bit_rate = 64000 * cookie->format.u.encoded_audio.output.channel_count;
	}

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
	delete [] cookie->fake_private_data;
	
	delete cookie;
	return B_OK;
}


status_t
mkvReader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
	media_format *format, const void **infoBuffer, size_t *infoSize)
{
	mkv_cookie *cookie = (mkv_cookie *)_cookie;
	if (cookie) {
		*frameCount = cookie->frame_count;
		*duration = cookie->duration;
		*format = cookie->format;
		*infoBuffer = cookie->private_data;
		*infoSize = cookie->private_data_size;
	}
	return B_OK;
}


status_t
mkvReader::Seek(void *cookie, uint32 flags, int64 *frame, bigtime_t *time)
{
	// Seek to the specified frame or time
	mkv_cookie *mkvcookie = (mkv_cookie *)cookie;

	TRACE("mkvReader::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		*time = bigtime_t(*frame * (1000000.0 / mkvcookie->frame_rate));
	}
	
	if (flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
		mkv_Seek(mkvcookie->file, *time * 1000, MKVF_SEEK_TO_PREV_KEYFRAME);
	} else if (flags & B_MEDIA_SEEK_CLOSEST_FORWARD) {
		mkv_Seek(mkvcookie->file, *time * 1000, 0);
		mkv_SkipToKeyframe(mkvcookie->file);
	} else {
		mkv_Seek(mkvcookie->file, *time * 1000, 0);
	}

	int64 timecode;
	timecode = mkv_GetLowestQTimecode(mkvcookie->file);
	if (timecode < 0)
		return B_ERROR;

	*time = timecode / 1000;
	*frame = get_frame_count_by_frame_rate(timecode, mkvcookie->frame_rate);

	return B_OK;
}

status_t
mkvReader::FindKeyFrame(void* cookie, uint32 flags,
							int64* frame, bigtime_t* time)
{
	// Find the nearest keyframe to the given time or frame.

	mkv_cookie *mkvcookie = (mkv_cookie *)cookie;

	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		// convert frame to time as matroska seeks by time
		*time = bigtime_t(*frame * (1000000.0 / mkvcookie->frame_rate));
	}

	TRACE("mkvReader::FindKeyFrame: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	if (flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
		mkv_Seek(mkvcookie->file, *time * 1000, MKVF_SEEK_TO_PREV_KEYFRAME);
	} else if (flags & B_MEDIA_SEEK_CLOSEST_FORWARD) {
		mkv_Seek(mkvcookie->file, *time * 1000, 0);
		mkv_SkipToKeyframe(mkvcookie->file);
	} else {
		mkv_Seek(mkvcookie->file, *time * 1000, 0);
	}
	
	int64 timecode;
	timecode = mkv_GetLowestQTimecode(mkvcookie->file);
	if (timecode < 0)
		return B_ERROR;

	// convert time found to frame
	*time = timecode / 1000;
	*frame = get_frame_count_by_frame_rate(timecode, mkvcookie->frame_rate);

	TRACE("Found keyframe at frame %Ld time %Ld\n",*frame,*time);

	return B_OK;
}

status_t
mkvReader::GetNextChunk(void *_cookie,
						const void **chunkBuffer, size_t *chunkSize,
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
	if (res > 0)
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
