/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 */


#include "MixerOutput.h"

#include <MediaNode.h>

#include "MixerCore.h"
#include "MixerDebug.h"
#include "MixerUtils.h"


MixerOutput::MixerOutput(MixerCore *core, const media_output &output)
	:
	fCore(core),
	fOutput(output),
	fOutputChannelCount(0),
	fOutputChannelInfo(0),
	fOutputByteSwap(0),
	fMuted(false)
{
	fix_multiaudio_format(&fOutput.format.u.raw_audio);
	PRINT_OUTPUT("MixerOutput::MixerOutput", fOutput);
	PRINT_CHANNEL_MASK(fOutput.format);

	UpdateOutputChannels();
	UpdateByteOrderSwap();
}


MixerOutput::~MixerOutput()
{
	delete fOutputChannelInfo;
	delete fOutputByteSwap;
}


media_output &
MixerOutput::MediaOutput()
{
	return fOutput;
}


void
MixerOutput::ChangeFormat(const media_multi_audio_format &format)
{
	fOutput.format.u.raw_audio = format;
	fix_multiaudio_format(&fOutput.format.u.raw_audio);

	PRINT_OUTPUT("MixerOutput::ChangeFormat", fOutput);
	PRINT_CHANNEL_MASK(fOutput.format);

	UpdateOutputChannels();
	UpdateByteOrderSwap();
}


void
MixerOutput::UpdateByteOrderSwap()
{
	delete fOutputByteSwap;
	fOutputByteSwap = 0;

	// perhaps we need byte swapping
	if (fOutput.format.u.raw_audio.byte_order != B_MEDIA_HOST_ENDIAN) {
		if (	fOutput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
			 ||	fOutput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_INT
			 ||	fOutput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT) {
			fOutputByteSwap = new ByteSwap(fOutput.format.u.raw_audio.format);
		}
	}
}


void
MixerOutput::UpdateOutputChannels()
{
	output_chan_info *oldInfo = fOutputChannelInfo;
	int oldCount = fOutputChannelCount;

	fOutputChannelCount = fOutput.format.u.raw_audio.channel_count;
	fOutputChannelInfo = new output_chan_info[fOutputChannelCount];
	for (int i = 0; i < fOutputChannelCount; i++) {
		fOutputChannelInfo[i].channel_type = GetChannelType(i, fOutput.format.u.raw_audio.channel_mask);
		fOutputChannelInfo[i].channel_gain = 1.0;
		fOutputChannelInfo[i].source_count = 1;
		fOutputChannelInfo[i].source_gain[0] = 1.0;
		fOutputChannelInfo[i].source_type[0] = fOutputChannelInfo[i].channel_type;
		// all the cached values are 0.0 for a new channel
		for (int j = 0; j < MAX_CHANNEL_TYPES; j++)
			fOutputChannelInfo[i].source_gain_cache[j] = 0.0;
	}

	AssignDefaultSources();

	// apply old gains and sources, overriding the 1.0 gain defaults for the old channels
	if (oldInfo != 0 && oldCount != 0) {
		for (int i = 0; i < fOutputChannelCount; i++) {
			for (int j = 0; j < oldCount; j++) {
				if (fOutputChannelInfo[i].channel_type == oldInfo[j].channel_type) {
					fOutputChannelInfo[i].channel_gain = oldInfo[j].channel_gain;
					fOutputChannelInfo[i].source_count = oldInfo[j].source_count;
					for (int k = 0; k < fOutputChannelInfo[i].source_count; k++) {
						fOutputChannelInfo[i].source_gain[k] = oldInfo[j].source_gain[k];
						fOutputChannelInfo[i].source_type[k] = oldInfo[j].source_type[k];
					}
					// also copy the old gain cache
					for (int k = 0; k < MAX_CHANNEL_TYPES; k++)
						fOutputChannelInfo[i].source_gain_cache[k] = oldInfo[j].source_gain_cache[k];
					break;
				}
			}
		}
		// also delete the old info array
		delete [] oldInfo;
	}
	for (int i = 0; i < fOutputChannelCount; i++)
		TRACE("UpdateOutputChannels: output channel %d, type %2d, gain %.3f\n", i, fOutputChannelInfo[i].channel_type, fOutputChannelInfo[i].channel_gain);
}


