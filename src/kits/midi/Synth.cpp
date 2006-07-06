/*
 * Copyright 2006, Haiku.
 * 
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Leveque
 *		Matthijs Hollemans
 */

#include <string.h>

#include "debug.h"
#include <Path.h>
#include <Synth.h>
#include "SoftSynth.h"

BSynth* be_synth = NULL;

using namespace BPrivate;


BSynth::BSynth()
{
	_Init();
}


BSynth::BSynth(synth_mode mode)
{
	_Init();
	fSynthMode = mode;
}


BSynth::~BSynth()
{
	delete fSynth;
	be_synth = NULL;
}


status_t 
BSynth::LoadSynthData(entry_ref* instrumentsFile)
{
	if (instrumentsFile == NULL) {
		return B_BAD_VALUE;
	}
	
	BPath path(instrumentsFile);
	status_t err = path.InitCheck();
	if (err != B_OK)
		return err;
	return fSynth->SetInstrumentsFile(path.Path());
}


status_t 
BSynth::LoadSynthData(synth_mode mode)
{
	// Our softsynth doesn't support multiple modes like Be's synth did.
	// Therefore, if you use this function, the synth will revert to its
	// default instruments bank. However, we do keep track of the current
	// synth_mode here, in order not to confuse old applications.

	fSynthMode = mode;
	if (fSynthMode == B_SAMPLES_ONLY) {
		fprintf(stderr, "[midi] LoadSynthData: BSamples is not supported\n");
	}

	return fSynth->SetDefaultInstrumentsFile();
}


synth_mode 
BSynth::SynthMode()
{
	return fSynthMode;
}


void 
BSynth::Unload()
{
	fSynth->Unload();
}


bool 
BSynth::IsLoaded() const
{
	return fSynth->IsLoaded();
}


status_t 
BSynth::SetSamplingRate(int32 sample_rate)
{
	return fSynth->SetSamplingRate(sample_rate);
}


int32 
BSynth::SamplingRate() const
{
	return fSynth->SamplingRate();
}


status_t 
BSynth::SetInterpolation(interpolation_mode interp_mode)
{
	return fSynth->SetInterpolation(interp_mode);
}


interpolation_mode 
BSynth::Interpolation() const
{
	return fSynth->Interpolation();
}


void 
BSynth::SetReverb(reverb_mode rev_mode)
{
	fSynth->SetReverb(rev_mode);
}


reverb_mode 
BSynth::Reverb() const
{
	return fSynth->Reverb();
}


status_t 
BSynth::EnableReverb(bool reverb_enabled)
{
	return fSynth->EnableReverb(reverb_enabled);
}


bool 
BSynth::IsReverbEnabled() const
{
	return fSynth->IsReverbEnabled();
}


status_t 
BSynth::SetVoiceLimits(
	int16 maxSynthVoices, int16 maxSampleVoices, int16 limiterThreshhold)
{
	status_t err = B_OK;
	err = fSynth->SetMaxVoices(maxSynthVoices);
	if (err == B_OK) {
		err = fSynth->SetLimiterThreshold(limiterThreshhold);
	}
	return err;
}


int16 
BSynth::MaxSynthVoices() const
{
	return fSynth->MaxVoices();
}


int16 
BSynth::MaxSampleVoices() const
{
	fprintf(stderr, "[midi] MaxSampleVoices: BSamples not supported\n");
	return 0;
}


int16 
BSynth::LimiterThreshhold() const
{
	return fSynth->LimiterThreshold();
}


void 
BSynth::SetSynthVolume(double theVolume)
{
	fSynth->SetVolume(theVolume);
}


double 
BSynth::SynthVolume() const
{
	return fSynth->Volume();
}


void 
BSynth::SetSampleVolume(double theVolume)
{
	fprintf(stderr, "[midi] SetSampleVolume: BSamples not supported\n");
}


double 
BSynth::SampleVolume(void) const
{
	fprintf(stderr, "[midi] SampleVolume: BSamples not supported\n");
	return 0;
}


status_t 
BSynth::GetAudio(int16* pLeft, int16* pRight, int32 max_samples) const
{
	// We don't print a "not supported" message here. That would cause
	// significant slowdowns because applications ask for this many times.
	
	memset(pLeft, 0, max_samples * sizeof(int16));
	memset(pRight, 0, max_samples * sizeof(int16));
	
	return max_samples;
}


void 
BSynth::Pause()
{
	fSynth->Pause();
}


void 
BSynth::Resume()
{
	fSynth->Resume();
}


void 
BSynth::SetControllerHook(int16 controller, synth_controller_hook cback)
{
	fprintf(stderr, "[midi] SetControllerHook is not supported\n");
}


void 
BSynth::_Init()
{
	delete be_synth;
	be_synth = this;
	fSynthMode = B_NO_SYNTH;
	fClientCount = 0;
	fSynth = new BSoftSynth();
}


int32 
BSynth::CountClients() const
{
	return fClientCount;
}


void BSynth::_ReservedSynth1() { }
void BSynth::_ReservedSynth2() { }
void BSynth::_ReservedSynth3() { }
void BSynth::_ReservedSynth4() { }

