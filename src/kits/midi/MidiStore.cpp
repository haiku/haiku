#include "MidiStore.h"
#include "MidiEvent.h"

#include <File.h>
#include <string.h> //For strcmp(...)
#include <stdio.h> //For printf(...)
#include <Debug.h>
#include <List.h>

//-----------------------------------------------
//#define DEBUG
//-----------------------------------------------

//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------

int CompareEvents(const void* event1, const void* event2)
{
	return ((BMidiEvent*)event1)->time - ((BMidiEvent*)event2)->time;
}

//-----------------------------------------------

BMidiStore::BMidiStore()
{
	events = new BList();
	fFile = NULL;
	fFileBuffer = NULL;
	fFileBufferMax = 0;
	fNeedsSorting = false;
	fDivision = 480;
	fNumTracks = 1;
	fStartTime = 0;
	fCurrEvent = 0;
}

//-----------------------------------------------

BMidiStore::~BMidiStore()
{
	int32 count = events->CountItems();
	for (int i = count - 1; i >= 0; i--) 
	{
		delete (BMidiEvent*) events->RemoveItem(i);
	}
	delete events;
}

//-----------------------------------------------

void BMidiStore::NoteOff(uchar chan, uchar note, uchar vel, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_NOTE_OFF, chan, note, vel);
}

//-----------------------------------------------

void BMidiStore::NoteOn(uchar chan, uchar note, uchar vel, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_NOTE_ON, chan, note, vel);
}

//-----------------------------------------------

void BMidiStore::KeyPressure(uchar chan, uchar note, uchar pres, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_KEY_PRESSURE, chan, note, pres);
}

//-----------------------------------------------

void BMidiStore::ControlChange(uchar chan, uchar ctrl_num, uchar ctrl_val, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_CONTROL_CHANGE, chan, ctrl_num, ctrl_val);
}

//-----------------------------------------------

void BMidiStore::ProgramChange(uchar chan, uchar prog_num, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_PROGRAM_CHANGE, chan, prog_num);
}

//-----------------------------------------------

void BMidiStore::ChannelPressure(uchar chan, uchar pres, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_CHANNEL_PRESSURE, chan, pres);
}

//-----------------------------------------------

void BMidiStore::PitchBend(uchar chan, uchar lsb, uchar msb, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_PITCH_BEND, chan, lsb, msb);
}
//-----------------------------------------------

void BMidiStore::SystemExclusive(void *data, size_t data_len, uint32 time)
{
	BMidiEvent *e = new BMidiEvent;
	e->opcode = BMidiEvent::OP_SYSTEM_EXCLUSIVE;
	e->time = time;
	e->systemExclusive.data = new uint8[data_len];
	memcpy(e->systemExclusive.data, data, data_len);
	e->systemExclusive.dataLength = data_len;
	AddSystemExclusive(e, sizeof(BMidiEvent));
}

//-----------------------------------------------

void BMidiStore::SystemCommon(uchar stat_byte, uchar data1, uchar data2, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_SYSTEM_COMMON, stat_byte, data1, data2);
}

//-----------------------------------------------

void BMidiStore::SystemRealTime(uchar stat_byte, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_SYSTEM_REAL_TIME, stat_byte);
}

//-----------------------------------------------

void BMidiStore::TempoChange(int32 bpm, uint32 time)
{
	AddEvent(time, true, BMidiEvent::OP_TEMPO_CHANGE, bpm);
}

//-----------------------------------------------

status_t BMidiStore::Import(const entry_ref *ref)
{//ToTest
status_t status = B_NO_ERROR;

	fFile = new BFile(ref, B_READ_ONLY);

	status = fFile->InitCheck();
	if (status != B_OK)
	{
		delete fFile;
		return status;
	}

	status = ReadHeader();
	if (status != B_OK)
	{
		delete fFile;
		return status;
	}

	Read16Bit(); //Format of the MidiFile
	fNumTracks = Read16Bit();
	fDivision = Read16Bit();
	fDivisionFactor = 1 / fDivision;

	status = B_OK;
	for(fCurrTrack = 0; fCurrTrack < fNumTracks; fCurrTrack++)
	{
	char mtrk[4];
		if (ReadMT(mtrk) == false)
		{
			PRINT("Error while reading MTrk\n");
			status = B_BAD_MIDI_DATA;
			break;
		}
		PRINT(("Printing Track %ld\n", fCurrTrack));

		fFileBufferSize = Read32Bit();
		if (fFileBufferSize > fFileBufferMax)
		{
			fFileBufferMax = fFileBufferSize;
			delete fFileBuffer;
			fFileBuffer = new uchar[fFileBufferSize];
		}
		if (ReadTrack() == false)
		{
			PRINT(("Error while reading Trak %ld\n", fCurrTrack));
			status = B_BAD_MIDI_DATA;
			break;
		}
	}

	delete fFile;
	delete fFileBuffer;
	fFileBuffer = NULL;
	SortEvents(true);
return status;
}

