/*
 * Copyright 2002-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */

/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * Copyright (c) 2021 Panagiotis Vasilopoulos
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


#include "MidiServerApp.h"

#include <new>

#include <AboutWindow.h>
#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include "debug.h"
#include "protocol.h"
#include "PortDrivers.h"
#include "ServerDefs.h"


using std::nothrow;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "midi_server"


MidiServerApp::MidiServerApp(status_t& error)
	:
	BServer(MIDI_SERVER_SIGNATURE, true, &error)
{
	TRACE(("Running Haiku MIDI server"))

	fNextID = 1;
	fDeviceWatcher = new(std::nothrow) DeviceWatcher();
	if (fDeviceWatcher != NULL)
		fDeviceWatcher->Run();
}


MidiServerApp::~MidiServerApp()
{
	if (fDeviceWatcher && fDeviceWatcher->Lock())
		fDeviceWatcher->Quit();

	for (int32 t = 0; t < _CountApps(); ++t) {
		delete _AppAt(t);
	}

	for (int32 t = 0; t < _CountEndpoints(); ++t) {
		delete _EndpointAt(t);
	}
}


void
MidiServerApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(B_TRANSLATE_SYSTEM_NAME(
		"Haiku MIDI Server"), MIDI_SERVER_SIGNATURE);
	window->AddDescription(B_TRANSLATE(
		"Notes disguised as bytes\n"
		"propagating to endpoints-\n"
		"An aural delight."));

	const char* extraCopyrights[] = {
		"2002-2004 Matthijs Hollemans",
		"2021 Panagiotis Vasilopoulos",
		NULL
	};

	const char* authors[] = {
		"Humdinger",
		"Matthijs Hollemans",
		"Oliver Tappe",
		"Panagiotis Vasilopoulos",
		"Philippe Houdoin",
		NULL
	};

	window->AddCopyright(2021, "Haiku, Inc.", extraCopyrights);
	window->AddAuthors(authors);

	window->Show();
}


void
MidiServerApp::MessageReceived(BMessage* msg)
{
#ifdef DEBUG
	printf("IN "); msg->PrintToStream();
#endif

	switch (msg->what) {
		case MSG_REGISTER_APP:
			_OnRegisterApp(msg);
			break;
		case MSG_CREATE_ENDPOINT:
			_OnCreateEndpoint(msg);
			break;
		case MSG_DELETE_ENDPOINT:
			_OnDeleteEndpoint(msg);
			break;
		case MSG_PURGE_ENDPOINT:
			_OnPurgeEndpoint(msg);
			break;
		case MSG_CHANGE_ENDPOINT:
			_OnChangeEndpoint(msg);
			break;
		case MSG_CONNECT_ENDPOINTS:
			_OnConnectDisconnect(msg);
			break;
		case MSG_DISCONNECT_ENDPOINTS:
			_OnConnectDisconnect(msg);
			break;

		default:
			super::MessageReceived(msg);
			break;
	}
}


void
MidiServerApp::_OnRegisterApp(BMessage* msg)
{
	TRACE(("MidiServerApp::_OnRegisterApp"))

	// We only send the "app registered" message upon success,
	// so if anything goes wrong here, we do not let the app
	// know about it, and we consider it unregistered. (Most
	// likely, the app is dead. If not, it freezes forever
	// in anticipation of a message that will never arrive.)

	app_t* app = new app_t;

	if (msg->FindMessenger("midi:messenger", &app->messenger) == B_OK
		&& _SendAllEndpoints(app)
		&& _SendAllConnections(app)) {
		BMessage reply;
		reply.what = MSG_APP_REGISTERED;

		if (_SendNotification(app, &reply)) {
			fApps.AddItem(app);
#ifdef DEBUG
			_DumpApps();
#endif
			return;
		}
	}

	delete app;
}


