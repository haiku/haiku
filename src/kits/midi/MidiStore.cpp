/**
 * @file MidiStore.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 * @author Paul Stadler
 */

#include <iostream>
#include <fstream>
#include <string.h>

#include <Entry.h>
#include <File.h>
#include <List.h>
#include <Path.h>

#include "debug.h"
#include "MidiEvent.h"
#include "MidiStore.h"

//------------------------------------------------------------------------------

BMidiStore::BMidiStore()
{
	events = new BList();
	tempo = 60;
	curEvent = 0;
}

//------------------------------------------------------------------------------

BMidiStore::~BMidiStore()
{
	int32 count = events->CountItems();

	for (int i = count - 1; i >= 0; i--) 
	{
		delete (BMidiEvent*) events->RemoveItem(i);
	}

	delete events;
}

//------------------------------------------------------------------------------

void BMidiStore::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_NOTE_OFF;
	event->time = B_NOW;
	event->noteOff.channel = channel;
	event->noteOff.note = note;
	event->noteOff.velocity = velocity;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_NOTE_ON;
	event->time = B_NOW;
	event->noteOn.channel = channel;
	event->noteOn.note = note;
	event->noteOn.velocity = velocity;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_KEY_PRESSURE;
	event->time = B_NOW;
	event->keyPressure.channel = channel;
	event->keyPressure.note = note;
	event->keyPressure.pressure = pressure;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_CONTROL_CHANGE;
	event->time = B_NOW;
	event->controlChange.channel = channel;
	event->controlChange.controlNumber = controlNumber;
	event->controlChange.controlValue = controlValue;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_PROGRAM_CHANGE;
	event->time = B_NOW;
	event->programChange.channel = channel;
	event->programChange.programNumber = programNumber;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_CHANNEL_PRESSURE;
	event->time = B_NOW;
	event->channelPressure.channel = channel;
	event->channelPressure.pressure = pressure;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_PITCH_BEND;
	event->time = B_NOW;
	event->pitchBend.channel = channel;
	event->pitchBend.lsb = lsb;
	event->pitchBend.msb = msb;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::SystemExclusive(void* data, size_t dataLength, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_SYSTEM_EXCLUSIVE;
	event->time = B_NOW;
	event->systemExclusive.data = (uint8*) data;
	event->systemExclusive.dataLength = dataLength;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_SYSTEM_COMMON;
	event->time = B_NOW;
	event->systemCommon.status = status;
	event->systemCommon.data1 = data1;
	event->systemCommon.data2 = data2;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::SystemRealTime(uchar status, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_SYSTEM_REAL_TIME;
	event->time = B_NOW;
	event->systemRealTime.status = status;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::TempoChange(int32 beatsPerMinute, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_TEMPO_CHANGE;
	event->time = B_NOW;
	event->tempoChange.beatsPerMinute = beatsPerMinute;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

void BMidiStore::AllNotesOff(bool justChannel, uint32 time) 
{ 
	BMidiEvent* event = new BMidiEvent;
	event->opcode = BMidiEvent::OP_ALL_NOTES_OFF;
	event->time = B_NOW;
	event->allNotesOff.justChannel = justChannel;
	events->AddItem(event);
}

//------------------------------------------------------------------------------

status_t BMidiStore::Import(const entry_ref* ref)
{
	status_t status = B_OK;

	try
	{
		file = new BFile(ref, B_READ_ONLY);

		status = file->InitCheck();
		if (status != B_OK) { throw status; }

		ReadFileHeader();

		for (int32 t = 0; t < numTracks; ++t)
		{
			TRACE(("[midi] Reading track %d\n", t))

			ReadTrackHeader();
			ReadTrack();
		} 

		SortEvents(true);
	}
	catch (status_t err)
	{
		status = err;
	}

	delete file;
	return status;
}


//------------------------------------------------------------------------------

status_t BMidiStore::Export(const entry_ref* ref, int32 format)
{
	return B_OK;
}

//------------------------------------------------------------------------------

void BMidiStore::SortEvents(bool force)
{
	events->SortItems(CompareEvents);
}

//------------------------------------------------------------------------------

uint32 BMidiStore::CountEvents() const
{
	return events->CountItems();
}

//------------------------------------------------------------------------------

uint32 BMidiStore::CurrentEvent() const
{
	return curEvent;
}

//------------------------------------------------------------------------------

void BMidiStore::SetCurrentEvent(uint32 eventNumber)
{
	curEvent = eventNumber;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::DeltaOfEvent(uint32 eventNumber) const
{
	BMidiEvent* first = EventAt(0);

	if (first == NULL) { return 0; }

	BMidiEvent* event = EventAt(eventNumber);

	if (event == NULL) { return 0; }

	return event->time - first->time;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::EventAtDelta(uint32 time) const
{
	uint32 count = CountEvents();
	uint32 delta = 0;

	for (uint32 t = 0; t < count; ++t)
	{
		if (delta >= time) 
		{ 
			return t; 
		}
		else
		{
			delta += EventAt(t)->time;
		}
	}
	
	return 0;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::BeginTime() const
{
	return startTime;
}

//------------------------------------------------------------------------------

void BMidiStore::SetTempo(int32 beatsPerMinute)
{
	tempo = beatsPerMinute;
}

//------------------------------------------------------------------------------

int32 BMidiStore::Tempo() const
{
	return tempo;
}

//------------------------------------------------------------------------------

void BMidiStore::_ReservedMidiStore1() { }
void BMidiStore::_ReservedMidiStore2() { }

//------------------------------------------------------------------------------

void BMidiStore::Run()
{
	startTime = B_NOW;

	while (KeepRunning()) 
	{
		BMidiEvent* event = EventAt(curEvent);

		if (event == NULL) { return; }

		uint32 oldTime = event->time;
		event->time += startTime;
		SprayMidiEvent(event);
		event->time = oldTime;

		++curEvent;
	}
}

//------------------------------------------------------------------------------

BMidiEvent* BMidiStore::EventAt(uint32 index) const
{
	return (BMidiEvent*) events->ItemAt(index);
}

//------------------------------------------------------------------------------

int BMidiStore::CompareEvents(const void* event1, const void* event2) 
{
	return ((BMidiEvent*) event1)->time - ((BMidiEvent*) event2)->time;
}

//------------------------------------------------------------------------------

void BMidiStore::ReadFileHeader()
{
	uint8 header[4];
	ReadData(header, 4);

	if (strncmp((char*) header, "MThd", 4) != 0)
	{
		throw B_BAD_MIDI_DATA;
	}
	
	int32 length = Read32Bit();

	if (length != 6) { throw B_BAD_MIDI_DATA; }

	Read16Bit();  // skip format

	numTracks = Read16Bit();
	division  = Read16Bit();

	if (numTracks == 0) { throw B_BAD_MIDI_DATA; }
	if (division  == 0) { throw B_BAD_MIDI_DATA; }

	TRACE(("[midi] numTracks = %d, division = %d\n", numTracks, division))
}

//------------------------------------------------------------------------------

void BMidiStore::ReadTrackHeader()
{
	uint8 header[4];
	ReadData(header, 4);

	if (strncmp((char*) header, "MTrk", 4) != 0)
	{
		throw B_BAD_MIDI_DATA;
	}

	trackSize = Read32Bit();

	TRACE(("[midi] trackSize = %d\n", trackSize))
}	

//------------------------------------------------------------------------------

void BMidiStore::ReadTrack()
{
	/* See <http://home.concepts.nl/~hollies/midi/midi1todo.html> */
}

//------------------------------------------------------------------------------

int32 BMidiStore::Read32Bit()
{
	uint8 buf[4];
	ReadData(buf, 4);
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

//------------------------------------------------------------------------------

int16 BMidiStore::Read16Bit()
{
	uint8 buf[2];
	ReadData(buf, 2);
	return (buf[0] << 8) | buf[1];
}

//------------------------------------------------------------------------------

void BMidiStore::ReadData(uint8* data, size_t size)
{
	ssize_t res = file->Read(data, size);

	if (res != size)
	{
		throw (status_t) res;
	}
}

//------------------------------------------------------------------------------

