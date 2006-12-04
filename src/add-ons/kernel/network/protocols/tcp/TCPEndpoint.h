/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef TCP_ENDPOINT_H
#define TCP_ENDPOINT_H


#include "tcp.h"
#include "BufferQueue.h"

#include <net_protocol.h>
#include <net_stack.h>
#include <util/DoublyLinkedList.h>

#include <stddef.h>


class EndpointManager;

class TCPEndpoint : public net_protocol {
	public:
		TCPEndpoint(net_socket *socket);
		~TCPEndpoint();

		status_t InitCheck() const;

		recursive_lock &Lock() { return fLock; }

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
		bool IsBound() const;

		status_t DelayedAcknowledge();
		status_t SendAcknowledge();
		status_t UpdateTimeWait();
		int32 ListenReceive(tcp_segment_header& segment, net_buffer *buffer);
		int32 SynchronizeSentReceive(tcp_segment_header& segment,
			net_buffer *buffer);
		int32 Receive(tcp_segment_header& segment, net_buffer *buffer);

	private:
		friend class EndpointManager;

		void _StartPersistTimer();
		void _EnterTimeWait();
		uint8 _CurrentFlags();
		bool _ShouldSendSegment(tcp_segment_header &segment, uint32 length,
			bool outstandingAcknowledge);
		status_t _SendQueued(bool force = false);

		static void _TimeWaitTimer(net_timer *timer, void *data);
		static void _RetransmitTimer(net_timer *timer, void *data);
		static void _PersistTimer(net_timer *timer, void *data);
		static void _DelayedAcknowledgeTimer(net_timer *timer, void *data);

		TCPEndpoint		*fConnectionHashNext;
		TCPEndpoint		*fEndpointHashNext;
		TCPEndpoint		*fEndpointNextWithSamePort;

		recursive_lock	fLock;
		sem_id			fReceiveLock;
		sem_id			fSendLock;
		sem_id			fAcceptSemaphore;
		uint8			fOptions;

		uint8			fSendWindowShift;
		uint8			fReceiveWindowShift;

		tcp_sequence	fSendUnacknowledged;
		tcp_sequence	fSendNext;
		tcp_sequence	fSendMax;
		uint32			fSendWindow;
		uint32			fSendMaxWindow;
		uint32			fSendMaxSegmentSize;
		BufferQueue		fSendQueue;
		tcp_sequence	fLastAcknowledgeSent;
		tcp_sequence	fInitialSendSequence;
		uint32			fDuplicateAcknowledgeCount;

		net_route 		*fRoute;
			// TODO: don't use a net_route, but a net_route_info!!!

		tcp_sequence	fReceiveNext;
		tcp_sequence	fReceiveMaxAdvertised;
		uint32			fReceiveWindow;
		uint32			fReceiveMaxSegmentSize;
		BufferQueue		fReceiveQueue;
		tcp_sequence	fInitialReceiveSequence;

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
		uint32			fFlags;
		status_t		fError;

		// timer
		net_timer		fRetransmitTimer;
		net_timer		fPersistTimer;
		net_timer		fDelayedAcknowledgeTimer;
		net_timer		fTimeWaitTimer;
};

#endif	// TCP_ENDPOINT_H
