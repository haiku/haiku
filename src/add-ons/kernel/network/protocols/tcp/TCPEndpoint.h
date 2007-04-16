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
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <stddef.h>


class EndpointManager;

class WaitList {
public:
	WaitList(const char *name);
	~WaitList();

	status_t InitCheck() const;

	status_t Wait(RecursiveLocker &, bigtime_t timeout, bool wakeNext = true);
	void Signal();

private:
	sem_id fSem;
};


class TCPEndpoint : public net_protocol {
	public:
		TCPEndpoint(net_socket *socket);
		~TCPEndpoint();

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
		ssize_t SendAvailable();
		status_t ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer);
		ssize_t ReadAvailable();

		tcp_state State() const { return fState; }
		bool IsBound() const;

		void DeleteSocket();

		status_t DelayedAcknowledge();
		status_t SendAcknowledge();
		status_t UpdateTimeWait();

		int32 SegmentReceived(tcp_segment_header& segment, net_buffer *buffer);
		int32 Spawn(TCPEndpoint *parent, tcp_segment_header& segment,
			net_buffer *buffer);

		net_domain *Domain() const
			{ return socket->first_protocol->module->get_domain(
				socket->first_protocol); }
		net_address_module_info *AddressModule() const
			{ return Domain()->address_module; }

	private:
		friend class EndpointManager;

		void _StartPersistTimer();
		void _EnterTimeWait();
		uint8 _CurrentFlags();
		bool _ShouldSendSegment(tcp_segment_header &segment, uint32 length,
			bool outstandingAcknowledge);
		status_t _SendQueued(bool force = false);
		int _GetMSS(const struct sockaddr *) const;
		status_t _ShutdownEgress(bool closing);
		ssize_t _AvailableData() const;
		void _NotifyReader();
		bool _ShouldReceive() const;
		int32 _ListenReceive(tcp_segment_header& segment, net_buffer *buffer);
		int32 _SynchronizeSentReceive(tcp_segment_header& segment,
			net_buffer *buffer);
		int32 _SegmentReceived(tcp_segment_header& segment, net_buffer *buffer);
		int32 _Receive(tcp_segment_header& segment, net_buffer *buffer);
		void _UpdateTimestamps(tcp_segment_header& segment,
			size_t segmentLength, bool checkSequence);
		void _MarkEstablished();
		status_t _WaitForEstablished(RecursiveLocker &lock, bigtime_t timeout);

		static void _TimeWaitTimer(net_timer *timer, void *data);
		static void _RetransmitTimer(net_timer *timer, void *data);
		static void _PersistTimer(net_timer *timer, void *data);
		static void _DelayedAcknowledgeTimer(net_timer *timer, void *data);

		EndpointManager *fManager;

		TCPEndpoint		*fConnectionHashNext;
		TCPEndpoint		*fEndpointHashNext;
		TCPEndpoint		*fEndpointNextWithSamePort;

		recursive_lock	fLock;
		WaitList		fReceiveList;
		WaitList		fSendList;
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

		uint32			fReceivedTSval;
		bool			fUsingTimestamps;

		uint32			fCongestionWindow;
		uint32			fSlowStartThreshold;

		tcp_state		fState;
		uint32			fFlags;
		status_t		fError;

		bool			fSpawned;

		// timer
		net_timer		fRetransmitTimer;
		net_timer		fPersistTimer;
		net_timer		fDelayedAcknowledgeTimer;
		net_timer		fTimeWaitTimer;
};

#endif	// TCP_ENDPOINT_H
