/*
 * Copyright 2003-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 */


#include "MixerCore.h"

#include <Buffer.h>
#include <string.h>
#include <TimeSource.h> // TODO: debug only

#include "ByteSwap.h"
#include "Interpolate.h"
#include "MixerInput.h"
#include "MixerUtils.h"
#include "Resampler.h"


MixerInput::MixerInput(MixerCore* core, const media_input& input,
	float mixFrameRate, int32 mixFrameCount)
	:
	fCore(core),
 	fInput(input),
	fInputByteSwap(NULL),
	fEnabled(true),
	fInputChannelInfo(NULL),
	fInputChannelCount(0),
	fInputChannelMask(0),
	fMixerChannelInfo(0),
	fMixerChannelCount(0),
	fMixBuffer(NULL),
	fMixBufferFrameRate(0),
	fMixBufferFrameCount(0),
	fLastDataFrameWritten(-1),
	fLastDataAvailableTime(-1),
	fFractionalFrames(0.0),
	fResampler(NULL),
	fRtmPool(NULL),
	fUserOverridesChannelDestinations(false)
{
	fix_multiaudio_format(&fInput.format.u.raw_audio);
	PRINT_INPUT("MixerInput::MixerInput", fInput);
	PRINT_CHANNEL_MASK(fInput.format);

	ASSERT(fInput.format.u.raw_audio.channel_count > 0);

	for (int i = 0; i <	MAX_CHANNEL_TYPES; i++)
		fChannelTypeGain[i] = 1.0f;

	fInputChannelCount = fInput.format.u.raw_audio.channel_count;
	fInputChannelMask = fInput.format.u.raw_audio.channel_mask;
	fInputChannelInfo = new input_chan_info[fInputChannelCount];

	// perhaps we need byte swapping
	if (fInput.format.u.raw_audio.byte_order != B_MEDIA_HOST_ENDIAN) {
		if (fInput.format.u.raw_audio.format
				== media_raw_audio_format::B_AUDIO_FLOAT
			|| fInput.format.u.raw_audio.format
				== media_raw_audio_format::B_AUDIO_INT
			|| fInput.format.u.raw_audio.format
				== media_raw_audio_format::B_AUDIO_SHORT) {
			fInputByteSwap = new ByteSwap(fInput.format.u.raw_audio.format);
		}
	}

	// initialize fInputChannelInfo
	for (int i = 0; i < fInputChannelCount; i++) {
		fInputChannelInfo[i].buffer_base = 0;
			// will be set by SetMixBufferFormat()
		fInputChannelInfo[i].destination_mask = 0;
			// will be set by _UpdateInputChannelDestinationMask()
		fInputChannelInfo[i].gain = 1.0;
	}

	UpdateResamplingAlgorithm();

	// fMixerChannelInfo and fMixerChannelCount will be initialized by
	// _UpdateInputChannelDestinations()
	SetMixBufferFormat((int32)mixFrameRate, mixFrameCount);
}


MixerInput::~MixerInput()
{
	if (fMixBuffer)
		rtm_free(fMixBuffer);
	if (fRtmPool)
		rtm_delete_pool(fRtmPool);
	delete[] fInputChannelInfo;
	delete[] fMixerChannelInfo;

	// delete resamplers
	if (fResampler != NULL) {
		for (int i = 0; i < fInputChannelCount; i++)
			delete fResampler[i];
		delete[] fResampler;
	}
	delete fInputByteSwap;
}


int32
MixerInput::ID()
{
	return fInput.destination.id;
}


media_input&
MixerInput::MediaInput()
{
	return fInput;
}


