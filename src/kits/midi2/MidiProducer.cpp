/**
 * @file MidiProducer.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 */

#include "debug.h"
#include "MidiProducer.h"
#include "MidiEndpoint.h"

//------------------------------------------------------------------------------

status_t BMidiProducer::Connect(BMidiConsumer* toObject)
{
/*
	if (toObject != NULL)
	{
		if (fConnections->Add(toObject) == true)
		{
			fConnectionCount++;
		}
	}
	BMidiRoster *roster = BMidiRoster::MidiRoster();
	return roster->Connect(this, toObject);
*/
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiProducer::Disconnect(BMidiConsumer* toObject)
{
/*
	if (toObject != NULL)
	{
		if (fConnections->Remove(toObject) == true)
		{
			fConnectionCount--;
			BMidiRoster *roster = BMidiRoster::MidiRoster();
			return roster->Disconnect(this, toObject);
		}
	}
*/
	return B_ERROR;
}

//------------------------------------------------------------------------------

bool BMidiProducer::IsConnected(BMidiConsumer* toObject) const
{
//	return fConnections->IsIn(toObject);
	return false;
}

//------------------------------------------------------------------------------

BList* BMidiProducer::Connections() const
{
//	return fConnections;
	return NULL;
}

//------------------------------------------------------------------------------

BMidiProducer::BMidiProducer(const char* name)
	: BMidiEndpoint(name)
{
/*
	fConnections = new BMidiList();
	fConnectionCount = 1;
	fLock = BLocker("BMidiProducer Lock");
*/
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

