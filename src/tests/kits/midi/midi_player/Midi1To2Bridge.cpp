/*

Midi1To2Bridge.cpp

Copyright (c) 2002 OpenBeOS.

Midi Kit 1 input to Midi Kit 2 input (consumer) bridge.
A Midi Kit 1 input node that passes the midi events via a
Midi Kit 2 producer to Midi Kit 2 consumers.

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

#include "Midi1To2Bridge.h"

Midi1To2Bridge::Midi1To2Bridge(const char *name) {
	fOutput = new BMidiLocalProducer(name);
	fOutput->Register();
}

Midi1To2Bridge::~Midi1To2Bridge() {
	fOutput->Unregister();
	fOutput->Release();
}

void Midi1To2Bridge::NoteOff(uchar channel,
						uchar note,
						uchar velocity,
						uint32 time = B_NOW) {
	fOutput->SprayNoteOff(channel-1, note, velocity, ToBigtime(time));
}

void Midi1To2Bridge::NoteOn(uchar channel,
					   uchar note,
					   uchar velocity,
			    	   uint32 time = B_NOW) {
	fOutput->SprayNoteOn(channel-1, note, velocity, ToBigtime(time));
}


void Midi1To2Bridge::KeyPressure(uchar channel,
							uchar note,
							uchar pressure,
							uint32 time = B_NOW) {
	fOutput->SprayKeyPressure(channel-1, note, pressure, ToBigtime(time));
}


void Midi1To2Bridge::ControlChange(uchar channel,
							  uchar controlNumber,
							  uchar controlValue,
							  uint32 time = B_NOW) {
	fOutput->SprayControlChange(channel-1, controlNumber, controlValue, ToBigtime(time));
}


void Midi1To2Bridge::ProgramChange(uchar channel,
								uchar programNumber,
							  	uint32 time = B_NOW) {
	fOutput->SprayProgramChange(channel-1, programNumber, ToBigtime(time));
}


void Midi1To2Bridge::ChannelPressure(uchar channel,
								uchar pressure,
								uint32 time = B_NOW) {
	fOutput->SprayChannelPressure(channel-1, pressure, ToBigtime(time));
}


void Midi1To2Bridge::PitchBend(uchar channel,
						  uchar lsb,
						  uchar msb,
			    		  uint32 time = B_NOW) {
	fOutput->SprayPitchBend(channel-1, lsb, msb, ToBigtime(time));
}


void Midi1To2Bridge::SystemExclusive(void* data,
								size_t dataLength,
								uint32 time = B_NOW) {
	fOutput->SpraySystemExclusive(data, dataLength, ToBigtime(time));
}


void Midi1To2Bridge::SystemCommon(uchar statusByte,
							 uchar data1,
							 uchar data2,
							 uint32 time = B_NOW) {
	fOutput->SpraySystemCommon(statusByte, data1, data2, ToBigtime(time));
}


void Midi1To2Bridge::SystemRealTime(uchar statusByte, uint32 time = B_NOW) {
	fOutput->SpraySystemRealTime(statusByte, ToBigtime(time));
}


void Midi1To2Bridge::TempoChange(int32 bpm, uint32 time = B_NOW) {
	fOutput->SprayTempoChange(bpm, ToBigtime(time));
}


void Midi1To2Bridge::AllNotesOff(bool justChannel = true, uint32 time = B_NOW) {
	bigtime_t t = ToBigtime(time);
	for (uchar channel = 0; channel < 16; channel ++) {
		fOutput->SprayControlChange(channel, B_ALL_NOTES_OFF, 0, t);
	}
}

