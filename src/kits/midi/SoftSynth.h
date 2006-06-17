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

#ifndef _SOFT_SYNTH_H
#define _SOFT_SYNTH_H

/*
	WORK IN PROGRESS!
	
	This version of SoftSynth is a wrapper for Michael Pfeiffer's port
	of TiMidity++ 2.11.3/3 (http://www.bebits.com/app/2736).

	It works, but not good enough yet. Playback from MidiPlayer sounds
	a lot worse than using TiMidity in standalone mode. Either TiMidity's
	MidiConsumer doesn't work properly or the Midi Kit messes things up;
	I haven't investigated yet.

	To try it out, download TiMidity and the sound files (30MB):
	http://bepdf.sourceforge.net/download/midi/TiMidity++-2.11.3_3.x86.zip
	http://bepdf.sourceforge.net/download/midi/eawpats11_full_beos.zip

	Follow the instructions in the archive to install. Then double-click
	"Start TiMidity Server" to launch TiMidity. The server will publish a
	consumer endpoint. You can verify this with PatchBay.
	
	Build the Haiku Midi Kit. Put libmidi.so and libmidi2.so in ~/config/lib.
	Quit the BeOS midi_server. Launch the Haiku midi_server.
	
	Build the Haiku MidiPlayer (or use the BeOS MidiPlayer). Start it and
	choose a MIDI file. If all went fine, you will hear TiMidity play back 
	the song. Just not very well. :-)
	
	Note: You can still use the Midi Kit if you don't install TiMidity, 
	but the software synth will simply make no sound.
 */

#include <Midi.h>
#include <Synth.h>

#include <SoundPlayer.h>

#include <fluidsynth.h>

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

	void Init();
	void Done();
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

#endif // _SYNTH_CONSUMER_H
