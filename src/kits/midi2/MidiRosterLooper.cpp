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
#include <MidiProducer.h>
#include <MidiRoster.h>
#include "MidiRosterLooper.h"
#include "protocol.h"

using namespace BPrivate;


BMidiRosterLooper::BMidiRosterLooper()
	: BLooper("MidiRosterLooper")
{
	fInitLock = -1;
	fRoster = NULL;
	fWatcher = NULL;
}


BMidiRosterLooper::~BMidiRosterLooper()
{
	StopWatching();

	if (fInitLock >= B_OK) {
		delete_sem(fInitLock); 
	}

	// At this point, our list may still contain endpoints with a 
	// zero reference count. These objects are proxies for remote 
	// endpoints, so we can safely delete them. If the list also 
	// contains endpoints with a non-zero refcount (which can be 
	// either remote or local), we will output a warning message.
	// It would have been better to jump into the debugger, but I
	// did not want to risk breaking any (misbehaving) old apps.

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		BMidiEndpoint* endp = EndpointAt(t);
		if (endp->fRefCount > 0) {
			fprintf(
				stderr, "[midi] WARNING: Endpoint %ld (%p) has "
				"not been Release()d properly (refcount = %ld)\n", 
				endp->ID(), endp, endp->fRefCount);
		} else {
			delete endp;
		}
	}
}


bool 
BMidiRosterLooper::Init(BMidiRoster* roster_)
{
	ASSERT(roster_ != NULL)

	fRoster = roster_;

	// We create a semaphore with a zero count. BMidiRoster's
	// MidiRoster() method will try to acquire this semaphore,
	// but blocks because the count is 0. When we receive the
	// "app registered" message in our MessageReceived() hook,
	// we release the semaphore and MidiRoster() will unblock.

	fInitLock = create_sem(0, "InitLock");

	if (fInitLock < B_OK) {
		WARN("Could not create semaphore")
		return false;
	}

	thread_id threadId = Run();

	if (threadId < B_OK) {
		WARN("Could not start looper thread") 
		return false;
	}

	return true;
}


BMidiEndpoint* 
BMidiRosterLooper::NextEndpoint(int32* id)
{
	ASSERT(id != NULL)

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		BMidiEndpoint* endp = EndpointAt(t);
		if (endp->ID() > *id) {
			if (endp->IsRemote() && endp->IsRegistered()) {
				*id = endp->ID();
				return endp;
			}
		}
	}

	return NULL;
}


BMidiEndpoint* 
BMidiRosterLooper::FindEndpoint(int32 id)
{
	for (int32 t = 0; t < CountEndpoints(); ++t) {
		BMidiEndpoint* endp = EndpointAt(t);
		if (endp->ID() == id) {
			return endp;
		}
	} 

	return NULL;
}


void 
BMidiRosterLooper::AddEndpoint(BMidiEndpoint* endp)
{
	ASSERT(endp != NULL)
	ASSERT(!fEndpoints.HasItem(endp))

	// We store the endpoints sorted by ID, because that
	// simplifies the implementation of NextEndpoint().
	// Although the midi_server assigns IDs in ascending
	// order, we can't assume that the mNEW messages also
	// are delivered in this order (mostly they will be).

	int32 t;
	for (t = CountEndpoints(); t > 0; --t) {
		BMidiEndpoint* other = EndpointAt(t - 1);
		if (endp->ID() > other->ID()) {
			break;
		}	
	}
	fEndpoints.AddItem(endp, t);

	#ifdef DEBUG
	DumpEndpoints();
	#endif
}


void 
BMidiRosterLooper::RemoveEndpoint(BMidiEndpoint* endp)
{
	ASSERT(endp != NULL)
	ASSERT(fEndpoints.HasItem(endp))

	fEndpoints.RemoveItem(endp);

	if (endp->IsConsumer()) {
		DisconnectDeadConsumer((BMidiConsumer*) endp);
	} else {
		DisconnectDeadProducer((BMidiProducer*) endp);
	}
	
	#ifdef DEBUG
	DumpEndpoints();
	#endif
}


void 
BMidiRosterLooper::StartWatching(const BMessenger* watcher_)
{
	ASSERT(watcher_ != NULL)

	StopWatching();
	fWatcher = new BMessenger(*watcher_);

	AllEndpoints();
	AllConnections();
}


void 
BMidiRosterLooper::StopWatching()
{
	delete fWatcher;
	fWatcher = NULL;
}


