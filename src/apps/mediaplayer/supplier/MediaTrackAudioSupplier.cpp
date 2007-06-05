/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "MediaTrackAudioSupplier.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <MediaTrack.h>

using std::nothrow;

// constructor
MediaTrackAudioSupplier::MediaTrackAudioSupplier(BMediaTrack* track)
	: AudioSupplier()
	, fMediaTrack(track)

	, fPerformanceTime(0)
	, fDuration(0)
{
	if (!fMediaTrack) {
		printf("MediaTrackAudioSupplier() - no media track\n");
		return;
	}

	fFormat.u.raw_audio = media_multi_audio_format::wildcard;
	#ifdef __HAIKU__
	  fFormat.u.raw_audio.format = media_multi_audio_format::B_AUDIO_FLOAT;
	#endif
	status_t ret = fMediaTrack->DecodedFormat(&fFormat);
	if (ret < B_OK) {
		printf("MediaTrackAudioSupplier() - "
			"fMediaTrack->DecodedFormat(): %s\n", strerror(ret));
		return;
	}

	fDuration = fMediaTrack->Duration();
//
//	for (bigtime_t time = 0; time < fDuration; time += 10000) {
//		bigtime_t keyFrameTime = time;
//		fMediaTrack->FindKeyFrameForTime(&keyFrameTime,
//			B_MEDIA_SEEK_CLOSEST_BACKWARD);
//		printf("audio keyframe time for time: %lld -> %lld\n", time, keyFrameTime);
//	}
}

// destructor
MediaTrackAudioSupplier::~MediaTrackAudioSupplier()
{
}


media_format
MediaTrackAudioSupplier::Format() const
{
	return fFormat;
}


status_t
MediaTrackAudioSupplier::GetEncodedFormat(media_format* format) const
{
	if (!fMediaTrack)
		return B_NO_INIT;
	return fMediaTrack->EncodedFormat(format);
}


status_t
MediaTrackAudioSupplier::GetCodecInfo(media_codec_info* info) const
{
	if (!fMediaTrack)
		return B_NO_INIT;
	return fMediaTrack->GetCodecInfo(info);
}


status_t
MediaTrackAudioSupplier::ReadFrames(void* buffer, int64* framesRead,
	bigtime_t* performanceTime)
{
	if (!fMediaTrack)
		return B_NO_INIT;
	if (!buffer || !framesRead)
		return B_BAD_VALUE;

	media_header mediaHeader;
	status_t ret = fMediaTrack->ReadFrames(buffer, framesRead, &mediaHeader);

	if (ret < B_OK) {
		// further analyse the error
		if (fDuration == 0 || (double)fPerformanceTime / fDuration > 0.95) {
			// some codecs don't behave well, or maybe somes files are bad,
			// they don't report the end of the stream correctly
			// NOTE: "more than 95% of the stream" is of course pure guess,
			// but it fixed the problem I had with some files
			ret = B_LAST_BUFFER_ERROR;
		}
		printf("MediaTrackAudioSupplier::ReadFrame() - "
			"error while reading frames: %s\n", strerror(ret));
	} else {
		fPerformanceTime = mediaHeader.start_time;
	}

	if (performanceTime)
		*performanceTime = fPerformanceTime;

	return ret;
}


status_t
MediaTrackAudioSupplier::SeekToTime(bigtime_t* performanceTime)
{
	if (!fMediaTrack)
		return B_NO_INIT;

bigtime_t _performanceTime = *performanceTime;
	status_t ret = fMediaTrack->SeekToTime(performanceTime);
	if (ret == B_OK) {
printf("seeked: %lld -> %lld\n", _performanceTime, *performanceTime);
		fPerformanceTime = *performanceTime;
	}

	return ret;
}