//-----------------------------------------------

status_t BMidiStore::Export(const entry_ref *ref, int32 format)
{//ToDo
	fFile = new BFile(ref, B_READ_WRITE);
	status_t status = fFile->InitCheck();
	if (status != B_OK)
	{
		delete fFile;
		return status;
	}

	SortEvents(true);
	WriteHeaderChunk(format);

off_t position_of_length = 0;

	if (format == 0)
	{//Format is 0 just one track
		position_of_length = fFile->Position() + 4;
		WriteTrack(-1);
		fFile->Seek(position_of_length, SEEK_SET);
		Write32Bit(fNumBytesWritten);
		fFile->Seek(0, SEEK_END);
	}

	if (format == 1)
	{//Format is 1, number of track is fNumTracks initialized when import file
		for (int32 i = 0; i < fNumTracks; i++)
		{
			position_of_length = fFile->Position() + 4;
			WriteTrack(i);
			fFile->Seek(position_of_length, SEEK_SET);
			Write32Bit(fNumBytesWritten);
			fFile->Seek(0, SEEK_END);
		}
	}

return B_OK;
}

//-----------------------------------------------

void BMidiStore::SortEvents(bool force)
{
	events->SortItems(CompareEvents);
}

//-----------------------------------------------

uint32 BMidiStore::CountEvents() const
{//ToTest
return events->CountItems();
}

//-----------------------------------------------

uint32 BMidiStore::CurrentEvent() const
{//ToTest
	return fCurrEvent;
}

//-----------------------------------------------

void BMidiStore::SetCurrentEvent(uint32 event_num)
{//ToTest
	fCurrEvent = event_num;
}

//-----------------------------------------------

uint32 BMidiStore::DeltaOfEvent(uint32 event_num) const
{//ToDo
return 0;
}

//-----------------------------------------------

uint32 BMidiStore::EventAtDelta(uint32 time) const
{//ToDo
return 0;
}

//-----------------------------------------------

uint32 BMidiStore::BeginTime() const
{//ToTest
		return fStartTime;
}

//-----------------------------------------------

void BMidiStore::SetTempo(int32 bpm)
{//ToTest
	fTempo = bpm;
}

//-----------------------------------------------

int32 BMidiStore::Tempo() const
{//ToTest
	return fTempo;
}

//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------

void BMidiStore::Run()
{
	fStartTime = B_NOW;
	SortEvents(true);
	while (KeepRunning())
	{
		BMidiEvent* event = (BMidiEvent*)events->ItemAt(fCurrEvent);

		if (event == NULL) return;

		printf("Curr Event : %ld, Time : %ld\n", fCurrEvent, event->time);

		fCurrTime = event->time;
		event->time += fStartTime;
//		SprayMidiEvent(event);
		event->time = fCurrTime;

		fCurrEvent++;
	}
}

//-----------------------------------------------

