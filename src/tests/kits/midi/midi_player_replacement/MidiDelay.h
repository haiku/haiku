/*

MidiDelay.h

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

#ifndef _MIDI_DELAY_H
#define _MIDI_DELAY_H

#include <Midi.h>

class MidiDelay : public BMidi {
	
	static inline bigtime_t ToBigtime(uint32 time) { return time * (bigtime_t)1000; }	
	
public:
	
virtual	void	NoteOff(uchar channel, 
						uchar note, 
						uchar velocity,
						uint32 time = B_NOW);

virtual	void	NoteOn(uchar channel, 
					   uchar note, 
					   uchar velocity,
			    	   uint32 time = B_NOW);

virtual	void	KeyPressure(uchar channel, 
							uchar note, 
							uchar pressure,
							uint32 time = B_NOW);

virtual	void	ControlChange(uchar channel, 
							  uchar controlNumber,
							  uchar controlValue, 
							  uint32 time = B_NOW);

virtual	void	ProgramChange(uchar channel, 
								uchar programNumber,
							  	uint32 time = B_NOW);

virtual	void	ChannelPressure(uchar channel, 
								uchar pressure, 
								uint32 time = B_NOW);

virtual	void	PitchBend(uchar channel, 
						  uchar lsb, 
						  uchar msb,
			    		  uint32 time = B_NOW);

virtual	void	SystemExclusive(void* data, 
								size_t dataLength, 
								uint32 time = B_NOW);

virtual	void	SystemCommon(uchar statusByte, 
							 uchar data1, 
							 uchar data2,
							 uint32 time = B_NOW);

virtual	void	SystemRealTime(uchar statusByte, uint32 time = B_NOW);

virtual	void	TempoChange(int32 bpm, uint32 time = B_NOW);

virtual void	AllNotesOff(bool justChannel = true, uint32 time = B_NOW);
};

#endif
