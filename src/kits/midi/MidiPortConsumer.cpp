/**
 * @file MidiPort.cpp
 * 
 * @author Jerome Leveque
 */

//----------------------------------------

#include "MidiPortConsumer.h"

//----------------------------------------

BMidiPortConsumer::BMidiPortConsumer(BMidiPort *toPort, const char *name = NULL)
		:  BMidiLocalConsumer(name), port(toPort)
{
}
	
//-----------------

void BMidiPortConsumer::NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	port->SprayNoteOff(channel, note, velocity, B_NOW);
}

//-----------------

void BMidiPortConsumer::NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	port->SprayNoteOn(channel, note, velocity, B_NOW);
}

//-----------------

void BMidiPortConsumer::KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	port->SprayKeyPressure(channel, note, pressure, B_NOW);
}

//-----------------

void BMidiPortConsumer::ControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	port->SprayControlChange(channel, controlNumber, controlValue, B_NOW);
}

//-----------------

void BMidiPortConsumer::ProgramChange(uchar channel, uchar programNumber, bigtime_t time)
{
	port->SprayProgramChange(channel, programNumber, B_NOW);
}

//-----------------

void BMidiPortConsumer::ChannelPressure(uchar channel, uchar pressure, bigtime_t time)
{
	port->SprayChannelPressure(channel, pressure, B_NOW);
}

//-----------------

void BMidiPortConsumer::PitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	port->SprayPitchBend(channel, lsb, msb, B_NOW);
}

//-----------------

void BMidiPortConsumer::SystemExclusive(void *data, size_t dataLength, bigtime_t time)
{
	port->SpraySystemExclusive(data, dataLength, B_NOW);
}

//-----------------

void BMidiPortConsumer::SystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time)
{
	port->SpraySystemCommon(statusByte, data1, data2, B_NOW);
}

//-----------------

void BMidiPortConsumer::SystemRealTime(uchar statusByte, bigtime_t time)
{
	port->SpraySystemRealTime(statusByte, B_NOW);
}

//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
