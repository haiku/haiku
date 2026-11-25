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
	:
	BMidiLocalConsumer(name),
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
	:
	BMidiLocalProducer(name),
	fFileDescriptor(fd),
	fKeepRunning(true)
{
	BString tmp = name;
	tmp << " producer";

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
	uint8 bytes[256];
	ssize_t bytesCount = 0;
	uint8* nextByte = NULL;
	bigtime_t timestamp = 0;

	uint8 msgBuf[3];
	uint8* msgPtr = NULL;
	size_t msgSize = 0;
	size_t needed = 0;
	uint8 runningStatus = 0;

	bool haveSysEx = false;
	uint8* sysexBuffer = NULL;
	size_t sysexBufferSize = 0;
	size_t sysexSize = 0;

	while (fKeepRunning) {
		if (bytesCount == 0) {
			bytesCount = read(fFileDescriptor, bytes, sizeof(bytes));
			if (bytesCount < 0) {
				if (errno == B_CANCELED)
					fKeepRunning = false;
				else
					perror("Error reading data from driver");
				break;
			}

			timestamp = system_time();
			nextByte = bytes;
		}

		uint8 next = *nextByte++;
		bytesCount--;

		if (haveSysEx) {
			// System Exclusive mode
			if (next < 0x80) {
				// System Exclusive data byte
				sysexBuffer[sysexSize++] = next;
				if (sysexSize == sysexBufferSize) {
					uint8* sysexBiggerBuffer = (uint8*)realloc(sysexBuffer, sysexBufferSize * 2);
					if (sysexBiggerBuffer == NULL) {
						// Out of Memory: drop this sysex sequence
						haveSysEx = false;
						continue;
					}
					sysexBufferSize *= 2;
					sysexBuffer = sysexBiggerBuffer;
				}
				continue;
			} else if ((next & 0xF8) == 0xF8) {
				// System Realtime interleaved in System Exclusive sequence
				SpraySystemRealTime(next, timestamp);
				continue;
			} else {
				// Whatever byte, this one ends the running SysEx sequence
				SpraySystemExclusive(sysexBuffer, sysexSize, timestamp);
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
					if (sysexBuffer == NULL || sysexBufferSize > 4 * B_PAGE_SIZE) {
						// first SysEx, or previous(es) SysEx was/were a bit large:
						// let's (re)start with a reasonable buffer size
						free(sysexBuffer);
						sysexBufferSize = B_PAGE_SIZE;
						sysexBuffer = (uint8*)malloc(sysexBufferSize);
					}
					sysexSize = 0;
					haveSysEx = (sysexBuffer != NULL);
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
			if (--needed == 0)
				_SprayMessage(msgBuf, timestamp);
		} else if (runningStatus != 0) {
			// Repeated voice command
			msgBuf[0] = runningStatus;
			msgBuf[1] = next;
			msgPtr = msgBuf + 2;
			needed = msgSize - 2;
			if (needed == 0)
				_SprayMessage(msgBuf, timestamp);
		}

	}	// while fKeepRunning

	free(sysexBuffer);

	return fKeepRunning ? errno : B_OK;
}


void
MidiPortProducer::_SprayMessage(const uint8* msg, bigtime_t timestamp)
{
	switch (msg[0] & 0xF0) {
		case B_NOTE_OFF:
			SprayNoteOff(msg[0] & 0x0F, msg[1], msg[2], timestamp);
			return;

		case B_NOTE_ON:
			SprayNoteOn(msg[0] & 0x0F, msg[1], msg[2], timestamp);
			return;

		case B_KEY_PRESSURE:
			SprayKeyPressure(msg[0] & 0x0F, msg[1], msg[2], timestamp);
			return;

		case B_CONTROL_CHANGE:
			SprayControlChange(msg[0] & 0x0F, msg[1], msg[2], timestamp);
			return;

		case B_PROGRAM_CHANGE:
			SprayProgramChange(msg[0] & 0x0F, msg[1], timestamp);
			return;

		case B_CHANNEL_PRESSURE:
			SprayChannelPressure(msg[0] & 0x0F, msg[1], timestamp);
			return;

		case B_PITCH_BEND:
			SprayPitchBend(msg[0] & 0x0F, msg[1], msg[2], timestamp);
			return;
	}

	switch (msg[0]) {
		case B_SONG_POSITION:
			SpraySystemCommon(msg[0], msg[1], msg[2], timestamp);
			break;

		case B_MIDI_TIME_CODE:
		case B_SONG_SELECT:
		case B_CABLE_MESSAGE:
			SpraySystemCommon(msg[0], msg[1], 0, timestamp);
			break;
	}
}
