#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

#include "debug.h"
#include "ByteSwap.h"
#include <Buffer.h>

#define MAX_SOURCES 20

class MixerCore;

class MixerOutput
{
public:
	MixerOutput(MixerCore *core, const media_output &output);
	~MixerOutput();
	
	media_output & MediaOutput();
	
	void ChangeFormat(const media_multi_audio_format &format);
	
	uint32 GetOutputChannelCount();
	uint32 GetOutputChannelType(int channel);
	void SetOutputChannelGain(int channel, float gain);
	float GetOutputChannelGain(int channel);
	
	uint32 GetOutputChannelSourceCount(int channel);
	void AddOutputChannelSource(int channel, uint32 source_designation, float source_gain);
	void RemoveOutputChannelSource(int channel, uint32 source_designation);
	void SetOutputChannelSourceGain(int channel, uint32 source_designation, float source_gain);
	float GetOutputChannelSourceGain(int channel, uint32 source_designation);
	void GetOutputChannelSourceAt(int channel, int index, uint32 *source_designation, float *source_gain);
	
	void SetMuted(bool yesno);
	bool IsMuted();

	// only for use by MixerCore
	void GetMixerChannelInfo(int channel, int index, int *type, float *gain);
	void AdjustByteOrder(BBuffer *buffer);

private:
	void UpdateByteOrderSwap();
	void UpdateOutputChannels();
	void AssignDefaultSources();
	
private:
	struct output_chan_info {
		uint32 designation;	// only one bit set
		float gain;
		int source_count;
		float source_gain[MAX_SOURCES];
		int source_type[MAX_SOURCES];
	};	

	MixerCore 			*fCore;
	media_output 		fOutput;
	
	uint32				fOutputChannelCount;
	output_chan_info 	*fOutputChannelInfo; //array
	ByteSwap			*fOutputByteSwap;
	
	bool				fMuted;
};

inline uint32 MixerOutput::GetOutputChannelCount()
{
	return fOutputChannelCount;
}

inline float MixerOutput::GetOutputChannelGain(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 1.0;
	return fOutputChannelInfo[channel].gain;
}

inline uint32 MixerOutput::GetOutputChannelSourceCount(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 0;
	return fOutputChannelInfo[channel].source_count;
}

inline void MixerOutput::GetMixerChannelInfo(int channel, int index, int *type, float *gain)
{
	ASSERT(channel >= 0 && channel < fOutputChannelCount);
	ASSERT(index >= 0 && index < fOutputChannelInfo[channel].source_count);
	*type = fOutputChannelInfo[channel].source_type[index];
	*gain = fOutputChannelInfo[channel].source_gain[index];
}

inline void MixerOutput::AdjustByteOrder(BBuffer *buffer)
{
	if (fOutputByteSwap)
		fOutputByteSwap->Swap(buffer->Data(), buffer->SizeUsed());
}

inline bool MixerOutput::IsMuted()
{
	return fMuted;
}

#endif