void
MixerInput::BufferReceived(BBuffer* buffer)
{
	void* data;
	size_t size;
	bigtime_t start;
	bigtime_t buffer_duration;

	if (!fMixBuffer) {
		ERROR("MixerInput::BufferReceived: dropped incoming buffer as we "
			"don't have a mix buffer\n");
		return;
	}

	data = buffer->Data();
	size = buffer->SizeUsed();
	start = buffer->Header()->start_time;
	buffer_duration = duration_for_frames(fInput.format.u.raw_audio.frame_rate,
		size / bytes_per_frame(fInput.format.u.raw_audio));
	if (start < 0) {
		ERROR("MixerInput::BufferReceived: buffer with negative start time of "
			"%Ld dropped\n", start);
		return;
	}

	// swap the byte order of this buffer, if necessary
	if (fInputByteSwap)
		fInputByteSwap->Swap(data, size);

	int offset = frames_for_duration(fMixBufferFrameRate, start)
		% fMixBufferFrameCount;

	PRINT(4, "MixerInput::BufferReceived: buffer start %10Ld, offset %6d\n",
		start, offset);

	int in_frames = size / bytes_per_frame(fInput.format.u.raw_audio);
	double frames = double(in_frames * fMixBufferFrameRate)
		/ fInput.format.u.raw_audio.frame_rate;
	int out_frames = int(frames);
	fFractionalFrames += frames - double(out_frames);
	if (fFractionalFrames >= 1.0) {
		fFractionalFrames -= 1.0;
		out_frames++;
	}

	// if fLastDataFrameWritten != -1, then we have a valid last position
	// and can do glitch compensation
	if (fLastDataFrameWritten >= 0) {
		int expected_frame = (fLastDataFrameWritten + 1)
			% fMixBufferFrameCount;
		if (offset != expected_frame) {
			// due to rounding and other errors, offset might be off by +/- 1
			// this is not really a bad glitch, we just adjust the position
			if (offset == fLastDataFrameWritten) {
//				printf("MixerInput::BufferReceived: -1 frame GLITCH! last "
//					"frame was %ld, expected frame was %d, new frame is %d\n",
//					fLastDataFrameWritten, expected_frame, offset);
				offset = expected_frame;
			} else if (offset == ((fLastDataFrameWritten + 2)
				% fMixBufferFrameCount)) {
//				printf("MixerInput::BufferReceived: +1 frame GLITCH! last "
//					"frame was %ld, expected frame was %d, new frame is %d\n",
//					fLastDataFrameWritten, expected_frame, offset);
				offset = expected_frame;
			} else {
				printf("MixerInput::BufferReceived: GLITCH! last frame was "
					"%4ld, expected frame was %4d, new frame is %4d\n",
					fLastDataFrameWritten, expected_frame, offset);

				if (start > fLastDataAvailableTime) {
					if ((start - fLastDataAvailableTime)
						< (buffer_duration / 10)) {
						// buffer is less than 10% of buffer duration too late
						printf("short glitch, buffer too late, time delta "
							"%Ld\n", start - fLastDataAvailableTime);
						offset = expected_frame;
						out_frames++;
					} else {
						// buffer more than 10% of buffer duration too late
						// TODO: zerofill buffer
						printf("MAJOR glitch, buffer too late, time delta "
							"%Ld\n", start - fLastDataAvailableTime);
					}
				} else { // start <= fLastDataAvailableTime
					// the new buffer is too early
					if ((fLastDataAvailableTime - start)
						< (buffer_duration / 10)) {
						// buffer is less than 10% of buffer duration too early
						printf("short glitch, buffer too early, time delta "
							"%Ld\n", fLastDataAvailableTime - start);
						offset = expected_frame;
						out_frames--;
						if (out_frames < 1)
							out_frames = 1;
					} else {
						// buffer more than 10% of buffer duration too early
						// TODO: zerofill buffer
						printf("MAJOR glitch, buffer too early, time delta "
							"%Ld\n", fLastDataAvailableTime - start);
					}
				}
			}
		}
	}

//	printf("data arrived for %10Ld to %10Ld, storing at frames %ld to %ld\n",
//		start,
//		start + duration_for_frames(fInput.format.u.raw_audio.frame_rate,
//		frames_per_buffer(fInput.format.u.raw_audio)), offset,
//		offset + out_frames);
	if (offset + out_frames > fMixBufferFrameCount) {
		int out_frames1 = fMixBufferFrameCount - offset;
		int out_frames2 = out_frames - out_frames1;
		int in_frames1 = (out_frames1 * in_frames) / out_frames;
		int in_frames2 = in_frames - in_frames1;

//		printf("at %10Ld, data arrived for %10Ld to %10Ld, storing at "
//			"frames %ld to %ld and %ld to %ld\n", fCore->fTimeSource->Now(),
//			start,
//			start + duration_for_frames(fInput.format.u.raw_audio.frame_rate,
//			frames_per_buffer(fInput.format.u.raw_audio)), offset,
//			offset + out_frames1 - 1, 0, out_frames2 - 1);
		PRINT(3, "at %10Ld, data arrived for %10Ld to %10Ld, storing at "
			"frames %ld to %ld and %ld to %ld\n", fCore->fTimeSource->Now(),
			start,
			start + duration_for_frames(fInput.format.u.raw_audio.frame_rate,
			frames_per_buffer(fInput.format.u.raw_audio)), offset,
			offset + out_frames1 - 1, 0, out_frames2 - 1);
		PRINT(5, "  in_frames %5d, out_frames %5d, in_frames1 %5d, "
			"out_frames1 %5d, in_frames2 %5d, out_frames2 %5d\n",
			in_frames, out_frames, in_frames1, out_frames1, in_frames2,
			out_frames2);

		fLastDataFrameWritten = out_frames2 - 1;

		// convert offset from frames into bytes
		offset *= sizeof(float) * fInputChannelCount;

		for (int i = 0; i < fInputChannelCount; i++) {
			fResampler[i]->Resample(
				reinterpret_cast<char*>(data)
					+ i * bytes_per_sample(fInput.format.u.raw_audio),
				bytes_per_frame(fInput.format.u.raw_audio), in_frames1,
				reinterpret_cast<char*>(fInputChannelInfo[i].buffer_base)
					+ offset, fInputChannelCount * sizeof(float), out_frames1,
				fInputChannelInfo[i].gain);

			fResampler[i]->Resample(
				reinterpret_cast<char*>(data)
					+ i * bytes_per_sample(fInput.format.u.raw_audio)
					+ in_frames1 * bytes_per_frame(fInput.format.u.raw_audio),
				bytes_per_frame(fInput.format.u.raw_audio), in_frames2,
				reinterpret_cast<char*>(fInputChannelInfo[i].buffer_base),
				fInputChannelCount * sizeof(float), out_frames2,
				fInputChannelInfo[i].gain);

		}
	} else {
//		printf("at %10Ld, data arrived for %10Ld to %10Ld, storing at "
//			"frames %ld to %ld\n", fCore->fTimeSource->Now(), start,
//			start + duration_for_frames(fInput.format.u.raw_audio.frame_rate,
//			frames_per_buffer(fInput.format.u.raw_audio)), offset,
//			offset + out_frames - 1);
		PRINT(3, "at %10Ld, data arrived for %10Ld to %10Ld, storing at "
			"frames %ld to %ld\n", fCore->fTimeSource->Now(), start,
			start + duration_for_frames(fInput.format.u.raw_audio.frame_rate,
			frames_per_buffer(fInput.format.u.raw_audio)), offset,
			offset + out_frames - 1);
		PRINT(5, "  in_frames %5d, out_frames %5d\n", in_frames, out_frames);

		fLastDataFrameWritten = offset + out_frames - 1;
		// convert offset from frames into bytes
		offset *= sizeof(float) * fInputChannelCount;
		for (int i = 0; i < fInputChannelCount; i++) {
			fResampler[i]->Resample(
				reinterpret_cast<char*>(data)
					+ i * bytes_per_sample(fInput.format.u.raw_audio),
				bytes_per_frame(fInput.format.u.raw_audio), in_frames,
				reinterpret_cast<char*>(fInputChannelInfo[i].buffer_base)
					+ offset, fInputChannelCount * sizeof(float),
				out_frames, fInputChannelInfo[i].gain);
		}
	}
	fLastDataAvailableTime = start + buffer_duration;
}


