#ifndef _MIXER_INPUT_H
#define _MIXER_INPUT_H

#include <RealtimeAlloc.h>
#include <MediaNode.h>
#include "MixerCore.h"
#include "MixerUtils.h" // DEBUG only
#include "MixerDebug.h"

class ByteSwap;
class Resampler;

class MixerInput
{
public:
	MixerInput(MixerCore *core, const media_input &input, float mixFrameRate, int32 mixFrameCount);
	~MixerInput();
	
	int32	ID();
	
	void	BufferReceived(BBuffer *buffer);
	
	media_input & MediaInput();

	// The physical input channels
	int		GetInputChannelCount();
	int		GetInputChannelType(int channel);
	void	SetInputChannelGain(int channel, float gain);
	float	GetInputChannelGain(int channel);

	// The destinations for each channel
	void 	AddInputChannelDestination(int channel, int destination_type);
	void 	RemoveInputChannelDestination(int channel, int destination_type);
//	void 	SetInputChannelDestinationGain(int channel, int destination_type, float gain);
//	float	GetInputChannelDestinationGain(int channel, int destination_type);
	bool 	HasInputChannelDestination(int channel, int destination_type);
	int 	GetInputChannelForDestination(int destination_type); // returns -1 if not found
	
	// The virtual mixer channels that are generated from destinations
	int		GetMixerChannelCount();
	void 	SetMixerChannelGain(int mixer_channel, float gain);
	float	GetMixerChannelGain(int mixer_channel);
	int		GetMixerChannelType(int mixer_channel);
	
	void	SetEnabled(bool yesno);
	bool	IsEnabled();

	// only for use by MixerCore
	bool	GetMixerChannelInfo(int mixer_channel, int64 framepos, bigtime_t time, const float **buffer, uint32 *sample_offset, int *type, float *gain);

protected:
	friend class MixerCore;
	void SetMixBufferFormat(int32 framerate, int32 frames);
	
private:
	void UpdateInputChannelDestinationMask();
	void UpdateInputChannelDestinations();

private:
	struct input_chan_info {
		float	*buffer_base;
		uint32	destination_mask;	// multiple or no bits sets
		float	gain;
	};	
	struct mixer_chan_info {
		float	*buffer_base;
		int		destination_type;
		float	destination_gain;
	};
	
private:
	MixerCore		*fCore;
	media_input		fInput;
	ByteSwap		*fInputByteSwap;
	
	float			fChannelTypeGain[MAX_CHANNEL_TYPES];
	
	bool			fEnabled;

	input_chan_info *fInputChannelInfo; // array
	int				fInputChannelCount;
	uint32			fInputChannelMask;

	mixer_chan_info *fMixerChannelInfo; // array
	int				fMixerChannelCount;

	float 			*fMixBuffer;
	
	int32 			fMixBufferFrameRate;
	int				fMixBufferFrameCount;
	
	int32			fLastDataFrameWritten;
	bigtime_t		fLastDataAvailableTime;
	
	double			fFractionalFrames;
	
	Resampler		**fResampler; // array
	rtm_pool		*fRtmPool;

	bool			fUserOverridesChannelDestinations;
	
	int32			debugMixBufferFrames;
};

inline int
MixerInput::GetMixerChannelCount()
{
	return fMixerChannelCount;
}

inline bool
MixerInput::GetMixerChannelInfo(int mixer_channel, int64 framepos, bigtime_t time, const float **buffer, uint32 *sample_offset, int *type, float *gain)
{
	ASSERT(fMixBuffer); // this function should not be called if we don't have a mix buffer!
	ASSERT(mixer_channel >= 0 && mixer_channel < fMixerChannelCount);
	if (!fEnabled)
		return false;

#if 1
	if (time < (fLastDataAvailableTime - duration_for_frames(fMixBufferFrameRate, fMixBufferFrameCount))
		|| (time + duration_for_frames(fMixBufferFrameRate, debugMixBufferFrames)) >= fLastDataAvailableTime)
		ERROR("MixerInput::GetMixerChannelInfo: reading wrong data, have %Ld to %Ld, reading from %Ld to %Ld\n",
				fLastDataAvailableTime - duration_for_frames(fMixBufferFrameRate, fMixBufferFrameCount), fLastDataAvailableTime, time, time + duration_for_frames(fMixBufferFrameRate, debugMixBufferFrames));
#endif

	if (time > fLastDataAvailableTime)
		return false;
	
	int32 offset = framepos % fMixBufferFrameCount;
	if (mixer_channel == 0) PRINT(3, "GetMixerChannelInfo: frames %ld to %ld\n", offset, offset + debugMixBufferFrames - 1);
	*buffer = reinterpret_cast<float *>(reinterpret_cast<char *>(fMixerChannelInfo[mixer_channel].buffer_base) + (offset * sizeof(float) * fInputChannelCount));
	*sample_offset = sizeof(float) * fInputChannelCount;
	*type = fMixerChannelInfo[mixer_channel].destination_type;
	*gain = fMixerChannelInfo[mixer_channel].destination_gain;
	return true;
}

#endif
