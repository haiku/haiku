/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
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

#include <File.h>
#include <List.h>
#include <stdlib.h>
#include <string.h>

#include "MidiDefs.h"
#include "MidiStore.h"
#include "debug.h"

//------------------------------------------------------------------------------

struct BMidiEvent 
{
	BMidiEvent()
	{
		byte1  = 0;
		byte2  = 0;
		byte3  = 0;
		data   = NULL;
		length = 0;
	}

	~BMidiEvent()
	{
		free(data);
	}

	uint32 time;    // either ticks or milliseconds
	bool   ticks;   // event is from MIDI file
	uchar  byte1;
	uchar  byte2;
	uchar  byte3;
	void*  data;    // sysex data
	size_t length;  // sysex data size
	int32  tempo;   // beats per minute
};

//------------------------------------------------------------------------------

static int compare_events(const void* event1, const void* event2)
{
	BMidiEvent* e1 = *((BMidiEvent**) event1);
	BMidiEvent* e2 = *((BMidiEvent**) event2);

	return (e1->time - e2->time);
}

//------------------------------------------------------------------------------

BMidiStore::BMidiStore()
{
	events = new BList;
	currentEvent = 0;
	startTime = 0;
	needsSorting = false;
	beatsPerMinute = 60;
	ticksPerBeat = 240;
	file = NULL;
	hookFunc = NULL;
	looping = false;
	paused = false;
	finished = false;
}

//------------------------------------------------------------------------------

BMidiStore::~BMidiStore()
{
	for (int32 t = 0; t < events->CountItems(); ++t)
	{
		delete EventAt(t);
	}
	delete events;
}

//------------------------------------------------------------------------------