void BMidiStore::AddEvent(uint32 time, bool inMS, uchar type, uchar data1 = 0, uchar data2 = 0, uchar data3 = 0, uchar data4 = 0)
{//ToTest
	if (!inMS)
	{
		time = TicksToMilliseconds(time);
	}
	BMidiEvent *e = new BMidiEvent();
	e->time = time;
	switch (type)
	{
		case BMidiEvent::OP_NOTE_OFF :
					e->opcode = BMidiEvent::OP_NOTE_OFF;
					e->noteOff.channel = data1;
					e->noteOff.note = data2;
					e->noteOff.velocity = data3;
					break;
		case BMidiEvent::OP_NOTE_ON :
					e->opcode = BMidiEvent::OP_NOTE_ON;
					e->noteOn.channel = data1;
					e->noteOn.note = data2;
					e->noteOn.velocity = data3;
					break;
		case BMidiEvent::OP_KEY_PRESSURE :
					e->opcode = BMidiEvent::OP_KEY_PRESSURE;
					e->keyPressure.channel = data1;
					e->keyPressure.note = data2;
					e->keyPressure.pressure = data3;
					break;
		case BMidiEvent::OP_CONTROL_CHANGE :
					e->opcode = BMidiEvent::OP_CONTROL_CHANGE;
					e->controlChange.channel = data1;
					e->controlChange.controlNumber = data2;
					e->controlChange.controlValue = data3;
					break;
		case BMidiEvent::OP_PROGRAM_CHANGE :
					e->opcode = BMidiEvent::OP_PROGRAM_CHANGE;
					e->programChange.channel = data1;
					e->programChange.programNumber = data2;
					break;
		case BMidiEvent::OP_CHANNEL_PRESSURE :
					e->opcode = BMidiEvent::OP_CHANNEL_PRESSURE;
					e->channelPressure.channel = data1;
					e->channelPressure.pressure = data2;
					break;
		case BMidiEvent::OP_PITCH_BEND :
					e->opcode = BMidiEvent::OP_PITCH_BEND;
					e->pitchBend.channel = data1;
					e->pitchBend.lsb = data2;
					e->pitchBend.msb = data3;
					break;
		case BMidiEvent::OP_SYSTEM_COMMON :
					e->opcode = BMidiEvent::OP_SYSTEM_COMMON;
					e->systemCommon.status = data1;
					e->systemCommon.data1 = data2;
					e->systemCommon.data2 = data3;
					break;
		case BMidiEvent::OP_SYSTEM_REAL_TIME :
					e->opcode = BMidiEvent::OP_SYSTEM_REAL_TIME;
					e->systemRealTime.status = data1;
					break;
		case BMidiEvent::OP_TEMPO_CHANGE :
					e->opcode = BMidiEvent::OP_TEMPO_CHANGE;
					e->tempoChange.beatsPerMinute = data1;
					break;
		case BMidiEvent::OP_ALL_NOTES_OFF :
					e->opcode = BMidiEvent::OP_ALL_NOTES_OFF;
					e->allNotesOff.justChannel = data1;
					break;
		default :
			delete e;
			return;
	}
	events->AddItem(e);
}

//-----------------------------------------------

void BMidiStore::AddSystemExclusive(void* data, size_t dataLength)
{//ToTest
//	BMidiEvent *e = new BMidiEvent();
//	e->time = 0;
//	e->systemExclusive.data = (uint8*)data;
//	e->systemExclusive.dataLength = dataLength;
	events->AddItem(data);
}

//-----------------------------------------------

status_t BMidiStore::ReadHeader()
{//ToTest
char mthd[4];
	if (ReadMT(mthd) == false)
		return B_ERROR;
	if(strncmp(mthd, "MThd", 4) != 0)
	{
		return B_BAD_MIDI_DATA;
	}

int32 length = Read32Bit();
	if (length != 6)
	{
		return B_BAD_MIDI_DATA;
	}
return B_OK;
}

//-----------------------------------------------

bool BMidiStore::ReadMT(char *data)
{//ToTest
	return fFile->Read(data, 4) == 4;
}

//-----------------------------------------------

int32 BMidiStore::Read32Bit()
{//ToTest
uchar char32bit[4];
	fFile->Read(char32bit, 4);
return To32Bit(char32bit[0], char32bit[1], char32bit[2], char32bit[3]);
}

//-----------------------------------------------

int32 BMidiStore::EGetC()
{//ToDo
return 0;
}

//-----------------------------------------------

int32 BMidiStore::To32Bit(int32 data1, int32 data2, int32 data3, int32 data4)
{//ToTest
	return data1 << 24 | data2 << 16 | data3 << 8 | data4;
}

//-----------------------------------------------

int32 BMidiStore::Read16Bit()
{//ToTest
uchar char16bit[2];
	fFile->Read(char16bit, 2);
return To16Bit(char16bit[0], char16bit[1]);
}

//-----------------------------------------------

int32 BMidiStore::To16Bit(int32 data1, int32 data2)
{//ToTest
	return (uint32)data1 << 8 | (uint32)data2;
}

//-----------------------------------------------

