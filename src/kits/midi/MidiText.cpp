/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Copyright (c) 2002 Jerome Leveque
 * Copyright (c) 2002 Paul Stadler
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

#include "debug.h"
#include "MidiText.h"

//------------------------------------------------------------------------------

BMidiText::BMidiText()
{
	startTime = 0;
}

//------------------------------------------------------------------------------

BMidiText::~BMidiText()
{
	// do nothing
}

//------------------------------------------------------------------------------

void BMidiText::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"B_NOTE OFF; channel = %d, note = %d, velocity = %d\n",
		channel, note, velocity);
}

//------------------------------------------------------------------------------

void BMidiText::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"B_NOTE ON; channel = %d, note = %d, velocity = %d\n",
		channel, note, velocity);
}

//------------------------------------------------------------------------------

void BMidiText::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"KEY PRESSURE; channel = %d, note = %d, pressure = %d\n",
		channel, note, pressure);
}

//------------------------------------------------------------------------------

void BMidiText::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"CONTROL CHANGE; channel = %d, control = %d, value = %d\n",
		channel, controlNumber, controlValue);
}

//------------------------------------------------------------------------------

void BMidiText::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"PROGRAM CHANGE; channel = %d, program = %d\n",
		channel, programNumber);
}

//------------------------------------------------------------------------------

void BMidiText::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"CHANNEL PRESSURE; channel = %d, pressure = %d\n",
		channel, pressure);
}

//------------------------------------------------------------------------------

void BMidiText::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"PITCH BEND; channel = %d, lsb = %d, msb = %d\n",
		channel, lsb, msb);
}

//------------------------------------------------------------------------------

void BMidiText::SystemExclusive(void* data, size_t length, uint32 time)
{
	WaitAndPrint(time);

	printf("SYSTEM EXCLUSIVE;\n");
	for (size_t t = 0; t < length; ++t) 
	{
		printf("%02X ", ((uint8*) data)[t]);
	}
	printf("\n");
}

//------------------------------------------------------------------------------

void BMidiText::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	WaitAndPrint(time);
	printf(
		"SYSTEM COMMON; status = %d, data1 = %d, data2 = %d\n",
		status, data1, data2);
}

//------------------------------------------------------------------------------

void BMidiText::SystemRealTime(uchar status, uint32 time)
{
	WaitAndPrint(time);
	printf("SYSTEM REAL TIME; status = %d\n", status);
}

//------------------------------------------------------------------------------

void BMidiText::ResetTimer(bool start)
{
	startTime = start ? B_NOW : 0;
}

//------------------------------------------------------------------------------

void BMidiText::_ReservedMidiText1() { }
void BMidiText::_ReservedMidiText2() { }
void BMidiText::_ReservedMidiText3() { }

//------------------------------------------------------------------------------

void BMidiText::Run()
{
	while (KeepRunning())
	{
		snooze(50000);
	}
}

//------------------------------------------------------------------------------

void BMidiText::WaitAndPrint(uint32 time) 
{
	if (startTime == 0) { startTime = time;	}

	SnoozeUntil(time);

	printf("%lu: ", time - startTime);	
}

//------------------------------------------------------------------------------
