/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "NetSender.h"

#include "StreamingRingBuffer.h"

#include <NetEndpoint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			/*debug_printf("NetSender: "x)*/
#define TRACE_ERROR(x...)	debug_printf("NetSender: "x)


NetSender::NetSender(BNetEndpoint *endpoint, StreamingRingBuffer *source)
	:
	fEndpoint(endpoint),
	fSource(source),
	fSenderThread(-1),
	fStopThread(false)
{
	fSenderThread = spawn_thread(_NetworkSenderEntry, "network sender",
		B_NORMAL_PRIORITY, this);
	resume_thread(fSenderThread);
}


NetSender::~NetSender()
{
	fStopThread = true;
	int32 result;
	wait_for_thread(fSenderThread, &result);
}


int32
NetSender::_NetworkSenderEntry(void *data)
{
	return ((NetSender *)data)->_NetworkSender();
}


status_t
NetSender::_NetworkSender()
{
	while (!fStopThread) {
		uint8 buffer[4096];
		int32 readSize = fSource->Read(buffer, sizeof(buffer), true);
		if (readSize < 0) {
			TRACE_ERROR("read failed, stopping sender thread: %s\n",
				strerror(readSize));
			return readSize;
		}

		while (readSize > 0) {
			int32 sendSize = fEndpoint->Send(buffer, readSize);
			if (sendSize < 0) {
				TRACE_ERROR("sending data failed: %s\n", strerror(sendSize));
				return sendSize;
			}

			readSize -= sendSize;
		}
	}

	return B_OK;
}