void
MixerInput::UpdateResamplingAlgorithm()
{
	if (fResampler != NULL) {
		for (int i = 0; i < fInputChannelCount; i++)
			delete fResampler[i];
		delete[] fResampler;
	}
	// create resamplers
	fResampler = new Resampler*[fInputChannelCount];
	for (int i = 0; i < fInputChannelCount; i++) {
		switch (fCore->Settings()->ResamplingAlgorithm()) {
			case 2:
				fResampler[i] = new Interpolate(
					fInput.format.u.raw_audio.format,
					media_raw_audio_format::B_AUDIO_FLOAT);
				break;
			default:
				fResampler[i] = new Resampler(
					fInput.format.u.raw_audio.format,
					media_raw_audio_format::B_AUDIO_FLOAT);
		}
	}
}


int
MixerInput::GetInputChannelCount()
{
	return fInputChannelCount;
}


void
MixerInput::AddInputChannelDestination(int channel, int destination_type)
{
	uint32 mask = ChannelTypeToChannelMask(destination_type);

	if (channel < 0 || channel >= fInputChannelCount)
		return;

	// test if it is already set
	if (fInputChannelInfo[channel].destination_mask & mask)
		return;

	// verify that no other channel has id
	if (-1 != GetInputChannelForDestination(destination_type)) {
		ERROR("MixerInput::AddInputChannelDestination: destination_type %d "
			"already assigned to channel %d\n", destination_type,
			GetInputChannelForDestination(destination_type));
		return;
	}

	// add it to specified channel
	fInputChannelInfo[channel].destination_mask |= mask;

	fUserOverridesChannelDestinations = true;
	_UpdateInputChannelDestinations();
}


