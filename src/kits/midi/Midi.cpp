/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Copyright (c) 2002 Michael Pfeiffer
 * Copyright (c) 2002 Jerome Leveque
 * Copyright (c) 2002 Paul Stadler
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

#include <List.h>
#include <MidiProducer.h>

#include "Midi.h"
#include "MidiGlue.h"
#include "debug.h"

using namespace BPrivate;

//------------------------------------------------------------------------------

status_t _run_thread(void* data)
{
	BMidi* midi = (BMidi*) data;
	midi->Run();
	midi->isRunning = false;
	return 0;
}

//------------------------------------------------------------------------------

BMidi::BMidi()
{
	connections = new BList;
	threadId    = -1;
	isRunning   = false;

	producer = new BMidiLocalProducer("MidiGlue(out)");
	consumer = new BMidiGlue(this, "MidiGlue(in)");
}

//------------------------------------------------------------------------------

BMidi::~BMidi()
{
	Stop();

	status_t result;
	wait_for_thread(threadId, &result);

	producer->Release();
	consumer->Release();

	delete connections;
}

//------------------------------------------------------------------------------

void BMidi::NoteOff(uchar channel, uchar note, uchar velocity, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::NoteOn(uchar channel, uchar note, uchar velocity, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::ProgramChange(uchar channel, uchar programNumber, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::SystemExclusive(void* data, size_t length, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::SystemCommon(uchar status, uchar data1, uchar data2, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::SystemRealTime(uchar status, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::TempoChange(int32 beatsPerMinute, uint32 time)
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::AllNotesOff(bool justChannel, uint32 time)
{
	for (uchar channel = 1; channel <= 16; ++channel)
	{
		SprayControlChange(channel, B_ALL_NOTES_OFF, 0, time);

		if (!justChannel)
		{
			for (uchar note = 0; note <= 0x7F; ++note)
			{
				SprayNoteOff(channel, note, 0, time);
			}
		}
	}
}

//------------------------------------------------------------------------------

status_t BMidi::Start()
{
	if (isRunning) { return B_OK; }

	status_t err = spawn_thread(
		_run_thread, "MidiRunThread", B_URGENT_PRIORITY, this);

	if (err < B_OK) { return err; }

	threadId  = err;
	isRunning = true;

	err = resume_thread(threadId);
	if (err != B_OK)
	{
		threadId  = -1;
		isRunning = false;
	}

	return err;
}

//------------------------------------------------------------------------------

void BMidi::Stop()
{
	threadId = -1;
}
    
//------------------------------------------------------------------------------

bool BMidi::IsRunning() const
{
	return isRunning;
}

//------------------------------------------------------------------------------

void BMidi::Connect(BMidi* toObject)
{
	if (toObject != NULL)
	{
		if (producer->Connect(toObject->consumer) == B_OK)
		{
			connections->AddItem(toObject);
		}
	}
}

//------------------------------------------------------------------------------

void BMidi::Disconnect(BMidi* fromObject)
{
	if (fromObject != NULL)
	{
		if (producer->Disconnect(fromObject->consumer) == B_OK)
		{
			connections->RemoveItem(fromObject);
		}
	}
}

//------------------------------------------------------------------------------

bool BMidi::IsConnected(BMidi* toObject) const
{
	if (toObject != NULL)
	{
		return producer->IsConnected(toObject->consumer);
	}

	return false;
}

//------------------------------------------------------------------------------

BList* BMidi::Connections() const
{
	return connections;
}

//------------------------------------------------------------------------------

void BMidi::SnoozeUntil(uint32 time) const
{
	snooze_until(MAKE_BIGTIME(time), B_SYSTEM_TIMEBASE);
}

//------------------------------------------------------------------------------

bool BMidi::KeepRunning()
{
	return (threadId != -1);
}

//------------------------------------------------------------------------------

void BMidi::_ReservedMidi1() {}
void BMidi::_ReservedMidi2() {}
void BMidi::_ReservedMidi3() {}

//------------------------------------------------------------------------------

void BMidi::Run()
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidi::SprayNoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time) const
{
	producer->SprayNoteOff(
		channel - 1, note, velocity, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayNoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time) const
{
	producer->SprayNoteOn(
		channel - 1, note, velocity, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayKeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time) const
{
	producer->SprayKeyPressure(
		channel - 1, note, pressure, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, 
	uint32 time) const
{
	producer->SprayControlChange(
		channel - 1, controlNumber, controlValue, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayProgramChange(
	uchar channel, uchar programNumber, uint32 time) const
{
	producer->SprayProgramChange(
		channel - 1, programNumber, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayChannelPressure(
	uchar channel, uchar pressure, uint32 time) const
{
	producer->SprayChannelPressure(
		channel - 1, pressure, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayPitchBend(
	uchar channel, uchar lsb, uchar msb, uint32 time) const
{
	producer->SprayPitchBend(channel - 1, lsb, msb, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SpraySystemExclusive(
	void* data, size_t length, uint32 time) const
{
	producer->SpraySystemExclusive(data, length, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SpraySystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time) const
{
	producer->SpraySystemCommon(status, data1, data2, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SpraySystemRealTime(uchar status, uint32 time) const
{
	producer->SpraySystemRealTime(status, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidi::SprayTempoChange(int32 beatsPerMinute, uint32 time) const
{
	producer->SprayTempoChange(beatsPerMinute, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------
