/*
 * Copyright (c) 2003-2004 Matthijs Hollemans
 * Copyright (c) 2004 Christian Packmann
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
		SpawnThread, "MidiPortProducer", B_URGENT_PRIORITY, this);

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
	uint8 msgBuf[3];
	uint8* sysexBuf = NULL;

	uint8* msgPtr = NULL;
	size_t msgSize = 0;
	size_t needed = 0;
	uint8 runningStatus = 0;

	bool haveSysEx = false;
	size_t sysexAlloc = 0;
	size_t sysexSize = 0;

	uint8 next = 0;

	while (keepRunning)
	{
		if (read(fd, &next, 1) != 1)
		{
			perror("Error reading data from driver");
			return B_ERROR;
		}

		if (haveSysEx)  // System Exclusive
		{
			if (next < 0x80)  // data byte
			{
				sysexBuf[sysexSize++] = next;
				if (sysexSize == sysexAlloc)
				{
					sysexAlloc *= 2;
					sysexBuf = (uint8*) realloc(sysexBuf, sysexAlloc);
				}
				continue;
			}
			else if (next < 0xF8)  // end of sysex
			{
				SpraySystemExclusive(sysexBuf, sysexSize);
				haveSysEx = false;
			}
		}
		
		if ((next & 0xF8) == 0xF8)  // System Realtime
		{
			SpraySystemRealTime(next);
		}
		else if ((next & 0xF0) == 0xF0)  // System Common
		{
			runningStatus = 0;
			msgBuf[0] = next;
			msgPtr = msgBuf + 1;
			switch (next)
			{
				case B_SYS_EX_START:
					sysexAlloc = 4096;
					sysexBuf = (uint8*) malloc(sysexAlloc);
					sysexBuf[0] = next;
					sysexSize = 1;
					haveSysEx = true;
					break;

				case B_SONG_POSITION:
					needed = 2;
					msgSize = 3;
					break;

				case B_MIDI_TIME_CODE:
				case B_SONG_SELECT:
				case B_CABLE_MESSAGE:
					needed = 1;
					msgSize = 2;
					break;
			
				case B_TUNE_REQUEST:
				case B_SYS_EX_END:
					SpraySystemCommon(next, 0, 0);
					break;
			}			
		}
		else if ((next & 0x80) == 0x80)  // Voice message
		{
			runningStatus = next;
			msgBuf[0] = next;
			msgPtr = msgBuf + 1;
			switch (next & 0xF0)
			{
				case B_NOTE_OFF:
				case B_NOTE_ON:
				case B_KEY_PRESSURE:
				case B_CONTROL_CHANGE:
				case B_PITCH_BEND:
					needed = 2;
					msgSize = 3;
					break;

				case B_PROGRAM_CHANGE:
				case B_CHANNEL_PRESSURE:
					needed = 1;
					msgSize = 2;
					break;
			}
		}
		else if (needed > 0)  // Data bytes to complete message
		{
			*msgPtr++ = next;
			if (--needed == 0)
			{
				switch (msgBuf[0] & 0xF0)
				{
					case B_NOTE_OFF:
						SprayNoteOff(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2]);
						break;

					case B_NOTE_ON:
						SprayNoteOn(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2]);
						break;

					case B_KEY_PRESSURE:
						SprayKeyPressure(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2]);
						break;

					case B_CONTROL_CHANGE:
						SprayControlChange(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2]);
						break;

					case B_PROGRAM_CHANGE:
						SprayProgramChange(msgBuf[0] & 0x0F, msgBuf[1]);
						break;

					case B_CHANNEL_PRESSURE:
						SprayChannelPressure(msgBuf[0] & 0x0F, msgBuf[1]);
						break;

					case B_PITCH_BEND:
						SprayPitchBend(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2]);
						break;
				}

				switch (msgBuf[0])
				{
					case B_SONG_POSITION:
						SpraySystemCommon(msgBuf[0], msgBuf[1], msgBuf[2]);
						break;

					case B_MIDI_TIME_CODE:
					case B_SONG_SELECT:
					case B_CABLE_MESSAGE:
						SpraySystemCommon(msgBuf[0], msgBuf[1], 0);
						break;
				}
			}
		}
		else if (runningStatus != 0)  // Repeated voice command
		{
			msgBuf[0] = runningStatus;
			msgBuf[1] = next;
			msgPtr = msgBuf + 2;
			needed = msgSize - 2;
		}
	}

	if (haveSysEx)
	{
		free(sysexBuf);
	}

	return B_OK;
}

//------------------------------------------------------------------------------
