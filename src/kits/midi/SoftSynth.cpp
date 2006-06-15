/* 
 * Copyright (c) 2004-2005 Matthijs Hollemans
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

#include <MidiRoster.h>
#include <MidiConsumer.h>
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "MidiGlue.h"   // for MAKE_BIGTIME
#include "SoftSynth.h"

using namespace BPrivate;


BSoftSynth::BSoftSynth()
: 	fSynth(NULL),
	fSettings(NULL),
	fSoundPlayer(NULL)
{
	fInstrumentsFile = NULL;
	SetDefaultInstrumentsFile();

	fSampleRate = 44100;
	fInterpMode = B_LINEAR_INTERPOLATION;
	fMaxVoices = 256;
	fLimiterThreshold = 7;
	fReverbEnabled = true;
	fReverbMode = B_REVERB_BALLROOM;

	Init();
}


BSoftSynth::~BSoftSynth()
{
	// Note: it is possible that we don't get deleted. When BSynth is
	// created, it is assigned to the global variable be_synth. While
	// BSynth is alive, it keeps a copy of BSoftSynth around too. Not
	// a big deal, but the Midi Kit will complain (on stdout) that we 
	// didn't release our endpoints.

	Unload();
	Done();
}


bool 
BSoftSynth::InitCheck(void) const
{
	return fInitCheck;
}


void 
BSoftSynth::Unload(void)
{
	/* TODO: purge samples from memory */
	
	free(fInstrumentsFile);
	fInstrumentsFile = NULL;
}


bool 
BSoftSynth::IsLoaded(void) const
{
	return fInstrumentsFile != NULL;
}


status_t 
BSoftSynth::SetDefaultInstrumentsFile()
{
	BPath path;
	if (B_OK == find_directory(B_SYNTH_DIRECTORY, &path, false, NULL)) {
		path.Append(B_BIG_SYNTH_FILE);
		return SetInstrumentsFile(path.Path());
	}
	
	return B_ERROR;
}


status_t 
BSoftSynth::SetInstrumentsFile(const char* path)
{
	if (path == NULL)
		return B_BAD_VALUE;
	
	if (IsLoaded())
		Unload();
	
	fInstrumentsFile = strdup(path);
	return B_OK;
}


status_t 
BSoftSynth::LoadAllInstruments()
{
	/* TODO: Should load all of the instruments from the sample bank. */

	UNIMPLEMENTED
	return B_OK;
}


status_t 
BSoftSynth::LoadInstrument(int16 instrument)
{
	UNIMPLEMENTED
	return B_OK;
}


status_t 
BSoftSynth::UnloadInstrument(int16 instrument)
{
	UNIMPLEMENTED
	return B_OK;
}


status_t 
BSoftSynth::RemapInstrument(int16 from, int16 to)
{
	UNIMPLEMENTED
	return B_OK;
}


void 
BSoftSynth::FlushInstrumentCache(bool startStopCache)
{
	// TODO: we may decide not to support this function because it's weird!
	
	UNIMPLEMENTED
}


void 
BSoftSynth::SetVolume(double volume)
{
	if (volume >= 0.0) {
		fluid_synth_set_gain(fSynth, volume);
	}
}


double 
BSoftSynth::Volume(void) const
{
	return fluid_synth_get_gain(fSynth);
}


status_t 
BSoftSynth::SetSamplingRate(int32 rate)
{
	if (rate == 22050 || rate == 44100 || rate == 48000) {
		fSampleRate = rate;
		Init();
		return B_OK;
	}
	
	return B_BAD_VALUE;
}


int32 
BSoftSynth::SamplingRate() const
{
	return fSampleRate;
}


status_t 
BSoftSynth::SetInterpolation(interpolation_mode mode)
{
	// not used because our synth uses the same format than the soundplayer
	fInterpMode = mode;
	return B_OK;
}


interpolation_mode 
BSoftSynth::Interpolation() const
{
	return fInterpMode;
}


status_t 
BSoftSynth::EnableReverb(bool enabled)
{
	fReverbEnabled = enabled;
	fluid_synth_set_reverb_on(fSynth, enabled);
	return B_OK;
}


bool 
BSoftSynth::IsReverbEnabled() const
{
	return fReverbEnabled;
}


void 
BSoftSynth::SetReverb(reverb_mode mode)
{
	// TODO: this function could change depending on the synth back-end.

	fReverbMode = mode;
}


