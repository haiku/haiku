#include <Buffer.h>
#include <string.h>
#include <TimeSource.h> // XXX debug only
#include "MixerInput.h"
#include "MixerCore.h"
#include "MixerUtils.h"
#include "Resampler.h"
#include "ByteSwap.h"
#include "debug.h"

template<class t> const t & max(const t &t1, const t &t2) { return (t1 > t2) ?  t1 : t2; }

MixerInput::MixerInput(MixerCore *core, const media_input &input, float mixFrameRate, int32 mixFrameCount)
 :	fCore(core),
 	fInput(input),
	fInputByteSwap(0),
	fEnabled(true),
	fInputChannelInfo(0),
	fInputChannelCount(0),
	fInputChannelMask(0),
	fMixerChannelInfo(0),
	fMixerChannelCount(0),
	fMixBuffer(0),
	fMixBufferFrameRate(0),
	fMixBufferFrameCount(0),
	fLastDataAvailableTime(-1),
	fResampler(0),
	fRtmPool(0),
	fUserOverridesChannelDesignations(false)
{
	fix_multiaudio_format(&fInput.format.u.raw_audio);
	PRINT_INPUT("MixerInput::MixerInput", fInput);
	PRINT_CHANNEL_MASK(fInput.format);
	
	ASSERT(fInput.format.u.raw_audio.channel_count > 0);

	fInputChannelCount = fInput.format.u.raw_audio.channel_count;
	fInputChannelMask = fInput.format.u.raw_audio.channel_mask;
	fInputChannelInfo = new input_chan_info[fInputChannelCount];
	
	// perhaps we need byte swapping
	if (fInput.format.u.raw_audio.byte_order != B_MEDIA_HOST_ENDIAN) {
		if (	fInput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
			 ||	fInput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_INT
			 ||	fInput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT) {
			fInputByteSwap = new ByteSwap(fInput.format.u.raw_audio.format);
		}
	}
	
	// initialize fInputChannelInfo
	for (int i = 0; i < fInputChannelCount; i++) {
		fInputChannelInfo[i].buffer_base = 0;	// will be set by SetMixBufferFormat()
		fInputChannelInfo[i].designations = 0;	// will be set by UpdateChannelDesignations()
		fInputChannelInfo[i].gain = 1.0;
	}

	// create resamplers
	fResampler = new Resampler * [fInputChannelCount];
	for (int i = 0; i < fInputChannelCount; i++)
		fResampler[i] = new Resampler(fInput.format.u.raw_audio.format, media_raw_audio_format::B_AUDIO_FLOAT);
	
	// fMixerChannelInfo and fMixerChannelCount will be initialized by UpdateMixerChannels()

	SetMixBufferFormat(mixFrameRate, mixFrameCount);
	
	// XXX a test:
/*
	SetMixerChannelGain(0, 0.222);
	SetMixerChannelGain(1, 0.444);
	AddInputChannelDesignation(0, B_CHANNEL_REARRIGHT);
	SetMixerChannelGain(2, 0.666);
	AddInputChannelDesignation(1, B_CHANNEL_REARLEFT);
*/
}

MixerInput::~MixerInput()
{
	if (fMixBuffer)
		rtm_free(fMixBuffer);
	if (fRtmPool)
		rtm_delete_pool(fRtmPool);
	delete [] fInputChannelInfo;
	delete [] fMixerChannelInfo;

	// delete resamplers
	for (int i = 0; i < fInputChannelCount; i++)
		delete fResampler[i];
	delete [] fResampler;
}
	
