/**
 * @file MidiLocalProducer.cpp
 *
 * Implementation of the BMidiLocalProducer class.
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "MidiConsumer.h"
#include "MidiProducer.h"
#include "MidiRoster.h"
#include "protocol.h"

//------------------------------------------------------------------------------

BMidiLocalProducer::BMidiLocalProducer(const char* name)
	: BMidiProducer(name)
{
	TRACE(("BMidiLocalProducer::BMidiLocalProducer"))

	isLocal = true;
	refCount = 1;

	BMidiRoster::MidiRoster()->CreateLocal(this);
}

//------------------------------------------------------------------------------

BMidiLocalProducer::~BMidiLocalProducer()
{
	TRACE(("BMidiLocalProducer::~BMidiLocalProducer"))

	BMidiRoster::MidiRoster()->DeleteLocal(this);
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::Connected(BMidiConsumer* cons)
{
	ASSERT(cons != NULL)
	TRACE(("Connected() %ld to %ld", ID(), cons->ID()))

	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalProducer::Disconnected(BMidiConsumer* cons)
{
	ASSERT(cons != NULL)
	TRACE(("Disconnected() %ld from %ld", ID(), cons->ID()))

	// Do nothing.
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

void BMidiLocalProducer::_Reserved1() { }
void BMidiLocalProducer::_Reserved2() { }
void BMidiLocalProducer::_Reserved3() { }
void BMidiLocalProducer::_Reserved4() { }
void BMidiLocalProducer::_Reserved5() { }
void BMidiLocalProducer::_Reserved6() { }
void BMidiLocalProducer::_Reserved7() { }
void BMidiLocalProducer::_Reserved8() { }

//------------------------------------------------------------------------------
