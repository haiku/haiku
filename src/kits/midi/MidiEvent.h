//-----------------------------------------------------------------------------
//
#ifndef _MIDI_EVENT_H_
#define _MIDI_EVENT_H_

#include <MidiDefs.h>

class BMidiEvent {
public:
	BMidiEvent();
	~BMidiEvent();

	enum Opcode {
		OP_NONE,
		OP_NOTE_OFF,
		OP_NOTE_ON,
		OP_KEY_PRESSURE,
		OP_CONTROL_CHANGE,
		OP_PROGRAM_CHANGE,
		OP_CHANNEL_PRESSURE,
		OP_PITCH_BEND,
		OP_SYSTEM_EXCLUSIVE,
		OP_SYSTEM_COMMON,
		OP_SYSTEM_REAL_TIME,
		OP_TEMPO_CHANGE,
		OP_ALL_NOTES_OFF,
		OP_TRACK_END,
		OP_NUM
	};

	uint32 time;
	Opcode opcode;
	union Data {
		struct NoteOffData {
			uint8 channel;
			uint8 note;
			uint8 velocity;
		} note_off;
		struct NoteOnData {
			uint8 channel;
			uint8 note;
			uint8 velocity;
		} note_on;
		struct KeyPressureData {
			uint8 channel;
			uint8 note;
			uint8 pressure;
		} key_pressure;
		struct ControlChangeData {
			uint8 channel;
			uint8 number;
			uint8 value;
		} control_change;
		struct ProgramChangeData {
			uint8 channel;
			uint8 number;
		} program_change;
		struct ChannelPressureData {
			uint8 channel;
			uint8 pressure;
		} channel_pressure;
		struct PitchBendData {
			uint8 channel;
			uint8 lsb;
			uint8 msb;
		} pitch_bend;
		struct SystemExclusiveData {
			uint8 * data;
			int32 length;
		} system_exclusive;
		struct SystemCommonData {
			uint8 status;
			uint8 data1;
			uint8 data2;
		} system_common;
		struct SystemRealTimeData {
			uint8 status;
		} system_real_time;
		struct TempoChangeData {
			uint32 beats_per_minute;
		} tempo_change;
		struct AllNotesOffData {
			bool just_channel;
		} all_notes_off;
	} data;
};

#endif _MIDI_EVENT_H_