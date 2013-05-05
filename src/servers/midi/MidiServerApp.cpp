/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "debug.h"
#include "MidiServerApp.h"
#include "PortDrivers.h"
#include "ServerDefs.h"
#include "protocol.h"

#include <Alert.h>

#include <new>

using std::nothrow;

MidiServerApp::MidiServerApp()
	: BApplication(MIDI_SERVER_SIGNATURE)
{
	TRACE(("Running Haiku MIDI server"))

	nextId = 1;
	fDeviceWatcher = new(std::nothrow) DeviceWatcher();
	if (fDeviceWatcher != NULL)
		fDeviceWatcher->Run();
}


MidiServerApp::~MidiServerApp()
{
	if (fDeviceWatcher && fDeviceWatcher->Lock())
		fDeviceWatcher->Quit();

	for (int32 t = 0; t < CountApps(); ++t) {
		delete AppAt(t);
	}

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		delete EndpointAt(t);
	}
}


void
MidiServerApp::AboutRequested()
{
	BAlert* alert = new BAlert(0,
		"Haiku midi_server 1.0.0 alpha\n\n"
		"notes disguised as bytes\n"
		"propagating to endpoints,\n"
		"an aural delight",
		"OK", 0, 0, B_WIDTH_AS_USUAL, 
		B_INFO_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


void
MidiServerApp::MessageReceived(BMessage* msg)
{
	#ifdef DEBUG
	printf("IN "); msg->PrintToStream();
	#endif

	switch (msg->what) {
		case MSG_REGISTER_APP:         OnRegisterApp(msg);       break;
		case MSG_CREATE_ENDPOINT:      OnCreateEndpoint(msg);    break;
		case MSG_DELETE_ENDPOINT:      OnDeleteEndpoint(msg);    break;
		case MSG_PURGE_ENDPOINT:       OnPurgeEndpoint(msg);     break;
		case MSG_CHANGE_ENDPOINT:      OnChangeEndpoint(msg);    break;
		case MSG_CONNECT_ENDPOINTS:    OnConnectDisconnect(msg); break;
		case MSG_DISCONNECT_ENDPOINTS: OnConnectDisconnect(msg); break;

		default: super::MessageReceived(msg); break;
	}
}


void
MidiServerApp::OnRegisterApp(BMessage* msg)
{
	TRACE(("MidiServerApp::OnRegisterApp"))

	// We only send the "app registered" message upon success, 
	// so if anything goes wrong here, we do not let the app 
	// know about it, and we consider it unregistered. (Most
	// likely, the app is dead. If not, it freezes forever 
	// in anticipation of a message that will never arrive.)

	app_t* app = new app_t;

	if (msg->FindMessenger("midi:messenger", &app->messenger) == B_OK) {
		if (SendAllEndpoints(app)) {
			if (SendAllConnections(app)) {
				BMessage reply;
				reply.what = MSG_APP_REGISTERED;

				if (SendNotification(app, &reply)) {
					apps.AddItem(app);

					#ifdef DEBUG
					DumpApps();
					#endif

					return;
				}
			}
		}
	}

	delete app;
}


void
MidiServerApp::OnCreateEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::OnCreateEndpoint"))

	status_t err;
	endpoint_t* endp = new endpoint_t;

	endp->app = WhichApp(msg);
	if (endp->app == NULL) {
		err = B_ERROR;
	} else {
		err = B_BAD_VALUE;

		if (msg->FindBool("midi:consumer", &endp->consumer) == B_OK
			&& msg->FindBool("midi:registered", &endp->registered) == B_OK
			&& msg->FindString("midi:name", &endp->name) == B_OK
			&& msg->FindMessage("midi:properties", &endp->properties) == B_OK) {
			if (endp->consumer) {
				if (msg->FindInt32("midi:port", &endp->port) == B_OK
					&& msg->FindInt64("midi:latency", &endp->latency) == B_OK)
					err = B_OK;
			} else
				err = B_OK;
		}
	}

	BMessage reply;

	if (err == B_OK) {
		endp->id = nextId++;
		reply.AddInt32("midi:id", endp->id);
	}

	reply.AddInt32("midi:result", err);

	if (SendReply(endp->app, msg, &reply) && err == B_OK)
		AddEndpoint(msg, endp);
	else
		delete endp;
}


void
MidiServerApp::OnDeleteEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::OnDeleteEndpoint"))

	// Clients send the "delete endpoint" message from 
	// the BMidiEndpoint destructor, so there is no point 
	// sending a reply, because the endpoint object will 
	// be destroyed no matter what.

	app_t* app = WhichApp(msg);
	if (app != NULL) {
		endpoint_t* endp = WhichEndpoint(msg, app);
		if (endp != NULL)
			RemoveEndpoint(app, endp);
	}
}


