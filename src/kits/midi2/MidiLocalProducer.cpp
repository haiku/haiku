/**
 * @file MidiLocalProducer.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 */

#include "debug.h"
#include "MidiProducer.h"

//------------------------------------------------------------------------------

BMidiLocalProducer::BMidiLocalProducer(const char* name)
	: BMidiProducer(name)
{
	fFlags |= 0x10;
}

//------------------------------------------------------------------------------

BMidiLocalProducer::~BMidiLocalProducer()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::Connected(BMidiConsumer* dest)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::Disconnected(BMidiConsumer* dest)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayData(
	void* data, size_t length, bool atomic, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayNoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayNoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayKeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, 
	bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayProgramChange(
	uchar channel, uchar programNumber, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayChannelPressure(
	uchar channel, uchar pressure, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayPitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SpraySystemExclusive(
	void* data, size_t dataLength, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SpraySystemCommon(
	uchar statusByte, uchar data1, uchar data2, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SpraySystemRealTime(
	uchar statusByte, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayTempoChange(
	int32 bpm, bigtime_t time) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::SprayEvent(
	BMidiEvent* event, size_t length) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::_Reserved1() { }
void BMidiLocalProducer::_Reserved2() { }
void BMidiLocalProducer::_Reserved3() { }
void BMidiLocalProducer::_Reserved4() { }
void BMidiLocalProducer::_Reserved5() { }
void BMidiLocalProducer::_Reserved6() { }
void BMidiLocalProducer::_Reserved7() { }
void BMidiLocalProducer::_Reserved8() { }

//------------------------------------------------------------------------------