void
MixerInput::RemoveInputChannelDestination(int channel, int destination_type)
{
	uint32 mask = ChannelTypeToChannelMask(destination_type);

	if (channel < 0 || channel >= fInputChannelCount)
		return;

	// test if it is really set
	if ((fInputChannelInfo[channel].destination_mask & mask) == 0)
		return;

	// remove it from specified channel
	fInputChannelInfo[channel].destination_mask &= ~mask;

	fUserOverridesChannelDestinations = true;
	_UpdateInputChannelDestinations();
}


bool
MixerInput::HasInputChannelDestination(int channel, int destination_type)
{
	if (channel < 0 || channel >= fInputChannelCount)
		return false;
	if (destination_type < 0 || destination_type >= MAX_CHANNEL_TYPES)
		return false;
	return fInputChannelInfo[channel].destination_mask
		& ChannelTypeToChannelMask(destination_type);
}


int
MixerInput::GetInputChannelForDestination(int destination_type)
{
	if (destination_type < 0 || destination_type >= MAX_CHANNEL_TYPES)
		return -1;
	uint32 mask = ChannelTypeToChannelMask(destination_type);
	for (int chan = 0; chan < fInputChannelCount; chan++) {
		if (fInputChannelInfo[chan].destination_mask & mask)
			return chan;
	}
	return -1;
}


int
MixerInput::GetInputChannelType(int channel)
{
	if (channel < 0 || channel >= fInputChannelCount)
		return 0;
	return GetChannelType(channel, fInputChannelMask);
}


void
MixerInput::SetInputChannelGain(int channel, float gain)
{
	if (channel < 0 || channel >= fInputChannelCount)
		return;
	if (gain < 0.0f)
		gain = 0.0f;

	fInputChannelInfo[channel].gain = gain;
}


float
MixerInput::GetInputChannelGain(int channel)
{
	if (channel < 0 || channel >= fInputChannelCount)
		return 0.0f;
	return fInputChannelInfo[channel].gain;
}


void
MixerInput::_UpdateInputChannelDestinationMask()
{
	// is the user already messed with the assignmens, don't do anything.
	if (fUserOverridesChannelDestinations)
		return;

	TRACE("_UpdateInputChannelDestinationMask: enter\n");

	// first apply a 1:1 mapping
	for (int i = 0; i < fInputChannelCount; i++) {
		fInputChannelInfo[i].destination_mask = GetChannelMask(i,
			fInputChannelMask);
	}

	// specialize this, depending on the available physical output channels
	if (fCore->OutputChannelCount() <= 2) {
		// less or equal two channels
		if (fInputChannelCount == 1
			&& (GetChannelMask(0, fInputChannelMask)
				& (B_CHANNEL_LEFT | B_CHANNEL_RIGHT))) {
			fInputChannelInfo[0].destination_mask = B_CHANNEL_MONO;
		}
	} else {
		// more than two channel output card
		if (fInputChannelCount == 1
			&& (GetChannelMask(0, fInputChannelMask)
				& (B_CHANNEL_LEFT | B_CHANNEL_RIGHT))) {
			fInputChannelInfo[0].destination_mask = B_CHANNEL_MONO;
		}
		if (fInputChannelCount == 2
			&& (GetChannelMask(0, fInputChannelMask) & B_CHANNEL_LEFT)) {
			fInputChannelInfo[0].destination_mask
				= B_CHANNEL_LEFT | B_CHANNEL_REARLEFT;
		}
		if (fInputChannelCount == 2
			&& (GetChannelMask(0, fInputChannelMask) & B_CHANNEL_RIGHT)) {
			fInputChannelInfo[0].destination_mask
				= B_CHANNEL_RIGHT | B_CHANNEL_REARRIGHT;
		}
		if (fInputChannelCount == 2
			&& (GetChannelMask(1, fInputChannelMask) & B_CHANNEL_LEFT)) {
			fInputChannelInfo[1].destination_mask
				= B_CHANNEL_LEFT | B_CHANNEL_REARLEFT;
		}
		if (fInputChannelCount == 2
			&& (GetChannelMask(1, fInputChannelMask) & B_CHANNEL_RIGHT)) {
			fInputChannelInfo[1].destination_mask
				= B_CHANNEL_RIGHT | B_CHANNEL_REARRIGHT;
		}
	}

	for (int i = 0; i < fInputChannelCount; i++) {
		TRACE("_UpdateInputChannelDestinationMask: input channel %d, "
			"destination_mask 0x%08lX, base %p, gain %.3f\n", i,
			fInputChannelInfo[i].destination_mask,
			fInputChannelInfo[i].buffer_base, fInputChannelInfo[i].gain);
	}
	TRACE("_UpdateInputChannelDestinationMask: leave\n");
}


