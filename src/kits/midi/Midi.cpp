/**
 * @file Midi.cpp
 * 
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 * @author Paul Stadler
 */

#include <List.h>

#include "debug.h"
#include "Midi.h"
#include "MidiEvent.h"

#define MSG_CODE_PROCESS_EVENT     1
#define MSG_CODE_TERMINATE_THREAD  0

//------------------------------------------------------------------------------

BMidi::BMidi()
{
	connections = new BList();
	isRunning = false;

	inflowPort = create_port(1, "Inflow Port");

	inflowThread = spawn_thread(
		InflowThread, "BMidi Inflow Thread",
		B_REAL_TIME_PRIORITY, (void*) this);

	inflowAlive = (resume_thread(inflowThread) == B_OK);
}

//------------------------------------------------------------------------------

BMidi::~BMidi()
{
	write_port(inflowPort, MSG_CODE_TERMINATE_THREAD, NULL, 0);

	status_t exitValue;
	wait_for_thread(inflowThread, &exitValue);

	delete_port(inflowPort);
	delete connections;	
}

//------------------------------------------------------------------------------

void BMidi::NoteOff(uchar, uchar, uchar, uint32) { } 
void BMidi::NoteOn(uchar, uchar, uchar, uint32) { } 
void BMidi::KeyPressure(uchar, uchar, uchar, uint32) { }
void BMidi::ControlChange(uchar, uchar, uchar, uint32) { }
void BMidi::ProgramChange(uchar, uchar, uint32) { } 
void BMidi::ChannelPressure(uchar, uchar, uint32) { } 
void BMidi::PitchBend(uchar, uchar, uchar, uint32) { } 
void BMidi::SystemExclusive(void*, size_t, uint32) { } 
void BMidi::SystemCommon(uchar, uchar, uchar, uint32) { }
void BMidi::SystemRealTime(uchar, uint32) { } 
void BMidi::TempoChange(int32, uint32) { } 
void BMidi::AllNotesOff(bool, uint32) { }

//------------------------------------------------------------------------------

status_t BMidi::Start()
{
	runThread = spawn_thread(
		RunThread, "BMidi Run Thread",
		B_REAL_TIME_PRIORITY, (void*) this);

	status_t ret = resume_thread(runThread);

	if (ret == B_OK) 
	{ 
		isRunning = true; 
	}

	return ret;
}

//------------------------------------------------------------------------------

void BMidi::Stop()
{
	status_t exitValue;
	wait_for_thread(runThread, &exitValue);

	isRunning = false;
}

//------------------------------------------------------------------------------

bool BMidi::IsRunning() const
{
	return isRunning;
}

//------------------------------------------------------------------------------

void BMidi::Connect(BMidi* toObject)
{
	connections->AddItem(toObject);
}

//------------------------------------------------------------------------------

void BMidi::Disconnect(BMidi* fromObject)
{
	connections->RemoveItem(fromObject);
}

//------------------------------------------------------------------------------

bool BMidi::IsConnected(BMidi* toObject) const
{
	return connections->HasItem(toObject);
}

//------------------------------------------------------------------------------

BList* BMidi::Connections() const
{
	return connections;
}

//------------------------------------------------------------------------------

void BMidi::SnoozeUntil(uint32 time) const
{
	snooze_until((uint64) time * 1000, B_SYSTEM_TIMEBASE);
}

//------------------------------------------------------------------------------

