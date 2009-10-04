/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef NET_SENDER_H
#define NET_SENDER_H

#include <OS.h>
#include <SupportDefs.h>

class BNetEndpoint;
class StreamingRingBuffer;

class NetSender {
public:
								NetSender(BNetEndpoint *endpoint,
									StreamingRingBuffer *source);
								~NetSender();

private:
static	int32					_NetworkSenderEntry(void *data);
		status_t				_NetworkSender();

		BNetEndpoint *			fEndpoint;
		StreamingRingBuffer *	fSource;

		thread_id				fSenderThread;
		bool					fStopThread;
};

#endif // NET_SENDER_H