void
MixerInput::_UpdateInputChannelDestinations()
{
	int channel_count;
	uint32 all_bits;
	uint32 mask;

	TRACE("_UpdateInputChannelDestinations: enter\n");
	for (int i = 0; i < fInputChannelCount; i++) {
		TRACE("_UpdateInputChannelDestinations: input channel %d, "
			"destination_mask 0x%08lX, base %p, gain %.3f\n", i,
			fInputChannelInfo[i].destination_mask,
			fInputChannelInfo[i].buffer_base, fInputChannelInfo[i].gain);
	}

	all_bits = 0;
	for (int i = 0; i < fInputChannelCount; i++)
		all_bits |= fInputChannelInfo[i].destination_mask;

	TRACE("_UpdateInputChannelDestinations: all_bits = %08lx\n", all_bits);

	channel_count = count_nonzero_bits(all_bits);
	TRACE("_UpdateInputChannelDestinations: %d input channels, %d mixer "
		"channels (%d old)\n", fInputChannelCount, channel_count,
		fMixerChannelCount);
	if (channel_count != fMixerChannelCount) {
		delete [] fMixerChannelInfo;
		fMixerChannelInfo = new mixer_chan_info[channel_count];
		fMixerChannelCount = channel_count;
	}

	// assign each mixer channel one type
	// and the gain from the fChannelTypeGain[]
	mask = 1;
	for (int i = 0; i < fMixerChannelCount; i++) {
		while (mask != 0 && (all_bits & mask) == 0)
			mask <<= 1;
		fMixerChannelInfo[i].destination_type = ChannelMaskToChannelType(mask);
		fMixerChannelInfo[i].destination_gain
			= fChannelTypeGain[fMixerChannelInfo[i].destination_type];
		mask <<= 1;
	}

	// assign buffer_base pointer for each mixer channel
	for (int i = 0; i < fMixerChannelCount; i++) {
		int j;
		for (j = 0; j < fInputChannelCount; j++) {
			if (fInputChannelInfo[j].destination_mask
					& ChannelTypeToChannelMask(
						fMixerChannelInfo[i].destination_type)) {
				fMixerChannelInfo[i].buffer_base = fMixBuffer ? &fMixBuffer[j]
					: 0;
				break;
			}
		}
		if (j == fInputChannelCount) {
			ERROR("buffer assignment failed for mixer chan %d\n", i);
			fMixerChannelInfo[i].buffer_base = fMixBuffer;
		}
	}

	for (int i = 0; i < fMixerChannelCount; i++) {
		TRACE("_UpdateInputChannelDestinations: mixer channel %d, type %2d, "
			"base %p, gain %.3f\n", i, fMixerChannelInfo[i].destination_type,
			fMixerChannelInfo[i].buffer_base,
			fMixerChannelInfo[i].destination_gain);
	}

	TRACE("_UpdateInputChannelDestinations: leave\n");
}


