/*
 * Copyright 2002-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 */
#ifndef MIDI_SERVER_APP_H
#define MIDI_SERVER_APP_H


#include <Server.h>
#include <List.h>

#include "DeviceWatcher.h"


struct app_t;
struct endpoint_t;


/*!	The heart of the midi_server. This BApplication subclass
	keeps the roster of endpoints and applications, processes
	incoming messages from libmidi2.so, and notifies the apps
	when something interesting happens.
*/
class MidiServerApp : public BServer {
public:
								MidiServerApp(status_t& error);
	virtual						~MidiServerApp();

	virtual	void				AboutRequested();
	virtual	void				MessageReceived(BMessage* msg);

private:
	typedef BServer super;

			void				_OnRegisterApp(BMessage* msg);
			void				_OnCreateEndpoint(BMessage* msg);
			void				_OnDeleteEndpoint(BMessage* msg);
			void				_OnPurgeEndpoint(BMessage* msg);
			void				_OnChangeEndpoint(BMessage* msg);
			void				_OnConnectDisconnect(BMessage* msg);

			bool				_SendAllEndpoints(app_t* app);
			bool				_SendAllConnections(app_t* app);

			void				_AddEndpoint(BMessage* msg, endpoint_t* endp);
			void				_RemoveEndpoint(app_t* app, endpoint_t* endp);

			void				_DisconnectDeadConsumer(endpoint_t* cons);

			void				_MakeCreatedNotification(BMessage* msg,
									endpoint_t* endp);
			void				_MakeConnectedNotification(BMessage* msg,
									endpoint_t* prod, endpoint_t* cons,
									bool mustConnect);

			app_t*				_WhichApp(BMessage* msg);
			endpoint_t*			_WhichEndpoint(BMessage* msg, app_t* app);
			endpoint_t*			_FindEndpoint(int32 id);

			void				_NotifyAll(BMessage* msg, app_t* except);
			bool				_SendNotification(app_t* app, BMessage* msg);
			bool				_SendReply(app_t* app, BMessage* msg,
									BMessage* reply);

			void				_DeliveryError(app_t* app);

			int32				_CountApps();
			app_t*				_AppAt(int32 index);

			int32				_CountEndpoints();
			endpoint_t*			_EndpointAt(int32 index);

			int32				_CountConnections(endpoint_t* prod);
			endpoint_t*			_ConnectionAt(endpoint_t* prod, int32 index);

#ifdef DEBUG
			void				_DumpApps();
			void				_DumpEndpoints();
#endif

private:
	//! The registered applications.
			BList				fApps;

	//! All the endpoints in the system.
			BList				fEndpoints;

	//! The ID we will assign to the next new endpoint.
			int32				fNextID;

	//! Watch endpoints from /dev/midi drivers.
			DeviceWatcher*		fDeviceWatcher;
};


#endif // MIDI_SERVER_APP_H