void 
BMidiRosterLooper::MessageReceived(BMessage* msg)
{
	#ifdef DEBUG
	printf("IN "); msg->PrintToStream();
	#endif

	switch (msg->what) {
		case MSG_APP_REGISTERED:         OnAppRegistered(msg);         break;
		case MSG_ENDPOINT_CREATED:       OnEndpointCreated(msg);       break;
		case MSG_ENDPOINT_DELETED:       OnEndpointDeleted(msg);       break;
		case MSG_ENDPOINT_CHANGED:       OnEndpointChanged(msg);       break;
		case MSG_ENDPOINTS_CONNECTED:    OnConnectedDisconnected(msg); break;
		case MSG_ENDPOINTS_DISCONNECTED: OnConnectedDisconnected(msg); break;

		default: super::MessageReceived(msg); break;
	}	
}


void 
BMidiRosterLooper::OnAppRegistered(BMessage* msg)
{
	release_sem(fInitLock);
}


void 
BMidiRosterLooper::OnEndpointCreated(BMessage* msg)
{
	int32 id;
	bool isRegistered;
	BString name;
	BMessage properties;
	bool isConsumer;

	if ((msg->FindInt32("midi:id", &id) == B_OK)
		&&  (msg->FindBool("midi:registered", &isRegistered) == B_OK)
		&&  (msg->FindString("midi:name", &name) == B_OK)
		&&  (msg->FindMessage("midi:properties", &properties) == B_OK)
		&&  (msg->FindBool("midi:consumer", &isConsumer) == B_OK)) {
		if (isConsumer) {
			int32 port;
			bigtime_t latency;

			if ((msg->FindInt32("midi:port", &port) == B_OK)
				&&  (msg->FindInt64("midi:latency", &latency) == B_OK)) {
				BMidiConsumer* cons = new BMidiConsumer();
				cons->fName          = name;
				cons->fId            = id;
				cons->fIsRegistered  = isRegistered;
				cons->fPort          = port;
				cons->fLatency       = latency;
				*(cons->fProperties) = properties;
				AddEndpoint(cons);
				return;
			}
		} else { // producer
			BMidiProducer* prod = new BMidiProducer();
			prod->fName          = name;
			prod->fId            = id;
			prod->fIsRegistered  = isRegistered;
			*(prod->fProperties) = properties;
			AddEndpoint(prod);
			return;
		}
	}

	WARN("Could not create proxy for remote endpoint")
}


void 
BMidiRosterLooper::OnEndpointDeleted(BMessage* msg)
{
	int32 id;
	if (msg->FindInt32("midi:id", &id) == B_OK) {
		BMidiEndpoint* endp = FindEndpoint(id);
		if (endp != NULL) {
			RemoveEndpoint(endp);

			// If the client is watching, and the endpoint is
			// registered remote, we need to let it know that
			// the endpoint is now unregistered.

			if (endp->IsRemote() && endp->IsRegistered()) {
				if (fWatcher != NULL) {
					BMessage notify;
					notify.AddInt32("be:op", B_MIDI_UNREGISTERED);
					ChangeEvent(&notify, endp);
				}
			}

			// If the proxy object for this endpoint is no 
			// longer being used, we can delete it. However, 
			// if the refcount is not zero, we must defer 
			// destruction until the client Release()'s the 
			// object. We clear the "isRegistered" flag to
			// let the client know the object is now invalid.

			if (endp->fRefCount == 0) {
				delete endp;
			} else { // still being used
				endp->fIsRegistered = false;
				endp->fIsAlive = false;
			}

			return;
		}
	}

	WARN("Could not delete proxy for remote endpoint")
}


void 
BMidiRosterLooper::OnEndpointChanged(BMessage* msg)
{
	int32 id;
	if (msg->FindInt32("midi:id", &id) == B_OK) {
		BMidiEndpoint* endp = FindEndpoint(id);
		if ((endp != NULL) && endp->IsRemote()) {
			ChangeRegistered(msg, endp);
			ChangeName(msg, endp);
			ChangeProperties(msg, endp);
			ChangeLatency(msg, endp);

			#ifdef DEBUG
			DumpEndpoints();
			#endif

			return;
		}
	}

	WARN("Could not change endpoint attributes")
}


