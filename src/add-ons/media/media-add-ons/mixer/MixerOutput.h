#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

#include "debug.h"
#include "ByteSwap.h"
#include "MixerCore.h"
#include <Buffer.h>

/* The data storage for channel sources is optimized
 * for fast data retrieval by the MixerCore, which
 * will call GetOutputChannelSourceInfoAt() and
 * iterate through the first source_count array entries
 * for source_gain[] and source_type[]
 */

class MixerOutput
{
public:
	MixerOutput(MixerCore *core, const media_output &output);
	~MixerOutput();
	
	media_output & MediaOutput();
	
	void ChangeFormat(const media_multi_audio_format &format);
	
	// The physical output channels
	uint32	GetOutputChannelCount();
	uint32	GetOutputChannelType(int channel);
	void	SetOutputChannelGain(int channel, float gain);
	float	GetOutputChannelGain(int channel);
	
	// The Sources for each channel
	void	AddOutputChannelSource(int channel, int source_type, float source_gain);
	void	RemoveOutputChannelSource(int channel, int source_type);
	bool	SetOutputChannelSourceGain(int channel, int source_type, float source_gain);
	float	GetOutputChannelSourceGain(int channel, int source_type);
	bool	HasOutputChannelSource(int channel, int source_type);
	
	// The Output can be muted
	void	SetMuted(bool yesno);
	bool	IsMuted();

	// Only for use by MixerCore:

	// For iteration of a channel's sources
	uint32	GetOutputChannelSourceCount(int channel);
	void	GetOutputChannelSourceInfoAt(int channel, int source_index, int *source_type, float *source_gain);

	// To swap byteorder in a buffer is that is needed
	void	AdjustByteOrder(BBuffer *buffer);

private:
	void UpdateByteOrderSwap();
	void UpdateOutputChannels();
	void AssignDefaultSources();
	
private:
	
	/* An entry in the source array is not the same as the
	 * channel type, but the number should be the same
	 */
	 enum {
		MAX_SOURCE_ENTRIES = MAX_CHANNEL_TYPES
	};

	struct output_chan_info {
		int		channel_type;
		float	channel_gain;
		int		source_count;
		float	source_gain[MAX_SOURCE_ENTRIES];
		int		source_type[MAX_SOURCE_ENTRIES];
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
	return fOutputChannelInfo[channel].channel_gain;
}

inline uint32 MixerOutput::GetOutputChannelSourceCount(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 0;
	return fOutputChannelInfo[channel].source_count;
}

inline void MixerOutput::GetOutputChannelSourceInfoAt(int channel, int source_index, int *source_type, float *source_gain)
{
	ASSERT(channel >= 0 && channel < fOutputChannelCount);
	ASSERT(index >= 0 && source_index < fOutputChannelInfo[channel].source_count);
	*source_type = fOutputChannelInfo[channel].source_type[source_index];
	*source_gain = fOutputChannelInfo[channel].source_gain[source_index];
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
