#ifndef _MIXER_INPUT_H
#define _MIXER_INPUT_H

#include <RealtimeAlloc.h>
#include <MediaNode.h>
#include "debug.h"

class MixerCore;
class ByteSwap;
class Resampler;

class MixerInput
{
public:
	MixerInput(MixerCore *core, const media_input &input, float mixFrameRate, int32 mixFrameCount);
	~MixerInput();
	
	int32 ID();
	
	void BufferReceived(BBuffer *buffer);
	
	media_input & MediaInput();
	
	uint32 GetMixerChannelCount();
	void SetMixerChannelGain(int channel, float gain);
	float GetMixerChannelGain(int channel);
	
	uint32 GetInputChannelCount();
	void AddInputChannelDesignation(int channel, uint32 des);
	void RemoveInputChannelDesignation(int channel, uint32 des);
	uint32 GetInputChannelDesignations(int channel);
	uint32 GetInputChannelType(int channel);
	void SetInputChannelGain(int channel, float gain);
	float GetInputChannelGain(int channel);
	
	void SetEnabled(bool yesno);
	bool IsEnabled();

	// only for use by MixerCore
	bool GetMixerChannelInfo(int channel, int64 framepos, bigtime_t time, const float **buffer, uint32 *sample_offset, int *type, float *gain);
	int GetMixerChannelType(int channel);

protected:
	friend class MixerCore;
	void SetMixBufferFormat(int32 framerate, int32 frames);
	
private:
	void UpdateChannelDesignations();
	void UpdateMixerChannels();

private:
	struct input_chan_info {
		float *buffer_base;
		uint32 designations;	// multiple or no bits sets
		float gain;
	};	
	struct mixer_chan_info {
		float *buffer_base;
		int type;
		float gain;
	};
	
private:
	MixerCore		*fCore;
	media_input		fInput;
	ByteSwap		*fInputByteSwap;
	
	bool			fEnabled;

	input_chan_info *fInputChannelInfo; // array
	uint32			fInputChannelCount;
	uint32			fInputChannelMask;

	mixer_chan_info *fMixerChannelInfo; // array
	uint32			fMixerChannelCount;

	float 			*fMixBuffer;
	
	int32 			fMixBufferFrameRate;
	uint32			fMixBufferFrameCount;
	
	bigtime_t		fLastDataAvailableTime;
	
	Resampler		**fResampler; // array
	rtm_pool		*fRtmPool;

	bool			fUserOverridesChannelDesignations;
	
	int32			debugMixBufferFrames;
};

inline bool
MixerInput::GetMixerChannelInfo(int channel, int64 framepos, bigtime_t time, const float **buffer, uint32 *sample_offset, int *type, float *gain)
{
	ASSERT(fMixBuffer); // this function should not be called if we don't have a mix buffer!
	ASSERT(channel >= 0 && channel < fMixerChannelCount);
	if (time > fLastDataAvailableTime || !fEnabled)
		return false;
	
	int32 offset = framepos % fMixBufferFrameCount;
	if (channel == 0) PRINT(3, "GetMixerChannelInfo: frames %ld to %ld\n", offset, offset + debugMixBufferFrames - 1);
	*buffer = reinterpret_cast<float *>(reinterpret_cast<char *>(fMixerChannelInfo[channel].buffer_base) + (offset * sizeof(float) * fInputChannelCount));
	*sample_offset = sizeof(float) * fInputChannelCount;
	*type = fMixerChannelInfo[channel].type;
	*gain = fMixerChannelInfo[channel].gain;
	return true;
}

inline int
MixerInput::GetMixerChannelType(int channel)
{
	ASSERT(fMixBuffer); // this function should not be called if we don't have a mix buffer!
	ASSERT(channel >= 0 && channel < fMixerChannelCount);
	return fMixerChannelInfo[channel].type;
}

#endif
