/**
 * @file MidiConsumer.cpp
 *
 * Implementation of the BMidiConsumer class.
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "MidiConsumer.h"
#include "protocol.h"

//------------------------------------------------------------------------------

bigtime_t BMidiConsumer::Latency() const
{
	bigtime_t res = 0LL;

	if (LockLooper()) 
	{ 
		res = latency; 
		UnlockLooper(); 
	}

	return res;
}

//------------------------------------------------------------------------------

BMidiConsumer::BMidiConsumer(const char* name)
	: BMidiEndpoint(name)
{
	isConsumer = true;
	latency = 0LL;
	port = 0;
}

//------------------------------------------------------------------------------

BMidiConsumer::~BMidiConsumer()
{
	// Do nothing.
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
