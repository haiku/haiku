/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

#include <Buffer.h>
#include "MixerDebug.h"
#include "ByteSwap.h"
#include "MixerCore.h"

/* The data storage for channel sources is optimized
 * for fast data retrieval by the MixerCore, which
 * will call GetOutputChannelSourceInfoAt() and
 * iterate through the first source_count array entries
 * for source_gain[] and source_type[] arrays, they are
 * index by 0 to GetOutputChannelSourceCount() - 1.
 * To allow gain change for not active sources,
 * we use the source_gain_cache[] array that is indexed
 * by source_type.
 */

class MixerOutput
{
	public:
										MixerOutput(MixerCore *core,
													const media_output &output);
										~MixerOutput();
	
		media_output&					MediaOutput();
		void							ChangeFormat(
											const media_multi_audio_format &format);

		// The physical output channels
		int								GetOutputChannelCount();
		int								GetOutputChannelType(int channel);
		void							SetOutputChannelGain(int channel,
															float gain);
		float							GetOutputChannelGain(int channel);
	
		// The sources for each channel
		void							AddOutputChannelSource(int channel,
															int source_type);
		void							RemoveOutputChannelSource(int channel,
															int source_type);
		void							SetOutputChannelSourceGain(int channel,
															int source_type,
															float source_gain);
		float							GetOutputChannelSourceGain(int channel,
															int source_type);
		bool							HasOutputChannelSource(int channel,
															int source_type);
	
		// The output can be muted
		void							SetMuted(bool yesno);
		bool							IsMuted();

		// Only for use by MixerCore:
		// For iteration of a channel's sources
		int								GetOutputChannelSourceCount(int channel);
		void							GetOutputChannelSourceInfoAt(int channel,
															int source_index,
															int *source_type,
															float *source_gain);

		// To swap byteorder in a buffer is that is needed
		void							AdjustByteOrder(BBuffer *buffer);

	private:
		void							UpdateByteOrderSwap();
		void							UpdateOutputChannels();
		void							AssignDefaultSources();

	private:

		// An entry in the source array is not the same as the
	 	// channel type, but the count should be the same
		enum {
			MAX_SOURCE_ENTRIES = MAX_CHANNEL_TYPES
		};

		struct output_chan_info {
				int		channel_type;
				float	channel_gain;
				int		source_count;
				float	source_gain[MAX_SOURCE_ENTRIES];
				int		source_type[MAX_SOURCE_ENTRIES];
				float	source_gain_cache[MAX_CHANNEL_TYPES];
		};	

		MixerCore						*fCore;
		media_output					fOutput;
		int								fOutputChannelCount;
		output_chan_info				*fOutputChannelInfo; //array
		ByteSwap						*fOutputByteSwap;
		bool							fMuted;
};


inline int 
MixerOutput::GetOutputChannelCount()
{
	return fOutputChannelCount;
}


inline float 
MixerOutput::GetOutputChannelGain(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 1.0;
	return fOutputChannelInfo[channel].channel_gain;
}


inline int 
MixerOutput::GetOutputChannelSourceCount(int channel)
{
	ASSERT(channel >= 0 && channel < fOutputChannelCount);
	return fOutputChannelInfo[channel].source_count;
}


inline void MixerOutput::GetOutputChannelSourceInfoAt(int channel,
													int source_index,
													int *source_type,
													float *source_gain)
{
	ASSERT(channel >= 0 && channel < fOutputChannelCount);
	ASSERT(source_index >= 0 && source_index < fOutputChannelInfo[channel].source_count);
	*source_type = fOutputChannelInfo[channel].source_type[source_index];
	*source_gain = fOutputChannelInfo[channel].source_gain[source_index];
}


inline void
MixerOutput::AdjustByteOrder(BBuffer *buffer)
{
	if (fOutputByteSwap)
		fOutputByteSwap->Swap(buffer->Data(), buffer->SizeUsed());
}

inline bool
MixerOutput::IsMuted()
{
	return fMuted;
}

#endif
