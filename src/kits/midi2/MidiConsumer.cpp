/**
 * @file MidiConsumer.cpp
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "MidiConsumer.h"
#include "MidiEndpoint.h"

//------------------------------------------------------------------------------

bigtime_t BMidiConsumer::Latency() const
{
	UNIMPLEMENTED
	return 0LL;
}

//------------------------------------------------------------------------------

BMidiConsumer::BMidiConsumer(const char* name)
	: BMidiEndpoint(name)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BMidiConsumer::~BMidiConsumer()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiConsumer::_Reserved1() { }
void BMidiConsumer::_Reserved2() { }
void BMidiConsumer::_Reserved3() { }
void BMidiConsumer::_Reserved4() { }
void BMidiConsumer::_Reserved5() { }
void BMidiConsumer::_Reserved6() { }
void BMidiConsumer::_Reserved7() { }
void BMidiConsumer::_Reserved8() { }

//------------------------------------------------------------------------------