void
MixerInput::BufferReceived(BBuffer *buffer)
{
	void *data;
	size_t size;
	bigtime_t start;
	
	if (!fMixBuffer) {
		ERROR("MixerInput::BufferReceived: dropped incoming buffer as we don't have a mix buffer\n");
		return;
	}
	
	data = buffer->Data();
	size = buffer->SizeUsed();
	start = buffer->Header()->start_time;
	if (start < 0) {
		ERROR("MixerInput::BufferReceived: buffer with negative start time of %Ld dropped\n", start);
		return;
	}
	
	fLastDataAvailableTime = start + buffer_duration(fInput.format.u.raw_audio);

	// swap the byte order of this buffer, if necessary
	if (fInputByteSwap)
		fInputByteSwap->Swap(data, size);

	int32 offset = frames_for_duration(fMixBufferFrameRate, start) % fMixBufferFrameCount;

	PRINT(4, "MixerInput::BufferReceived: buffer start %10Ld, offset %6d\n", start, offset);

	int in_frames = size / bytes_per_frame(fInput.format.u.raw_audio);
	int out_frames = int((in_frames * fMixBufferFrameRate) / fInput.format.u.raw_audio.frame_rate); // XXX losing fractions
	// XXX we should better accumulate the fractions from previous buffers, and also check previous end offset
	// XXX to only add an output frame if needed (but we also need to use arrival times to cope with lost buffers).
	// XXX This workaround will very often (but not always) cause the first sample of the next buffer to overwrite
	// XXX the last sample of the current buffer (but that's better then having some random data at that place)
	if (fMixBufferFrameRate % int(0.5 + fInput.format.u.raw_audio.frame_rate) != 0)
		out_frames++;

	//printf("data arrived for %10Ld to %10Ld, storing at frames %ld to %ld\n", start, start + duration_for_frames(fInput.format.u.raw_audio.frame_rate, frames_per_buffer(fInput.format.u.raw_audio)), offset, offset + out_frames);
	
	if (offset + out_frames > fMixBufferFrameCount) {

		int out_frames1 = fMixBufferFrameCount - offset;
		int out_frames2 = out_frames - out_frames1;
		int in_frames1 = (out_frames1 * in_frames) / out_frames;
		int in_frames2 = in_frames - in_frames1;

		PRINT(3, "at %10Ld, data arrived for %10Ld to %10Ld, storing at frames %ld to %ld and %ld to %ld\n", fCore->fTimeSource->Now(), start, start + duration_for_frames(fInput.format.u.raw_audio.frame_rate, frames_per_buffer(fInput.format.u.raw_audio)), offset, offset + out_frames1 - 1, 0, out_frames2 - 1);

		PRINT(5, "  in_frames %5d, out_frames %5d, in_frames1 %5d, out_frames1 %5d, in_frames2 %5d, out_frames2 %5d\n",
			  in_frames, out_frames, in_frames1, out_frames1, in_frames2, out_frames2);

		offset *= sizeof(float) * fInputChannelCount; // convert offset from frames into bytes

		for (int i = 0; i < fInputChannelCount; i++) {
			fResampler[i]->Resample(reinterpret_cast<char *>(data) + i * bytes_per_sample(fInput.format.u.raw_audio),
									bytes_per_frame(fInput.format.u.raw_audio),
									in_frames1,
									reinterpret_cast<char *>(fInputChannelInfo[i].buffer_base) + offset,
									fInputChannelCount * sizeof(float),
									out_frames1,
									fInputChannelInfo[i].gain);
									
			fResampler[i]->Resample(reinterpret_cast<char *>(data) + i * bytes_per_sample(fInput.format.u.raw_audio) + in_frames1 * bytes_per_frame(fInput.format.u.raw_audio),
									bytes_per_frame(fInput.format.u.raw_audio),
									in_frames2,
									reinterpret_cast<char *>(fInputChannelInfo[i].buffer_base),
									fInputChannelCount * sizeof(float),
									out_frames2,
									fInputChannelInfo[i].gain);
		}
	} else {

		PRINT(3, "at %10Ld, data arrived for %10Ld to %10Ld, storing at frames %ld to %ld\n", fCore->fTimeSource->Now(), start, start + duration_for_frames(fInput.format.u.raw_audio.frame_rate, frames_per_buffer(fInput.format.u.raw_audio)), offset, offset + out_frames - 1);
		PRINT(5, "  in_frames %5d, out_frames %5d\n", in_frames, out_frames);

		offset *= sizeof(float) * fInputChannelCount; // convert offset from frames into bytes

		for (int i = 0; i < fInputChannelCount; i++) {
			fResampler[i]->Resample(reinterpret_cast<char *>(data) + i * bytes_per_sample(fInput.format.u.raw_audio),
									bytes_per_frame(fInput.format.u.raw_audio),
									in_frames,
									reinterpret_cast<char *>(fInputChannelInfo[i].buffer_base) + offset,
									fInputChannelCount * sizeof(float),
									out_frames,
									fInputChannelInfo[i].gain);
		}
	}
}