bool BMidiStore::ReadTrack()
{//ToTest
	if (fFile->Read(fFileBuffer, fFileBufferSize) != fFileBufferSize)
		return false;
fNeedsSorting = true;
fFileBufferIndex = 0;
uint32 ticks = 0;
uint32 time = 0; //in ms
uchar data = 0;
uint32 ForTempo = 0;
//fStartTime = 0;
	while (fFileBufferIndex < (fFileBufferSize - 3))
	{
		ticks += ReadVariNum();
		time = /*fStartTime + */TicksToMilliseconds(ticks);
		uchar temp = fFileBuffer[fFileBufferIndex];
		if (temp > 0x7F)
		{
			data = temp;
			fFileBufferIndex++;
		}
		if (data < 0x80)
		{
			PRINT(("Big Problem, data : %d, Index : %ld\n", data, fFileBufferIndex));
			return false;
		}
		switch (data & 0xF0)
		{
			case B_NOTE_OFF :
				NoteOff(data & 0x0F, fFileBuffer[fFileBufferIndex],
							fFileBuffer[fFileBufferIndex + 1], time);
				fFileBufferIndex += 2;
				break;
			case B_NOTE_ON :
				NoteOn(data & 0x0F, fFileBuffer[fFileBufferIndex],
							fFileBuffer[fFileBufferIndex + 1], time);
				fFileBufferIndex += 2;
				break;
			case B_KEY_PRESSURE :
				KeyPressure(data & 0x0F, fFileBuffer[fFileBufferIndex],
							fFileBuffer[fFileBufferIndex + 1], time);
				fFileBufferIndex += 2;
				break;
			case B_CONTROL_CHANGE :
				ControlChange(data & 0x0F, fFileBuffer[fFileBufferIndex],
							fFileBuffer[fFileBufferIndex + 1], time);
				fFileBufferIndex += 2;
				break;
			case B_PITCH_BEND :
				PitchBend(data & 0x0F, fFileBuffer[fFileBufferIndex],
							fFileBuffer[fFileBufferIndex + 1], time);
				fFileBufferIndex += 2;
				break;
			case B_PROGRAM_CHANGE :
				ProgramChange(data & 0x0F, fFileBuffer[fFileBufferIndex], time);
				fFileBufferIndex += 2;
				break;
			case B_CHANNEL_PRESSURE :
				ChannelPressure(data & 0x0F, fFileBuffer[fFileBufferIndex], time);
				fFileBufferIndex += 1;
				break;
		}
		switch (data)
		{
			case B_SYS_EX_START :
				temp = MsgLength();
				SystemExclusive(&fFileBuffer[fFileBufferIndex - 1], temp + 2, time);
				fFileBufferIndex += temp;
				break;
			case B_SONG_POSITION :
				SystemCommon(data, fFileBuffer[fFileBufferIndex], fFileBuffer[fFileBufferIndex + 1], time);
				fFileBufferIndex += 2;
				break;
			case B_MIDI_TIME_CODE :
			case B_SONG_SELECT :
				SystemCommon(data, fFileBuffer[fFileBufferIndex], 0, time);
				fFileBufferIndex += 1;
				break;
			case B_TUNE_REQUEST :
			case B_TIMING_CLOCK :
				SystemCommon(data, 0, 0, time);
				break;
			case B_START :
			case B_CONTINUE :
			case B_STOP :
			case B_ACTIVE_SENSING :
				SystemRealTime(data, time);
				break;
			case B_SYSTEM_RESET :
				temp = fFileBuffer[fFileBufferIndex];
				if (temp == 0x2F)
					return true;
				if (temp == 0x51)
				{//Calcul of the tempo is wrong
					fFileBufferIndex +=2;
					ForTempo = fFileBuffer[fFileBufferIndex++] << 8;
					ForTempo |= fFileBuffer[fFileBufferIndex++];
					ForTempo = (ForTempo << 8) | fFileBuffer[fFileBufferIndex++];
//					ForTempo /= 2500;
					TempoChange(ForTempo, time);
				}
				else
				{
					temp = fFileBuffer[fFileBufferIndex + 1] + 3;
					SystemExclusive(&fFileBuffer[fFileBufferIndex - 1], temp, time);
					fFileBufferIndex += temp - 1;
				}
				break;
			default :
				if (data >= 0xF0)
					printf("Midi Data not Defined\n");
		}
	}
return true;
}

//-----------------------------------------------

int32 BMidiStore::ReadVariNum()
{//ToTest
int32 result = 0;
uchar tempo = 0;
	while (fFileBufferIndex < fFileBufferSize)
	{
		tempo = fFileBuffer[fFileBufferIndex++];
		result |= tempo & 0x7F;
		if ((tempo & 0x80) == 0)
			return result;
		result <<= 7;
	}
return B_ERROR;
}

//-----------------------------------------------

void BMidiStore::ChannelMessage(int32, int32, int32)
{//ToDo

}

//-----------------------------------------------

void BMidiStore::MsgInit()
{//ToDo

}

//-----------------------------------------------

void BMidiStore::MsgAdd(int32)
{//ToDo

}

