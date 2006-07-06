/*
 * Copyright 2006, Haiku.
 *
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */

#include "debug.h"
#include <MidiConsumer.h>
#include <MidiRoster.h>
#include "MidiRosterLooper.h"
#include "protocol.h"

using namespace BPrivate;

// The midi_debug_level and midi_dispatcher_priority symbols 
// were exported by Be's libmidi2, and even though they do not 
// appear in the headers, some apps may still be using them. 
// For backwards compatibility's sake, we export those symbols 
// as well, even though we do not use them for anything.

// Not used. For backwards compatibility only.
int32 midi_debug_level = 0;

// Not used. For backwards compatibility only.
int32 midi_dispatcher_priority = B_REAL_TIME_PRIORITY;

//------------------------------------------------------------------------------

// The one and only BMidiRoster instance, which is created
// the first time the client app calls MidiRoster(). It is
// destroyed by the BMidiRosterKiller when the app quits.
static BMidiRoster* roster = NULL;

// Destroys the BMidiRoster instance when the app quits.
namespace BPrivate
{
	static struct BMidiRosterKiller
	{
		~BMidiRosterKiller()
		{
			delete roster;
		}
	} 
	midi_roster_killer;
}


BMidiEndpoint* 
BMidiRoster::NextEndpoint(int32* id)
{
	BMidiEndpoint* endp = NULL;

	if (id != NULL) {
		BMidiRosterLooper* looper = MidiRoster()->fLooper;
		if (looper->Lock()) {
			endp = looper->NextEndpoint(id);
			if (endp != NULL) {
				endp->Acquire();
			}
			looper->Unlock();
		}
	}

	return endp;
} 


BMidiProducer* 
BMidiRoster::NextProducer(int32* id)
{
	BMidiEndpoint* endp;

	while ((endp = NextEndpoint(id)) != NULL) {
		if (endp->IsProducer()) {
			return (BMidiProducer*) endp;
		}
		endp->Release();
	}

	return NULL;
}


BMidiConsumer* 
BMidiRoster::NextConsumer(int32* id)
{
	BMidiEndpoint* endp;

	while ((endp = NextEndpoint(id)) != NULL) {
		if (endp->IsConsumer()) {
			return (BMidiConsumer*) endp;
		}
		endp->Release();
	}

	return NULL;
}


BMidiEndpoint* 
BMidiRoster::FindEndpoint(int32 id, bool localOnly)
{
	BMidiEndpoint* endp = NULL;

	BMidiRosterLooper* looper = MidiRoster()->fLooper;
	if (looper->Lock()) {
		endp = looper->FindEndpoint(id);

		if ((endp != NULL) && endp->IsRemote()) {
			if (localOnly || !endp->IsRegistered()) {
				endp = NULL;
			}
		}

		if (endp != NULL) {
			endp->Acquire();
		}

		looper->Unlock();
	}

	return endp;
} 


BMidiProducer* 
BMidiRoster::FindProducer(int32 id, bool localOnly)
{
	BMidiEndpoint* endp = FindEndpoint(id, localOnly);

	if ((endp != NULL) && !endp->IsProducer()) {
		endp->Release();
		endp = NULL;
	}

	return (BMidiProducer*) endp;
}


BMidiConsumer* 
BMidiRoster::FindConsumer(int32 id, bool localOnly)
{
	BMidiEndpoint* endp = FindEndpoint(id, localOnly);

	if ((endp != NULL) && !endp->IsConsumer()) {
		endp->Release();
		endp = NULL;
	}

	return (BMidiConsumer*) endp;
}


void 
BMidiRoster::StartWatching(const BMessenger* msngr)
{
	if (msngr == NULL) {
		WARN("StartWatching does not accept a NULL messenger")
	} else {
		BMidiRosterLooper* looper = MidiRoster()->fLooper;
		if (looper->Lock()) {
			looper->StartWatching(msngr);
			looper->Unlock();
		}
	}
}


void 
BMidiRoster::StopWatching()
{
	BMidiRosterLooper* looper = MidiRoster()->fLooper;
	if (looper->Lock()) {
		looper->StopWatching();
		looper->Unlock();
	}
}


status_t 
BMidiRoster::Register(BMidiEndpoint* endp)
{
	if (endp != NULL) {
		return endp->Register();
	}

	return B_BAD_VALUE;
}


status_t 
BMidiRoster::Unregister(BMidiEndpoint* endp)
{
	if (endp != NULL) {
		return endp->Unregister();
	}

	return B_BAD_VALUE;
}


BMidiRoster* 
BMidiRoster::MidiRoster()
{
	if (roster == NULL) {
		new BMidiRoster();
	}

	return roster;
}