void 
BMidiRosterLooper::OnConnectedDisconnected(BMessage* msg)
{
	int32 prodId, consId;
	if ((msg->FindInt32("midi:producer", &prodId) == B_OK)
		&&  (msg->FindInt32("midi:consumer", &consId) == B_OK)) {
		BMidiEndpoint* endp1 = FindEndpoint(prodId);
		BMidiEndpoint* endp2 = FindEndpoint(consId);

		if ((endp1 != NULL) && endp1->IsProducer()) {
			if ((endp2 != NULL) && endp2->IsConsumer()) {
				BMidiProducer* prod = (BMidiProducer*) endp1;
				BMidiConsumer* cons = (BMidiConsumer*) endp2;

				bool mustConnect = (msg->what == MSG_ENDPOINTS_CONNECTED);

				if (mustConnect) {
					prod->ConnectionMade(cons);
				} else {
					prod->ConnectionBroken(cons);
				}

				if (fWatcher != NULL) {
					ConnectionEvent(prod, cons, mustConnect);
				}

				#ifdef DEBUG
				DumpEndpoints();
				#endif

				return;
			}
		}
	}

	WARN("Could not connect/disconnect endpoints")
}


void 
BMidiRosterLooper::ChangeRegistered(BMessage* msg, BMidiEndpoint* endp)
{
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)

	bool isRegistered;
	if (msg->FindBool("midi:registered", &isRegistered) == B_OK) {
		if (endp->fIsRegistered != isRegistered) {
			endp->fIsRegistered = isRegistered;

			if (fWatcher != NULL) {
				BMessage notify;
				if (isRegistered) {
					notify.AddInt32("be:op", B_MIDI_REGISTERED);
				} else {
					notify.AddInt32("be:op", B_MIDI_UNREGISTERED);
				}
				ChangeEvent(&notify, endp);
			}
		}
	}
}


void 
BMidiRosterLooper::ChangeName(BMessage* msg, BMidiEndpoint* endp)
{
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)

	BString name;
	if (msg->FindString("midi:name", &name) == B_OK) {
		if (endp->fName != name) {
			endp->fName = name;

			if ((fWatcher != NULL) && endp->IsRegistered()) {
				BMessage notify;
				notify.AddInt32("be:op", B_MIDI_CHANGED_NAME);
				notify.AddString("be:name", name);
				ChangeEvent(&notify, endp);
			}
		}
	}
}


void 
BMidiRosterLooper::ChangeProperties(BMessage* msg, BMidiEndpoint* endp)
{
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)

	BMessage properties;
	if (msg->FindMessage("midi:properties", &properties) == B_OK) {
		*(endp->fProperties) = properties;

		if ((fWatcher != NULL) && endp->IsRegistered()) {
			BMessage notify;
			notify.AddInt32("be:op", B_MIDI_CHANGED_PROPERTIES);
			notify.AddMessage("be:properties", &properties);
			ChangeEvent(&notify, endp);
		}
	}
}


void 
BMidiRosterLooper::ChangeLatency(BMessage* msg, BMidiEndpoint* endp)
{
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)

	bigtime_t latency;
	if (msg->FindInt64("midi:latency", &latency) == B_OK) {
		if (endp->IsConsumer()) {
			BMidiConsumer* cons = (BMidiConsumer*) endp;
			if (cons->fLatency != latency) {
				cons->fLatency = latency;

				if ((fWatcher != NULL) && cons->IsRegistered()) {
					BMessage notify;
					notify.AddInt32("be:op", B_MIDI_CHANGED_LATENCY);
					notify.AddInt64("be:latency", latency);
					ChangeEvent(&notify, endp);
				}
			}
		}
	}
}


void 
BMidiRosterLooper::AllEndpoints()
{
	BMessage notify;
	for (int32 t = 0; t < CountEndpoints(); ++t) {
		BMidiEndpoint* endp = EndpointAt(t);
		if (endp->IsRemote() && endp->IsRegistered()) {
			notify.MakeEmpty();
			notify.AddInt32("be:op", B_MIDI_REGISTERED);
			ChangeEvent(&notify, endp);
		}
	}
}


void 
BMidiRosterLooper::AllConnections()
{
	for (int32 t = 0; t < CountEndpoints(); ++t) {
		BMidiEndpoint* endp = EndpointAt(t);
		if (endp->IsRemote() && endp->IsRegistered()) {
			if (endp->IsProducer()) {
				BMidiProducer* prod = (BMidiProducer*) endp;
				if (prod->LockProducer()) {
					for (int32 k = 0; k < prod->CountConsumers(); ++k) {
						ConnectionEvent(prod, prod->ConsumerAt(k), true);
					}
					prod->UnlockProducer();
				}
			}
		}
	}
}


