/*

MidiDelay.cpp

Copyright (c) 2002 OpenBeOS. 

A filter for Midi Kit 1 that sprays midi events 
when the performance time is reached. 

Author: 
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <OS.h>

#include "MidiDelay.h"

void MidiDelay::NoteOff(uchar channel, 
						uchar note, 
						uchar velocity,
						uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayNoteOff(channel, note, velocity, time);
}

void MidiDelay::NoteOn(uchar channel, 
					   uchar note, 
					   uchar velocity,
			    	   uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayNoteOn(channel, note, velocity, time);
}


void MidiDelay::KeyPressure(uchar channel, 
							uchar note, 
							uchar pressure,
							uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayKeyPressure(channel, note, pressure, time);
}


void MidiDelay::ControlChange(uchar channel, 
							  uchar controlNumber,
							  uchar controlValue, 
							  uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayControlChange(channel, controlNumber, controlValue, time);
}


void MidiDelay::ProgramChange(uchar channel, 
								uchar programNumber,
							  	uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayProgramChange(channel, programNumber, time);
}


void MidiDelay::ChannelPressure(uchar channel, 
								uchar pressure, 
								uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayChannelPressure(channel, pressure, time);
}


void MidiDelay::PitchBend(uchar channel, 
						  uchar lsb, 
						  uchar msb,
			    		  uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayPitchBend(channel, lsb, msb, time);
}


void MidiDelay::SystemExclusive(void* data, 
								size_t dataLength, 
								uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SpraySystemExclusive(data, dataLength, time);
}


void MidiDelay::SystemCommon(uchar statusByte, 
							 uchar data1, 
							 uchar data2,
							 uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SpraySystemCommon(statusByte, data1, data2, time);
}


void MidiDelay::SystemRealTime(uchar statusByte, uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SpraySystemRealTime(statusByte, time);
}


void MidiDelay::TempoChange(int32 bpm, uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	SprayTempoChange(bpm, time);
}


void MidiDelay::AllNotesOff(bool justChannel = true, uint32 time = B_NOW) {
	snooze_until(ToBigtime(time), B_SYSTEM_TIMEBASE);
	for (uchar channel = 0; channel < 16; channel ++) {
		SprayControlChange(channel, B_ALL_NOTES_OFF, 0, time);
	}
}