void
MidiServerApp::OnPurgeEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::OnPurgeEndpoint"))

	// This performs the same task as OnDeleteEndpoint(),
	// except that this message was send by the midi_server
	// itself, so we don't check that the app that made the 
	// request really is the owner of the endpoint. (But we
	// _do_ check that the message came from the server.)

	if (!msg->IsSourceRemote()) {
		int32 id;
		if (msg->FindInt32("midi:id", &id) == B_OK) {
			endpoint_t* endp = FindEndpoint(id);
			if (endp != NULL)
				RemoveEndpoint(NULL, endp);
		}
	}
}


void
MidiServerApp::OnChangeEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::OnChangeEndpoint"))

	endpoint_t* endp = NULL;
	status_t err;

	app_t* app = WhichApp(msg);
	if (app == NULL) 
		err = B_ERROR; 
	else {
		endp = WhichEndpoint(msg, app);
		if (endp == NULL) 
			err = B_BAD_VALUE; 
		else
			err = B_OK;
	}

	BMessage reply;
	reply.AddInt32("midi:result", err);

	if (SendReply(app, msg, &reply) && err == B_OK) {
		TRACE(("Endpoint %ld (%p) changed", endp->id, endp))

		BMessage notify;
		notify.what = MSG_ENDPOINT_CHANGED;
		notify.AddInt32("midi:id", endp->id);

		bool registered;
		if (msg->FindBool("midi:registered", &registered) == B_OK) {
			notify.AddBool("midi:registered", registered);
			endp->registered = registered;
		}

		BString name;
		if (msg->FindString("midi:name", &name) == B_OK) {
			notify.AddString("midi:name", name);
			endp->name = name;
		}

		BMessage properties;
		if (msg->FindMessage("midi:properties", &properties) == B_OK) {
			notify.AddMessage("midi:properties", &properties);
			endp->properties = properties;
		}

		bigtime_t latency;
		if (msg->FindInt64("midi:latency", &latency) == B_OK) {
			notify.AddInt64("midi:latency", latency);
			endp->latency = latency;
		}

		NotifyAll(&notify, app);

		#ifdef DEBUG
		DumpEndpoints();
		#endif
	}
}


void
MidiServerApp::OnConnectDisconnect(BMessage* msg)
{
	TRACE(("MidiServerApp::OnConnectDisconnect"))

	bool mustConnect = (msg->what == MSG_CONNECT_ENDPOINTS);

	status_t err;
	endpoint_t* prod = NULL;
	endpoint_t* cons = NULL;

	app_t* app = WhichApp(msg);
	if (app == NULL) 
		err = B_ERROR; 
	else {
		err = B_BAD_VALUE;

		int32 prodId, consId;
		if (msg->FindInt32("midi:producer", &prodId) == B_OK
			&& msg->FindInt32("midi:consumer", &consId) == B_OK) {
			prod = FindEndpoint(prodId);
			cons = FindEndpoint(consId);

			if (prod != NULL && !prod->consumer) {
				if (cons != NULL && cons->consumer) {
					// It is an error to connect two endpoints that
					// are already connected, or to disconnect two
					// endpoints that are not connected at all.

					if (mustConnect == prod->connections.HasItem(cons))
						err = B_ERROR;
					else
						err = B_OK;
				}
			}
		}
	}

	BMessage reply;
	reply.AddInt32("midi:result", err);

	if (SendReply(app, msg, &reply) && err == B_OK) {
		if (mustConnect) {
			TRACE(("Connection made: %ld ---> %ld", prod->id, cons->id))

			prod->connections.AddItem(cons);
		} else {
			TRACE(("Connection broken: %ld -X-> %ld", prod->id, cons->id))

			prod->connections.RemoveItem(cons);
		}

		BMessage notify;
		MakeConnectedNotification(&notify, prod, cons, mustConnect);
		NotifyAll(&notify, app);

		#ifdef DEBUG
		DumpEndpoints();
		#endif
	}
}


bool
MidiServerApp::SendAllEndpoints(app_t* app)
{
	ASSERT(app != NULL)

	BMessage notify;

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		endpoint_t* endp = EndpointAt(t);

		MakeCreatedNotification(&notify, endp);

		if (!SendNotification(app, &notify))
			return false;
	}

	return true;
}


bool
MidiServerApp::SendAllConnections(app_t* app)
{
	ASSERT(app != NULL)

	BMessage notify;

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		endpoint_t* prod = EndpointAt(t);
		if (!prod->consumer) {
			for (int32 k = 0; k < CountConnections(prod); ++k) {
				endpoint_t* cons = ConnectionAt(prod, k);

				MakeConnectedNotification(&notify, prod, cons, true);

				if (!SendNotification(app, &notify))
					return false;
			}
		}
	}

	return true;
}


