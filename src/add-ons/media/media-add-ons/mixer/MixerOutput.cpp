#include <MediaNode.h>
#include "MixerOutput.h"
#include "MixerCore.h"
#include "MixerUtils.h"
#include "debug.h"

MixerOutput::MixerOutput(MixerCore *core, const media_output &output)
 :	fCore(core),
	fOutput(output),
	fOutputChannelCount(0),
	fOutputChannelInfo(0)
{
	fix_multiaudio_format(&fOutput.format.u.raw_audio);
	PRINT_OUTPUT("MixerOutput::MixerOutput", fOutput);
	PRINT_CHANNEL_MASK(fOutput.format);

	UpdateOutputChannels();
}

MixerOutput::~MixerOutput()
{
	delete fOutputChannelInfo;
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
}

void
MixerOutput::UpdateOutputChannels()
{
	output_chan_info *oldInfo = fOutputChannelInfo;
	uint32 oldCount = fOutputChannelCount;

	fOutputChannelCount = fOutput.format.u.raw_audio.channel_count;
	fOutputChannelInfo = new output_chan_info[fOutputChannelCount];
	for (int i = 0; i < fOutputChannelCount; i++) {
		fOutputChannelInfo[i].designation = GetChannelMask(i, fOutput.format.u.raw_audio.channel_mask);
		fOutputChannelInfo[i].gain = 1.0;
		fOutputChannelInfo[i].source_count = 1;
		fOutputChannelInfo[i].source_gain[0] = 1.0;
		fOutputChannelInfo[i].source_type[0] = ChannelMaskToChannelType(fOutputChannelInfo[i].designation);
	}
	AssignDefaultSources();
	
	// apply old gains and sources, overriding the 1.0 gain defaults for the old channels
	if (oldInfo != 0 && oldCount != 0) {
		for (int i = 0; i < fOutputChannelCount; i++) {
			for (int j = 0; j < oldCount; j++) {
				if (fOutputChannelInfo[i].designation == oldInfo[j].designation) {
					fOutputChannelInfo[i].gain == oldInfo[j].gain;
					fOutputChannelInfo[i].source_count == oldInfo[j].source_count;
					for (int k = 0; k < fOutputChannelInfo[i].source_count; k++) {
						fOutputChannelInfo[i].source_gain[k] == oldInfo[j].source_gain[k];
						fOutputChannelInfo[i].source_type[k] == oldInfo[j].source_type[k];
					}
					break;
				}
			}
		}
		// also delete the old info array
		delete [] oldInfo;
	}
	for (int i = 0; i < fOutputChannelCount; i++)
		printf("UpdateOutputChannels: output channel %d, des 0x%08X (type %2d), gain %.3f\n", i, fOutputChannelInfo[i].designation, ChannelMaskToChannelType(fOutputChannelInfo[i].designation), fOutputChannelInfo[i].gain);
}

void
MixerOutput::AssignDefaultSources()
{
	// XXX assign default sources
	for (int i = 0; i < fOutputChannelCount; i++) {
		for (int j = 0; j < fOutputChannelInfo[i].source_count; j++) {
			printf("AssignDefaultSources: output channel %d, source %d: des 0x%08X (type %2d), gain %.3f\n", i, j, ChannelTypeToChannelMask(fOutputChannelInfo[i].source_type[j]), fOutputChannelInfo[i].source_type[j], fOutputChannelInfo[i].source_gain[j]);
		}
	}
}

uint32
MixerOutput::GetOutputChannelDesignation(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 0;
	return fOutputChannelInfo[channel].designation;
}

void
MixerOutput::SetOutputChannelGain(int channel, float gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	fOutputChannelInfo[channel].gain = gain;
}

void
MixerOutput::AddOutputChannelSource(int channel, uint32 source_designation, float source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	if (fOutputChannelInfo[channel].source_count == MAX_SOURCES)
		return;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type)
			return;
	}
	fOutputChannelInfo[channel].source_type[fOutputChannelInfo[channel].source_count] = source_type;
	fOutputChannelInfo[channel].source_gain[fOutputChannelInfo[channel].source_count] = source_gain;
	fOutputChannelInfo[channel].source_count++;
}

void
MixerOutput::RemoveOutputChannelSource(int channel, uint32 source_designation)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			fOutputChannelInfo[channel].source_type[i] = fOutputChannelInfo[channel].source_type[fOutputChannelInfo[channel].source_count - 1];
			fOutputChannelInfo[channel].source_gain[i] = fOutputChannelInfo[channel].source_gain[fOutputChannelInfo[channel].source_count - 1];
			fOutputChannelInfo[channel].source_count--;
			return;
		}
	}
}

void
MixerOutput::SetOutputChannelSourceGain(int channel, uint32 source_designation, float source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			fOutputChannelInfo[channel].source_gain[i] = source_gain;
			return;
		}
	}
}

float
MixerOutput::GetOutputChannelSourceGain(int channel, uint32 source_designation)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 1.0;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			return fOutputChannelInfo[channel].source_gain[i];
		}
	}
	return 1.0;
}

void
MixerOutput::GetOutputChannelSourceAt(int channel, int index, uint32 *source_designation, float *source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	if (index >= fOutputChannelInfo[channel].source_count) {
		// ERROR
		*source_gain = 1.0;
		*source_designation = 0;
		return;
	}
	*source_gain = fOutputChannelInfo[channel].source_gain[index];
	*source_designation = ChannelTypeToChannelMask(fOutputChannelInfo[channel].source_type[index]);
}
