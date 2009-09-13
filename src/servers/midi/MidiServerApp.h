/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */
#ifndef MIDI_SERVER_APP_H
#define MIDI_SERVER_APP_H

#include <Application.h>
#include <List.h>

#include "DeviceWatcher.h"

struct app_t;
struct endpoint_t;

// The heart of the midi_server. This BApplication subclass
// keeps the roster of endpoints and applications, processes
// incoming messages from libmidi2.so, and notifies the apps
// when something interesting happens.
class MidiServerApp : public BApplication
{
public:

	MidiServerApp();
	virtual ~MidiServerApp();

	virtual void AboutRequested();
	virtual void MessageReceived(BMessage* msg);

private:

	typedef BApplication super;

	void OnRegisterApp(BMessage* msg);
	void OnCreateEndpoint(BMessage* msg);
	void OnDeleteEndpoint(BMessage* msg);
	void OnPurgeEndpoint(BMessage* msg);
	void OnChangeEndpoint(BMessage* msg);
	void OnConnectDisconnect(BMessage* msg);

	// Sends an app MSG_ENDPOINT_CREATED notifications for
	// all current endpoints. Used when the app registers.
	bool SendAllEndpoints(app_t* app);

	// Sends an app MSG_ENDPOINTS_CONNECTED notifications for 
	// all current connections. Used when the app registers.
	bool SendAllConnections(app_t* app);

	// Adds the specified endpoint to the roster, and notifies 
	// all other applications about this event.
	void AddEndpoint(BMessage* msg, endpoint_t* endp);

	// Removes an endpoint from the roster, and notifies all 
	// other apps about this event. "app" is the application 
	// that the endpoint belongs to; if it is NULL, the app 
	// no longer exists and we're purging the endpoint.
	void RemoveEndpoint(app_t* app, endpoint_t* endp);

	// Removes a consumer from the list of connections of
	// all the producers it is connected to, just before 
	// we remove it from the roster.
	void DisconnectDeadConsumer(endpoint_t* cons);

	// Fills up a MSG_ENDPOINT_CREATED message.
	void MakeCreatedNotification(BMessage* msg, endpoint_t* endp);

	// Fills up a MSG_ENDPOINTS_(DIS)CONNECTED message.
	void MakeConnectedNotification(
		BMessage* msg, endpoint_t* prod, endpoint_t* cons, bool mustConnect);

	// Figures out which application a message came from.
	// Returns NULL if the application is not registered.
	app_t* WhichApp(BMessage* msg);

	// Looks at the "midi:id" field from a message, and returns
	// the endpoint object that corresponds to that ID. It also 
	// checks whether the application specified by "app" really 
	// owns the endpoint. Returns NULL on error.
	endpoint_t* WhichEndpoint(BMessage* msg, app_t* app);

	// Returns the endpoint with the specified ID, or
	// NULL if no such endpoint exists on the roster.
	endpoint_t* FindEndpoint(int32 id);

	// Sends notification messages to all registered apps,
	// except to the application that triggered the event.
	// The "except" app is allowed to be NULL.
	void NotifyAll(BMessage* msg, app_t* except);

	// Sends a notification message to an application, which is
	// not necessarily registered yet. Applications never reply 
	// to such notification messages.
	bool SendNotification(app_t* app, BMessage* msg);

	// Sends a reply to a request made by an application. 
	// If "app" is NULL, the application is not registered
	// (and the reply should contain an error code).
	bool SendReply(app_t* app, BMessage* msg, BMessage* reply);

	// Removes an app and all of its endpoints from the roster
	// if a reply or notification message cannot be delivered.
	// (Waiting for communications to fail is actually our only 
	// way to get rid of stale endpoints.)
	void DeliveryError(app_t* app);

	int32 CountApps();
	app_t* AppAt(int32 index);

	int32 CountEndpoints();
	endpoint_t* EndpointAt(int32 index);

	int32 CountConnections(endpoint_t* prod);
	endpoint_t* ConnectionAt(endpoint_t* prod, int32 index);

	// The registered applications.
	BList apps;

	// All the endpoints in the system.
	BList endpoints;
	
	// The ID we will assign to the next new endpoint.
	int32 nextId;

	// Watch endpoints from /dev/midi drivers.
	DeviceWatcher* fDeviceWatcher;

	#ifdef DEBUG
	void DumpApps();
	void DumpEndpoints();
	#endif
};

#endif // MIDI_SERVER_APP_H
