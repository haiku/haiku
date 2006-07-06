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

#ifndef _MIDI_GLUE_H
#define _MIDI_GLUE_H

#include <MidiPort.h>
#include <MidiConsumer.h>

#define MAKE_TIME(t)    (t / (bigtime_t)1000)
#define MAKE_BIGTIME(t) (t * (bigtime_t)1000)

namespace BPrivate {

class BMidiGlue: public BMidiLocalConsumer
{
public:

	BMidiGlue(BMidi* midiObject, const char* name = NULL);
	
	virtual void NoteOff(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual void NoteOn(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual void KeyPressure(
		uchar channel, uchar note, uchar pressure, bigtime_t time);

	virtual void ControlChange(
		uchar channel, uchar controlNumber, uchar controlValue,
		bigtime_t time);

	virtual void ProgramChange(
		uchar channel, uchar programNumber, bigtime_t time);

	virtual void ChannelPressure(
		uchar channel, uchar pressure, bigtime_t time);

	virtual void PitchBend(
		uchar channel, uchar lsb, uchar msb, bigtime_t time);

	virtual void SystemExclusive(
		void* data, size_t length, bigtime_t time);

	virtual void SystemCommon(
		uchar status, uchar data1, uchar data2, bigtime_t time);

	virtual void SystemRealTime(uchar status, bigtime_t time);

	virtual void TempoChange(int32 beatsPerMinute, bigtime_t time);

private:

	BMidi* fMidiObject;
};

class BMidiPortGlue: public BMidiLocalConsumer
{
public:

	BMidiPortGlue(BMidiPort* midiObject, const char* name = NULL);
	
	virtual void NoteOff(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual void NoteOn(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual void KeyPressure(
		uchar channel, uchar note, uchar pressure, bigtime_t time);

	virtual void ControlChange(
		uchar channel, uchar controlNumber, uchar controlValue,
		bigtime_t time);

	virtual void ProgramChange(
		uchar channel, uchar programNumber, bigtime_t time);

	virtual void ChannelPressure(
		uchar channel, uchar pressure, bigtime_t time);

	virtual void PitchBend(
		uchar channel, uchar lsb, uchar msb, bigtime_t time);

	virtual void SystemExclusive(
		void* data, size_t length, bigtime_t time);

	virtual void SystemCommon(
		uchar status, uchar data1, uchar data2, bigtime_t time);

	virtual void SystemRealTime(uchar status, bigtime_t time);

	virtual void TempoChange(int32 beatsPerMinute, bigtime_t time);

private:

	BMidiPort* fMidiObject;
};

} // namespace BPrivate

#endif // _MIDI_GLUE_H
