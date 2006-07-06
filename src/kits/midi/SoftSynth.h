/*
 * Copyright 2006, Haiku.
 * 
 * Copyright (c) 2004-2005 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Jérôme Duval
 *		Jérôme Leveque
 *		Matthijs Hollemans
 */

#ifndef _SOFT_SYNTH_H
#define _SOFT_SYNTH_H

/*
	This version of SoftSynth is a wrapper libfluidsynth.so.
 */

#include <fluidsynth.h>
#include <Midi.h>
#include <SoundPlayer.h>
#include <Synth.h>

class BMidiConsumer;
class BMidiSynth;
class BSynth;

namespace BPrivate {

class BSoftSynth
{
public:

	bool InitCheck();

	void Unload();
	bool IsLoaded() const;

	status_t SetDefaultInstrumentsFile();
	status_t SetInstrumentsFile(const char* path);

	status_t LoadAllInstruments();
	status_t LoadInstrument(int16 instrument);
	status_t UnloadInstrument(int16 instrument);
	status_t RemapInstrument(int16 from, int16 to);
	void FlushInstrumentCache(bool startStopCache);

	void SetVolume(double volume);
	double Volume(void) const;

	status_t SetSamplingRate(int32 rate);
	int32 SamplingRate() const;

	status_t SetInterpolation(interpolation_mode mode);
	interpolation_mode Interpolation() const;

	status_t EnableReverb(bool enabled);
	bool IsReverbEnabled() const;
	void SetReverb(reverb_mode mode);
	reverb_mode Reverb() const;

	status_t SetMaxVoices(int32 max);
	int16 MaxVoices(void) const;

	status_t SetLimiterThreshold(int32 threshold);
	int16 LimiterThreshold(void) const;

	void Pause(void);
	void Resume(void);

	void NoteOff(uchar, uchar, uchar, uint32);
	void NoteOn(uchar, uchar, uchar, uint32);
	void KeyPressure(uchar, uchar, uchar, uint32);
	void ControlChange(uchar, uchar, uchar, uint32);
	void ProgramChange(uchar, uchar, uint32);
 	void ChannelPressure(uchar, uchar, uint32);
	void PitchBend(uchar, uchar, uchar, uint32);
	void SystemExclusive(void*, size_t, uint32);
	void SystemCommon(uchar, uchar, uchar, uint32);
	void SystemRealTime(uchar, uint32);
	void TempoChange(int32, uint32);
	void AllNotesOff(bool, uint32);

private:

	friend class ::BSynth;
	friend class ::BMidiSynth;

	BSoftSynth();
	~BSoftSynth();

	void _Init();
	void _Done();
	static void PlayBuffer(void * cookie, void * data, size_t size, const media_raw_audio_format & format);
	
	bool fInitCheck;
	char* fInstrumentsFile;
	int32 fSampleRate;
	interpolation_mode fInterpMode;
	int16 fMaxVoices;
	int16 fLimiterThreshold;
	reverb_mode fReverbMode;
	bool fReverbEnabled;

	fluid_synth_t *fSynth;
	fluid_settings_t* fSettings;

	BSoundPlayer *fSoundPlayer;
};

} // namespace BPrivate

#endif // _SOFT_SYNTH_H
