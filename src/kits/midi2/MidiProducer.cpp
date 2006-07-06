/*
 * Copyright 2006, Haiku.
 * 
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */

#include "debug.h"
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <MidiRoster.h>
#include "MidiRosterLooper.h"
#include "protocol.h"


status_t 
BMidiProducer::Connect(BMidiConsumer* cons)
{
	if (cons == NULL) {
		WARN("Connect() does not accept a NULL consumer")
		return B_BAD_VALUE;
	}
	if (!IsValid() || !cons->IsValid()) {
		return B_ERROR;
	}
	return SendConnectRequest(cons, true);
}


status_t 
BMidiProducer::Disconnect(BMidiConsumer* cons)
{
	if (cons == NULL) {
		WARN("Disconnect() does not accept a NULL consumer")
		return B_BAD_VALUE;
	} 
	if (!IsValid() || !cons->IsValid()) {
		return B_ERROR;
	}
	return SendConnectRequest(cons, false);
}


bool 
BMidiProducer::IsConnected(BMidiConsumer* cons) const
{
	bool isConnected = false;

	if (cons != NULL) {
		if (LockProducer()) {
			isConnected = fConnections->HasItem(cons);
			UnlockProducer();
		}
	}
	
	return isConnected;
}


BList* 
BMidiProducer::Connections() const
{
	BList* list = new BList();

	if (LockProducer()) {
		for (int32 t = 0; t < CountConsumers(); ++t) {
			BMidiConsumer* cons = ConsumerAt(t);
			cons->Acquire();
			list->AddItem(cons);
		}

		UnlockProducer();
	}

	return list;
}


BMidiProducer::BMidiProducer(const char* name)
	: BMidiEndpoint(name),
	fLocker("MidiProducerLock")
{
	fIsConsumer = false;
	fConnections = new BList();
}


BMidiProducer::~BMidiProducer()
{
	delete fConnections;
}


void BMidiProducer::_Reserved1() { }
void BMidiProducer::_Reserved2() { }
void BMidiProducer::_Reserved3() { } 
void BMidiProducer::_Reserved4() { } 
void BMidiProducer::_Reserved5() { } 
void BMidiProducer::_Reserved6() { }
void BMidiProducer::_Reserved7() { }
void BMidiProducer::_Reserved8() { }


status_t 
BMidiProducer::SendConnectRequest(
	BMidiConsumer* cons, bool mustConnect)
{
	ASSERT(cons != NULL)

	BMessage msg, reply;

	if (mustConnect) {
		msg.what = MSG_CONNECT_ENDPOINTS;
	} else {
		msg.what = MSG_DISCONNECT_ENDPOINTS;
	}

	msg.AddInt32("midi:producer", ID());
	msg.AddInt32("midi:consumer", cons->ID());

	status_t err = BMidiRoster::MidiRoster()->SendRequest(&msg, &reply);
	if (err != B_OK) 
		return err;

	status_t res;
	if (reply.FindInt32("midi:result", &res) == B_OK) {
		if (res == B_OK) {
			if (mustConnect) {
				ConnectionMade(cons);
			} else {
				ConnectionBroken(cons);
			}

			#ifdef DEBUG
			BMidiRoster::MidiRoster()->fLooper->DumpEndpoints();
			#endif
		}

		return res;
	}

	return B_ERROR;
}


void 
BMidiProducer::ConnectionMade(BMidiConsumer* consumer)
{
	if (consumer == NULL)
		return;

	if (LockProducer()) {
		ASSERT(!fConnections->HasItem(consumer))

		fConnections->AddItem(consumer);
		UnlockProducer();
	}

	if (IsLocal()) {
		((BMidiLocalProducer*) this)->Connected(consumer);
	}
}


bool 
BMidiProducer::ConnectionBroken(BMidiConsumer* consumer)
{
	if (consumer == NULL)
		return false;
	
	bool wasConnected = false;

	if (LockProducer()) {
		wasConnected = fConnections->RemoveItem(consumer);
		UnlockProducer();
	}

	if (wasConnected && IsLocal()) {
		((BMidiLocalProducer*) this)->Disconnected(consumer);
	}

	return wasConnected;
}


int32 
BMidiProducer::CountConsumers() const
{
	return fConnections->CountItems();
}


BMidiConsumer* 
BMidiProducer::ConsumerAt(int32 index) const
{
	ASSERT(index >= 0 && index < CountConsumers())

	return (BMidiConsumer*) fConnections->ItemAt(index);
}


bool 
BMidiProducer::LockProducer() const
{
	return fLocker.Lock();
}


void 
BMidiProducer::UnlockProducer() const
{
	fLocker.Unlock();
}

