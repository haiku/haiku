
#include "debug.h"
#include "MidiProducer.h"
#include "MidiEndpoint.h"

//------------------------------------------------------------------------------

status_t BMidiProducer::Connect(BMidiConsumer* toObject)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiProducer::Disconnect(BMidiConsumer* toObject)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

bool BMidiProducer::IsConnected(BMidiConsumer* toObject) const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

BList* BMidiProducer::Connections() const
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiProducer::BMidiProducer(const char* name)
	: BMidiEndpoint(name)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BMidiProducer::~BMidiProducer()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiProducer::_Reserved1() { }
void BMidiProducer::_Reserved2() { }
void BMidiProducer::_Reserved3() { } 
void BMidiProducer::_Reserved4() { } 
void BMidiProducer::_Reserved5() { } 
void BMidiProducer::_Reserved6() { }
void BMidiProducer::_Reserved7() { }
void BMidiProducer::_Reserved8() { }

//------------------------------------------------------------------------------