void
MidiServerApp::_OnCreateEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::_OnCreateEndpoint"))

	status_t status;
	endpoint_t* endpoint = new endpoint_t;

	endpoint->app = _WhichApp(msg);
	if (endpoint->app == NULL) {
		status = B_ERROR;
	} else {
		status = B_BAD_VALUE;

		if (msg->FindBool("midi:consumer", &endpoint->consumer) == B_OK
			&& msg->FindBool("midi:registered", &endpoint->registered) == B_OK
			&& msg->FindString("midi:name", &endpoint->name) == B_OK
			&& msg->FindMessage("midi:properties", &endpoint->properties)
					== B_OK) {
			if (endpoint->consumer) {
				if (msg->FindInt32("midi:port", &endpoint->port) == B_OK
					&& msg->FindInt64("midi:latency", &endpoint->latency)
							== B_OK)
					status = B_OK;
			} else
				status = B_OK;
		}
	}

	BMessage reply;

	if (status == B_OK) {
		endpoint->id = fNextID++;
		reply.AddInt32("midi:id", endpoint->id);
	}

	reply.AddInt32("midi:result", status);

	if (_SendReply(endpoint->app, msg, &reply) && status == B_OK)
		_AddEndpoint(msg, endpoint);
	else
		delete endpoint;
}


void
MidiServerApp::_OnDeleteEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::_OnDeleteEndpoint"))

	// Clients send the "delete endpoint" message from
	// the BMidiEndpoint destructor, so there is no point
	// sending a reply, because the endpoint object will
	// be destroyed no matter what.

	app_t* app = _WhichApp(msg);
	if (app != NULL) {
		endpoint_t* endpoint = _WhichEndpoint(msg, app);
		if (endpoint != NULL)
			_RemoveEndpoint(app, endpoint);
	}
}


void
MidiServerApp::_OnPurgeEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::_OnPurgeEndpoint"))

	// This performs the same task as OnDeleteEndpoint(),
	// except that this message was send by the midi_server
	// itself, so we don't check that the app that made the
	// request really is the owner of the endpoint. (But we
	// _do_ check that the message came from the server.)

	if (!msg->IsSourceRemote()) {
		int32 id;
		if (msg->FindInt32("midi:id", &id) == B_OK) {
			endpoint_t* endpoint = _FindEndpoint(id);
			if (endpoint != NULL)
				_RemoveEndpoint(NULL, endpoint);
		}
	}
}


void
MidiServerApp::_OnChangeEndpoint(BMessage* msg)
{
	TRACE(("MidiServerApp::_OnChangeEndpoint"))

	endpoint_t* endpoint = NULL;
	status_t status;

	app_t* app = _WhichApp(msg);
	if (app == NULL)
		status = B_ERROR;
	else {
		endpoint = _WhichEndpoint(msg, app);
		if (endpoint == NULL)
			status = B_BAD_VALUE;
		else
			status = B_OK;
	}

	BMessage reply;
	reply.AddInt32("midi:result", status);

	if (_SendReply(app, msg, &reply) && status == B_OK) {
		TRACE(("Endpoint %" B_PRId32 " (%p) changed", endpoint->id, endpoint))

		BMessage notify;
		notify.what = MSG_ENDPOINT_CHANGED;
		notify.AddInt32("midi:id", endpoint->id);

		bool registered;
		if (msg->FindBool("midi:registered", &registered) == B_OK) {
			notify.AddBool("midi:registered", registered);
			endpoint->registered = registered;
		}

		BString name;
		if (msg->FindString("midi:name", &name) == B_OK) {
			notify.AddString("midi:name", name);
			endpoint->name = name;
		}

		BMessage properties;
		if (msg->FindMessage("midi:properties", &properties) == B_OK) {
			notify.AddMessage("midi:properties", &properties);
			endpoint->properties = properties;
		}

		bigtime_t latency;
		if (msg->FindInt64("midi:latency", &latency) == B_OK) {
			notify.AddInt64("midi:latency", latency);
			endpoint->latency = latency;
		}

		_NotifyAll(&notify, app);

#ifdef DEBUG
		_DumpEndpoints();
#endif
	}
}


