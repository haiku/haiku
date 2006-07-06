/*
 * Copyright 2006, Haiku.
 *
 * Copyright (c) 2003 Matthijs Hollemans
 * Copyright (c) 2002 Jerome Leveque
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Jérôme Leveque
 */

#include <Midi.h>
#include "MidiGlue.h"
#include <MidiPort.h>

using namespace BPrivate;


BMidiGlue::BMidiGlue(BMidi* midiObject_, const char* name)
	: BMidiLocalConsumer(name)
{
	fMidiObject = midiObject_;
}
	

void 
BMidiGlue::NoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	fMidiObject->NoteOff(channel + 1, note, velocity, MAKE_TIME(time));
}


void 
BMidiGlue::NoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	fMidiObject->NoteOn(channel + 1, note, velocity, MAKE_TIME(time));
}


void 
BMidiGlue::KeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	fMidiObject->KeyPressure(channel + 1, note, pressure, MAKE_TIME(time));
}


void 
BMidiGlue::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	fMidiObject->ControlChange(
		channel + 1, controlNumber, controlValue, MAKE_TIME(time));
}


void 
BMidiGlue::ProgramChange(
	uchar channel, uchar programNumber, bigtime_t time)
{
	fMidiObject->ProgramChange(channel + 1, programNumber, MAKE_TIME(time));
}


void 
BMidiGlue::ChannelPressure(
	uchar channel, uchar pressure, bigtime_t time)
{
	fMidiObject->ChannelPressure(channel + 1, pressure, MAKE_TIME(time));
}


void 
BMidiGlue::PitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	fMidiObject->PitchBend(channel + 1, lsb, msb, MAKE_TIME(time));
}


void 
BMidiGlue::SystemExclusive(
	void* data, size_t length, bigtime_t time)
{
	fMidiObject->SystemExclusive(data, length, MAKE_TIME(time));
}


void 
BMidiGlue::SystemCommon(
	uchar status, uchar data1, uchar data2, bigtime_t time)
{
	fMidiObject->SystemCommon(status, data1, data2, MAKE_TIME(time));
}


void 
BMidiGlue::SystemRealTime(uchar status, bigtime_t time)
{
	fMidiObject->SystemRealTime(status, MAKE_TIME(time));
}


void 
BMidiGlue::TempoChange(int32 beatsPerMinute, bigtime_t time)
{
	fMidiObject->TempoChange(beatsPerMinute, MAKE_TIME(time));
}


BMidiPortGlue::BMidiPortGlue(BMidiPort* midiObject_, const char* name)
	: BMidiLocalConsumer(name)
{
	fMidiObject = midiObject_;
}
	

void 
BMidiPortGlue::NoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	fMidiObject->SprayNoteOff(channel + 1, note, velocity, MAKE_TIME(time));
}


void 
BMidiPortGlue::NoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	fMidiObject->SprayNoteOn(channel + 1, note, velocity, MAKE_TIME(time));
}


void 
BMidiPortGlue::KeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	fMidiObject->SprayKeyPressure(
		channel + 1, note, pressure, MAKE_TIME(time));
}


void 
BMidiPortGlue::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	fMidiObject->SprayControlChange(
		channel + 1, controlNumber, controlValue, MAKE_TIME(time));
}


void 
BMidiPortGlue::ProgramChange(
	uchar channel, uchar programNumber, bigtime_t time)
{
	fMidiObject->SprayProgramChange(
		channel + 1, programNumber, MAKE_TIME(time));
}


void 
BMidiPortGlue::ChannelPressure(
	uchar channel, uchar pressure, bigtime_t time)
{
	fMidiObject->SprayChannelPressure(channel + 1, pressure, MAKE_TIME(time));
}


void 
BMidiPortGlue::PitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	fMidiObject->SprayPitchBend(channel + 1, lsb, msb, MAKE_TIME(time));
}


void 
BMidiPortGlue::SystemExclusive(
	void* data, size_t length, bigtime_t time)
{
	fMidiObject->SpraySystemExclusive(data, length, MAKE_TIME(time));
}


void 
BMidiPortGlue::SystemCommon(
	uchar status, uchar data1, uchar data2, bigtime_t time)
{
	fMidiObject->SpraySystemCommon(status, data1, data2, MAKE_TIME(time));
}


void 
BMidiPortGlue::SystemRealTime(uchar status, bigtime_t time)
{
	fMidiObject->SpraySystemRealTime(status, MAKE_TIME(time));
}


void 
BMidiPortGlue::TempoChange(int32 beatsPerMinute, bigtime_t time)
{
	fMidiObject->SprayTempoChange(beatsPerMinute, MAKE_TIME(time));
}