void
MixerOutput::AssignDefaultSources()
{
	uint32 mask = fOutput.format.u.raw_audio.channel_mask;
	int count = fOutputChannelCount;

	// assign default sources for a few known setups,
	// everything else is left unchanged (it already is 1:1)
	if (count == 1 && mask & (B_CHANNEL_LEFT | B_CHANNEL_RIGHT)) {
		// we have only one phycial output channel, and use it as a mix of
		// left, right, rear-left, rear-right, center and sub
		TRACE("AssignDefaultSources: 1 channel setup\n");
		fOutputChannelInfo[0].source_count = 9;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 1.0;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[0].source_gain[2] = 0.8;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[0].source_gain[3] = 0.8;
		fOutputChannelInfo[0].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[0].source_gain[4] = 0.7;
		fOutputChannelInfo[0].source_type[4] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[0].source_gain[5] = 0.6;
		fOutputChannelInfo[0].source_type[5] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[0].source_gain[6] = 1.0;
		fOutputChannelInfo[0].source_type[6] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[0].source_gain[7] = 0.7;
		fOutputChannelInfo[0].source_type[7] = ChannelMaskToChannelType(B_CHANNEL_SIDE_LEFT);
		fOutputChannelInfo[0].source_gain[8] = 0.7;
		fOutputChannelInfo[0].source_type[8] = ChannelMaskToChannelType(B_CHANNEL_SIDE_RIGHT);
	} else if (count == 2 && mask == (B_CHANNEL_LEFT | B_CHANNEL_RIGHT)) {
		// we have have two phycial output channels
		TRACE("AssignDefaultSources: 2 channel setup\n");
		// left channel:
		fOutputChannelInfo[0].source_count = 6;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 0.8;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[0].source_gain[2] = 0.7;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[0].source_gain[3] = 0.6;
		fOutputChannelInfo[0].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[0].source_gain[4] = 1.0;
		fOutputChannelInfo[0].source_type[4] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[0].source_gain[5] = 0.7;
		fOutputChannelInfo[0].source_type[5] = ChannelMaskToChannelType(B_CHANNEL_SIDE_LEFT);
		// right channel:
		fOutputChannelInfo[1].source_count = 6;
		fOutputChannelInfo[1].source_gain[0] = 1.0;
		fOutputChannelInfo[1].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[1].source_gain[1] = 0.8;
		fOutputChannelInfo[1].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[1].source_gain[2] = 0.7;
		fOutputChannelInfo[1].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[1].source_gain[3] = 0.6;
		fOutputChannelInfo[1].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[1].source_gain[4] = 1.0;
		fOutputChannelInfo[1].source_type[4] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[1].source_gain[5] = 0.7;
		fOutputChannelInfo[1].source_type[5] = ChannelMaskToChannelType(B_CHANNEL_SIDE_RIGHT);
	} else if (count == 4 && mask == (B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT)) {
		TRACE("AssignDefaultSources: 4 channel setup\n");
		// left channel:
		fOutputChannelInfo[0].source_count = 5;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 0.7;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[0].source_gain[2] = 0.6;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[0].source_gain[3] = 1.0;
		fOutputChannelInfo[0].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[0].source_gain[4] = 0.6;
		fOutputChannelInfo[0].source_type[4] = ChannelMaskToChannelType(B_CHANNEL_SIDE_LEFT);
		// right channel:
		fOutputChannelInfo[1].source_count = 5;
		fOutputChannelInfo[1].source_gain[0] = 1.0;
		fOutputChannelInfo[1].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[1].source_gain[1] = 0.7;
		fOutputChannelInfo[1].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[1].source_gain[2] = 0.6;
		fOutputChannelInfo[1].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[1].source_gain[3] = 1.0;
		fOutputChannelInfo[1].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[1].source_gain[4] = 0.6;
		fOutputChannelInfo[1].source_type[4] = ChannelMaskToChannelType(B_CHANNEL_SIDE_RIGHT);
		// rear-left channel:
		fOutputChannelInfo[2].source_count = 4;
		fOutputChannelInfo[2].source_gain[0] = 1.0;
		fOutputChannelInfo[2].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[2].source_gain[1] = 0.6;
		fOutputChannelInfo[2].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[2].source_gain[2] = 0.9;
		fOutputChannelInfo[2].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[2].source_gain[3] = 0.6;
		fOutputChannelInfo[2].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SIDE_LEFT);
		// rear-right channel:
		fOutputChannelInfo[3].source_count = 4;
		fOutputChannelInfo[3].source_gain[0] = 1.0;
		fOutputChannelInfo[3].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[3].source_gain[1] = 0.6;
		fOutputChannelInfo[3].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[3].source_gain[2] = 0.9;
		fOutputChannelInfo[3].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[3].source_gain[3] = 0.6;
		fOutputChannelInfo[3].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SIDE_RIGHT);
	} else if (count == 5 && mask == (B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_CENTER)) {
		TRACE("AssignDefaultSources: 5 channel setup\n");
		// left channel:
		fOutputChannelInfo[0].source_count = 4;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 0.6;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[0].source_gain[2] = 1.0;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[0].source_gain[3] = 0.6;
		fOutputChannelInfo[0].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SIDE_LEFT);
		// right channel:
		fOutputChannelInfo[1].source_count = 4;
		fOutputChannelInfo[1].source_gain[0] = 1.0;
		fOutputChannelInfo[1].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[1].source_gain[1] = 0.6;
		fOutputChannelInfo[1].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[1].source_gain[2] = 1.0;
		fOutputChannelInfo[1].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[1].source_gain[3] = 0.6;
		fOutputChannelInfo[1].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SIDE_RIGHT);
		// rear-left channel:
		fOutputChannelInfo[2].source_count = 4;
		fOutputChannelInfo[2].source_gain[0] = 1.0;
		fOutputChannelInfo[2].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[2].source_gain[1] = 0.6;
		fOutputChannelInfo[2].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[2].source_gain[2] = 0.9;
		fOutputChannelInfo[2].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[2].source_gain[3] = 0.6;
		fOutputChannelInfo[2].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SIDE_LEFT);
		// rear-right channel:
		fOutputChannelInfo[3].source_count = 4;
		fOutputChannelInfo[3].source_gain[0] = 1.0;
		fOutputChannelInfo[3].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[3].source_gain[1] = 0.6;
		fOutputChannelInfo[3].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[3].source_gain[2] = 0.9;
		fOutputChannelInfo[3].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
		fOutputChannelInfo[3].source_gain[3] = 0.6;
		fOutputChannelInfo[3].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SIDE_RIGHT);
		// center channel:
		fOutputChannelInfo[4].source_count = 3;
		fOutputChannelInfo[4].source_gain[0] = 1.0;
		fOutputChannelInfo[4].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[4].source_gain[1] = 0.5;
		fOutputChannelInfo[4].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		fOutputChannelInfo[4].source_gain[2] = 0.8;
		fOutputChannelInfo[4].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_MONO);
	} else {
		TRACE("AssignDefaultSources: no default setup, adding mono channel to first two channels\n");
		if (count >= 1) {
			// this is not a left channel, but we add the mono channel anyway
			fOutputChannelInfo[0].source_gain[fOutputChannelInfo[0].source_count] = 1.0;
			fOutputChannelInfo[0].source_type[fOutputChannelInfo[0].source_count] = ChannelMaskToChannelType(B_CHANNEL_MONO);
			fOutputChannelInfo[0].source_count++;
		}
		if (count >= 2) {
			// this is not a right channel, but we add the mono channel anyway
			fOutputChannelInfo[1].source_gain[fOutputChannelInfo[1].source_count] = 1.0;
			fOutputChannelInfo[1].source_type[fOutputChannelInfo[1].source_count] = ChannelMaskToChannelType(B_CHANNEL_MONO);
			fOutputChannelInfo[1].source_count++;
		}
	}

	for (int i = 0; i < fOutputChannelCount; i++) {
		for (int j = 0; j < fOutputChannelInfo[i].source_count; j++) {
			TRACE("AssignDefaultSources: output channel %d, source index %d: source_type %2d, source_gain %.3f\n", i, j, fOutputChannelInfo[i].source_type[j], fOutputChannelInfo[i].source_gain[j]);
		}
	}
}


