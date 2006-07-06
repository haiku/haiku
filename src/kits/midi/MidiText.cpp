/*
 * Copyright 2006, Haiku.
 *
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Copyright (c) 2002 Jerome Leveque
 * Copyright (c) 2002 Paul Stadler
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Leveque
 *		Matthijs Hollemans
 * 		Paul Stadler
 */

#include <stdio.h>
#include <MidiText.h>

#include "debug.h"


BMidiText::BMidiText()
{
	fStartTime = 0;
}


BMidiText::~BMidiText()
{
	// do nothing
}


void 
BMidiText::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"B_NOTE OFF; channel = %d, note = %d, velocity = %d\n",
		channel, note, velocity);
}


void 
BMidiText::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"B_NOTE ON; channel = %d, note = %d, velocity = %d\n",
		channel, note, velocity);
}


void 
BMidiText::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"KEY PRESSURE; channel = %d, note = %d, pressure = %d\n",
		channel, note, pressure);
}


void 
BMidiText::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"CONTROL CHANGE; channel = %d, control = %d, value = %d\n",
		channel, controlNumber, controlValue);
}


void 
BMidiText::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"PROGRAM CHANGE; channel = %d, program = %d\n",
		channel, programNumber);
}


void 
BMidiText::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"CHANNEL PRESSURE; channel = %d, pressure = %d\n",
		channel, pressure);
}


void 
BMidiText::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"PITCH BEND; channel = %d, lsb = %d, msb = %d\n",
		channel, lsb, msb);
}


void 
BMidiText::SystemExclusive(void* data, size_t length, uint32 time)
{
	_WaitAndPrint(time);

	printf("SYSTEM EXCLUSIVE;\n");
	for (size_t t = 0; t < length; ++t) 
		printf("%02X ", ((uint8*) data)[t]);
	printf("\n");
}


void 
BMidiText::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	_WaitAndPrint(time);
	printf(
		"SYSTEM COMMON; status = %d, data1 = %d, data2 = %d\n",
		status, data1, data2);
}


void 
BMidiText::SystemRealTime(uchar status, uint32 time)
{
	_WaitAndPrint(time);
	printf("SYSTEM REAL TIME; status = %d\n", status);
}


void 
BMidiText::ResetTimer(bool start)
{
	fStartTime = start ? B_NOW : 0;
}


void BMidiText::_ReservedMidiText1() { }
void BMidiText::_ReservedMidiText2() { }
void BMidiText::_ReservedMidiText3() { }


void 
BMidiText::Run()
{
	while (KeepRunning())
		snooze(50000);
}


void 
BMidiText::_WaitAndPrint(uint32 time) 
{
	if (fStartTime == 0) 
		fStartTime = time;

	SnoozeUntil(time);

	printf("%lu: ", time - fStartTime);	
}

