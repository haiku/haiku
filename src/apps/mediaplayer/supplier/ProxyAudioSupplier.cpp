/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */


#include "ProxyAudioSupplier.h"

#include <algorithm>
#include <new>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <List.h>

#include "AudioTrackSupplier.h"
#include "AudioAdapter.h"
#include "AudioVolumeConverter.h"
#include "PlaybackManager.h"

using std::nothrow;
using std::swap;


//#define TRACE_PROXY_AUDIO_SUPPLIER
#ifdef TRACE_PROXY_AUDIO_SUPPLIER
# define TRACE(x...)	printf("ProxyAudioSupplier::"); printf(x)
# define ERROR(x...)	fprintf(stderr, "ProxyAudioSupplier::"); fprintf(stderr, x)
#else
# define TRACE(x...)
# define ERROR(x...)	fprintf(stderr, "ProxyAudioSupplier::"); fprintf(stderr, x)
#endif


struct PlayingInterval {
	PlayingInterval(bigtime_t startTime, bigtime_t endTime)
		:
		start_time(startTime),
		end_time(endTime)
	{
	}

	bigtime_t	start_time;
	bigtime_t	end_time;
	bigtime_t	x_start_time;
	bigtime_t	x_end_time;
	float		speed;
};


ProxyAudioSupplier::ProxyAudioSupplier(PlaybackManager* playbackManager)
	:
	fSupplierLock("audio supplier lock"),

	fPlaybackManager(playbackManager),
	fVideoFrameRate(25.0),
	fVolume(1.0),

	fSupplier(NULL),
	fAdapter(NULL),
	fVolumeConverter(NULL),
	fAudioResampler()
{
	TRACE("ProxyAudioSupplier()\n");
}


ProxyAudioSupplier::~ProxyAudioSupplier()
{
	TRACE("~ProxyAudioSupplier()\n");
	delete fAdapter;
	delete fVolumeConverter;
}


bigtime_t
ProxyAudioSupplier::InitialLatency() const
{
	BAutolock _(fSupplierLock);

	if (fSupplier == NULL)
		return 0;

	return fSupplier->InitialLatency();
}


status_t
ProxyAudioSupplier::GetFrames(void* buffer, int64 frameCount,
	bigtime_t startTime, bigtime_t endTime)
{
	TRACE("GetFrames(%p, frameCount: %lld, time interval: %lld - %lld)\n",
		buffer, frameCount, startTime, endTime);

	// Create a list of playing intervals which compose the supplied
	// performance time interval.
	BList playingIntervals;
	status_t error = fPlaybackManager->LockWithTimeout(10000);
	if (error == B_OK) {
		bigtime_t intervalStartTime = startTime;
		while (intervalStartTime < endTime) {
			PlayingInterval* interval
				= new (nothrow) PlayingInterval(intervalStartTime, endTime);
			if (!interval) {
				error = B_NO_MEMORY;
				break;
			}
			fPlaybackManager->GetPlaylistTimeInterval(
				interval->start_time, interval->end_time,
				interval->x_start_time, interval->x_end_time,
				interval->speed);
			if (intervalStartTime == interval->end_time) {
				delete interval;
				error = B_ERROR;
				ERROR("GetFrames() - zero duration audio interval! start "
					"time: %lld\n", intervalStartTime);
				break;
			}
			if (!playingIntervals.AddItem(interval)) {
				delete interval;
				error = B_NO_MEMORY;
				ERROR("GetFrames() - Out of memory\n");
				break;
			}
			intervalStartTime = interval->end_time;
		}
		fPlaybackManager->SetCurrentAudioTime(endTime);
		fPlaybackManager->Unlock();
	} else if (error == B_TIMED_OUT) {
		TRACE("GetFrames() - LOCKING THE PLAYBACK MANAGER TIMED OUT!!!\n");
	}

	BAutolock _(fSupplierLock);

	if (!fSupplier)
		return B_ERROR;

	// retrieve the audio data for each interval.
	#ifdef TRACE_PROXY_AUDIO_SUPPLIER
	int32 intervalIndex = 0;
	#endif

	int64 framesRead = 0;
	while (!playingIntervals.IsEmpty()) {
		PlayingInterval* interval
			= (PlayingInterval*)playingIntervals.RemoveItem(0L);
		if (error != B_OK) {
			delete interval;
			continue;
		}

		// get playing direction
		int32 playingDirection = 0;
		if (interval->speed > 0)
			playingDirection = 1;
		else if (interval->speed < 0)
			playingDirection = -1;
		float absSpeed = interval->speed * playingDirection;
		int64 framesToRead = _AudioFrameForTime(interval->end_time)
			- _AudioFrameForTime(interval->start_time);

		TRACE("GetFrames() - interval (%ld) [%lld, %lld]: [%lld, %lld], "
			"frames: %lld\n", intervalIndex,
			interval->start_time, interval->end_time,
			interval->x_start_time, interval->x_end_time,
			framesToRead);

		// not playing
		if (absSpeed == 0)
			_ReadSilence(buffer, framesToRead);
		// playing
		else {
			fAudioResampler.SetInOffset(
				_AudioFrameForTime(interval->x_start_time));
			fAudioResampler.SetTimeScale(absSpeed);
			error = fAudioResampler.Read(buffer, 0, framesToRead);
			// backwards -> reverse frames
			if (error == B_OK && interval->speed < 0)
				_ReverseFrames(buffer, framesToRead);
		}
		// read silence on error
		if (error != B_OK) {
			_ReadSilence(buffer, framesToRead);
			error = B_OK;
		}
		framesRead += framesToRead;
		buffer = _SkipFrames(buffer, framesToRead);
		delete interval;

		#ifdef TRACE_PROXY_AUDIO_SUPPLIER
		intervalIndex++;
		#endif
	}
	// read silence on error
	if (error != B_OK) {
		_ReadSilence(buffer, frameCount);
		error = B_OK;
	}

	TRACE("GetFrames() done\n");

	return error;
}


