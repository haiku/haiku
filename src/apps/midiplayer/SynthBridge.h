/*
 * Copyright (c) 2004 Matthijs Hollemans
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

#ifndef SYNTH_BRIDGE_H
#define SYNTH_BRIDGE_H

#include <MidiConsumer.h>
#include <MidiSynth.h>

class SynthBridge : public BMidiLocalConsumer
{
public:

	SynthBridge();

	bool Init(synth_mode mode);

	BMidiSynth* MidiSynth();

	virtual	void NoteOff(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual	void NoteOn(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual	void KeyPressure(
		uchar channel, uchar note, uchar pressure, bigtime_t time);

	virtual	void ControlChange(
		uchar channel, uchar controlNumber, uchar controlValue, 
		bigtime_t time);

	virtual	void ProgramChange(
		uchar channel, uchar programNumber, bigtime_t time);

	virtual	void ChannelPressure(
		uchar channel, uchar pressure, bigtime_t time);

	virtual	void PitchBend(
		uchar channel, uchar lsb, uchar msb, bigtime_t time);

	virtual void AllNotesOff(
		bool justChannel, bigtime_t time);

protected:

	virtual ~SynthBridge() { }

private:

	BMidiSynth midiSynth;
};

#endif // SYNTH_BRIDGE_H