void BMidi::SprayNoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_NOTE_OFF;
	event.noteOff.channel = channel;
	event.noteOff.note = note;
	event.noteOff.velocity = velocity;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayNoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_NOTE_ON;
	event.noteOn.channel = channel;
	event.noteOn.note = note;
	event.noteOn.velocity = velocity;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayKeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_KEY_PRESSURE;
	event.keyPressure.channel = channel;
	event.keyPressure.note = note;
	event.keyPressure.pressure = pressure;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, 
	uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_CONTROL_CHANGE;
	event.controlChange.channel = channel;
	event.controlChange.controlNumber = controlNumber;
	event.controlChange.controlValue = controlValue;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayProgramChange(
	uchar channel, uchar programNumber, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_PROGRAM_CHANGE;
	event.programChange.channel = channel;
	event.programChange.programNumber = programNumber;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayChannelPressure(
	uchar channel, uchar pressure, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_CHANNEL_PRESSURE;
	event.channelPressure.channel = channel;
	event.channelPressure.pressure = pressure;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayPitchBend(
	uchar channel, uchar lsb, uchar msb, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_PITCH_BEND;
	event.pitchBend.channel = channel;
	event.pitchBend.lsb = lsb;
	event.pitchBend.msb = msb;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SpraySystemExclusive(
	void* data, size_t dataLength, uint32 time) const
{
	//TODO: Should this data be duplicated!!??

	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_SYSTEM_EXCLUSIVE;
	event.systemExclusive.data = (uint8*) data;
	event.systemExclusive.dataLength = dataLength;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SpraySystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_SYSTEM_COMMON;
	event.systemCommon.status = status;
	event.systemCommon.data1 = data1;
	event.systemCommon.data2 = data2;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SpraySystemRealTime(uchar status, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_SYSTEM_REAL_TIME;
	event.systemRealTime.status = status;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

void BMidi::SprayTempoChange(int32 beatsPerMinute, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_TEMPO_CHANGE;
	event.tempoChange.beatsPerMinute = beatsPerMinute;
	SprayMidiEvent(&event);
}

//------------------------------------------------------------------------------

bool BMidi::KeepRunning()
{
	return isRunning;
}

//------------------------------------------------------------------------------

void BMidi::_ReservedMidi1() { }
void BMidi::_ReservedMidi2() { }
void BMidi::_ReservedMidi3() { }

//------------------------------------------------------------------------------

void BMidi::Run()
{
	while (KeepRunning())
	{
		snooze(50000);
	}
}

//------------------------------------------------------------------------------

void BMidi::Inflow()
{
	BMidiEvent event;
	int32 code;
	size_t len;

	while (true) 
	{
		len = read_port(inflowPort, &code, &event, sizeof(event));
	
		if (code == MSG_CODE_TERMINATE_THREAD) { return; }

		if (len != sizeof(event)) { continue; }  // ignore errors

		switch (event.opcode) 
		{
			case BMidiEvent::OP_NONE:
			case BMidiEvent::OP_TRACK_END:
			case BMidiEvent::OP_ALL_NOTES_OFF:
				break;

			case BMidiEvent::OP_NOTE_OFF:
				NoteOff(
					event.noteOff.channel,
					event.noteOff.note,
					event.noteOff.velocity,
					event.time);
				break;

			case BMidiEvent::OP_NOTE_ON:
				NoteOff(
					event.noteOn.channel,
					event.noteOn.note,
					event.noteOn.velocity,
					event.time);
				break;

			case BMidiEvent::OP_KEY_PRESSURE:
				KeyPressure(
					event.keyPressure.channel,
					event.keyPressure.note,
					event.keyPressure.pressure,
					event.time);
				break;

			case BMidiEvent::OP_CONTROL_CHANGE:
				ControlChange(
					event.controlChange.channel,
					event.controlChange.controlNumber,
					event.controlChange.controlValue,
					event.time);
				break;

			case BMidiEvent::OP_PROGRAM_CHANGE:
				ProgramChange(
					event.programChange.channel,
					event.programChange.programNumber,
					event.time);
				break;

			case BMidiEvent::OP_CHANNEL_PRESSURE:
				ChannelPressure(
					event.channelPressure.channel,
					event.channelPressure.pressure,
					event.time);
				break;

			case BMidiEvent::OP_PITCH_BEND:
				PitchBend(
					event.pitchBend.channel,
					event.pitchBend.lsb,
					event.pitchBend.msb,
					event.time);
				break;

			case BMidiEvent::OP_SYSTEM_EXCLUSIVE:
				SystemExclusive(
					event.systemExclusive.data,
					event.systemExclusive.dataLength,
					event.time);
				break;

			case BMidiEvent::OP_SYSTEM_COMMON:
				SystemCommon(
					event.systemCommon.status,
					event.systemCommon.data1,
					event.systemCommon.data2,
					event.time);
				break;

			case BMidiEvent::OP_SYSTEM_REAL_TIME:
				SystemRealTime(
					event.systemRealTime.status, 
					event.time);
				break;

			case BMidiEvent::OP_TEMPO_CHANGE:
				TempoChange(
					event.tempoChange.beatsPerMinute, 
					event.time);
				break;

			default: break;
		}
	}
}

//------------------------------------------------------------------------------

void BMidi::SprayMidiEvent(BMidiEvent* event) const
{
	TRACE(("[midi] BMidi::SprayMidiEvent, event time = %u, "
		   "current time = %u\n", event->time, B_NOW))

	int32 count = connections->CountItems();

	for (int32 i = 0; i < count; ++i) 
	{
		write_port(
			((BMidi*) connections->ItemAt(i))->inflowPort, 
			MSG_CODE_PROCESS_EVENT, event, sizeof(*event));
	}
}

//------------------------------------------------------------------------------

int32 BMidi::RunThread(void* data) 
{
	((BMidi*) data)->Run(); 
	return 0;
}

//------------------------------------------------------------------------------

int32 BMidi::InflowThread(void* data) 
{
	((BMidi*) data)->Inflow();
	return 0;
}

//------------------------------------------------------------------------------

