/* 
 * Copyright (c) 2004 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <FindDirectory.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "SoftSynth.h"

using namespace BPrivate;

//------------------------------------------------------------------------------

BSoftSynth::BSoftSynth()
{
	instrumentsFile = NULL;
	SetDefaultInstrumentsFile();

	sampleRate = 22050;
	interpMode = B_LINEAR_INTERPOLATION;
	maxVoices = 28;
	limiterThreshold = 7;
	reverbEnabled = true;
	reverbMode = B_REVERB_BALLROOM;
	volumeScale = 1.0;
}

//------------------------------------------------------------------------------

BSoftSynth::~BSoftSynth()
{
	Unload();
}

//------------------------------------------------------------------------------

void BSoftSynth::Unload(void)
{
	/* TODO: purge samples from memory */
	
	free(instrumentsFile);
	instrumentsFile = NULL;
}

//------------------------------------------------------------------------------

bool BSoftSynth::IsLoaded(void) const
{
	return instrumentsFile != NULL;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::SetDefaultInstrumentsFile()
{
	char buf[B_PATH_NAME_LENGTH + 1];

	if (B_OK == find_directory(
			B_SYNTH_DIRECTORY, NULL, false, buf, B_PATH_NAME_LENGTH))
	{
		strcat(buf, B_BIG_SYNTH_FILE);
		return SetInstrumentsFile(buf);
	}
	
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::SetInstrumentsFile(const char* path)
{
	if (path == NULL)
	{
		return B_BAD_VALUE;
	}
	
	if (IsLoaded())
	{
		Unload();
	}
	
	instrumentsFile = strdup(path);
	return B_OK;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::LoadAllInstruments()
{
	/* TODO: Should load all of the instruments from the sample bank. */

	UNIMPLEMENTED
	return B_OK;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::LoadInstrument(int16 instrument)
{
	UNIMPLEMENTED
	return B_OK;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::UnloadInstrument(int16 instrument)
{
	UNIMPLEMENTED
	return B_OK;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::RemapInstrument(int16 from, int16 to)
{
	UNIMPLEMENTED
	return B_OK;
}

//------------------------------------------------------------------------------

void BSoftSynth::FlushInstrumentCache(bool startStopCache)
{
	// TODO: we may decide not to support this function because it's weird!
	
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::SetVolume(double volume)
{
	if (volume >= 0.0)
	{
		volumeScale = volume;
	}
}

//------------------------------------------------------------------------------

double BSoftSynth::Volume(void) const
{
	return volumeScale;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::SetSamplingRate(int32 rate)
{
	// TODO: According to the BeBook, we should round rate to the nearest
	// acceptable value. However, this function may change depending on the 
	// softsynth back-end we'll use.

	if (rate == 11025 || rate == 22050 || rate == 44100)
	{
		sampleRate = rate;
		return B_OK;
	}
	
	return B_BAD_VALUE;
}

//------------------------------------------------------------------------------

int32 BSoftSynth::SamplingRate() const
{
	return sampleRate;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::SetInterpolation(interpolation_mode mode)
{
	// TODO: this function could change depending on the synth back-end.
	
	interpMode = mode;
	return B_OK;
}

//------------------------------------------------------------------------------

interpolation_mode BSoftSynth::Interpolation() const
{
	return interpMode;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::EnableReverb(bool enabled)
{
	reverbEnabled = enabled;
	return B_OK;
}

//------------------------------------------------------------------------------

bool BSoftSynth::IsReverbEnabled() const
{
	return reverbEnabled;
}

//------------------------------------------------------------------------------

void BSoftSynth::SetReverb(reverb_mode mode)
{
	// TODO: this function could change depending on the synth back-end.

	reverbMode = mode;
}

//------------------------------------------------------------------------------

reverb_mode BSoftSynth::Reverb() const
{
	return reverbMode;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::SetMaxVoices(int32 max)
{
	// TODO: this function could change depending on the synth back-end.

	if (max > 0 && max <= 32)
	{
		maxVoices = max;
		return B_OK;
	}
	
	return B_BAD_VALUE;
}

//------------------------------------------------------------------------------

int16 BSoftSynth::MaxVoices(void) const
{
	return maxVoices;
}

//------------------------------------------------------------------------------

status_t BSoftSynth::SetLimiterThreshold(int32 threshold)
{
	// TODO: this function could change depending on the synth back-end.

	if (threshold > 0 && threshold <= 32)
	{
		limiterThreshold = threshold;
		return B_OK;
	}
	
	return B_BAD_VALUE;
}

//------------------------------------------------------------------------------

int16 BSoftSynth::LimiterThreshold(void) const
{
	return limiterThreshold;
}

//------------------------------------------------------------------------------

void BSoftSynth::Pause(void)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::Resume(void)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::ChannelPressure(
	uchar channel, uchar pressure, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::PitchBend(
	uchar channel, uchar lsb, uchar msb, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::SystemExclusive(void* data, size_t length, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::SystemRealTime(uchar status, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::TempoChange(int32 beatsPerMinute, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSoftSynth::AllNotesOff(bool justChannel, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------
