/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef NET_RECEIVER_H
#define NET_RECEIVER_H

#include <AutoDeleter.h>
#include <OS.h>
#include <SupportDefs.h>

class BNetEndpoint;
class StreamingRingBuffer;

typedef status_t (*NewConnectionCallback)(void *cookie, BNetEndpoint &endpoint);


class NetReceiver {
public:
								NetReceiver(BNetEndpoint *endpoint,
									StreamingRingBuffer *target,
									NewConnectionCallback callback = NULL,
									void *newConnectionCookie = NULL);
								~NetReceiver();

		BNetEndpoint *			Endpoint() { return fEndpoint.Get(); }

private:
static	int32					_NetworkReceiverEntry(void *data);
		status_t				_Listen();
		status_t				_Transfer();

		BNetEndpoint *			fListener;
		StreamingRingBuffer *	fTarget;

		thread_id				fReceiverThread;
		bool					fStopThread;

		NewConnectionCallback	fNewConnectionCallback;
		void *					fNewConnectionCookie;

		ObjectDeleter<BNetEndpoint>
								fEndpoint;
};

#endif // NET_RECEIVER_H
