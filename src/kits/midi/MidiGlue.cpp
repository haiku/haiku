/* 
 * Copyright (c) 2003 Matthijs Hollemans
 * Copyright (c) 2002 Jerome Leveque
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

#include "Midi.h"
#include "MidiGlue.h"
#include "MidiPort.h"

using namespace BPrivate;

//------------------------------------------------------------------------------

BMidiGlue::BMidiGlue(BMidi* midiObject_, const char* name)
	: BMidiLocalConsumer(name)
{
	midiObject = midiObject_;
}
	
//------------------------------------------------------------------------------

void BMidiGlue::NoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	midiObject->NoteOff(channel + 1, note, velocity, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::NoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	midiObject->NoteOn(channel + 1, note, velocity, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::KeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	midiObject->KeyPressure(channel + 1, note, pressure, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	midiObject->ControlChange(
		channel + 1, controlNumber, controlValue, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::ProgramChange(
	uchar channel, uchar programNumber, bigtime_t time)
{
	midiObject->ProgramChange(channel + 1, programNumber, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::ChannelPressure(
	uchar channel, uchar pressure, bigtime_t time)
{
	midiObject->ChannelPressure(channel + 1, pressure, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::PitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	midiObject->PitchBend(channel + 1, lsb, msb, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::SystemExclusive(
	void* data, size_t length, bigtime_t time)
{
	midiObject->SystemExclusive(data, length, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::SystemCommon(
	uchar status, uchar data1, uchar data2, bigtime_t time)
{
	midiObject->SystemCommon(status, data1, data2, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::SystemRealTime(uchar status, bigtime_t time)
{
	midiObject->SystemRealTime(status, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiGlue::TempoChange(int32 beatsPerMinute, bigtime_t time)
{
	midiObject->TempoChange(beatsPerMinute, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

BMidiPortGlue::BMidiPortGlue(BMidiPort* midiObject_, const char* name)
	: BMidiLocalConsumer(name)
{
	midiObject = midiObject_;
}
	
//------------------------------------------------------------------------------

void BMidiPortGlue::NoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	midiObject->SprayNoteOff(channel + 1, note, velocity, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::NoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	midiObject->SprayNoteOn(channel + 1, note, velocity, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::KeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	midiObject->SprayKeyPressure(
		channel + 1, note, pressure, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	midiObject->SprayControlChange(
		channel + 1, controlNumber, controlValue, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::ProgramChange(
	uchar channel, uchar programNumber, bigtime_t time)
{
	midiObject->SprayProgramChange(
		channel + 1, programNumber, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::ChannelPressure(
	uchar channel, uchar pressure, bigtime_t time)
{
	midiObject->SprayChannelPressure(channel + 1, pressure, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::PitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	midiObject->SprayPitchBend(channel + 1, lsb, msb, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::SystemExclusive(
	void* data, size_t length, bigtime_t time)
{
	midiObject->SpraySystemExclusive(data, length, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::SystemCommon(
	uchar status, uchar data1, uchar data2, bigtime_t time)
{
	midiObject->SpraySystemCommon(status, data1, data2, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::SystemRealTime(uchar status, bigtime_t time)
{
	midiObject->SpraySystemRealTime(status, MAKE_TIME(time));
}

//------------------------------------------------------------------------------

void BMidiPortGlue::TempoChange(int32 beatsPerMinute, bigtime_t time)
{
	midiObject->SprayTempoChange(beatsPerMinute, MAKE_TIME(time));
}

//------------------------------------------------------------------------------