void
MidiServerApp::_OnConnectDisconnect(BMessage* msg)
{
	TRACE(("MidiServerApp::_OnConnectDisconnect"))

	bool mustConnect = msg->what == MSG_CONNECT_ENDPOINTS;

	status_t status;
	endpoint_t* producer = NULL;
	endpoint_t* consumer = NULL;

	app_t* app = _WhichApp(msg);
	if (app == NULL)
		status = B_ERROR;
	else {
		status = B_BAD_VALUE;

		int32 producerID;
		int32 consumerID;
		if (msg->FindInt32("midi:producer", &producerID) == B_OK
			&& msg->FindInt32("midi:consumer", &consumerID) == B_OK) {
			producer = _FindEndpoint(producerID);
			consumer = _FindEndpoint(consumerID);

			if (producer != NULL && !producer->consumer) {
				if (consumer != NULL && consumer->consumer) {
					// It is an error to connect two endpoints that
					// are already connected, or to disconnect two
					// endpoints that are not connected at all.

					if (mustConnect == producer->connections.HasItem(consumer))
						status = B_ERROR;
					else
						status = B_OK;
				}
			}
		}
	}

	BMessage reply;
	reply.AddInt32("midi:result", status);

	if (_SendReply(app, msg, &reply) && status == B_OK) {
		if (mustConnect) {
			TRACE(("Connection made: %" B_PRId32 " ---> %" B_PRId32,
				producer->id, consumer->id))

			producer->connections.AddItem(consumer);
		} else {
			TRACE(("Connection broken: %" B_PRId32 " -X-> %" B_PRId32,
				producer->id, consumer->id))

			producer->connections.RemoveItem(consumer);
		}

		BMessage notify;
		_MakeConnectedNotification(&notify, producer, consumer, mustConnect);
		_NotifyAll(&notify, app);

#ifdef DEBUG
		_DumpEndpoints();
#endif
	}
}


/*!	Sends an app MSG_ENDPOINT_CREATED notifications for
	all current endpoints. Used when the app registers.
*/
bool
MidiServerApp::_SendAllEndpoints(app_t* app)
{
	ASSERT(app != NULL)

	BMessage notify;

	for (int32 t = 0; t < _CountEndpoints(); ++t) {
		endpoint_t* endpoint = _EndpointAt(t);

		_MakeCreatedNotification(&notify, endpoint);

		if (!_SendNotification(app, &notify))
			return false;
	}

	return true;
}


/*!	Sends an app MSG_ENDPOINTS_CONNECTED notifications for
	all current connections. Used when the app registers.
*/
bool
MidiServerApp::_SendAllConnections(app_t* app)
{
	ASSERT(app != NULL)

	BMessage notify;

	for (int32 t = 0; t < _CountEndpoints(); ++t) {
		endpoint_t* producer = _EndpointAt(t);
		if (!producer->consumer) {
			for (int32 k = 0; k < _CountConnections(producer); ++k) {
				endpoint_t* consumer = _ConnectionAt(producer, k);

				_MakeConnectedNotification(&notify, producer, consumer, true);

				if (!_SendNotification(app, &notify))
					return false;
			}
		}
	}

	return true;
}


/*!	Adds the specified endpoint to the roster, and notifies
	all other applications about this event.
*/
void
MidiServerApp::_AddEndpoint(BMessage* msg, endpoint_t* endpoint)
{
	ASSERT(msg != NULL)
	ASSERT(endpoint != NULL)
	ASSERT(!fEndpoints.HasItem(endpoint))

	TRACE(("Endpoint %" B_PRId32 " (%p) added", endpoint->id, endpoint))

	fEndpoints.AddItem(endpoint);

	BMessage notify;
	_MakeCreatedNotification(&notify, endpoint);
	_NotifyAll(&notify, endpoint->app);

#ifdef DEBUG
	_DumpEndpoints();
#endif
}


/*!	Removes an endpoint from the roster, and notifies all
	other apps about this event. "app" is the application
	that the endpoint belongs to; if it is NULL, the app
	no longer exists and we're purging the endpoint.
*/
void
MidiServerApp::_RemoveEndpoint(app_t* app, endpoint_t* endpoint)
{
	ASSERT(endpoint != NULL)
	ASSERT(fEndpoints.HasItem(endpoint))

	TRACE(("Endpoint %" B_PRId32 " (%p) removed", endpoint->id, endpoint))

	fEndpoints.RemoveItem(endpoint);

	if (endpoint->consumer)
		_DisconnectDeadConsumer(endpoint);

	BMessage notify;
	notify.what = MSG_ENDPOINT_DELETED;
	notify.AddInt32("midi:id", endpoint->id);
	_NotifyAll(&notify, app);

	delete endpoint;

#ifdef DEBUG
	_DumpEndpoints();
#endif
}


/*!	Removes a consumer from the list of connections of
	all the producers it is connected to, just before
	we remove it from the roster.
*/
void
MidiServerApp::_DisconnectDeadConsumer(endpoint_t* consumer)
{
	ASSERT(consumer != NULL)
	ASSERT(consumer->consumer)

	for (int32 t = 0; t < _CountEndpoints(); ++t) {
		endpoint_t* producer = _EndpointAt(t);
		if (!producer->consumer)
			producer->connections.RemoveItem(consumer);
	}
}