int
MixerOutput::GetOutputChannelType(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 0;
	return fOutputChannelInfo[channel].channel_type;
}


void
MixerOutput::SetOutputChannelGain(int channel, float gain)
{
	TRACE("SetOutputChannelGain chan %d, gain %.5f\n", channel, gain);
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	fOutputChannelInfo[channel].channel_gain = gain;
}


void
MixerOutput::AddOutputChannelSource(int channel, int source_type)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	if (source_type < 0 || source_type >= MAX_CHANNEL_TYPES)
		return;
	if (fOutputChannelInfo[channel].source_count == MAX_SOURCE_ENTRIES)
		return;
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type)
			return;
	}
	// when adding a new source, use the current gain value from cache
	float source_gain = fOutputChannelInfo[channel].source_gain_cache[source_type];
	fOutputChannelInfo[channel].source_type[fOutputChannelInfo[channel].source_count] = source_type;
	fOutputChannelInfo[channel].source_gain[fOutputChannelInfo[channel].source_count] = source_gain;
	fOutputChannelInfo[channel].source_count++;
}


void
MixerOutput::RemoveOutputChannelSource(int channel, int source_type)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			// when removing a source, save the current gain value into the cache
			fOutputChannelInfo[channel].source_gain_cache[source_type] = fOutputChannelInfo[channel].source_gain[i];
			// remove the entry
			fOutputChannelInfo[channel].source_type[i] = fOutputChannelInfo[channel].source_type[fOutputChannelInfo[channel].source_count - 1];
			fOutputChannelInfo[channel].source_gain[i] = fOutputChannelInfo[channel].source_gain[fOutputChannelInfo[channel].source_count - 1];
			fOutputChannelInfo[channel].source_count--;
			return;
		}
	}
}


void
MixerOutput::SetOutputChannelSourceGain(int channel, int source_type, float source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	// set gain for active source
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			fOutputChannelInfo[channel].source_gain[i] = source_gain;
			return;
		}
	}
	// we don't have an active source of that type, save gain in cache
	if (source_type < 0 || source_type >= MAX_CHANNEL_TYPES)
		return;
	fOutputChannelInfo[channel].source_gain_cache[source_type] = source_gain;
}


float
MixerOutput::GetOutputChannelSourceGain(int channel, int source_type)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 0.0;
	// get gain for active source
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			return fOutputChannelInfo[channel].source_gain[i];
		}
	}
	// we don't have an active source of that type, get gain from cache
	if (source_type < 0 || source_type >= MAX_CHANNEL_TYPES)
		return 0.0;
	return fOutputChannelInfo[channel].source_gain_cache[source_type];
}


bool
MixerOutput::HasOutputChannelSource(int channel, int source_type)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return false;
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			return true;
		}
	}
	return false;
}


void
MixerOutput::SetMuted(bool yesno)
{
	fMuted = yesno;
}