BMidiRoster::BMidiRoster()
{
	TRACE(("BMidiRoster::BMidiRoster"))

	// While our constructor is executing, some function may 
	// call MidiRoster() again, which causes an endless loop. 
	// To prevent this, we immediately fill in "roster"; now
	// subsequent calls to MidiRoster() won't mess up things.

	roster = this;

	fLooper = new BMidiRosterLooper();

	if (!fLooper->Init(this)) 
		return;

	BMessage msg;
	msg.what = MSG_REGISTER_APP;
	msg.AddMessenger("midi:messenger", BMessenger(fLooper));
	fServer = new BMessenger(MIDI_SERVER_SIGNATURE);

	if (fServer->SendMessage(&msg, fLooper, TIMEOUT) != B_OK) {
		WARN("Cannot send request to midi_server"); 
		return;
	}	

	// Although unlikely, we may receive the midi_server's
	// "app registered" reply before we lock the semaphore.
	// In that case, BMidiRosterLooper's MessageReceived()
	// will bump the semaphore count, and our acquire_sem() 
	// can grab the semaphore safely (without blocking).

	acquire_sem(fLooper->fInitLock);
}


BMidiRoster::~BMidiRoster()
{
	TRACE(("BMidiRoster::~BMidiRoster"))

	if (fLooper != NULL) {
		fLooper->Lock();
		fLooper->Quit();
	}
	delete fServer;
}


void BMidiRoster::_Reserved1() { }
void BMidiRoster::_Reserved2() { }
void BMidiRoster::_Reserved3() { }
void BMidiRoster::_Reserved4() { }
void BMidiRoster::_Reserved5() { }
void BMidiRoster::_Reserved6() { }
void BMidiRoster::_Reserved7() { }
void BMidiRoster::_Reserved8() { }


void 
BMidiRoster::CreateLocal(BMidiEndpoint* endp)
{
	ASSERT(endp != NULL)

	// We are being called from the BMidiLocalConsumer or 
	// BMidiLocalProducer constructor, so there is no need 
	// to lock anything, because at this point there cannot 
	// be multiple threads accessing the endpoint's data.

	BMessage msg;
	msg.what = MSG_CREATE_ENDPOINT;
	msg.AddBool("midi:consumer", endp->fIsConsumer);
	msg.AddBool("midi:registered", endp->fIsRegistered);
	msg.AddString("midi:name", endp->Name());
	msg.AddMessage("midi:properties", endp->fProperties);

	if (endp->IsConsumer()) {
		BMidiConsumer* consumer = (BMidiConsumer*) endp;
		msg.AddInt32("midi:port", consumer->fPort);
		msg.AddInt64("midi:latency", consumer->fLatency);
	}

	BMessage reply;
	if (SendRequest(&msg, &reply) == B_OK) {
		status_t res;
		if (reply.FindInt32("midi:result", &res) == B_OK) {
			if (res == B_OK) {
				int32 id;
				if (reply.FindInt32("midi:id", &id) == B_OK) {
					endp->fId = id;

					if (fLooper->Lock()) {
						fLooper->AddEndpoint(endp);
						fLooper->Unlock();
					}
				}
			}
		}
	}

	// There are many things that can go wrong when creating 
	// a new endpoint, but BMidiEndpoint has no InitCheck()
	// method to check for this. (You can, however, see if the
	// endpoint's ID is 0 after the constructor returns, or 
	// call IsValid().) In any case, you should still Release() 
	// the endpoint to delete the object. (This is different 
	// from Be's implementation, which bumps the refcount only 
	// when creation succeeded. If you call Release(), you'll
	// trip an assertion, so you can't delete these endpoints.)
}


void 
BMidiRoster::DeleteLocal(BMidiEndpoint* endp)
{
	ASSERT(endp != NULL)

	BMessage msg;
	msg.what = MSG_DELETE_ENDPOINT;
	msg.AddInt32("midi:id", endp->ID());

	// Note: this is always called from BMidiLocalConsumer's 
	// or BMidiLocalProducer's destructor, so we don't expect 
	// a reply from the server. If something went wrong, the 
	// object will be destroyed regardless.

	fServer->SendMessage(&msg, (BHandler*) NULL, TIMEOUT);

	// If the endpoint was successfully created, we must remove
	// it from the list of endpoints. If creation failed, then
	// we didn't put the endpoint on that list. If the endpoint
	// was connected to anything, we must also disconnect it.

	if (endp->ID() > 0) {
		if (fLooper->Lock()) {
			fLooper->RemoveEndpoint(endp);
			fLooper->Unlock();
		}
	}
}


status_t 
BMidiRoster::SendRequest(BMessage* msg, BMessage* reply)
{
	ASSERT(msg != NULL)
	ASSERT(reply != NULL)

	status_t err = fServer->SendMessage(msg, reply, TIMEOUT, TIMEOUT);

	if (err != B_OK) {
		WARN("Cannot send request to midi_server"); 
	}

	#ifdef DEBUG
	if (err == B_OK) {
		printf("REPLY "); reply->PrintToStream();
	}
	#endif

	return err;
}

