/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

//#include "WebWorker.h"

#include <Locker.h>
#include <ObjectList.h>
#include <OS.h>
#include <SupportDefs.h>

class BNetEndpoint;
class WebHandler;
class WebWorker;

class WebServer {
public:
								WebServer(BNetEndpoint *listener);
								~WebServer();

//		BNetEndpoint *			Endpoint() { return fEndpoint; }

		void					AddHandler(WebHandler *handler);

private:
static	int32					_NetworkReceiverEntry(void *data);
		status_t				_NetworkReceiver();

		BNetEndpoint *			fListener;

		thread_id				fReceiverThread;
		bool					fStopThread;

		BLocker					fLocker;
		BObjectList<WebHandler>	fHandlers;
		BObjectList<WebWorker>	fWorkers;
//		BNetEndpoint *			fEndpoint;
};

#endif // WEB_SERVER_H
