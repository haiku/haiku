#ifndef _MIXER_INPUT_H
#define _MIXER_INPUT_H

class MixerCore;
class ByteSwap;
class Resampler;

class MixerInput
{
public:
	MixerInput(MixerCore *core, const media_input &input, float mixFrameRate, int32 mixFrameCount, bigtime_t mixStartTime);
	~MixerInput();
	
	int32 ID();
	
	void BufferReceived(BBuffer *buffer);
	
	media_input & MediaInput();
	
	uint32 GetMixerChannelCount();
	void GetMixerChannelInfo(int channel, int64 framepos, const float **buffer, uint32 *sample_offset, int *type, float *gain);
	void SetMixerChannelGain(int channel, float gain);
	float GetMixerChannelGain(int channel);
	
	void AddInputChannelDesignation(int channel, uint32 des);
	void RemoveInputChannelDesignation(int channel, uint32 des);
	uint32 GetInputChannelDesignations(int channel);
	uint32 GetInputChannelType(int channel);
	void SetInputChannelGain(int channel, float gain);
	float GetInputChannelGain(int channel);

protected:
	friend class MixerCore;
	void SetMixBufferFormat(int32 framerate, int32 frames, bigtime_t starttime);
	
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

	input_chan_info *fInputChannelInfo; // array
	uint32			fInputChannelCount;
	uint32			fInputChannelMask;

	mixer_chan_info *fMixerChannelInfo; // array
	uint32			fMixerChannelCount;

	float 			*fMixBuffer;
	
	float 			fMixBufferFrameRate;
	uint32			fMixBufferFrameCount;
	bigtime_t		fMixBufferStartTime;
	
	Resampler		**fResampler; // array

	bool			fUserOverridesChannelDesignations;
};

#endif