//! Fills up a MSG_ENDPOINT_CREATED message.
void
MidiServerApp::_MakeCreatedNotification(BMessage* msg, endpoint_t* endpoint)
{
	ASSERT(msg != NULL)
	ASSERT(endpoint != NULL)

	msg->MakeEmpty();
	msg->what = MSG_ENDPOINT_CREATED;
	msg->AddInt32("midi:id", endpoint->id);
	msg->AddBool("midi:consumer", endpoint->consumer);
	msg->AddBool("midi:registered", endpoint->registered);
	msg->AddString("midi:name", endpoint->name);
	msg->AddMessage("midi:properties", &endpoint->properties);

	if (endpoint->consumer) {
		msg->AddInt32("midi:port", endpoint->port);
		msg->AddInt64("midi:latency", endpoint->latency);
	}
}


//! Fills up a MSG_ENDPOINTS_(DIS)CONNECTED message.
void
MidiServerApp::_MakeConnectedNotification(BMessage* msg, endpoint_t* producer,
	endpoint_t* consumer, bool mustConnect)
{
	ASSERT(msg != NULL)
	ASSERT(producer != NULL)
	ASSERT(consumer != NULL)
	ASSERT(!producer->consumer)
	ASSERT(consumer->consumer)

	msg->MakeEmpty();

	if (mustConnect)
		msg->what = MSG_ENDPOINTS_CONNECTED;
	else
		msg->what = MSG_ENDPOINTS_DISCONNECTED;

	msg->AddInt32("midi:producer", producer->id);
	msg->AddInt32("midi:consumer", consumer->id);
}


/*!	Figures out which application a message came from.
	Returns NULL if the application is not registered.
*/
app_t*
MidiServerApp::_WhichApp(BMessage* msg)
{
	ASSERT(msg != NULL)

	BMessenger retadr = msg->ReturnAddress();

	for (int32 t = 0; t < _CountApps(); ++t) {
		app_t* app = _AppAt(t);
		if (app->messenger.Team() == retadr.Team())
			return app;
	}

	TRACE(("Application %" B_PRId32 " is not registered", retadr.Team()))

	return NULL;
}


/*!	Looks at the "midi:id" field from a message, and returns
	the endpoint object that corresponds to that ID. It also
	checks whether the application specified by "app" really
	owns the endpoint. Returns NULL on error.
*/
endpoint_t*
MidiServerApp::_WhichEndpoint(BMessage* msg, app_t* app)
{
	ASSERT(msg != NULL)
	ASSERT(app != NULL)

	int32 id;
	if (msg->FindInt32("midi:id", &id) == B_OK) {
		endpoint_t* endpoint = _FindEndpoint(id);
		if (endpoint != NULL && endpoint->app == app)
			return endpoint;
	}

	TRACE(("Endpoint not found or wrong app"))
	return NULL;
}


/*!	Returns the endpoint with the specified ID, or
	\c NULL if no such endpoint exists on the roster.
*/
endpoint_t*
MidiServerApp::_FindEndpoint(int32 id)
{
	if (id > 0) {
		for (int32 t = 0; t < _CountEndpoints(); ++t) {
			endpoint_t* endpoint = _EndpointAt(t);
			if (endpoint->id == id)
				return endpoint;
		}
	}

	TRACE(("Endpoint %" B_PRId32 " not found", id))
	return NULL;
}


/*!	Sends notification messages to all registered apps,
	except to the application that triggered the event.
	The "except" app is allowed to be NULL.
*/
void
MidiServerApp::_NotifyAll(BMessage* msg, app_t* except)
{
	ASSERT(msg != NULL)

	for (int32 t = _CountApps() - 1; t >= 0; --t) {
		app_t* app = _AppAt(t);
		if (app != except && !_SendNotification(app, msg)) {
			delete (app_t*)fApps.RemoveItem(t);
#ifdef DEBUG
			_DumpApps();
#endif
		}
	}
}


/*!	Sends a notification message to an application, which is
	not necessarily registered yet. Applications never reply
	to such notification messages.
*/
bool
MidiServerApp::_SendNotification(app_t* app, BMessage* msg)
{
	ASSERT(app != NULL)
	ASSERT(msg != NULL)

	status_t status = app->messenger.SendMessage(msg, (BHandler*) NULL,
		TIMEOUT);
	if (status != B_OK)
		_DeliveryError(app);

	return status == B_OK;
}