void 
BMidiRosterLooper::ChangeEvent(BMessage* msg, BMidiEndpoint* endp)
{
	ASSERT(fWatcher != NULL)
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)

	msg->what = B_MIDI_EVENT;
	msg->AddInt32("be:id", endp->ID());

	if (endp->IsConsumer()) {
		msg->AddString("be:type", "consumer");
	} else {
		msg->AddString("be:type", "producer");
	}

	fWatcher->SendMessage(msg);
}


void 
BMidiRosterLooper::ConnectionEvent(
	BMidiProducer* prod, BMidiConsumer* cons, bool mustConnect)
{
	ASSERT(fWatcher != NULL)
	ASSERT(prod != NULL)
	ASSERT(cons != NULL)

	BMessage notify;
	notify.what = B_MIDI_EVENT;
	notify.AddInt32("be:producer", prod->ID());
	notify.AddInt32("be:consumer", cons->ID());

	if (mustConnect) {
		notify.AddInt32("be:op", B_MIDI_CONNECTED);
	} else {
		notify.AddInt32("be:op", B_MIDI_DISCONNECTED);
	}

	fWatcher->SendMessage(&notify);
}


void 
BMidiRosterLooper::DisconnectDeadConsumer(BMidiConsumer* cons)
{
	ASSERT(cons != NULL)

	// Note: Rather than looping through each producer's list
	// of connected consumers, we let ConnectionBroken() tell
	// us whether the consumer really was connected.

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		BMidiEndpoint* endp = EndpointAt(t);
		if (endp->IsProducer()) {
			BMidiProducer* prod = (BMidiProducer*) endp;
			if (prod->ConnectionBroken(cons)) {
				if (cons->IsRemote() && (fWatcher != NULL)) {
					ConnectionEvent(prod, cons, false);
				}
			}
		}
	}
}


void 
BMidiRosterLooper::DisconnectDeadProducer(BMidiProducer* prod)
{
	ASSERT(prod != NULL)

	// We don't need to lock or remove the consumers from
	// the producer's list of connections, because when this
	// function is called, we're destroying the object.

	if (prod->IsRemote() && (fWatcher != NULL)) {
		for (int32 t = 0; t < prod->CountConsumers(); ++t) {
			ConnectionEvent(prod, prod->ConsumerAt(t), false);
		}
	}
}


int32 
BMidiRosterLooper::CountEndpoints()
{
	return fEndpoints.CountItems();
}


BMidiEndpoint* 
BMidiRosterLooper::EndpointAt(int32 index)
{
	ASSERT(index >= 0 && index < CountEndpoints())

	return (BMidiEndpoint*) fEndpoints.ItemAt(index);
}


#ifdef DEBUG
void 
BMidiRosterLooper::DumpEndpoints()
{
	if (Lock()) {
		printf("*** START DumpEndpoints\n");

		for (int32 t = 0; t < CountEndpoints(); ++t) {
			BMidiEndpoint* endp = EndpointAt(t);

			printf("\tendpoint %ld (%p):\n", t, endp);

			printf(
				"\t\tid %ld, name '%s', %s, %s, %s, %s, refcount %ld\n", 
				endp->ID(), endp->Name(),
				endp->IsConsumer() ? "consumer" : "producer", 
				endp->IsRegistered() ? "registered" : "unregistered", 
				endp->IsLocal() ? "local" : "remote", 
				endp->IsValid() ? "valid" : "invalid", endp->fRefCount);

			printf("\t\tproperties: "); 
			endp->fProperties->PrintToStream();

			if (endp->IsConsumer()) {
				BMidiConsumer* cons = (BMidiConsumer*) endp;
				printf("\t\tport %ld, latency %Ld\n", 
						cons->fPort, cons->fLatency);
			} else {
				BMidiProducer* prod = (BMidiProducer*) endp;
				if (prod->LockProducer()) {
					printf("\t\tconnections:\n");
					for (int32 k = 0; k < prod->CountConsumers(); ++k) {
						BMidiConsumer* cons = prod->ConsumerAt(k);
						printf("\t\t\tid %ld (%p)\n", cons->ID(), cons);
					}
					prod->UnlockProducer();
				}
			}
		}

		printf("*** END DumpEndpoints\n");
		Unlock();
	}
}
#endif

