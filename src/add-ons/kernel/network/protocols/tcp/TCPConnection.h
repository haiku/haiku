/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H


#include "tcp.h"

#include <net_protocol.h>
#include <net_stack.h>

#include <stddef.h>


class TCPConnection : public net_protocol {
	public:
		TCPConnection(net_socket *socket);
		~TCPConnection();

		status_t Open();
		status_t Close();
		status_t Free();
		status_t Connect(const struct sockaddr *address);
		status_t Accept(struct net_socket **_acceptedSocket);
		status_t Bind(struct sockaddr *address);
		status_t Unbind(struct sockaddr *address);
		status_t Listen(int count);
		status_t Shutdown(int direction);
		status_t SendData(net_buffer *buffer);
		status_t SendRoutedData(net_route *route, net_buffer *buffer);
		size_t SendAvailable();
		status_t ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer);
		size_t ReadAvailable();

		status_t ReceiveData(net_buffer *buffer);

		static void ResendSegment(struct net_timer *timer, void *data);
		static int Compare(void *_packet, const void *_key);
		static uint32 Hash(void *_packet, const void *_key, uint32 range);
		static int32 HashOffset() { return offsetof(TCPConnection, fHashLink); }

	private:
		status_t _SendQueuedData(uint16 flags, bool empty);
		status_t _EnqueueReceivedData(net_buffer *buffer, uint32 sequenceNumber);
		status_t _Reset(uint32 sequenceNum, uint32 acknowledgeNum);

		static void _TimeWait(struct net_timer *timer, void *data);

		uint32			fLastByteAckd;
		uint32			fNextByteToSend;
		uint32			fNextByteToWrite;

		uint32			fNextByteToRead;
		uint32			fNextByteExpected;
		uint32			fLastByteReceived;

		bigtime_t		fAvgRTT;

		net_buffer		*fSendBuffer;
		net_buffer		*fReceiveBuffer;

		struct list 	fReorderQueue;
		struct list 	fWaitQueue;

		TCPConnection	*fHashLink;
		tcp_state		fState;
		status_t		fError;
		benaphore		fLock;
		net_timer		fTimer;

		net_route 		*fRoute;
			// TODO: don't use a net_route, but a net_route_info!!!
};

#endif	// TCP_CONNECTION_H