void
ProxyAudioSupplier::SetFormat(const media_format& format)
{
//printf("ProxyAudioSupplier::SetFormat()\n");
	#ifdef TRACE_PROXY_AUDIO_SUPPLIER
		char string[256];
		string_for_format(format, string, 256);
		TRACE("SetFormat(%s)\n", string);
	#endif

	BAutolock _(fSupplierLock);

	fAudioResampler.SetFormat(format);

	// In case SetSupplier was called before, we need
	// to adapt to the new format, or maybe the format
	// was still invalid.
	SetSupplier(fSupplier, fVideoFrameRate);
}


const media_format&
ProxyAudioSupplier::Format() const
{
	return fAudioResampler.Format();
}


status_t
ProxyAudioSupplier::InitCheck() const
{
	status_t ret = AudioSupplier::InitCheck();
	if (ret < B_OK)
		return ret;
	return B_OK;
}


void
ProxyAudioSupplier::SetSupplier(AudioTrackSupplier* supplier,
	float videoFrameRate)
{
//printf("ProxyAudioSupplier::SetSupplier(%p, %.1f)\n", supplier,
//videoFrameRate);
	TRACE("SetSupplier(%p, %.1f)\n", supplier, videoFrameRate);

	BAutolock _(fSupplierLock);

	fSupplier = supplier;
	fVideoFrameRate = videoFrameRate;

	delete fAdapter;
	delete fVolumeConverter;

	fAdapter = new AudioAdapter(fSupplier, Format());
	fVolumeConverter = new AudioVolumeConverter(fAdapter, fVolume);

	fAudioResampler.SetSource(fVolumeConverter);
}


void
ProxyAudioSupplier::SetVolume(float volume)
{
	BAutolock _(fSupplierLock);
	fVolume = volume;
	if (fVolumeConverter)
		fVolumeConverter->SetVolume(volume);
}


float
ProxyAudioSupplier::Volume()
{
	BAutolock _(fSupplierLock);
	return fVolume;
}


// #pragma mark - audio/video/frame/time conversion


int64
ProxyAudioSupplier::_AudioFrameForVideoFrame(int64 frame) const
{
	if (!fSupplier) {
		return (int64)((double)frame * Format().u.raw_audio.frame_rate
			/ fVideoFrameRate);
	}
	const media_format& format = fSupplier->Format();
	return (int64)((double)frame * format.u.raw_audio.frame_rate
		/ fVideoFrameRate);
}


int64
ProxyAudioSupplier::_VideoFrameForAudioFrame(int64 frame) const
{
	if (!fSupplier) {
		return (int64)((double)frame * fVideoFrameRate
			/ Format().u.raw_audio.frame_rate);
	}

	const media_format& format = fSupplier->Format();
	return (int64)((double)frame * fVideoFrameRate
		/ format.u.raw_audio.frame_rate);
}


int64
ProxyAudioSupplier::_AudioFrameForTime(bigtime_t time) const
{
	return (int64)((double)time * Format().u.raw_audio.frame_rate
		/ 1000000.0);
}


int64
ProxyAudioSupplier::_VideoFrameForTime(bigtime_t time) const
{
	return (int64)((double)time * fVideoFrameRate / 1000000.0);
}


// #pragma mark - utility


void
ProxyAudioSupplier::_ReadSilence(void* buffer, int64 frames) const
{
	memset(buffer, 0, (char*)_SkipFrames(buffer, frames) - (char*)buffer);
}


void
ProxyAudioSupplier::_ReverseFrames(void* buffer, int64 frames) const
{
	int32 sampleSize = Format().u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 frameSize = sampleSize * Format().u.raw_audio.channel_count;
	char* front = (char*)buffer;
	char* back = (char*)buffer + (frames - 1) * frameSize;
	while (front < back) {
		for (int32 i = 0; i < frameSize; i++)
			swap(front[i], back[i]);
		front += frameSize;
		back -= frameSize;
	}
}


void*
ProxyAudioSupplier::_SkipFrames(void* buffer, int64 frames) const
{
	int32 sampleSize = Format().u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 frameSize = sampleSize * Format().u.raw_audio.channel_count;
	return (char*)buffer + frames * frameSize;
}

