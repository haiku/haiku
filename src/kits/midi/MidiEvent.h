/**
 * @file MidiEvent.h
 *
 * @author Matthijs Hollemans
 * @author Paul Stadler
 */

#ifndef _MIDI_EVENT_H
#define _MIDI_EVENT_H

#include "MidiDefs.h"

/**
 * Describes a MIDI message and its attributes.
 */
class BMidiEvent 
{
public:
	BMidiEvent();
	~BMidiEvent();

	enum
	{
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
	}
	opcode;

	uint32 time;

	union 
	{
		struct 
		{
			uchar channel;
			uchar note;
			uchar velocity;
		} 
		noteOff;

		struct 
		{
			uchar channel;
			uchar note;
			uchar velocity;
		} 
		noteOn;

		struct
		{
			uchar channel;
			uchar note;
			uchar pressure;
		} 
		keyPressure;

		struct 
		{
			uchar channel;
			uchar controlNumber;
			uchar controlValue;
		} 
		controlChange;

		struct 
		{
			uchar channel;
			uchar programNumber;
		} 
		programChange;

		struct 
		{
			uchar channel;
			uchar pressure;
		} 
		channelPressure;

		struct 
		{
			uchar channel;
			uchar lsb;
			uchar msb;
		} 
		pitchBend;

		struct 
		{
			uint8* data;
			size_t dataLength;
		} 
		systemExclusive;

		struct 
		{
			uchar status;
			uchar data1;
			uchar data2;
		} 
		systemCommon;

		struct 
		{
			uchar status;
		} 
		systemRealTime;

		struct 
		{
			uint32 beatsPerMinute;
		} 
		tempoChange;

		struct 
		{
			bool justChannel;
		} 
		allNotesOff;
	};
};

#endif _MIDI_EVENT_H