// Note: The following code is outcommented on purpose
// and is about to be modified at a later point
/*
void
MixerInput::SetInputChannelDestinationGain(int channel, int destination_type,
	float gain)
{
	TRACE("SetInputChannelDestinationGain: channel %d, destination_type %d,
		gain %.4f\n", channel, destination_type, gain);
	// we don't need the channel, as each destination_type can only exist
	// once for each MixerInput, but we use it for parameter validation
	// and to have a interface similar to MixerOutput
	if (channel < 0 || channel >= fMixerChannelCount)
		return;
	if (destination_type < 0 || destination_type >= MAX_CHANNEL_TYPES)
		return;
	if (gain < 0.0f)
		gain = 0.0f;
	fChannelTypeGain[destination_type] = gain;
	for (int i = 0; i < fMixerChannelCount; i++) {
		if (fMixerChannelInfo[i].destination_type == destination_type) {
			fMixerChannelInfo[i].destination_gain = gain;
			return;
		}
	}
}


float
MixerInput::GetInputChannelDestinationGain(int channel, int destination_type)
{
	// we don't need the channel, as each destination_type can only exist
	// once for each MixerInput, but we use it for parameter validation
	// and to have a interface similar to MixerOutput
	if (channel < 0 || channel >= fMixerChannelCount)
		return 0.0f;
	if (destination_type < 0 || destination_type >= MAX_CHANNEL_TYPES)
		return 0.0f;
	return fChannelTypeGain[destination_type];
}
*/


void
MixerInput::SetMixerChannelGain(int mixer_channel, float gain)
{
	if (mixer_channel < 0 || mixer_channel >= fMixerChannelCount)
		return;
	if (gain < 0.0f)
		gain = 0.0f;

	fMixerChannelInfo[mixer_channel].destination_gain = gain;
	fChannelTypeGain[fMixerChannelInfo[mixer_channel].destination_type] = gain;
}


float
MixerInput::GetMixerChannelGain(int mixer_channel)
{
	if (mixer_channel < 0 || mixer_channel >= fMixerChannelCount)
		return 0.0;
	return fMixerChannelInfo[mixer_channel].destination_gain;
}


int
MixerInput::GetMixerChannelType(int mixer_channel)
{
	if (mixer_channel < 0 || mixer_channel >= fMixerChannelCount)
		return -1;
	return fMixerChannelInfo[mixer_channel].destination_type;
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
	TRACE("MixerInput::SetMixBufferFormat: framerate %ld, frames %ld\n",
		framerate, frames);

	fMixBufferFrameRate = framerate;
	fDebugMixBufferFrames = frames;

	// frames and/or framerate can be 0 (if no output is connected)
	if (framerate == 0 || frames == 0) {
		if (fMixBuffer != NULL) {
			rtm_free(fMixBuffer);
			fMixBuffer = NULL;
		}
		for (int i = 0; i < fInputChannelCount; i++)
			fInputChannelInfo[i].buffer_base = 0;
		fMixBufferFrameCount = 0;

		_UpdateInputChannelDestinationMask();
		_UpdateInputChannelDestinations();
		return;
	}

	// make fMixBufferFrameCount an integral multiple of frames,
	// but at least 3 times duration of our input buffer
	// and at least 2 times duration of the output buffer
	bigtime_t inputBufferLength  = duration_for_frames(
		fInput.format.u.raw_audio.frame_rate,
		frames_per_buffer(fInput.format.u.raw_audio));
	bigtime_t outputBufferLength = duration_for_frames(framerate, frames);
	bigtime_t mixerBufferLength
		= max_c(3 * inputBufferLength, 2 * outputBufferLength);
	int temp = frames_for_duration(framerate, mixerBufferLength);
	fMixBufferFrameCount = ((temp / frames) + 1) * frames;

	TRACE("  inputBufferLength    %10Ld\n", inputBufferLength);
	TRACE("  outputBufferLength   %10Ld\n", outputBufferLength);
	TRACE("  mixerBufferLength    %10Ld\n", mixerBufferLength);
	TRACE("  fMixBufferFrameCount %10d\n", fMixBufferFrameCount);

	ASSERT((fMixBufferFrameCount % frames) == 0);

	fLastDataFrameWritten = -1;
	fFractionalFrames = 0.0;

	rtm_free(fMixBuffer);
	rtm_delete_pool(fRtmPool);

	int size = sizeof(float) * fInputChannelCount * fMixBufferFrameCount;
	if (rtm_create_pool(&fRtmPool, size) != B_OK)
		fRtmPool = NULL;

	fMixBuffer = (float*)rtm_alloc(fRtmPool, size);
	if (fMixBuffer == NULL)
		return;

	memset(fMixBuffer, 0, size);

	for (int i = 0; i < fInputChannelCount; i++)
		fInputChannelInfo[i].buffer_base = &fMixBuffer[i];

	_UpdateInputChannelDestinationMask();
	_UpdateInputChannelDestinations();
}
