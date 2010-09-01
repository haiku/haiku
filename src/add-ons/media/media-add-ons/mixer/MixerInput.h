/*
 * Copyright 2003-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 */
#ifndef _MIXER_INPUT_H
#define _MIXER_INPUT_H


#include <MediaNode.h>
#include <RealtimeAlloc.h>

#include "MixerCore.h"
#include "MixerDebug.h"
#include "MixerUtils.h"


class ByteSwap;
class Resampler;


class MixerInput {
public:
								MixerInput(MixerCore* core,
									const media_input& input,
									float mixFrameRate, int32 mixFrameCount);
								~MixerInput();

			int32				ID();
			media_input&		MediaInput();

			void				BufferReceived(BBuffer* buffer);

			void				UpdateResamplingAlgorithm();

	// The physical input channels
			int					GetInputChannelCount();
			int					GetInputChannelType(int channel);
			void				SetInputChannelGain(int channel, float gain);
			float				GetInputChannelGain(int channel);

	// The destinations for each channel
			void				AddInputChannelDestination(int channel,
									int destinationType);
			void				RemoveInputChannelDestination(int channel,
									int destinationType);
			bool				HasInputChannelDestination(int channel,
									int destinationType);
			int 				GetInputChannelForDestination(
									int destinationType);
				// returns -1 if not found

	// The virtual mixer channels that are generated from destinations
			int					GetMixerChannelCount();
			void				SetMixerChannelGain(int mixerChannel,
									float gain);
			float				GetMixerChannelGain(int mixerChannel);
			int					GetMixerChannelType(int mixerChannel);

			void				SetEnabled(bool enabled);
			bool				IsEnabled();

	// only for use by MixerCore
			bool				GetMixerChannelInfo(int mixerChannel,
									int64 framepos, bigtime_t time,
									const float** _buffer,
									uint32* _sampleOffset, int* _type,
									float* _gain);

protected:
	friend class MixerCore;

			void				SetMixBufferFormat(int32 framerate,
									int32 frames);

private:
			void				_UpdateInputChannelDestinationMask();
			void				_UpdateInputChannelDestinations();

	struct input_chan_info {
		float*					buffer_base;
		uint32					destination_mask;	// multiple or no bits sets
		float					gain;
	};

	struct mixer_chan_info {
		float*					buffer_base;
		int						destination_type;
		float					destination_gain;
	};

private:
			MixerCore*			fCore;
			media_input			fInput;
			ByteSwap*			fInputByteSwap;
			float				fChannelTypeGain[MAX_CHANNEL_TYPES];
			bool				fEnabled;
			input_chan_info*	fInputChannelInfo; // array
			int					fInputChannelCount;
			uint32				fInputChannelMask;
			mixer_chan_info*	fMixerChannelInfo; // array
			int					fMixerChannelCount;
			float*				fMixBuffer;
			int32				fMixBufferFrameRate;
			int					fMixBufferFrameCount;
			int32				fLastDataFrameWritten;
			bigtime_t			fLastDataAvailableTime;
			double				fFractionalFrames;
			Resampler**			fResampler; // array
			rtm_pool*			fRtmPool;
			bool				fUserOverridesChannelDestinations;
			int32				fDebugMixBufferFrames;
};


inline int
MixerInput::GetMixerChannelCount()
{
	return fMixerChannelCount;
}


inline bool
MixerInput::GetMixerChannelInfo(int mixerChannel, int64 framepos,
	bigtime_t time, const float** buffer, uint32* sampleOffset, int* type,
	float* gain)
{
	// this function should not be called if we don't have a mix buffer!
	ASSERT(fMixBuffer != NULL);
	ASSERT(mixerChannel >= 0 && mixerChannel < fMixerChannelCount);
	if (!fEnabled)
		return false;

#if DEBUG
	if (time < (fLastDataAvailableTime - duration_for_frames(
			fMixBufferFrameRate, fMixBufferFrameCount))
		|| (time + duration_for_frames(fMixBufferFrameRate,
			fDebugMixBufferFrames)) >= fLastDataAvailableTime) {
		// Print this error for the first channel only.
		if (mixerChannel == 0) {
			ERROR("MixerInput::GetMixerChannelInfo: reading wrong data, have %Ld "
				"to %Ld, reading from %Ld to %Ld\n",
				fLastDataAvailableTime - duration_for_frames(fMixBufferFrameRate,
					fMixBufferFrameCount), fLastDataAvailableTime, time,
				time + duration_for_frames(fMixBufferFrameRate,
				fDebugMixBufferFrames));
		}
	}
#endif

	if (time > fLastDataAvailableTime)
		return false;

	int32 offset = framepos % fMixBufferFrameCount;
	if (mixerChannel == 0) {
		PRINT(3, "GetMixerChannelInfo: frames %ld to %ld\n", offset,
			offset + fDebugMixBufferFrames - 1);
	}
	*buffer = reinterpret_cast<float*>(reinterpret_cast<char*>(
		fMixerChannelInfo[mixerChannel].buffer_base)
		+ (offset * sizeof(float) * fInputChannelCount));
	*sampleOffset = sizeof(float) * fInputChannelCount;
	*type = fMixerChannelInfo[mixerChannel].destination_type;
	*gain = fMixerChannelInfo[mixerChannel].destination_gain;
	return true;
}

#endif	// _MIXER_INPUT_H
