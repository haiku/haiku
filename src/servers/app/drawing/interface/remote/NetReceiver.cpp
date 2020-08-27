/*
 * Copyright 2009, 2017, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "NetReceiver.h"
#include "RemoteMessage.h"

#include "StreamingRingBuffer.h"

#include <NetEndpoint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			/*debug_printf("NetReceiver: " x)*/
#define TRACE_ERROR(x...)	debug_printf("NetReceiver: " x)


NetReceiver::NetReceiver(BNetEndpoint *listener, StreamingRingBuffer *target,
	NewConnectionCallback newConnectionCallback, void *newConnectionCookie)
	:
	fListener(listener),
	fTarget(target),
	fReceiverThread(-1),
	fStopThread(false),
	fNewConnectionCallback(newConnectionCallback),
	fNewConnectionCookie(newConnectionCookie),
	fEndpoint(newConnectionCallback == NULL ? listener : NULL)
{
	fReceiverThread = spawn_thread(_NetworkReceiverEntry, "network receiver",
		B_NORMAL_PRIORITY, this);
	resume_thread(fReceiverThread);
}


NetReceiver::~NetReceiver()
{
	fStopThread = true;
	fEndpoint.Unset();

	suspend_thread(fReceiverThread);
	resume_thread(fReceiverThread);
}


int32
NetReceiver::_NetworkReceiverEntry(void *data)
{
	NetReceiver *receiver = (NetReceiver *)data;
	if (receiver->fNewConnectionCallback)
		return receiver->_Listen();
	else
		return receiver->_Transfer();
}


status_t
NetReceiver::_Listen()
{
	status_t result = fListener->Listen();
	if (result != B_OK) {
		TRACE_ERROR("failed to listen on port: %s\n", strerror(result));
		return result;
	}

	while (!fStopThread) {
		fEndpoint.SetTo(fListener->Accept(5000));
		if (fEndpoint.Get() == NULL) {
			TRACE("got NULL endpoint from accept\n");
			continue;
		}

		TRACE("new endpoint connection: %p\n", fEndpoint);

		if (fNewConnectionCallback != NULL
			&& fNewConnectionCallback(
				fNewConnectionCookie, *fEndpoint.Get()) != B_OK)
		{
			TRACE("connection callback rejected connection\n");
			continue;
		}

		_Transfer();
	}

	return B_OK;
}


status_t
NetReceiver::_Transfer()
{
	int32 errorCount = 0;

	while (!fStopThread) {
		uint8 buffer[4096];
		int32 readSize = fEndpoint->Receive(buffer, sizeof(buffer));
		if (readSize < 0) {
			TRACE_ERROR("read failed, closing connection: %s\n",
				strerror(readSize));
			return readSize;
		}

		if (readSize == 0) {
			TRACE("read 0 bytes, retrying\n");
			snooze(100 * 1000);
			errorCount++;
			if (errorCount == 5) {
				TRACE_ERROR("failed to read, assuming disconnect\n");
				return B_ERROR;
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

	return B_OK;
}
