/*
 * Copyright 2005-2006, Haiku.
 * 
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */

#include <stdlib.h>

#include "debug.h"
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <MidiRoster.h>
#include "protocol.h"


BMidiLocalProducer::BMidiLocalProducer(const char* name)
	: BMidiProducer(name)
{
	TRACE(("BMidiLocalProducer::BMidiLocalProducer"))

	fIsLocal = true;
	fRefCount = 1;

	BMidiRoster::MidiRoster()->CreateLocal(this);
}


BMidiLocalProducer::~BMidiLocalProducer()
{
	TRACE(("BMidiLocalProducer::~BMidiLocalProducer"))

	BMidiRoster::MidiRoster()->DeleteLocal(this);
}


void
BMidiLocalProducer::Connected(BMidiConsumer* cons)
{
	ASSERT(cons != NULL)
	TRACE(("Connected() %ld to %ld", ID(), cons->ID()))

	// Do nothing.
}


void
BMidiLocalProducer::Disconnected(BMidiConsumer* cons)
{
	ASSERT(cons != NULL)
	TRACE(("Disconnected() %ld from %ld", ID(), cons->ID()))

	// Do nothing.
}


void
BMidiLocalProducer::SprayData(void* data, size_t length,
	bool atomic, bigtime_t time) const
{
	SprayEvent(data, length, atomic, time);
}


void
BMidiLocalProducer::SprayNoteOff(uchar channel, uchar note,
	uchar velocity, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[3];
		data[0] = B_NOTE_OFF + channel;
		data[1] = note;
		data[2] = velocity;

		SprayEvent(&data, 3, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SprayNoteOn(uchar channel, uchar note,
	uchar velocity, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[3];
		data[0] = B_NOTE_ON + channel;
		data[1] = note;
		data[2] = velocity;

		SprayEvent(&data, 3, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SprayKeyPressure(uchar channel, uchar note,
	uchar pressure, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[3];
		data[0] = B_KEY_PRESSURE + channel;
		data[1] = note;
		data[2] = pressure;

		SprayEvent(&data, 3, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SprayControlChange(uchar channel,
	uchar controlNumber, uchar controlValue, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[3];
		data[0] = B_CONTROL_CHANGE + channel;
		data[1] = controlNumber;
		data[2] = controlValue;

		SprayEvent(&data, 3, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SprayProgramChange(uchar channel,
	uchar programNumber, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[2];
		data[0] = B_PROGRAM_CHANGE + channel;
		data[1] = programNumber;

		SprayEvent(&data, 2, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SprayChannelPressure(uchar channel,
	uchar pressure, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[2];
		data[0] = B_CHANNEL_PRESSURE + channel;
		data[1] = pressure;

		SprayEvent(&data, 2, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SprayPitchBend(uchar channel,
	uchar lsb, uchar msb, bigtime_t time) const
{
	if (channel < 16) {
		uchar data[3];
		data[0] = B_PITCH_BEND + channel;
		data[1] = lsb;
		data[2] = msb;

		SprayEvent(&data, 3, true, time);
	} else {
		debugger("invalid MIDI channel");
	}
}


void
BMidiLocalProducer::SpraySystemExclusive(void* data,
	size_t length, bigtime_t time) const
{
	SprayEvent(data, length, true, time, true);
}


void
BMidiLocalProducer::SpraySystemCommon(uchar status, uchar data1,
	uchar data2, bigtime_t time) const
{
	size_t len;
	uchar data[3];
	data[0] = status;
	data[1] = data1;
	data[2] = data2;

	switch (status) {
		case B_TUNE_REQUEST:
		case B_SYS_EX_END:
			len = 1;
			break;

		case B_CABLE_MESSAGE:
		case B_MIDI_TIME_CODE:
		case B_SONG_SELECT:
			len = 2;
			break;

		case B_SONG_POSITION:
			len = 3;
			break;

		default:
			debugger("invalid system common status");
			len = 0;
	}

	SprayEvent(&data, len, true, time);
}


void
BMidiLocalProducer::SpraySystemRealTime(uchar status,
	bigtime_t time) const
{
	if (status >= B_TIMING_CLOCK)
		SprayEvent(&status, 1, true, time);
	else
		debugger("invalid real time status");
}


void
BMidiLocalProducer::SprayTempoChange(int32 beatsPerMinute,
	bigtime_t time) const
{
	int32 tempo = 60000000 / beatsPerMinute;

	uchar data[6];
	data[0] = 0xFF;
	data[1] = 0x51;
	data[2] = 0x03;
	data[3] = tempo >> 16;
	data[4] = tempo >> 8;
	data[5] = tempo;

	SprayEvent(&data, 6, true, time);
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::_Reserved1() { }
void BMidiLocalProducer::_Reserved2() { }
void BMidiLocalProducer::_Reserved3() { }
void BMidiLocalProducer::_Reserved4() { }
void BMidiLocalProducer::_Reserved5() { }
void BMidiLocalProducer::_Reserved6() { }
void BMidiLocalProducer::_Reserved7() { }
void BMidiLocalProducer::_Reserved8() { }

//------------------------------------------------------------------------------

void
BMidiLocalProducer::SprayEvent(const void* data, size_t length,
	bool atomic, bigtime_t time, bool sysex) const
{
	if (LockProducer()) {
		if (CountConsumers() > 0) {
			// We don't just send the MIDI event data to all connected
			// consumers, we also send a header. The header contains our
			// ID (4 bytes), the consumer's ID (4 bytes), the performance
			// time (8 bytes), whether the data is atomic (1 byte), and
			// padding (3 bytes). The MIDI event data follows the header.

			size_t buf_size = 20 + length;
			if (sysex) {
				// add 0xF0 and 0xF7 markers
				buf_size += 2;
			}

			uint8* buffer = (uint8*)malloc(buf_size);
			if (buffer != NULL) {
				*((uint32*)    (buffer +  0)) = fId;
				*((bigtime_t*) (buffer +  8)) = time;
				*((uint32*)    (buffer + 16)) = 0;
				*((bool*)      (buffer + 16)) = atomic;

				if (sysex) {
					*((uint8*) (buffer + 20)) = B_SYS_EX_START;
					if (data != NULL)
						memcpy(buffer + 21, data, length);

					*((uint8*) (buffer + buf_size - 1)) = B_SYS_EX_END;
				} else if (data != NULL) {
					memcpy(buffer + 20, data, length);
				}

				for (int32 t = 0; t < CountConsumers(); ++t) {
					BMidiConsumer* cons = ConsumerAt(t);
					*((uint32*) (buffer + 4)) = cons->fId;

					#ifdef DEBUG
					printf("*** spraying: ");
					for (uint32 t = 0; t < buf_size; ++t)
					{
						printf("%02X, ", buffer[t]);
					}
					printf("\n");
					#endif

					write_port(cons->fPort, 0, buffer, buf_size);
				}				

				free(buffer);
			}
		}

		UnlockProducer();
	}
}

