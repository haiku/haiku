/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */
#ifndef WEB_WORKER_H
#define WEB_WORKER_H

#include <OS.h>
#include <SupportDefs.h>

class BNetEndpoint;
class StreamingRingBuffer;
class WebHandler;

class WebWorker {
public:
								WebWorker(BNetEndpoint *endpoint,
									WebHandler *handler);
								~WebWorker();

		BNetEndpoint *			Endpoint() { return fEndpoint; }

private:
static	int32					_WorkEntry(void *data);
		status_t				_Work();

		WebHandler *			fHandler;

		thread_id				fWorkThread;
		bool					fStopThread;

		BNetEndpoint *			fEndpoint;
};

#endif // WEB_WORKER_H
