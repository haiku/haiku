/**
 * @file MidiText.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 * @author Paul Stadler
 */

#include <iostream>

#include "debug.h"
#include "MidiEvent.h"
#include "MidiText.h"

//------------------------------------------------------------------------------

BMidiText::BMidiText()
{
	startTime = 0;
}

//------------------------------------------------------------------------------

BMidiText::~BMidiText()
{
	// Do nothing
}

//------------------------------------------------------------------------------

void BMidiText::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	WaitAndPrint(time);
	cout << ": NOTE OFF; channel = " << (int) channel
		 << ", note = " << (int) note 
		 << ", velocity = " << (int) velocity << endl;
}

//------------------------------------------------------------------------------

void BMidiText::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	WaitAndPrint(time);
	cout << ": NOTE ON; channel = " << (int) channel
		 << ", note = " << (int) note 
		 << ", velocity = " << (int) velocity << endl;
}

//------------------------------------------------------------------------------

void BMidiText::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	WaitAndPrint(time);
	cout << ": KEY PRESSURE; channel = " << (int) channel
		 << ", note = " << (int) note
		 << ", pressure = " << (int) pressure << endl;
}

//------------------------------------------------------------------------------

void BMidiText::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	WaitAndPrint(time);
	cout << ": CONTROL CHANGE; channel = " << (int) channel
		 << ", control = " << (int) controlNumber
		 << ", value = "<< (int) controlValue << endl;
}

//------------------------------------------------------------------------------

void BMidiText::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	WaitAndPrint(time);
	cout << ": PROGRAM CHANGE; channel = " << (int) channel
		 << ", program = " << (int) programNumber << endl;
}

//------------------------------------------------------------------------------

void BMidiText::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	WaitAndPrint(time);
	cout << ": CHANNEL PRESSURE; channel = " << (int) channel
		 << ", pressure = " << (int) pressure << endl;
}

//------------------------------------------------------------------------------

void BMidiText::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	WaitAndPrint(time);
	cout << ": PITCH BEND; channel = " << (int) channel << hex
		 << ", lsb = " << (int) lsb 
		 << ", msb = " << (int) msb << endl;
}

//------------------------------------------------------------------------------

void BMidiText::SystemExclusive(void* data, size_t dataLength, uint32 time)
{
	WaitAndPrint(time);

	cout << ": SYSTEM EXCLUSIVE;";
	for (size_t i = 0; i < dataLength; i++) 
	{
		cout << " " << hex << (int) ((uint8*) data)[i];
	}
	cout << endl;
}

//------------------------------------------------------------------------------

void BMidiText::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	WaitAndPrint(time);
	cout << ": SYSTEM COMMON; status = " << hex << (int) status
		 << ", data1 = " << (int) data1 
		 << ", data2 = " << (int) data2 << endl;                              
}

//------------------------------------------------------------------------------

void BMidiText::SystemRealTime(uchar status, uint32 time)
{
	WaitAndPrint(time);
	cout << ": SYSTEM REAL TIME; status = " << hex << (int) status << endl;
}

//------------------------------------------------------------------------------

void BMidiText::TempoChange(int32 beatsPerMinute, uint32 time) 
{ 
	WaitAndPrint(time);
	cout << ": TEMPO CHANGE; beatsperminute = " << beatsPerMinute << endl;	
} 

//------------------------------------------------------------------------------

void BMidiText::AllNotesOff(bool justChannel, uint32 time) 
{ 
	WaitAndPrint(time);
	cout << ": ALL NOTES OFF;" << endl;
}

//------------------------------------------------------------------------------

void BMidiText::ResetTimer(bool start)
{
	startTime = start ? B_NOW : 0;
}

//------------------------------------------------------------------------------

void BMidiText::_ReservedMidiText1() { }

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

	cout << dec << (time - startTime);	
}

//------------------------------------------------------------------------------