reverb_mode 
BSoftSynth::Reverb() const
{
	return fReverbMode;
}


status_t 
BSoftSynth::SetMaxVoices(int32 max)
{
	if (max > 0 && max <= 4096) {
		fMaxVoices = max;
		Init();
		return B_OK;
	}
	
	return B_BAD_VALUE;
}


int16 
BSoftSynth::MaxVoices(void) const
{
	return fMaxVoices;
}


status_t 
BSoftSynth::SetLimiterThreshold(int32 threshold)
{
	// not used
	if (threshold > 0 && threshold <= 32) {
		fLimiterThreshold = threshold;
		return B_OK;
	}
	
	return B_BAD_VALUE;
}


int16 
BSoftSynth::LimiterThreshold(void) const
{
	return fLimiterThreshold;
}


void 
BSoftSynth::Pause(void)
{
	UNIMPLEMENTED
}


void 
BSoftSynth::Resume(void)
{
	UNIMPLEMENTED
}


void 
BSoftSynth::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		fluid_synth_noteoff(fSynth, channel - 1, note);	// velocity isn't used in FS
	}
}


void 
BSoftSynth::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		fluid_synth_noteon(fSynth, channel - 1, note, velocity);
	}
}


void 
BSoftSynth::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		// unavailable
	}
}


void 
BSoftSynth::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		fluid_synth_cc(fSynth, channel - 1, controlNumber, controlValue);
	}
}


void 
BSoftSynth::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		fluid_synth_program_change(fSynth, channel - 1, programNumber);
	}
}


void 
BSoftSynth::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		//unavailable
	}
}


void 
BSoftSynth::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		// fluid_synth only accepts an int
		fluid_synth_pitch_bend(fSynth, channel - 1, ((uint32)(msb & 0x7f) << 8) | (lsb & 0x7f));
	}
}


void 
BSoftSynth::SystemExclusive(void* data, size_t length, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		// unsupported
	}
}


void 
BSoftSynth::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		// unsupported
	}
}


void 
BSoftSynth::SystemRealTime(uchar status, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		// unsupported
	}
}


void 
BSoftSynth::TempoChange(int32 beatsPerMinute, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
		// unsupported
	}
}


void 
BSoftSynth::AllNotesOff(bool justChannel, uint32 time)
{
	if (InitCheck()) {
		snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);

		// from BMidi::AllNotesOff
		for (uchar channel = 1; channel <= 16; ++channel) {
			fluid_synth_cc(fSynth, channel - 1, B_ALL_NOTES_OFF, 0);
	
			if (!justChannel) {
				for (uchar note = 0; note <= 0x7F; ++note) {
					fluid_synth_noteoff(fSynth, channel - 1, note);
				}
			}
		}
	}
}


void 
BSoftSynth::Init()
{
	fInitCheck = false;
	
	Done();

	fSettings = new_fluid_settings();
	fluid_settings_setnum(fSettings, "synth.sample-rate", fSampleRate);
	fluid_settings_setint(fSettings, "synth.polyphony", fMaxVoices);

	fSynth = new_fluid_synth(fSettings);

	media_raw_audio_format format;
	format.channel_count = 2;
	format.frame_rate = fSampleRate;
	format.format = media_raw_audio_format::B_AUDIO_FLOAT;

	fSoundPlayer = new BSoundPlayer(&format, "softsynth", &PlayBuffer, NULL, this);
	status_t err = fSoundPlayer->InitCheck();
	if (err != B_OK)
		return;

	fSoundPlayer->SetHasData(true);
	fSoundPlayer->Start();

	fInitCheck = true;
}


void 
BSoftSynth::Done()
{
	if (fSoundPlayer) {
		fSoundPlayer->SetHasData(false);
		fSoundPlayer->Stop();
		delete fSoundPlayer;
	}
	if (fSynth)
		delete_fluid_synth(fSynth);
	if (fSettings)
		delete_fluid_settings(fSettings);
}


void
BSoftSynth::PlayBuffer(void * cookie, void * data, size_t size, const media_raw_audio_format & format)
{
	BSoftSynth *synth = (BSoftSynth *)cookie;

	// we use float samples
	
	if (synth->fSynth)
		fluid_synth_write_float(synth->fSynth, size / sizeof(float) / format.channel_count, 
			data, 0, format.channel_count,
			data, 1, format.channel_count);
}

