/**
 * @file MidiConsumer.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 */

#include "debug.h"
#include "MidiConsumer.h"
#include "MidiEndpoint.h"

//------------------------------------------------------------------------------

bigtime_t BMidiConsumer::Latency() const
{
	return fLatency;
}

//------------------------------------------------------------------------------

BMidiConsumer::BMidiConsumer(const char* name)
	: BMidiEndpoint(name)
{
	fLatency = 0;
}

//------------------------------------------------------------------------------

BMidiConsumer::~BMidiConsumer()
{
	if (fEventPort != B_NO_MORE_PORTS)
		delete_port(fEventPort);
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

