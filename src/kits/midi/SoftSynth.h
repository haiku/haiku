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

#ifndef _SOFT_SYNTH_H
#define _SOFT_SYNTH_H

#include <Midi.h>
#include <Synth.h>

namespace BPrivate {

class BSoftSynth
{
public:

	void Unload(void);
	bool IsLoaded(void) const;

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

	friend class BSynth;
	friend class BMidiSynth;

	BSoftSynth();
	~BSoftSynth();

	char* instrumentsFile;
	int32 sampleRate;
	interpolation_mode interpMode;
	int16 maxVoices;
	int16 limiterThreshold;
	reverb_mode reverbMode;
	bool reverbEnabled;
	double volumeScale;
};

} // namespace BPrivate

#endif // _SYNTH_CONSUMER_H
