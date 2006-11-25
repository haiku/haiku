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
#include "BufferQueue.h"

#include <net_protocol.h>
#include <net_stack.h>
#include <util/DoublyLinkedList.h>

#include <stddef.h>


class TCPConnection : public net_protocol {
	public:
		TCPConnection(net_socket *socket);
		~TCPConnection();

		status_t InitCheck() const;

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
		size_t SendAvailable();
		status_t ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer);
		size_t ReadAvailable();

		tcp_state State() const { return fState; }

		status_t DelayedAcknowledge();
		status_t SendAcknowledge();
		status_t UpdateTimeWait();
		int32 ListenReceive(tcp_segment_header& segment, net_buffer *buffer);
		int32 SynchronizeSentReceive(tcp_segment_header& segment,
			net_buffer *buffer);
		int32 Receive(tcp_segment_header& segment, net_buffer *buffer);

		static void ResendSegment(struct net_timer *timer, void *data);
		static int Compare(void *_packet, const void *_key);
		static uint32 Hash(void *_packet, const void *_key, uint32 range);
		static int32 HashOffset() { return offsetof(TCPConnection, fHashNext); }

	private:
		bool _IsAcknowledgeValid(uint32 acknowledge);
		bool _IsSequenceValid(uint32 sequence, uint32 length);

		status_t _SendQueuedData(uint16 flags, bool empty);

		static void _TimeWait(struct net_timer *timer, void *data);

		TCPConnection	*fHashNext;

		//benaphore		fLock;
		sem_id			fReceiveLock;
		sem_id			fSendLock;

		uint8			fSendWindowShift;
		uint8			fReceiveWindowShift;

		uint32			fLastAcknowledged;
		uint32			fSendNext;
		uint32			fSendWindow;
		uint32			fMaxSegmentSize;
		BufferQueue		fSendQueue;

		net_route 		*fRoute;
			// TODO: don't use a net_route, but a net_route_info!!!

		uint32			fReceiveNext;
		uint32			fReceiveWindow;
		uint32			fMaxReceiveSize;
		BufferQueue		fReceiveQueue;

		// round trip time and retransmit timeout computation
		int32			fRoundTripTime;
		int32			fRetransmitTimeoutBase;
		bigtime_t		fRetransmitTimeout;
		int32			fRoundTripDeviation;
		bigtime_t		fTrackingTimeStamp;
		uint32			fTrackingSequence;
		bool			fTracking;

		uint32			fCongestionWindow;
		uint32			fSlowStartThreshold;

		tcp_state		fState;
		status_t		fError;
		vint32			fDelayedAcknowledge;

		// timer
		net_timer		fTimer;
};

#endif	// TCP_CONNECTION_H
