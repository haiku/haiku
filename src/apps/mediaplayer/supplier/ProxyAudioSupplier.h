/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROXY_AUDIO_SUPPLIER_H
#define PROXY_AUDIO_SUPPLIER_H


#include <Locker.h>

#include "AudioResampler.h"
#include "AudioSupplier.h"


class AudioTrackSupplier;
class AudioVolumeConverter;
class PlaybackManager;


class ProxyAudioSupplier : public AudioSupplier {
public:
								ProxyAudioSupplier(
									PlaybackManager* playbackManager);
	virtual						~ProxyAudioSupplier();

	// AudioSupplier interface
	virtual	status_t			GetFrames(void* buffer, int64 frameCount,
									bigtime_t startTime, bigtime_t endTime);

	virtual bigtime_t			InitialLatency() const;
	virtual	void				SetFormat(const media_format& format);
	virtual	const media_format&	Format() const;

	virtual	status_t			InitCheck() const;

	// ProxyAudioSupplier
			void				SetSupplier(AudioTrackSupplier* supplier,
									float videoFrameRate);
			void				SetVolume(float volume);
			float				Volume();

private:
			int64				_AudioFrameForVideoFrame(int64 frame) const;
			int64				_VideoFrameForAudioFrame(int64 frame) const;
			int64				_AudioFrameForTime(bigtime_t time) const;
			int64				_VideoFrameForTime(bigtime_t time) const;

			void				_ReadSilence(void* buffer, int64 frames) const;
			void				_ReverseFrames(void* buffer,
									int64 frames) const;
			void*				_SkipFrames(void* buffer, int64 frames) const;

private:
	mutable BLocker				fSupplierLock;

			PlaybackManager*	fPlaybackManager;
			float				fVideoFrameRate;
			float				fVolume;

			AudioTrackSupplier*	fSupplier;
			AudioReader*		fAdapter;
			AudioVolumeConverter* fVolumeConverter;
			AudioResampler		fAudioResampler;
};


#endif	// PROXY_AUDIO_SUPPLIER_H