media_input &
MixerInput::MediaInput()
{
	return fInput;
}

int32
MixerInput::ID()
{
	return fInput.destination.id;
}

uint32
MixerInput::GetInputChannelCount()
{
	return fInputChannelCount;
}

void
MixerInput::AddInputChannelDesignation(int channel, uint32 des)
{
	ASSERT(count_nonzero_bits(des) == 1);
	
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return;

	// test if it is already set
	if (fInputChannelInfo[channel].designations & des)
		return;

	// remove it from all other channels that might have it
	for (int i = 0; i < fInputChannelCount; i++)
		fInputChannelInfo[i].designations &= ~des;
	
	// add it to specified channel
	fInputChannelInfo[channel].designations |= des;

	fUserOverridesChannelDesignations = true;
	UpdateMixerChannels();
}

void
MixerInput::RemoveInputChannelDesignation(int channel, uint32 des)
{
	ASSERT(count_nonzero_bits(des) == 1);

	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return;

	// test if it is really set
	if ((fInputChannelInfo[channel].designations & des) == 0)
		return;

	// remove it from specified channel
	fInputChannelInfo[channel].designations &= ~des;

	fUserOverridesChannelDesignations = true;
	UpdateMixerChannels();
}

uint32
MixerInput::GetInputChannelDesignations(int channel)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return 0;
	return fInputChannelInfo[channel].designations;
}

uint32
MixerInput::GetInputChannelType(int channel)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return 0;
	return GetChannelMask(channel, fInputChannelMask);
}

void
MixerInput::SetInputChannelGain(int channel, float gain)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return;
	if (gain < 0.0f)
		gain = 0.0f;
		
	fInputChannelInfo[channel].gain = gain;
}

float
MixerInput::GetInputChannelGain(int channel)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return 0.0f;
	return fInputChannelInfo[channel].gain;
}

void
MixerInput::UpdateChannelDesignations()
{
	// is the user already messed with the assignmens, don't do anything.
	if (fUserOverridesChannelDesignations)
		return;

	TRACE("UpdateChannelDesignations: enter\n");

	// first apply a 1:1 mapping
	for (int i = 0; i < fInputChannelCount; i++)
		fInputChannelInfo[i].designations = GetChannelMask(i, fInputChannelMask);
	
	// specialize this, depending on the available physical output channels
	switch (fCore->OutputChannelCount()) {
		case 0:
		case 1:
			break;
			
		case 2:
			if (fInputChannelCount == 1 && (GetChannelMask(0, fInputChannelMask) & (B_CHANNEL_LEFT | B_CHANNEL_RIGHT))) {
				fInputChannelInfo[0].designations = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
			}
			break;
			
		default:
			if (fInputChannelCount == 1 && (GetChannelMask(0, fInputChannelMask) & (B_CHANNEL_LEFT | B_CHANNEL_RIGHT))) {
				fInputChannelInfo[0].designations = B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT;
			}
			if (fInputChannelCount == 2 && (GetChannelMask(0, fInputChannelMask) & B_CHANNEL_LEFT)) {
				fInputChannelInfo[0].designations = B_CHANNEL_LEFT | B_CHANNEL_REARLEFT;
			}
			if (fInputChannelCount == 2 && (GetChannelMask(0, fInputChannelMask) & B_CHANNEL_RIGHT)) {
				fInputChannelInfo[0].designations = B_CHANNEL_RIGHT | B_CHANNEL_REARRIGHT;
			}
			if (fInputChannelCount == 2 && (GetChannelMask(1, fInputChannelMask) & B_CHANNEL_LEFT)) {
				fInputChannelInfo[1].designations = B_CHANNEL_LEFT | B_CHANNEL_REARLEFT;
			}
			if (fInputChannelCount == 2 && (GetChannelMask(1, fInputChannelMask) & B_CHANNEL_RIGHT)) {
				fInputChannelInfo[1].designations = B_CHANNEL_RIGHT | B_CHANNEL_REARRIGHT;
			}
			break;
	}

	for (int i = 0; i < fInputChannelCount; i++)
		TRACE("UpdateChannelDesignations: input channel %d, designations 0x%08X, base %p, gain %.3f\n", i, fInputChannelInfo[i].designations, fInputChannelInfo[i].buffer_base, fInputChannelInfo[i].gain);

	TRACE("UpdateChannelDesignations: leave\n");
}