/*!	Sends a reply to a request made by an application.
	If "app" is NULL, the application is not registered
	(and the reply should contain an error code).
*/
bool
MidiServerApp::_SendReply(app_t* app, BMessage* msg, BMessage* reply)
{
	ASSERT(msg != NULL)
	ASSERT(reply != NULL)

	status_t status = msg->SendReply(reply, (BHandler*) NULL, TIMEOUT);
	if (status != B_OK && app != NULL) {
		_DeliveryError(app);
		fApps.RemoveItem(app);
		delete app;

#ifdef DEBUG
		_DumpApps();
#endif
	}

	return status == B_OK;
}


/*!	Removes an app and all of its endpoints from the roster
	if a reply or notification message cannot be delivered.
	(Waiting for communications to fail is actually our only
	way to get rid of stale endpoints.)
*/
void
MidiServerApp::_DeliveryError(app_t* app)
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

	for (int32 t = 0; t < _CountEndpoints(); ++t) {
		endpoint_t* endpoint = _EndpointAt(t);
		if (endpoint->app == app) {
			msg.MakeEmpty();
			msg.what = MSG_PURGE_ENDPOINT;
			msg.AddInt32("midi:id", endpoint->id);

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
MidiServerApp::_CountApps()
{
	return fApps.CountItems();
}


app_t*
MidiServerApp::_AppAt(int32 index)
{
	ASSERT(index >= 0 && index < _CountApps())

	return (app_t*)fApps.ItemAt(index);
}


int32
MidiServerApp::_CountEndpoints()
{
	return fEndpoints.CountItems();
}


endpoint_t*
MidiServerApp::_EndpointAt(int32 index)
{
	ASSERT(index >= 0 && index < _CountEndpoints())

	return (endpoint_t*)fEndpoints.ItemAt(index);
}


int32
MidiServerApp::_CountConnections(endpoint_t* producer)
{
	ASSERT(producer != NULL)
	ASSERT(!producer->consumer)

	return producer->connections.CountItems();
}


endpoint_t*
MidiServerApp::_ConnectionAt(endpoint_t* producer, int32 index)
{
	ASSERT(producer != NULL)
	ASSERT(!producer->consumer)
	ASSERT(index >= 0 && index < _CountConnections(producer))

	return (endpoint_t*)producer->connections.ItemAt(index);
}


#ifdef DEBUG
void
MidiServerApp::_DumpApps()
{
	printf("*** START DumpApps\n");

	for (int32 t = 0; t < _CountApps(); ++t) {
		app_t* app = _AppAt(t);

		printf("\tapp %" B_PRId32 " (%p): team %" B_PRId32 "\n", t, app,
			app->messenger.Team());
	}

	printf("*** END DumpApps\n");
}


void
MidiServerApp::_DumpEndpoints()
{
	printf("*** START DumpEndpoints\n");

	for (int32 t = 0; t < _CountEndpoints(); ++t) {
		endpoint_t* endpoint = _EndpointAt(t);

		printf("\tendpoint %" B_PRId32 " (%p):\n", t, endpoint);
		printf("\t\tid %" B_PRId32 ", name '%s', %s, %s, app %p\n",
			endpoint->id, endpoint->name.String(),
			endpoint->consumer ? "consumer" : "producer",
			endpoint->registered ? "registered" : "unregistered",
			endpoint->app);
		printf("\t\tproperties: "); endpoint->properties.PrintToStream();

		if (endpoint->consumer)
			printf("\t\tport %" B_PRId32 ", latency %" B_PRIdBIGTIME "\n",
				endpoint->port, endpoint->latency);
		else {
			printf("\t\tconnections:\n");
			for (int32 k = 0; k < _CountConnections(endpoint); ++k) {
				endpoint_t* consumer = _ConnectionAt(endpoint, k);
				printf("\t\t\tid %" B_PRId32 " (%p)\n", consumer->id, consumer);
			}
		}
	}

	printf("*** END DumpEndpoints\n");
}
#endif	// DEBUG


//	#pragma mark -


int
main()
{
	status_t status;
	MidiServerApp app(status);

	if (status == B_OK)
		app.Run();

	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
