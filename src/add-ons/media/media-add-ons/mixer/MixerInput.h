#ifndef _MIXER_INPUT_H
#define _MIXER_INPUT_H

class MixerCore;

class MixerInput
{
public:
	MixerInput(MixerCore *core, const media_input &input, float mixSampleRate, int32 mixFramesCount, bigtime_t mixStartTime);
	~MixerInput();
	
	int32 ID();
	
	void BufferReceived(BBuffer *buffer);
	
	media_input & MediaInput();
	
	uint32 GetMixerChannelCount();
	void GetMixerChannelInfo(int channel, const float **buffer, uint32 *sample_offset, uint32 *type, float *gain);
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
	void SetMixBufferFormat(float samplerate, int32 frames, bigtime_t starttime);
	
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
		uint32 designation;		// only one bit is set
		float gain;
	};
	
private:
	MixerCore		*fCore;
	media_input		fInput;

	input_chan_info *fInputChannelInfo; // array
	uint32			fInputChannelCount;
	uint32			fInputChannelMask;

	mixer_chan_info *fMixerChannelInfo; // array
	uint32			fMixerChannelCount;

	float 			*fMixBuffer;
	
	float 			fMixBufferSampleRate;
	uint32			fMixBufferFrames;
	bigtime_t		fMixBufferStartTime;

	bool			fUserOverridesChannelDesignations;
};

#endif
