/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef NET_RECEIVER_H
#define NET_RECEIVER_H

#include <OS.h>
#include <SupportDefs.h>

class BNetEndpoint;
class StreamingRingBuffer;

class NetReceiver {
public:
								NetReceiver(BNetEndpoint *listener,
									StreamingRingBuffer *target);
								~NetReceiver();

		BNetEndpoint *			Endpoint() { return fEndpoint; }

private:
static	int32					_NetworkReceiverEntry(void *data);
		status_t				_NetworkReceiver();

		BNetEndpoint *			fListener;
		StreamingRingBuffer *	fTarget;

		thread_id				fReceiverThread;
		bool					fStopThread;

		BNetEndpoint *			fEndpoint;
};

#endif // NET_RECEIVER_H
