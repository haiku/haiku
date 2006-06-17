/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
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

#include <string.h>

#include "debug.h"
#include <Path.h>
#include <Synth.h>
#include "SoftSynth.h"

BSynth* be_synth = NULL;

using namespace BPrivate;

//------------------------------------------------------------------------------

BSynth::BSynth()
{
	Init();
}

//------------------------------------------------------------------------------

BSynth::BSynth(synth_mode mode)
{
	Init();
	synthMode = mode;
}

//------------------------------------------------------------------------------

BSynth::~BSynth()
{
	delete synth;
	be_synth = NULL;
}

//------------------------------------------------------------------------------

status_t BSynth::LoadSynthData(entry_ref* instrumentsFile)
{
	if (instrumentsFile == NULL) {
		return B_BAD_VALUE;
	}
	
	BPath path(instrumentsFile);
	status_t err = path.InitCheck();
	if (err != B_OK)
		return err;
	return synth->SetInstrumentsFile(path.Path());
}

//------------------------------------------------------------------------------

status_t BSynth::LoadSynthData(synth_mode mode)
{
	// Our softsynth doesn't support multiple modes like Be's synth did.
	// Therefore, if you use this function, the synth will revert to its
	// default instruments bank. However, we do keep track of the current
	// synth_mode here, in order not to confuse old applications.

	synthMode = mode;
	if (synthMode == B_SAMPLES_ONLY)
	{
		fprintf(stderr, "[midi] LoadSynthData: BSamples is not supported\n");
	}

	return synth->SetDefaultInstrumentsFile();
}

//------------------------------------------------------------------------------

synth_mode BSynth::SynthMode(void)
{
	return synthMode;
}

//------------------------------------------------------------------------------

void BSynth::Unload(void)
{
	synth->Unload();
}

//------------------------------------------------------------------------------

bool BSynth::IsLoaded(void) const
{
	return synth->IsLoaded();
}

//------------------------------------------------------------------------------

status_t BSynth::SetSamplingRate(int32 sample_rate)
{
	return synth->SetSamplingRate(sample_rate);
}

//------------------------------------------------------------------------------

int32 BSynth::SamplingRate() const
{
	return synth->SamplingRate();
}

//------------------------------------------------------------------------------

status_t BSynth::SetInterpolation(interpolation_mode interp_mode)
{
	return synth->SetInterpolation(interp_mode);
}

//------------------------------------------------------------------------------

interpolation_mode BSynth::Interpolation() const
{
	return synth->Interpolation();
}

//------------------------------------------------------------------------------

void BSynth::SetReverb(reverb_mode rev_mode)
{
	synth->SetReverb(rev_mode);
}

//------------------------------------------------------------------------------

reverb_mode BSynth::Reverb() const
{
	return synth->Reverb();
}

//------------------------------------------------------------------------------

status_t BSynth::EnableReverb(bool reverb_enabled)
{
	return synth->EnableReverb(reverb_enabled);
}

//------------------------------------------------------------------------------

bool BSynth::IsReverbEnabled() const
{
	return synth->IsReverbEnabled();
}

//------------------------------------------------------------------------------

status_t BSynth::SetVoiceLimits(
	int16 maxSynthVoices, int16 maxSampleVoices, int16 limiterThreshhold)
{
	status_t err = B_OK;
	err = synth->SetMaxVoices(maxSynthVoices);
	if (err == B_OK)
	{
		err = synth->SetLimiterThreshold(limiterThreshhold);
	}
	return err;
}

//------------------------------------------------------------------------------

int16 BSynth::MaxSynthVoices(void) const
{
	return synth->MaxVoices();
}

//------------------------------------------------------------------------------

int16 BSynth::MaxSampleVoices(void) const
{
	fprintf(stderr, "[midi] MaxSampleVoices: BSamples not supported\n");
	return 0;
}

//------------------------------------------------------------------------------

int16 BSynth::LimiterThreshhold(void) const
{
	return synth->LimiterThreshold();
}

//------------------------------------------------------------------------------

void BSynth::SetSynthVolume(double theVolume)
{
	synth->SetVolume(theVolume);
}

//------------------------------------------------------------------------------

double BSynth::SynthVolume(void) const
{
	return synth->Volume();
}

//------------------------------------------------------------------------------

void BSynth::SetSampleVolume(double theVolume)
{
	fprintf(stderr, "[midi] SetSampleVolume: BSamples not supported\n");
}

//------------------------------------------------------------------------------

double BSynth::SampleVolume(void) const
{
	fprintf(stderr, "[midi] SampleVolume: BSamples not supported\n");
	return 0;
}

//------------------------------------------------------------------------------

status_t BSynth::GetAudio(
	int16* pLeft, int16* pRight, int32 max_samples) const
{
	// We don't print a "not supported" message here. That would cause
	// significant slowdowns because applications ask for this many times.
	
	memset(pLeft, 0, max_samples * sizeof(int16));
	memset(pRight, 0, max_samples * sizeof(int16));
	
	return max_samples;
}

//------------------------------------------------------------------------------

void BSynth::Pause(void)
{
	synth->Pause();
}

//------------------------------------------------------------------------------

void BSynth::Resume(void)
{
	synth->Resume();
}

//------------------------------------------------------------------------------

void BSynth::SetControllerHook(int16 controller, synth_controller_hook cback)
{
	fprintf(stderr, "[midi] SetControllerHook is not supported\n");
}

//------------------------------------------------------------------------------

void BSynth::Init()
{
	delete be_synth;
	be_synth = this;
	synthMode = B_NO_SYNTH;
	clientCount = 0;
	synth = new BSoftSynth();
}

//------------------------------------------------------------------------------

int32 BSynth::CountClients(void) const
{
	return clientCount;
}

//------------------------------------------------------------------------------

void BSynth::_ReservedSynth1() { }
void BSynth::_ReservedSynth2() { }
void BSynth::_ReservedSynth3() { }
void BSynth::_ReservedSynth4() { }

//------------------------------------------------------------------------------