void
MidiServerApp::AddEndpoint(BMessage* msg, endpoint_t* endp)
{
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)
	ASSERT(!endpoints.HasItem(endp))

	TRACE(("Endpoint %ld (%p) added", endp->id, endp))

	endpoints.AddItem(endp);

	BMessage notify;
	MakeCreatedNotification(&notify, endp);
	NotifyAll(&notify, endp->app);

	#ifdef DEBUG
	DumpEndpoints();
	#endif
}


void
MidiServerApp::RemoveEndpoint(app_t* app, endpoint_t* endp)
{
	ASSERT(endp != NULL)
	ASSERT(endpoints.HasItem(endp))

	TRACE(("Endpoint %ld (%p) removed", endp->id, endp))

	endpoints.RemoveItem(endp);

	if (endp->consumer)
		DisconnectDeadConsumer(endp);

	BMessage notify;
	notify.what = MSG_ENDPOINT_DELETED;
	notify.AddInt32("midi:id", endp->id);
	NotifyAll(&notify, app);

	delete endp;

	#ifdef DEBUG
	DumpEndpoints();
	#endif
}


void
MidiServerApp::DisconnectDeadConsumer(endpoint_t* cons)
{
	ASSERT(cons != NULL)
	ASSERT(cons->consumer)

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		endpoint_t* prod = EndpointAt(t);
		if (!prod->consumer)
			prod->connections.RemoveItem(cons);
	}
}


void
MidiServerApp::MakeCreatedNotification(BMessage* msg, endpoint_t* endp)
{
	ASSERT(msg != NULL)
	ASSERT(endp != NULL)

	msg->MakeEmpty();
	msg->what = MSG_ENDPOINT_CREATED;
	msg->AddInt32("midi:id", endp->id);
	msg->AddBool("midi:consumer", endp->consumer);
	msg->AddBool("midi:registered", endp->registered);
	msg->AddString("midi:name", endp->name);
	msg->AddMessage("midi:properties", &endp->properties);

	if (endp->consumer) {
		msg->AddInt32("midi:port", endp->port);
		msg->AddInt64("midi:latency", endp->latency);
	}
}


void
MidiServerApp::MakeConnectedNotification(BMessage* msg, endpoint_t* prod,
	endpoint_t* cons, bool mustConnect)
{
	ASSERT(msg != NULL)
	ASSERT(prod != NULL)
	ASSERT(cons != NULL)
	ASSERT(!prod->consumer)
	ASSERT(cons->consumer)

	msg->MakeEmpty();

	if (mustConnect)
		msg->what = MSG_ENDPOINTS_CONNECTED;
	else
		msg->what = MSG_ENDPOINTS_DISCONNECTED;

	msg->AddInt32("midi:producer", prod->id);
	msg->AddInt32("midi:consumer", cons->id);
}


app_t*
MidiServerApp::WhichApp(BMessage* msg)
{
	ASSERT(msg != NULL)

	BMessenger retadr = msg->ReturnAddress();

	for (int32 t = 0; t < CountApps(); ++t) {
		app_t* app = AppAt(t);
		if (app->messenger.Team() == retadr.Team())
			return app;
	}

	TRACE(("Application %ld is not registered", retadr.Team()))

	return NULL;
}


endpoint_t*
MidiServerApp::WhichEndpoint(BMessage* msg, app_t* app)
{
	ASSERT(msg != NULL)
	ASSERT(app != NULL)

	int32 id;
	if (msg->FindInt32("midi:id", &id) == B_OK) {
		endpoint_t* endp = FindEndpoint(id);
		if (endp != NULL && endp->app == app)
			return endp;
	}

	TRACE(("Endpoint not found or wrong app"))
	return NULL;
}


endpoint_t*
MidiServerApp::FindEndpoint(int32 id)
{
	if (id > 0) {
		for (int32 t = 0; t < CountEndpoints(); ++t) {
			endpoint_t* endp = EndpointAt(t);
			if (endp->id == id)
				return endp;
		}
	}

	TRACE(("Endpoint %ld not found", id))
	return NULL;
}


void
MidiServerApp::NotifyAll(BMessage* msg, app_t* except)
{
	ASSERT(msg != NULL)

	for (int32 t = CountApps() - 1; t >= 0; --t) {
		app_t* app = AppAt(t);
		if (app != except) {
			if (!SendNotification(app, msg)) {
				delete (app_t*)apps.RemoveItem(t);

				#ifdef DEBUG
				DumpApps();
				#endif
			}
		}
	}
}


