/*
 * Copyright (c) 2003 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "PortDrivers.h"

//------------------------------------------------------------------------------

MidiPortConsumer::MidiPortConsumer(int fd_, const char* name = NULL)
	: BMidiLocalConsumer(name)
{
	fd = fd_;
}

//------------------------------------------------------------------------------

void MidiPortConsumer::Data(
	uchar* data, size_t length, bool atomic, bigtime_t time)
{
	snooze_until(time - Latency(), B_SYSTEM_TIMEBASE);

	if (write(fd, data, length) == -1)
	{
		perror("Error sending data to driver");
	}
}

//------------------------------------------------------------------------------

MidiPortProducer::MidiPortProducer(int fd_, const char *name = NULL)
	: BMidiLocalProducer(name)
{
	fd = fd_;
	keepRunning = true;

	thread_id thread = spawn_thread(
		SpawnThread, "MidiDriversProducer", B_URGENT_PRIORITY, this);

	resume_thread(thread);
}

//------------------------------------------------------------------------------

MidiPortProducer::~MidiPortProducer()
{
	keepRunning = false;
}

//------------------------------------------------------------------------------

int32 MidiPortProducer::SpawnThread(void* data)
{
	return ((MidiPortProducer*) data)->GetData();
}

//------------------------------------------------------------------------------

int32 MidiPortProducer::GetData()
{
	uchar data[3];

	while (keepRunning)
	{
		if (read(fd, data, 1) != 1)
		{
			perror("Error reading data from driver");
			return B_ERROR;
		}

		switch (data[0] & 0xF0)
		{
			case B_NOTE_OFF:
				read(fd, data + 1, 2);
				SprayNoteOff(data[0] & 0x0F, data[1], data[2]);
				break;

			case B_NOTE_ON:
				read(fd, data + 1, 2);
				SprayNoteOn(data[0] & 0x0F, data[1], data[2]);
				break;

			case B_KEY_PRESSURE:
				read(fd, data + 1, 2);
				SprayKeyPressure(data[0] & 0x0F, data[1], data[2]);
				break;

			case B_CONTROL_CHANGE:
				read(fd, data + 1, 2);
				SprayControlChange(data[0] & 0x0F, data[1], data[2]);
				break;

			case B_PROGRAM_CHANGE:
				read(fd, data + 1, 1);
				SprayProgramChange(data[0] & 0x0F, data[1]);
				break;

			case B_CHANNEL_PRESSURE:
				read(fd, data + 1, 1);
				SprayChannelPressure(data[0] & 0x0F, data[1]);
				break;

			case B_PITCH_BEND:
				read(fd, data + 1, 2);
				SprayPitchBend(data[0] & 0x0F, data[1], data[2]);
				break;
		}

		switch (data[0])
		{
			case B_SYS_EX_START:
			{
				size_t msg_size = 4096;
				uchar* msg = (uchar*) malloc(msg_size);
				size_t count = 0;

				while (read(fd, msg + count, 1) == 1)
				{
					// Realtime events may interleave System Exclusives. Any
					// non-realtime status byte (not just 0xF7) ends a sysex.

					if (msg[count] >= 0xF8)
					{
						SpraySystemRealTime(msg[count]);
					} 
					else if (msg[count] >= 0xF0)
					{
						SpraySystemExclusive(msg, count - 1);
						break;
					}
					else  // a normal data byte
					{
						++count;
						if (count == msg_size)
						{
							msg_size *= 2;
							msg = (uchar*) realloc(msg, msg_size);
						}
					}
				}

				free(msg);
				break;
			}

			case B_TUNE_REQUEST:
			case B_SYS_EX_END:
				SpraySystemCommon(data[0], 0, 0);
				break;

			case B_MIDI_TIME_CODE:
			case B_SONG_SELECT:
			case B_CABLE_MESSAGE:
				read(fd, data + 1, 1);
				SpraySystemCommon(data[0], data[1], 0);
				break;

			case B_SONG_POSITION:
				read(fd, data + 1, 2);
				SpraySystemCommon(data[0], data[1], data[2]);
				break;

			case B_TIMING_CLOCK:
			case B_START:
			case B_CONTINUE:
			case B_STOP:
			case B_ACTIVE_SENSING:
			case B_SYSTEM_RESET:
				SpraySystemRealTime(data[0]);
				break;
		}
	}

	return B_OK;
}

//------------------------------------------------------------------------------