void BMidiStore::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_NOTE_OFF | (channel - 1);
	event->byte2 = note;
	event->byte3 = velocity;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_NOTE_ON | (channel - 1);
	event->byte2 = note;
	event->byte3 = velocity;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_KEY_PRESSURE | (channel - 1);
	event->byte2 = note;
	event->byte3 = pressure;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_CONTROL_CHANGE | (channel - 1);
	event->byte2 = controlNumber;
	event->byte3 = controlValue;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_PROGRAM_CHANGE | (channel - 1);
	event->byte2 = programNumber;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_CHANNEL_PRESSURE | (channel - 1);
	event->byte2 = pressure;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = B_PITCH_BEND | (channel - 1);
	event->byte2 = lsb;
	event->byte3 = msb;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::SystemExclusive(void* data, size_t length, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time   = time;
	event->ticks  = false;
	event->byte1  = B_SYS_EX_START;
	event->data   = malloc(length);
	event->length = length;
	memcpy(event->data, data, length);
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = status;
	event->byte2 = data1;
	event->byte3 = data2;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::SystemRealTime(uchar status, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = status;
	AddEvent(event);
}

//------------------------------------------------------------------------------

void BMidiStore::TempoChange(int32 beatsPerMinute, uint32 time)
{
	BMidiEvent* event = new BMidiEvent;
	event->time  = time;
	event->ticks = false;
	event->byte1 = 0xFF;
	event->byte2 = 0x51;
	event->byte3 = 0x03;
	event->tempo = beatsPerMinute;
	AddEvent(event);
}

//------------------------------------------------------------------------------

status_t BMidiStore::Import(const entry_ref* ref)
{
	try
	{
		file = new BFile(ref, B_READ_ONLY);
		if (file->InitCheck() != B_OK)
		{
			throw file->InitCheck();
		}

		char fourcc[4];
		ReadFourCC(fourcc);
		if (strncmp(fourcc, "MThd", 4) != 0)
		{
			throw (status_t) B_BAD_MIDI_DATA;
		}

		if (Read32Bit() != 6)
		{
			throw (status_t) B_BAD_MIDI_DATA;
		}

		format = Read16Bit();
		numTracks = Read16Bit();
		ticksPerBeat = Read16Bit();

		if (ticksPerBeat & 0x8000)  // we don't support SMPTE
		{                           // time codes, only ticks
			ticksPerBeat = 240;     // per quarter note
		}

		currTrack = 0;
		while (currTrack < numTracks)
		{
			ReadChunk(); 
		}
	}
	catch (status_t e)
	{
		delete file;
		file = NULL;
		return e;
	}

	SortEvents(true);

	delete file;
	file = NULL;
	return B_OK;
}

//------------------------------------------------------------------------------

status_t BMidiStore::Export(const entry_ref* ref, int32 format)
{
	try
	{
		file = new BFile(ref, B_READ_WRITE);
		if (file->InitCheck() != B_OK)
		{
			throw file->InitCheck();
		}

		SortEvents(true);

		WriteFourCC('M', 'T', 'h', 'd');
		Write32Bit(6);
		Write16Bit(0);  // we do only format 0
		Write16Bit(1);
		Write16Bit(ticksPerBeat);

		WriteTrack();
	}
	catch (status_t e)
	{
		delete file;
		file = NULL;
		return e;
	}

	delete file;
	file = NULL;	
	return B_OK;
}

//------------------------------------------------------------------------------

void BMidiStore::SortEvents(bool force)
{
	if (force || needsSorting)
	{
		events->SortItems(compare_events);
		needsSorting = false;
	}
}

//------------------------------------------------------------------------------

uint32 BMidiStore::CountEvents() const
{
	return events->CountItems();
}

//------------------------------------------------------------------------------

uint32 BMidiStore::CurrentEvent() const
{
	return currentEvent;
}

//------------------------------------------------------------------------------

void BMidiStore::SetCurrentEvent(uint32 eventNumber)
{
	currentEvent = eventNumber;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::DeltaOfEvent(uint32 eventNumber) const
{
	// Even though the BeBook says that the delta is the time span between
	// an event and the first event in the list, this doesn't appear to be
	// true for events that were captured from other BMidi objects such as
	// BMidiPort. For those events, we return the absolute timestamp. The
	// BeBook is correct for events from MIDI files, though.

	BMidiEvent* event = EventAt(eventNumber);
	if (event != NULL)
	{
		return GetEventTime(event);
	}

	return 0;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::EventAtDelta(uint32 time) const
{
	for (int32 t = 0; t < events->CountItems(); ++t)
	{
		if (GetEventTime(EventAt(t)) >= time) { return t; }
	}

	return 0;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::BeginTime() const
{
	return startTime;
}

//------------------------------------------------------------------------------

void BMidiStore::SetTempo(int32 beatsPerMinute_)
{
	beatsPerMinute = beatsPerMinute_;
}

//------------------------------------------------------------------------------

int32 BMidiStore::Tempo() const
{
	return beatsPerMinute;
}

//------------------------------------------------------------------------------

void BMidiStore::_ReservedMidiStore1() { }
void BMidiStore::_ReservedMidiStore2() { }
void BMidiStore::_ReservedMidiStore3() { }

//------------------------------------------------------------------------------

void BMidiStore::Run()
{
	// This rather compilicated Run() loop is not only used by BMidiStore
	// but also by BMidiSynthFile. The "paused", "finished", and "looping"
	// flags, and the "stop hook" are especially provided for the latter.

	paused = false;
	finished = false;
	
	int32 timeAdjust;
	uint32 baseTime;
	bool firstEvent = true;
	bool resetTime = false;

	while (KeepRunning())
	{
		if (paused)
		{
			resetTime = true;
			snooze(100000);
			continue;
		}
		
		BMidiEvent* event = EventAt(currentEvent);
		
		if (event == NULL)  // no more events
		{
			if (looping)
			{
				resetTime = true;
				currentEvent = 0;
			}
			else
			{
				break;
			}
		}

		if (firstEvent)
		{
			startTime = B_NOW;
			baseTime = startTime;
		}
		else if (resetTime)
		{
			baseTime = B_NOW;
		}

		if (firstEvent || resetTime)
		{		
			timeAdjust = baseTime - GetEventTime(event);
			SprayEvent(event, baseTime);
			firstEvent = false;
			resetTime = false;
		}
		else
		{
			SprayEvent(event, GetEventTime(event) + timeAdjust);
		}

		++currentEvent;
	}

	finished = true;
	paused = false;

	if (hookFunc != NULL)
	{
		(*hookFunc)(hookArg);
	}
}

//------------------------------------------------------------------------------

void BMidiStore::AddEvent(BMidiEvent* event)
{
	events->AddItem(event);
	needsSorting = true;
}

//------------------------------------------------------------------------------

void BMidiStore::SprayEvent(const BMidiEvent* event, uint32 time)
{
	uchar byte1 = event->byte1;
	uchar byte2 = event->byte2;
	uchar byte3 = event->byte3;

	switch (byte1 & 0xF0)
	{
		case B_NOTE_OFF:
			SprayNoteOff((byte1 & 0x0F) + 1, byte2, byte3, time);
			return;

		case B_NOTE_ON:
			SprayNoteOn((byte1 & 0x0F) + 1, byte2, byte3, time);
			return;

		case B_KEY_PRESSURE:
			SprayKeyPressure((byte1 & 0x0F) + 1, byte2, byte3, time);
			return;

		case B_CONTROL_CHANGE:
			SprayControlChange((byte1 & 0x0F) + 1, byte2, byte3, time);
			return;

		case B_PROGRAM_CHANGE:
			SprayProgramChange((byte1 & 0x0F) + 1, byte2, time);
			return;

		case B_CHANNEL_PRESSURE:
			SprayChannelPressure((byte1 & 0x0F) + 1, byte2, time);
			return;

		case B_PITCH_BEND:
			SprayPitchBend((byte1 & 0x0F) + 1, byte2, byte3, time);
			return;

		case 0xF0:
			switch (byte1)
			{
				case B_SYS_EX_START:
					SpraySystemExclusive(event->data, event->length, time);
					return;

				case B_MIDI_TIME_CODE:
				case B_SONG_POSITION:
				case B_SONG_SELECT:
				case B_CABLE_MESSAGE:
				case B_TUNE_REQUEST:
				case B_SYS_EX_END:
					SpraySystemCommon(byte1, byte2, byte3, time);
					return;

				case B_TIMING_CLOCK:
				case B_START:
				case B_CONTINUE:
				case B_STOP:
				case B_ACTIVE_SENSING:
					SpraySystemRealTime(byte1, time);
					return;

				case B_SYSTEM_RESET:
					if ((byte2 == 0x51) && (byte3 == 0x03))
					{
						SprayTempoChange(event->tempo, time);
						beatsPerMinute = event->tempo;
					}
					else
					{
						SpraySystemRealTime(byte1, time);
					}
					return;
			}
			return;
	}
}

//------------------------------------------------------------------------------

BMidiEvent* BMidiStore::EventAt(int32 index) const
{
	return (BMidiEvent*) events->ItemAt(index);
}

//------------------------------------------------------------------------------

uint32 BMidiStore::GetEventTime(const BMidiEvent* event) const
{
	if (event->ticks)
	{
		return TicksToMilliseconds(event->time);
	}
	else
	{
		return event->time;
	}
}

//------------------------------------------------------------------------------

uint32 BMidiStore::TicksToMilliseconds(uint32 ticks) const
{
	return ((uint64) ticks * 60000) / (beatsPerMinute * ticksPerBeat);
}

//------------------------------------------------------------------------------

uint32 BMidiStore::MillisecondsToTicks(uint32 ms) const
{
	return ((uint64) ms * beatsPerMinute * ticksPerBeat) / 60000;
}

//------------------------------------------------------------------------------

void BMidiStore::ReadFourCC(char* fourcc)
{
	if (file->Read(fourcc, 4) != 4)
	{
		throw (status_t) B_BAD_MIDI_DATA;
	}
}

//------------------------------------------------------------------------------

void BMidiStore::WriteFourCC(char a, char b, char c, char d)
{
	char fourcc[4] = { a, b, c, d };
	if (file->Write(fourcc, 4) != 4)
	{
		throw (status_t) B_ERROR;
	}
}

//------------------------------------------------------------------------------

uint32 BMidiStore::Read32Bit()
{
	uint8 buf[4];
	if (file->Read(buf, 4) != 4)
	{
		throw (status_t) B_BAD_MIDI_DATA;
	}

	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

//------------------------------------------------------------------------------

void BMidiStore::Write32Bit(uint32 val)
{
	uint8 buf[4];
	buf[0] = (val >> 24) & 0xFF;
	buf[1] = (val >> 16) & 0xFF;
	buf[2] = (val >>  8) & 0xFF;
	buf[3] =  val        & 0xFF;

	if (file->Write(buf, 4) != 4)
	{
		throw (status_t) B_ERROR;
	}
}

//------------------------------------------------------------------------------

uint16 BMidiStore::Read16Bit()
{
	uint8 buf[2];
	if (file->Read(buf, 2) != 2)
	{
		throw (status_t) B_BAD_MIDI_DATA;
	}

	return (buf[0] << 8) | buf[1];
}

//------------------------------------------------------------------------------

void BMidiStore::Write16Bit(uint16 val)
{
	uint8 buf[2];
	buf[0] = (val >>  8) & 0xFF;
	buf[1] =  val        & 0xFF;

	if (file->Write(buf, 2) != 2)
	{
		throw (status_t) B_ERROR;
	}
}

//------------------------------------------------------------------------------

uint8 BMidiStore::PeekByte()
{
	uint8 buf;
	if (file->Read(&buf, 1) != 1)
	{
		throw (status_t) B_BAD_MIDI_DATA;
	}

	if (file->Seek(-1, SEEK_CUR) < 0)
	{
		throw (status_t) B_ERROR;
	}

	return buf;
}

//------------------------------------------------------------------------------

uint8 BMidiStore::NextByte()
{
	uint8 buf;
	if (file->Read(&buf, 1) != 1)
	{
		throw (status_t) B_BAD_MIDI_DATA;
	}

	--byteCount;
	return buf;
}

//------------------------------------------------------------------------------

void BMidiStore::WriteByte(uint8 val)
{
	if (file->Write(&val, 1) != 1)
	{
		throw (status_t) B_ERROR;
	}

	++byteCount;
}

//------------------------------------------------------------------------------

void BMidiStore::SkipBytes(uint32 length)
{
	if (file->Seek(length, SEEK_CUR) < 0)
	{
		throw (status_t) B_BAD_MIDI_DATA;
	}

	byteCount -= length;
}

//------------------------------------------------------------------------------

uint32 BMidiStore::ReadVarLength()
{
	uint32 val;
	uint8 byte;

	if ((val = NextByte()) & 0x80)
	{
		val &= 0x7F;
		do
		{
			val = (val << 7) + ((byte = NextByte()) & 0x7F);
		}
		while (byte & 0x80);
	}

	return val;
}

//------------------------------------------------------------------------------

void BMidiStore::WriteVarLength(uint32 val)
{
	uint32 buffer = val & 0x7F;

	while ((val >>= 7))
	{
		buffer <<= 8;
		buffer |= ((val & 0x7F) | 0x80);
	}

	while (true)
	{
		WriteByte(buffer);
		if (buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}
}

//------------------------------------------------------------------------------

void BMidiStore::ReadChunk()
{
	char fourcc[4];
	ReadFourCC(fourcc);

	byteCount = Read32Bit();

	if (strncmp(fourcc, "MTrk", 4) == 0)
	{
		ReadTrack();
	}
	else
	{
		TRACE(("Skipping '%c%c%c%c' chunk (%lu bytes)",
			fourcc[0], fourcc[1], fourcc[2], fourcc[3], byteCount))

		SkipBytes(byteCount);
	}
}

//------------------------------------------------------------------------------

void BMidiStore::ReadTrack()
{
	uint8 status = 0;
	uint8 data1;
	uint8 data2;
	BMidiEvent* event;

	totalTicks = 0;

	while (byteCount > 0)
	{
		uint32 ticks = ReadVarLength();
		totalTicks += ticks;

		if (PeekByte() & 0x80)
		{
			status = NextByte();
		}

		switch (status & 0xF0)
		{
			case B_NOTE_OFF:
			case B_NOTE_ON:
			case B_KEY_PRESSURE:
			case B_CONTROL_CHANGE:
			case B_PITCH_BEND:
				data1 = NextByte();
				data2 = NextByte();
				event = new BMidiEvent;
				event->time  = totalTicks;
				event->ticks = true;
				event->byte1 = status;
				event->byte2 = data1;
				event->byte3 = data2;
				AddEvent(event);
				break;

			case B_PROGRAM_CHANGE:
			case B_CHANNEL_PRESSURE:
				data1 = NextByte();
				event = new BMidiEvent;
				event->time  = totalTicks;
				event->ticks = true;
				event->byte1 = status;
				event->byte2 = data1;
				AddEvent(event);
				break;

			case 0xF0:
				switch (status)
				{
					case B_SYS_EX_START:
						ReadSystemExclusive();
						break;

					case B_TUNE_REQUEST:
					case B_SYS_EX_END:
					case B_TIMING_CLOCK:
					case B_START:
					case B_CONTINUE:
					case B_STOP:
					case B_ACTIVE_SENSING:
						event = new BMidiEvent;
						event->time  = totalTicks;
						event->ticks = true;
						event->byte1 = status;
						AddEvent(event);
						break;

					case B_MIDI_TIME_CODE:
					case B_SONG_SELECT:
					case B_CABLE_MESSAGE:
						data1 = NextByte();
						event = new BMidiEvent;
						event->time  = totalTicks;
						event->ticks = true;
						event->byte1 = status;
						event->byte2 = data1;
						AddEvent(event);
						break;

					case B_SONG_POSITION:
						data1 = NextByte();
						data2 = NextByte();
						event = new BMidiEvent;
						event->time  = totalTicks;
						event->ticks = true;
						event->byte1 = status;
						event->byte2 = data1;
						event->byte3 = data2;
						AddEvent(event);
						break;

					case B_SYSTEM_RESET:
						ReadMetaEvent();
						break;
				}
				break;
		}

		event = NULL;
	}
		
	++currTrack;
}

//------------------------------------------------------------------------------

void BMidiStore::ReadSystemExclusive()
{
	// We do not import sysex's from MIDI files.

	SkipBytes(ReadVarLength());
}

//------------------------------------------------------------------------------

void BMidiStore::ReadMetaEvent()
{
	// We only import the Tempo Change meta event.

	uint8 type = NextByte();	
	uint32 length = ReadVarLength();

	if ((type == 0x51) && (length == 3))
	{
		uchar data[3];
		data[0] = NextByte();
		data[1] = NextByte();
		data[2] = NextByte();
		uint32 val = (data[0] << 16) | (data[1] << 8) | data[2];

		BMidiEvent* event = new BMidiEvent;
		event->time  = totalTicks;
		event->ticks = true;
		event->byte1 = 0xFF;
		event->byte2 = 0x51;
		event->byte3 = 0x03;
		event->tempo = 60000000 / val;
		AddEvent(event);
	}
	else
	{
		SkipBytes(length);
	}
}

//------------------------------------------------------------------------------

void BMidiStore::WriteTrack()
{
	WriteFourCC('M', 'T', 'r', 'k');
	off_t lengthPos = file->Position();
	Write32Bit(0);

	byteCount = 0;
	uint32 oldTime;
	uint32 newTime;

	for (uint32 t = 0; t < CountEvents(); ++t)
	{
		BMidiEvent* event = EventAt(t);

		if (event->ticks)
		{
			newTime = event->time;
		}
		else
		{
			newTime = MillisecondsToTicks(event->time);
		}

		if (t == 0)
		{
			WriteVarLength(0);
		}
		else
		{
			WriteVarLength(newTime - oldTime);
		}

		oldTime = newTime;

		switch (event->byte1 & 0xF0)
		{
			case B_NOTE_OFF:
			case B_NOTE_ON:
			case B_KEY_PRESSURE:
			case B_CONTROL_CHANGE:
			case B_PITCH_BEND:
				WriteByte(event->byte1);
				WriteByte(event->byte2);
				WriteByte(event->byte3);
				break;

			case B_PROGRAM_CHANGE:
			case B_CHANNEL_PRESSURE:
				WriteByte(event->byte1);
				WriteByte(event->byte2);
				break;

			case 0xF0:
				switch (event->byte1)
				{
					case B_SYS_EX_START:
						// We do not export sysex's.
						break;

					case B_TUNE_REQUEST:
					case B_SYS_EX_END:
					case B_TIMING_CLOCK:
					case B_START:
					case B_CONTINUE:
					case B_STOP:
					case B_ACTIVE_SENSING:
						WriteByte(event->byte1);
						break;

					case B_MIDI_TIME_CODE:
					case B_SONG_SELECT:
					case B_CABLE_MESSAGE:
						WriteByte(event->byte1);
						WriteByte(event->byte2);
						break;

					case B_SONG_POSITION:
						WriteByte(event->byte1);
						WriteByte(event->byte2);
						WriteByte(event->byte3);
						break;

					case B_SYSTEM_RESET:
						WriteMetaEvent(event);
						break;
				}
				break;
			
		}
	}

	WriteVarLength(0);
	WriteByte(0xFF);   // the end-of-track
	WriteByte(0x2F);   // marker is required
	WriteByte(0x00);  

	file->Seek(lengthPos, SEEK_SET);
	Write32Bit(byteCount);
	file->Seek(0, SEEK_END);
}

//------------------------------------------------------------------------------

void BMidiStore::WriteMetaEvent(BMidiEvent* event)
{
	// We only export the Tempo Change meta event.

	if ((event->byte2 == 0x51) && (event->byte3 == 0x03))
	{
		uint32 val = 60000000 / event->tempo;

		WriteByte(0xFF);
		WriteByte(0x51);
		WriteByte(0x03);
		WriteByte(val >> 16);
		WriteByte(val >> 8);
		WriteByte(val);
	}
}


//------------------------------------------------------------------------------
