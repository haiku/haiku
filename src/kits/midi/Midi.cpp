/**
 * @file Midi.cpp
 * 
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 * @author Paul Stadler
 */

#include <List.h>

#include "Midi.h"
#include "debug.h"
#include "MidiEvent.h"

//-----------------------------------------------

#define MSG_CODE_PROCESS_EVENT 1
#define MSG_CODE_TERMINATE_THREAD 0

//-----------------------------------------------

//-----------------------------------------------------------------------------
// Initialization Stuff
BMidi::BMidi()
{
	fConnectionList = new BList();
	fIsRunning = false;
	
	fInflowPort = create_port(1, "Inflow Port");
	
	fInflowTask = spawn_thread(
		_inflow_task_, "BMidi Inflow Thread",
		B_REAL_TIME_PRIORITY, (void *)this);
		
	fInflowAlive = resume_thread(fInflowTask) == B_OK;
}

BMidi::~BMidi()
{
	write_port(fInflowPort, MSG_CODE_TERMINATE_THREAD, NULL, 0);

	status_t exit_value;
	wait_for_thread(fInflowTask, &exit_value);

	delete_port(fInflowPort);
	delete fConnectionList;
}

//-----------------------------------------------------------------------------
// Stubs for midi event hooks.
void BMidi::NoteOff(uchar chan, uchar note, uchar vel, uint32 time)
{
}
void BMidi::NoteOn(uchar chan, uchar note, uchar vel, uint32 time)
{
}
void BMidi::KeyPressure(uchar chan, uchar note, uchar pres, uint32 time)
{
}
void BMidi::ControlChange(uchar chan, uchar ctrl_num, uchar ctrl_val, uint32 time)
{
}
void BMidi::ProgramChange(uchar chan, uchar prog_num, uint32 time)
{
}
void BMidi::ChannelPressure(uchar chan, uchar pres, uint32 time)
{
}
void BMidi::PitchBend(uchar chan, uchar lsb, uchar msb, uint32 time)
{
}
void BMidi::SystemExclusive(void * data, size_t data_len, uint32 time)
{
}
void BMidi::SystemCommon(uchar stat_byte, uchar data1, uchar data2, uint32 time)
{
}
void BMidi::SystemRealTime(uchar stat_byte, uint32 time)
{
}
void BMidi::TempoChange(int32 bpm, uint32 time)
{
}
void BMidi::AllNotesOff(bool just_chan, uint32 time)
{
}

//-----------------------------------------------------------------------------
// Public API Functions
status_t BMidi::Start()
{
	fRunTask = spawn_thread(_run_thread_,
		"BMidi Run Thread", B_REAL_TIME_PRIORITY, (void *)this);
	fIsRunning = true;
	status_t ret = resume_thread(fRunTask);
	if(ret != B_OK)
	{
		fIsRunning = false;
	}
	return B_OK;
}

//-----------------------------------------------------------------------------

void BMidi::Stop()
{
	status_t exit_value;
	status_t ret;
	fIsRunning = false;
	ret = wait_for_thread(fRunTask, &exit_value);
}
    
bool BMidi::IsRunning() const
{
	thread_info info;
	get_thread_info(fRunTask,&info);
	if(find_thread("BMidi Run Thread") == fRunTask)
		return true;
return false;
}

void BMidi::Connect(BMidi *to_object)
{
	fConnectionList->AddItem((void *)to_object);
}

void BMidi::Disconnect(BMidi *from_object)
{
	fConnectionList->RemoveItem((void *)from_object);
}

bool BMidi::IsConnected(BMidi * to_object) const
{
	return fConnectionList->HasItem((void *)to_object);
}

BList * BMidi::Connections() const
{
	return fConnectionList;
}

void BMidi::SnoozeUntil(uint32 time) const
{
	snooze_until((uint64)time*1000, B_SYSTEM_TIMEBASE);
}

bool BMidi::KeepRunning()
{
	return fIsRunning;
}

void BMidi::Run()
{
	while(KeepRunning())
		snooze(50000);
}

//-----------------------------------------------------------------------------
// Spray Functions
void BMidi::SprayMidiEvent(BMidiEvent *e) const
{
//	TRACE(("Hello"))
	int32 num_connections = fConnectionList->CountItems();
	BMidi *m;
	for(int32 i = 0; i < num_connections; i++)
	{
		m = (BMidi *)fConnectionList->ItemAt(i);
		write_port(m->fInflowPort, MSG_CODE_PROCESS_EVENT, e, sizeof(BMidiEvent));
	}
}

//----------------------

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

//----------------------

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

//----------------------

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

//----------------------

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

//----------------------

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

//----------------------

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

//----------------------

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

//----------------------

void BMidi::SpraySystemExclusive(
	void * data, size_t dataLength, uint32 time) const
{
	// Should this data be duplicated!!?? I think Yes.
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_SYSTEM_EXCLUSIVE;
	event.systemExclusive.data = new uint8[dataLength];
	memcpy(event.systemExclusive.data, data, dataLength);
	event.systemExclusive.dataLength = dataLength;
	SprayMidiEvent(&event);
}

//----------------------

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

//----------------------

void BMidi::SpraySystemRealTime(uchar status, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_SYSTEM_REAL_TIME;
	event.systemRealTime.status = status;
	SprayMidiEvent(&event);
}

//----------------------

void BMidi::SprayTempoChange(int32 beatsPerMinute, uint32 time) const
{
	BMidiEvent event;
	event.time = time;
	event.opcode = BMidiEvent::OP_TEMPO_CHANGE;
	event.tempoChange.beatsPerMinute = beatsPerMinute;
	SprayMidiEvent(&event);
}

//-----------------------------------------------------------------------------
// The Inflow Thread
void BMidi::InflowTask()
{
	BMidiEvent event;
	int32 code;
	size_t len;
	while(true)
	{
		len = read_port(fInflowPort, &code, &event, sizeof(BMidiEvent));

		if (code == MSG_CODE_TERMINATE_THREAD) { return; }

		if (len != sizeof(BMidiEvent)) { continue; }  //ignore errors

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
				NoteOn(
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

//----------------------
//----------------------
//----------------------
//----------------------
//----------------------
//----------------------
//----------------------
//----------------------

void BMidi::_ReservedMidi1() {}
void BMidi::_ReservedMidi2() {}
void BMidi::_ReservedMidi3() {}

//----------------------

status_t _run_thread_(void *data)
{
	((BMidi*)data)->Run();
	return 0;
}

//----------------------

status_t _inflow_task_(void * data)
{
	((BMidi*)data)->InflowTask();
	return 0;
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
