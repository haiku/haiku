/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */

#include "WebHandler.h"
#include "WebServer.h"
#include "WebWorker.h"

#include "StreamingRingBuffer.h"

#include <NetEndpoint.h>
#include <Autolock.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			debug_printf("WebServer: "x)
#define TRACE_ERROR(x...)	debug_printf("WebServer: "x)


WebServer::WebServer(BNetEndpoint *listener)
	:
	fListener(listener),
	fReceiverThread(-1),
	fStopThread(false),
	fLocker("WebServer locker")
{
	fReceiverThread = spawn_thread(_NetworkReceiverEntry, "html5 server",
		B_NORMAL_PRIORITY, this);
	resume_thread(fReceiverThread);
}


WebServer::~WebServer()
{
	fStopThread = true;

	if (fListener != NULL)
		fListener->Close();

	//int32 result;
	//wait_for_thread(fReceiverThread, &result);
		// TODO: find out why closing the endpoint doesn't notify the waiter

	kill_thread(fReceiverThread);
}


void
WebServer::AddHandler(WebHandler *handler)
{
	BAutolock lock(fLocker);
	fHandlers.AddItem(handler);
}

int32
WebServer::_NetworkReceiverEntry(void *data)
{
	return ((WebServer *)data)->_NetworkReceiver();
}


status_t
WebServer::_NetworkReceiver()
{
	status_t result = fListener->Listen();
	if (result != B_OK) {
		TRACE_ERROR("failed to listen on port: %s\n", strerror(result));
		return result;
	}

	fHandlers.SortItems(&WebHandler::_CallbackCompare);

	while (!fStopThread) {
		BNetEndpoint *endpoint = fListener->Accept(1000);
		if (endpoint == NULL)
			continue;

		TRACE("new endpoint connection: %p\n", endpoint);
		while (!fStopThread) {
			int32 errorCount = 0;
			uint8 buffer[4096];
			int32 readSize = endpoint->Receive(buffer, sizeof(buffer) - 1);
			if (readSize < 0) {
				TRACE_ERROR("read failed, closing connection: %s\n",
					strerror(readSize));
				delete endpoint;
				break;
			}

			if (readSize == 0) {
				TRACE("read 0 bytes, retrying\n");
				snooze(100 * 1000);
				errorCount++;
				if (errorCount == 5) {
					TRACE_ERROR("failed to read, assuming disconnect\n");
					delete endpoint;
					break;
				}
				continue;
			}
			buffer[readSize] = '\0';

			const char err404[] = "HTTP/1.1 404 Not found\r\n\r\n";

			// XXX: HACK HACK HACK
			const char *p = (const char *)buffer;
			if (strncmp(p, "GET ", 4) == 0) {
				p += 4;
				const char *s = strchr(p, ' ');
				if (s && strncmp(s, " HTTP/", 6) == 0) {
					if (*p == '/')
						p++;
					BString path(p, s - p);
					if (p == s)
						path = "desktop.html";
					TRACE("searching handler for '%s'\n", path.String());
					WebHandler *handler = fHandlers.BinarySearchByKey(
						path, &WebHandler::_CallbackCompare);
					if (handler) {
						TRACE("found handler '%s'\n", handler->Name().String());
						WebWorker *worker =
							new (std::nothrow) WebWorker(endpoint, handler);
						if (worker) {
							fWorkers.AddItem(worker);
							break;
						}
					}
				}
			}

			// some error
			endpoint->Send(err404, sizeof(err404) - 1);
			delete endpoint;
			break;

#if 0
			status_t result = fTarget->Write(buffer, readSize);
			if (result != B_OK) {
				TRACE_ERROR("writing to ring buffer failed: %s\n",
					strerror(result));
				return result;
			}
#endif
		}
	}

	return B_OK;
}
