/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "NetReceiver.h"

#include "StreamingRingBuffer.h"

#include <NetEndpoint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			/*debug_printf("NetReceiver: "x)*/
#define TRACE_ERROR(x...)	debug_printf("NetReceiver: "x)


NetReceiver::NetReceiver(BNetEndpoint *listener, StreamingRingBuffer *target)
	:
	fListener(listener),
	fTarget(target),
	fReceiverThread(-1),
	fStopThread(false),
	fEndpoint(NULL)
{
	fReceiverThread = spawn_thread(_NetworkReceiverEntry, "network receiver",
		B_NORMAL_PRIORITY, this);
	resume_thread(fReceiverThread);
}


NetReceiver::~NetReceiver()
{
	fStopThread = true;

	if (fEndpoint != NULL)
		fEndpoint->Close();

	//int32 result;
	//wait_for_thread(fReceiverThread, &result);
		// TODO: find out why closing the endpoint doesn't notify the waiter

	kill_thread(fReceiverThread);
}


int32
NetReceiver::_NetworkReceiverEntry(void *data)
{
	return ((NetReceiver *)data)->_NetworkReceiver();
}


status_t
NetReceiver::_NetworkReceiver()
{
	status_t result = fListener->Listen();
	if (result != B_OK) {
		TRACE_ERROR("failed to listen on port: %s\n", strerror(result));
		return result;
	}

	while (!fStopThread) {
		fEndpoint = fListener->Accept(1000);
		if (fEndpoint == NULL)
			continue;

		int32 errorCount = 0;
		TRACE("new endpoint connection: %p\n", fEndpoint);
		while (!fStopThread) {
			uint8 buffer[4096];
			int32 readSize = fEndpoint->Receive(buffer, sizeof(buffer));
			if (readSize < 0) {
				TRACE_ERROR("read failed, closing connection: %s\n",
					strerror(readSize));
				BNetEndpoint *endpoint = fEndpoint;
				fEndpoint = NULL;
				delete endpoint;
				return readSize;
			}

			if (readSize == 0) {
				TRACE("read 0 bytes, retrying\n");
				snooze(100 * 1000);
				errorCount++;
				if (errorCount == 5) {
					TRACE_ERROR("failed to read, assuming disconnect\n");
					break;
				}

				continue;
			}

			errorCount = 0;
			status_t result = fTarget->Write(buffer, readSize);
			if (result != B_OK) {
				TRACE_ERROR("writing to ring buffer failed: %s\n",
					strerror(result));
				return result;
			}
		}
	}

	return B_OK;
}
