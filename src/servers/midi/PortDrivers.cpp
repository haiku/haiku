/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Christian Packmann
 *		Jerome Leveque
 *		Philippe Houdoin
 *		Pete Goodeve
 */


#include "PortDrivers.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <String.h>

MidiPortConsumer::MidiPortConsumer(int fd, const char* name)
	: BMidiLocalConsumer(name),
	fFileDescriptor(fd)
{
}


void 
MidiPortConsumer::Data(uchar* data, size_t length, 
	bool atomic, bigtime_t time)
{
	snooze_until(time - Latency(), B_SYSTEM_TIMEBASE);

	if (write(fFileDescriptor, data, length) == -1) {
		perror("Error sending data to driver");
	}
}


// #pragma mark -


MidiPortProducer::MidiPortProducer(int fd, const char *name)
	: BMidiLocalProducer(name),
	fFileDescriptor(fd), fKeepRunning(true)
	
{
	BString tmp = name;
	tmp << " reader";
	
	fReaderThread = spawn_thread(
		_ReaderThread, tmp.String(), B_URGENT_PRIORITY, this);

	resume_thread(fReaderThread);
}


MidiPortProducer::~MidiPortProducer()
{
	fKeepRunning = false;
	
	status_t dummy;
	wait_for_thread(fReaderThread, &dummy);
}


int32 
MidiPortProducer::_ReaderThread(void* data)
{
	return ((MidiPortProducer*) data)->GetData();
}


int32 
MidiPortProducer::GetData()
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

	while (fKeepRunning) {
		if (read(fFileDescriptor, &next, 1) != 1) {
			if (errno == B_CANCELED) 
				fKeepRunning = false;
			else 
				perror("Error reading data from driver");
			break;
		}
	
		bigtime_t timestamp = system_time();

		if (haveSysEx) { 
			// System Exclusive mode
			if (next < 0x80) { 
				// System Exclusive data byte
				sysexBuf[sysexSize++] = next;
				if (sysexSize == sysexAlloc) {
					sysexAlloc *= 2;
					sysexBuf = (uint8*) realloc(sysexBuf, sysexAlloc);
				}
				continue;
			} else if ((next & 0xF8) == 0xF8) {
				// System Realtime interleaved in System Exclusive sequence
				SpraySystemRealTime(next, timestamp);
				continue;
			} else {  
				// Whatever byte, this one ends the running SysEx sequence
				SpraySystemExclusive(sysexBuf, sysexSize, timestamp);
				haveSysEx = false;
				if (next == B_SYS_EX_END) {
					// swallow SysEx end byte
					continue;
				}
				// any other byte, while ending the SysEx sequence, 
				// should be handled, not dropped
			}
		}
		
		if ((next & 0xF8) == 0xF8) {
			// System Realtime
			SpraySystemRealTime(next, timestamp);
		} else if ((next & 0xF0) == 0xF0) {
			// System Common
			runningStatus = 0;
			msgBuf[0] = next;
			msgPtr = msgBuf + 1;
			switch (next) {
				case B_SYS_EX_START:
					sysexAlloc = 4096;
					sysexBuf = (uint8*) malloc(sysexAlloc);
					sysexSize = 0;
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
			
				case B_SYS_EX_END:	
					// Unpaired with B_SYS_EX_START, but pass it anyway...
				case B_TUNE_REQUEST:
					SpraySystemCommon(next, 0, 0, timestamp);
					break;
			}			
		} else if ((next & 0x80) == 0x80) {
			// Voice message
			runningStatus = next;
			msgBuf[0] = next;
			msgPtr = msgBuf + 1;
			switch (next & 0xF0) {
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
		} else if (needed > 0) {
			// Data bytes to complete message
			*msgPtr++ = next;
			if (--needed == 0) {
				switch (msgBuf[0] & 0xF0) {
					case B_NOTE_OFF:
						SprayNoteOff(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2],
							timestamp);
						break;

					case B_NOTE_ON:
						SprayNoteOn(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2],
							timestamp);
						break;

					case B_KEY_PRESSURE:
						SprayKeyPressure(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2],
							timestamp);
						break;

					case B_CONTROL_CHANGE:
						SprayControlChange(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2],
							timestamp);
						break;

					case B_PROGRAM_CHANGE:
						SprayProgramChange(msgBuf[0] & 0x0F, msgBuf[1],
							timestamp);
						break;

					case B_CHANNEL_PRESSURE:
						SprayChannelPressure(msgBuf[0] & 0x0F, msgBuf[1],
							timestamp);
						break;

					case B_PITCH_BEND:
						SprayPitchBend(msgBuf[0] & 0x0F, msgBuf[1], msgBuf[2],
							timestamp);
						break;
				}

				switch (msgBuf[0]) {
					case B_SONG_POSITION:
						SpraySystemCommon(msgBuf[0], msgBuf[1], msgBuf[2],
							timestamp);
						break;

					case B_MIDI_TIME_CODE:
					case B_SONG_SELECT:
					case B_CABLE_MESSAGE:
						SpraySystemCommon(msgBuf[0], msgBuf[1], 0, timestamp);
						break;
				}
			}
		} else if (runningStatus != 0) {
			// Repeated voice command
			msgBuf[0] = runningStatus;
			msgBuf[1] = next;
			msgPtr = msgBuf + 2;
			needed = msgSize - 2;
		}
	}	// while fKeepRunning

	if (haveSysEx)
		free(sysexBuf);

	return fKeepRunning ? errno : B_OK;
}