void
MixerInput::UpdateMixerChannels()
{
	uint32 channel_count;
	uint32 all_bits;
	uint32 mask;
	
	mixer_chan_info *old_mixer_channel_info;
	uint32 old_mixer_channel_count;

	TRACE("UpdateMixerChannels: enter\n");
	
	for (int i = 0; i < fInputChannelCount; i++)
		TRACE("UpdateMixerChannels: input channel %d, designations 0x%08X, base %p, gain %.3f\n", i, fInputChannelInfo[i].designations, fInputChannelInfo[i].buffer_base, fInputChannelInfo[i].gain);
	
	all_bits = 0;
	for (int i = 0; i < fInputChannelCount; i++)
		all_bits |= fInputChannelInfo[i].designations;

	TRACE("UpdateMixerChannels: all_bits = %08x\n", all_bits);
	
	channel_count = count_nonzero_bits(all_bits);
		
	TRACE("UpdateMixerChannels: %ld input channels, %ld mixer channels (%ld old)\n", fInputChannelCount, channel_count, fMixerChannelCount);

	// If we resize the channel info array, we preserve the gain setting
	// by saving the old array until new assignments are finished, and
	// then applying the old gains. New gains are set to 1.0
	if (channel_count != fMixerChannelCount) {
		old_mixer_channel_info = fMixerChannelInfo;
		old_mixer_channel_count = fMixerChannelCount;
		fMixerChannelInfo = new mixer_chan_info[channel_count];
		fMixerChannelCount = channel_count;
		for (int i = 0; i < fMixerChannelCount; i++)
			fMixerChannelInfo[i].gain = 1.0;
	} else {
		old_mixer_channel_info = 0;
		old_mixer_channel_count = 0;
	}
	
	// assign each mixer channel one type
	for (int i  = 0, mask = 1; i < fMixerChannelCount; i++) {
		while (mask != 0 && (all_bits & mask) == 0)
			mask <<= 1;
		fMixerChannelInfo[i].type = ChannelMaskToChannelType(mask);
		mask <<= 1;
	}

	// assign buffer_base pointer for each mixer channel
	for (int i  = 0; i < fMixerChannelCount; i++) {
		int j;
		for (j = 0; j < fInputChannelCount; j++) {
			if (fInputChannelInfo[j].designations & ChannelTypeToChannelMask(fMixerChannelInfo[i].type)) {
				fMixerChannelInfo[i].buffer_base = fMixBuffer ? &fMixBuffer[j] : 0;
				break;
			}
		}
		if (j == fInputChannelCount) {
			ERROR("buffer assignment failed for mixer chan %d\n", i);
			fMixerChannelInfo[i].buffer_base = fMixBuffer;
		}
	}
	
	// apply old gains, overriding the 1.0 defaults for the old channels
	if (old_mixer_channel_info != 0) {
		for (int i  = 0; i < fMixerChannelCount; i++) {
			for (int j = 0; j < old_mixer_channel_count; j++) {
				if (fMixerChannelInfo[i].type == old_mixer_channel_info[j].type) {
					fMixerChannelInfo[i].gain = old_mixer_channel_info[j].gain;
					break;
				}
			}
		}
		// also delete the old info array
		delete [] old_mixer_channel_info;
	}

	for (int i = 0; i < fMixerChannelCount; i++)
		TRACE("UpdateMixerChannels: mixer channel %d, type %2d, des 0x%08X, base %p, gain %.3f\n", i, fMixerChannelInfo[i].type, ChannelTypeToChannelMask(fMixerChannelInfo[i].type), fMixerChannelInfo[i].buffer_base, fMixerChannelInfo[i].gain);

	TRACE("UpdateMixerChannels: leave\n");
}