//-----------------------------------------------

void BMidiStore::BiggerMsg()
{//ToDo

}

//-----------------------------------------------

void BMidiStore::MetaEvent(int32)
{//ToDo

}

//-----------------------------------------------

int32 BMidiStore::MsgLength()
{//ToTest
int32 t = 0;
	while (fFileBuffer[fFileBufferIndex + t] != 0xF7)
	{
		if (fFileBufferIndex + t > fFileBufferSize - 3)
			return -1;
		else
			t++;
	}
return t;
}

//-----------------------------------------------

uchar *BMidiStore::Msg()
{//ToDo
return 0;
}

//-----------------------------------------------

void BMidiStore::BadByte(int32)
{//ToDo

}

//-----------------------------------------------


int32 BMidiStore::PutC(int32 c)
{//ToDo
return 0;
}

//-----------------------------------------------

bool BMidiStore::WriteTrack(int32 track)
{//ToTest
uchar temp[4] = {0x00, 0x00, 0x00, 0x00};
	WriteTrackChunk(track);
	fNumBytesWritten = 0;
uint32 ticks = 0;
int32 i = 0;
// If track == -1 write all events
BMidiEvent *event = (BMidiEvent*)events->ItemAt(i++);
	if (event != NULL)
		fCurrTime = event->time;
	while (event != NULL)
	{
		ticks = MillisecondsToTicks(event->time - fCurrTime);
		switch(event->opcode)
		{
			case BMidiEvent::OP_NOTE_OFF :
					if ((track == -1) || (track == event->noteOff.channel))
					{
						temp[0] = event->noteOff.note;
						temp[1] = event->noteOff.velocity;
						WriteMidiEvent(ticks, B_NOTE_OFF, event->noteOff.channel, temp, 2);
					}
					break;
			case BMidiEvent::OP_NOTE_ON :
					if ((track == -1) || (track == event->noteOn.channel))
					{
						temp[0] = event->noteOn.note;
						temp[1] = event->noteOn.velocity;
						WriteMidiEvent(ticks, B_NOTE_ON, event->noteOn.channel, temp, 2);
					}
					break;
			case BMidiEvent::OP_KEY_PRESSURE :
					if ((track == -1) || (track == event->keyPressure.channel))
					{
						temp[0] = event->keyPressure.note;
						temp[1] = event->keyPressure.pressure;
						WriteMidiEvent(ticks, B_KEY_PRESSURE, event->keyPressure.channel, temp, 2);
					}
					break;
			case BMidiEvent::OP_CONTROL_CHANGE :
					if ((track == -1) || (track == event->controlChange.channel))
					{
						temp[0] = event->controlChange.controlNumber;
						temp[1] = event->controlChange.controlValue;
						WriteMidiEvent(ticks, B_CONTROL_CHANGE, event->controlChange.channel, temp, 2);
					}
					break;
			case BMidiEvent::OP_PROGRAM_CHANGE :
					if ((track == -1) || (track == event->programChange.channel))
					{
						temp[0] = event->programChange.programNumber;
						WriteMidiEvent(ticks, B_PROGRAM_CHANGE, event->programChange.channel, temp, 1);
					}
					break;
			case BMidiEvent::OP_CHANNEL_PRESSURE :
					if ((track == -1) || (track == event->channelPressure.channel))
					{
						temp[0] = event->channelPressure.pressure;
						WriteMidiEvent(ticks, B_CHANNEL_PRESSURE, event->channelPressure.channel, temp, 1);
					}
					break;
			case BMidiEvent::OP_PITCH_BEND :
					if ((track == -1) || (track == event->pitchBend.channel))
					{
						temp[0] = event->pitchBend.lsb;
						temp[1] = event->pitchBend.msb;
						WriteMidiEvent(ticks, B_PITCH_BEND, event->pitchBend.channel, temp, 2);
					}
					break;
			case BMidiEvent::OP_SYSTEM_COMMON :
					if ((track == -1) || (track == 0))
					{
						temp[0] = event->systemCommon.data1;
						temp[1] = event->systemCommon.data2;
						WriteMetaEvent(ticks, event->systemCommon.status, temp, 2);
					}
					break;
			case BMidiEvent::OP_SYSTEM_REAL_TIME :
					if ((track == -1) || (track == 0))
					{
						WriteMetaEvent(ticks, event->systemRealTime.status, temp, 0);
					}
					break;
			case BMidiEvent::OP_SYSTEM_EXCLUSIVE :
					if ((track == -1) || (track == 0))
					{
						WriteSystemExclusiveEvent(ticks, event->systemExclusive.data, event->systemExclusive.dataLength);
					}
					break;
			case BMidiEvent::OP_TEMPO_CHANGE :
					if ((track == -1) || (track == 0))
					{
						WriteTempo(ticks, event->tempoChange.beatsPerMinute);
					}
					break;
//Not used
			case BMidiEvent::OP_NONE :
			case BMidiEvent::OP_ALL_NOTES_OFF :
			case BMidiEvent::OP_TRACK_END :
					break;
		}
//		}
		fCurrTime = event->time;
		event = (BMidiEvent*)events->ItemAt(i++);
	}

	Write16Bit(0xFF2F);
	EPutC(0);
return true;
}

