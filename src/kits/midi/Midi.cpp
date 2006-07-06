/*
 * Copyright 2006, Haiku.
 * 
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Copyright (c) 2002 Michael Pfeiffer
 * Copyright (c) 2002 Jerome Leveque
 * Copyright (c) 2002 Paul Stadler
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Michael Pfeiffer
 *		Jérôme Leveque
 *		Paul Stadler
 */

#include <List.h>
#include <Midi.h>
#include <MidiProducer.h>

#include "MidiGlue.h"
#include "debug.h"

using namespace BPrivate;


status_t 
_run_thread(void* data)
{
	BMidi* midi = (BMidi*) data;
	midi->Run();
	midi->fIsRunning = false;
	return 0;
}


BMidi::BMidi()
{
	fConnections = new BList;
	fThreadId    = -1;
	fIsRunning   = false;

	fProducer = new BMidiLocalProducer("MidiGlue(out)");
	fConsumer = new BMidiGlue(this, "MidiGlue(in)");
}


BMidi::~BMidi()
{
	Stop();

	status_t result;
	wait_for_thread(fThreadId, &result);

	fProducer->Release();
	fConsumer->Release();

	delete fConnections;
}


void 
BMidi::NoteOff(uchar channel, uchar note, uchar velocity, uint32 time)
{
	// do nothing
}


void 
BMidi::NoteOn(uchar channel, uchar note, uchar velocity, uint32 time)
{
	// do nothing
}


void 
BMidi::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	// do nothing
}


void 
BMidi::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	// do nothing
}


void 
BMidi::ProgramChange(uchar channel, uchar programNumber, uint32 time)
{
	// do nothing
}


void 
BMidi::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	// do nothing
}


void 
BMidi::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	// do nothing
}


void 
BMidi::SystemExclusive(void* data, size_t length, uint32 time)
{
	// do nothing
}


void 
BMidi::SystemCommon(uchar status, uchar data1, uchar data2, uint32 time)
{
	// do nothing
}


void 
BMidi::SystemRealTime(uchar status, uint32 time)
{
	// do nothing
}


void 
BMidi::TempoChange(int32 beatsPerMinute, uint32 time)
{
	// do nothing
}


void 
BMidi::AllNotesOff(bool justChannel, uint32 time)
{
	for (uchar channel = 1; channel <= 16; ++channel) {
		SprayControlChange(channel, B_ALL_NOTES_OFF, 0, time);

		if (!justChannel) {
			for (uchar note = 0; note <= 0x7F; ++note) {
				SprayNoteOff(channel, note, 0, time);
			}
		}
	}
}


status_t 
BMidi::Start()
{
	if (fIsRunning) 
		return B_OK;

	status_t err = spawn_thread(
		_run_thread, "MidiRunThread", B_URGENT_PRIORITY, this);

	if (err < B_OK) 
		return err;

	fThreadId  = err;
	fIsRunning = true;

	err = resume_thread(fThreadId);
	if (err != B_OK) {
		fThreadId  = -1;
		fIsRunning = false;
	}

	return err;
}


void 
BMidi::Stop()
{
	AllNotesOff();
	fThreadId = -1;
}
    

bool 
BMidi::IsRunning() const
{
	return fIsRunning;
}


void 
BMidi::Connect(BMidi* toObject)
{
	if (toObject != NULL) {
		if (fProducer->Connect(toObject->fConsumer) == B_OK) {
			fConnections->AddItem(toObject);
		}
	}
}


void 
BMidi::Disconnect(BMidi* fromObject)
{
	if (fromObject == NULL)
		return;

	if (fProducer->Disconnect(fromObject->fConsumer) == B_OK) {
		fConnections->RemoveItem(fromObject);
	}
}


bool 
BMidi::IsConnected(BMidi* toObject) const
{
	if (toObject != NULL)
		return fProducer->IsConnected(toObject->fConsumer);

	return false;
}


BList* 
BMidi::Connections() const
{
	return fConnections;
}


void 
BMidi::SnoozeUntil(uint32 time) const
{
	snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
}


bool 
BMidi::KeepRunning()
{
	return (fThreadId != -1);
}


void BMidi::_ReservedMidi1() {}
void BMidi::_ReservedMidi2() {}
void BMidi::_ReservedMidi3() {}


void 
BMidi::Run()
{
	// do nothing
}


void 
BMidi::SprayNoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time) const
{
	fProducer->SprayNoteOff(
		channel - 1, note, velocity, MAKE_BIGTIME(time));
}


void 
BMidi::SprayNoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time) const
{
	fProducer->SprayNoteOn(
		channel - 1, note, velocity, MAKE_BIGTIME(time));
}


void 
BMidi::SprayKeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time) const
{
	fProducer->SprayKeyPressure(
		channel - 1, note, pressure, MAKE_BIGTIME(time));
}


void 
BMidi::SprayControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, 
	uint32 time) const
{
	fProducer->SprayControlChange(
		channel - 1, controlNumber, controlValue, MAKE_BIGTIME(time));
}


void 
BMidi::SprayProgramChange(
	uchar channel, uchar programNumber, uint32 time) const
{
	fProducer->SprayProgramChange(
		channel - 1, programNumber, MAKE_BIGTIME(time));
}


void 
BMidi::SprayChannelPressure(
	uchar channel, uchar pressure, uint32 time) const
{
	fProducer->SprayChannelPressure(
		channel - 1, pressure, MAKE_BIGTIME(time));
}


void 
BMidi::SprayPitchBend(
	uchar channel, uchar lsb, uchar msb, uint32 time) const
{
	fProducer->SprayPitchBend(channel - 1, lsb, msb, MAKE_BIGTIME(time));
}


void 
BMidi::SpraySystemExclusive(
	void* data, size_t length, uint32 time) const
{
	fProducer->SpraySystemExclusive(data, length, MAKE_BIGTIME(time));
}


void 
BMidi::SpraySystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time) const
{
	fProducer->SpraySystemCommon(status, data1, data2, MAKE_BIGTIME(time));
}


void 
BMidi::SpraySystemRealTime(uchar status, uint32 time) const
{
	fProducer->SpraySystemRealTime(status, MAKE_BIGTIME(time));
}


void 
BMidi::SprayTempoChange(int32 beatsPerMinute, uint32 time) const
{
	fProducer->SprayTempoChange(beatsPerMinute, MAKE_BIGTIME(time));
}