bool
MidiServerApp::SendNotification(app_t* app, BMessage* msg)
{
	ASSERT(app != NULL)
	ASSERT(msg != NULL)

	status_t err = app->messenger.SendMessage(msg, (BHandler*) NULL, TIMEOUT);

	if (err != B_OK)
		DeliveryError(app);

	return err == B_OK;
}


bool
MidiServerApp::SendReply(app_t* app, BMessage* msg, BMessage* reply)
{
	ASSERT(msg != NULL)
	ASSERT(reply != NULL)

	status_t err = msg->SendReply(reply, (BHandler*) NULL, TIMEOUT);

	if (err != B_OK && app != NULL) {
		DeliveryError(app);
		apps.RemoveItem(app);
		delete app;

		#ifdef DEBUG
		DumpApps();
		#endif
	}

	return err == B_OK;
}


void
MidiServerApp::DeliveryError(app_t* app)
{
	ASSERT(app != NULL)
	
	// We cannot communicate with the app, so we assume it's 
	// dead. We need to remove its endpoints from the roster, 
	// but we cannot do that right away; removing endpoints 
	// triggers a bunch of new notifications and we don't want 
	// those to get in the way of the notifications we are 
	// currently sending out. Instead, we consider the death
	// of an app as a separate event, and pretend that the 
	// now-dead app sent us delete requests for its endpoints.

	TRACE(("Delivery error; unregistering app (%p)", app))

	BMessage msg;

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		endpoint_t* endp = EndpointAt(t);
		if (endp->app == app) {
			msg.MakeEmpty();
			msg.what = MSG_PURGE_ENDPOINT;
			msg.AddInt32("midi:id", endp->id);

			// It is not safe to post a message to your own
			// looper's message queue, because you risk a 
			// deadlock if the queue is full. The chance of
			// that happening is fairly small, but just in 
			// case, we catch it with a timeout. Because this
			// situation is so unlikely, I decided to simply
			// forget about the whole "purge" message then.

			if (be_app_messenger.SendMessage(&msg, (BHandler*)NULL,
					TIMEOUT) != B_OK) {
				WARN("Could not deliver purge message")
			}
		}
	}
}


int32
MidiServerApp::CountApps()
{
	return apps.CountItems();
}


app_t*
MidiServerApp::AppAt(int32 index)
{
	ASSERT(index >= 0 && index < CountApps())

	return (app_t*)apps.ItemAt(index);
}


int32
MidiServerApp::CountEndpoints()
{
	return endpoints.CountItems();
}


endpoint_t*
MidiServerApp::EndpointAt(int32 index)
{
	ASSERT(index >= 0 && index < CountEndpoints())

	return (endpoint_t*)endpoints.ItemAt(index);
}


int32
MidiServerApp::CountConnections(endpoint_t* prod)
{
	ASSERT(prod != NULL)
	ASSERT(!prod->consumer)

	return prod->connections.CountItems();
}


endpoint_t*
MidiServerApp::ConnectionAt(endpoint_t* prod, int32 index)
{
	ASSERT(prod != NULL)
	ASSERT(!prod->consumer)
	ASSERT(index >= 0 && index < CountConnections(prod))

	return (endpoint_t*)prod->connections.ItemAt(index);
}


#ifdef DEBUG
void
MidiServerApp::DumpApps()
{
	printf("*** START DumpApps\n");

	for (int32 t = 0; t < CountApps(); ++t) {
		app_t* app = AppAt(t);

		printf("\tapp %ld (%p): team %ld\n", t, app, app->messenger.Team());
	}

	printf("*** END DumpApps\n");
}
#endif


#ifdef DEBUG
void
MidiServerApp::DumpEndpoints()
{
	printf("*** START DumpEndpoints\n");

	for (int32 t = 0; t < CountEndpoints(); ++t) {
		endpoint_t* endp = EndpointAt(t);

		printf("\tendpoint %ld (%p):\n", t, endp);
		printf("\t\tid %ld, name '%s', %s, %s, app %p\n", 
			endp->id, endp->name.String(),
			endp->consumer ? "consumer" : "producer",
			endp->registered ? "registered" : "unregistered",
			endp->app);
		printf("\t\tproperties: "); endp->properties.PrintToStream();

		if (endp->consumer)
			printf("\t\tport %ld, latency %Ld\n", endp->port, endp->latency);
		else {
			printf("\t\tconnections:\n");
			for (int32 k = 0; k < CountConnections(endp); ++k) {
				endpoint_t* cons = ConnectionAt(endp, k);
				printf("\t\t\tid %ld (%p)\n", cons->id, cons);
			}
		}
	}

	printf("*** END DumpEndpoints\n");
}
#endif


//	#pragma mark -


int
main()
{
	MidiServerApp app;
	app.Run();
	return 0;
}