//-----------------------------------------------

void BMidiStore::WriteTempoTrack()
{//ToDo
}

//-----------------------------------------------

bool BMidiStore::WriteTrackChunk(int32 whichTrack)
{//ToTest
	Write32Bit('MTrk');
	Write32Bit(6);
return true;
}

//-----------------------------------------------

void BMidiStore::WriteHeaderChunk(int32 format)
{//ToTest
	Write32Bit('MThd');
	Write32Bit(6);
	Write16Bit(format);
	if (format == 0)
		fNumTracks = 1;
	Write16Bit(fNumTracks);
	Write16Bit(fDivision);
}

//-----------------------------------------------

bool BMidiStore::WriteMidiEvent(uint32 deltaTime, uint32 type, uint32 channel, uchar* data, uint32 size)
{//ToTest
	WriteVarLen(deltaTime);
	if (EPutC(type | channel) != 1)
		return false;
	while (size > 0)
	{
		if (EPutC(*data) != 1)
			return false;
		data++;
		size--;
	}
return true;
}

//-----------------------------------------------

bool BMidiStore::WriteMetaEvent(uint32 deltaTime, uint32 type, uchar* data, uint32 size)
{//ToTest
	WriteVarLen(deltaTime);
	if (EPutC(type) != 1)
		return false;
	while (size > 0)
	{
		if (EPutC(*data) != 1)
			return false;
		data++;
		size--;
	}
return true;
}

//-----------------------------------------------

bool BMidiStore::WriteSystemExclusiveEvent(uint32 deltaTime, uchar* data, uint32 size)
{//ToTest
	WriteVarLen(deltaTime);
	while (size > 0)
	{
		if (EPutC(*data) != 1)
			return false;
		data++;
		size--;
	}
return true;
}

//-----------------------------------------------

void BMidiStore::WriteTempo(uint32 deltaTime, int32 tempo)
{//ToTest
	WriteVarLen(deltaTime);
	Write16Bit(0xFF51);
	EPutC(3);
	EPutC((tempo >> 16) & 0xFF);
	EPutC((tempo >> 8) & 0xFF);
	EPutC(tempo & 0xFF);
}

//-----------------------------------------------

void BMidiStore::WriteVarLen(uint32 value)
{//ToTest
uint32 buffer = value & 0x7F;
	while ( (value >>= 7) )
	{
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}
	while (true)
	{
		EPutC(buffer);
		if (buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}
}

//-----------------------------------------------

void BMidiStore::Write32Bit(uint32 data)
{//ToTest
	EPutC((data >> 24) & 0xFF);
	EPutC((data >> 16) & 0xFF);
	EPutC((data >> 8) & 0xFF);
	EPutC(data & 0xFF);
}

//-----------------------------------------------

void BMidiStore::Write16Bit(ushort data)
{//ToTest
	EPutC((data >> 8) & 0xFF);
	EPutC(data & 0xFF);
}

//-----------------------------------------------

int32 BMidiStore::EPutC(uchar c)
{//ToTest
	fNumBytesWritten++;
return fFile->Write(&c, 1);
}

//-----------------------------------------------


uint32 BMidiStore::TicksToMilliseconds(uint32 ticks) const
{//ToTest
	return ((uint64)ticks * 1000) / fDivision;
}

//-----------------------------------------------

uint32 BMidiStore::MillisecondsToTicks(uint32 ms) const
{//ToTest
	return ((uint64)ms * fDivision) / 1000;
}

//-----------------------------------------------

void BMidiStore::_ReservedMidiStore1()
{
}

//-----------------------------------------------

void BMidiStore::_ReservedMidiStore2()
{
}

//-----------------------------------------------

void BMidiStore::_ReservedMidiStore3()
{
}

//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
//-----------------------------------------------