uint32
MixerInput::GetMixerChannelCount()
{
	return fMixerChannelCount;
}

void
MixerInput::SetMixerChannelGain(int channel, float gain)
{
	TRACE("SetMixerChannelGain chan %d, gain %.5f\n", channel, gain);
	if (channel < 0 || channel >= fMixerChannelCount)
		return;
	if (gain < 0.0f)
		gain = 0.0f;
	fMixerChannelInfo[channel].gain = gain;
}
	
float
MixerInput::GetMixerChannelGain(int channel)
{
	if (channel < 0 || channel >= fMixerChannelCount)
		return 1.0;
	return fMixerChannelInfo[channel].gain;
}

void
MixerInput::SetEnabled(bool yesno)
{
	fEnabled = yesno;
}

bool
MixerInput::IsEnabled()
{
	return fEnabled;
}

void
MixerInput::SetMixBufferFormat(int32 framerate, int32 frames)
{
	TRACE("MixerInput::SetMixBufferFormat: framerate %ld, frames %ld\n", framerate, frames);

	fMixBufferFrameRate = framerate;
	debugMixBufferFrames = frames;

	// frames and/or framerate can be 0 (if no output is connected)
	if (framerate == 0 || frames == 0) {
		if (fMixBuffer) {
			rtm_free(fMixBuffer);
			fMixBuffer = 0;
		}
		for (int i = 0; i < fInputChannelCount; i++)
			fInputChannelInfo[i].buffer_base = 0;
		fMixBufferFrameCount = 0;

		UpdateChannelDesignations();
		UpdateMixerChannels();
		return;
	}

	// make fMixBufferFrameCount an integral multiple of frames,
	// but at least 3 times duration of our input buffer
	// and at least 2 times duration of the output buffer
	bigtime_t inputBufferLength  = duration_for_frames(fInput.format.u.raw_audio.frame_rate, frames_per_buffer(fInput.format.u.raw_audio));
	bigtime_t outputBufferLength = duration_for_frames(framerate, frames);
	bigtime_t mixerBufferLength = max_c(3 * inputBufferLength, 2 * outputBufferLength);
	int temp = frames_for_duration(framerate, mixerBufferLength);
	fMixBufferFrameCount = ((temp / frames) + 1) * frames;
	
	TRACE("  inputBufferLength  %10Ld\n", inputBufferLength);
	TRACE("  outputBufferLength %10Ld\n", outputBufferLength);
	TRACE("  mixerBufferLength  %10Ld\n", mixerBufferLength);
	TRACE("  fMixBufferFrameCount   %10ld\n", fMixBufferFrameCount);
	
	ASSERT((fMixBufferFrameCount % frames) == 0);
	
	if (fMixBuffer)
		rtm_free(fMixBuffer);
	if (fRtmPool)
		rtm_delete_pool(fRtmPool);
	int size = sizeof(float) * fInputChannelCount * fMixBufferFrameCount;
	if (B_OK != rtm_create_pool(&fRtmPool, size))
		fRtmPool = 0;
	fMixBuffer = (float *)rtm_alloc(fRtmPool, size);
	ASSERT(fMixBuffer);
	
	memset(fMixBuffer, 0, size); 

	for (int i = 0; i < fInputChannelCount; i++)
		fInputChannelInfo[i].buffer_base = &fMixBuffer[i];

	UpdateChannelDesignations();
	UpdateMixerChannels();
}
