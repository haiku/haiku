/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "L2capEndpoint.h"
#include "l2cap_address.h"
#include "l2cap_upper.h"
#include "l2cap_lower.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/L2CAP/btL2CAP.h>

#include <btDebug.h>


static inline bigtime_t
absolute_timeout(bigtime_t timeout)
{
	if (timeout == 0 || timeout == B_INFINITE_TIMEOUT)
		return timeout;

	// TODO: Make overflow safe!
	return timeout + system_time();
}


L2capEndpoint::L2capEndpoint(net_socket* socket)
	:
	ProtocolSocket(socket),
	fConfigurationSet(false),
	fEstablishSemaphore(-1),
	fPeerEndpoint(NULL),
	fChannel(NULL)
{
	CALLED();

	/* Set MTU and flow control settings to defaults */
	fConfiguration.imtu = L2CAP_MTU_DEFAULT;
	memcpy(&fConfiguration.iflow, &default_qos , sizeof(l2cap_flow_t) );

	fConfiguration.omtu = L2CAP_MTU_DEFAULT;
	memcpy(&fConfiguration.oflow, &default_qos , sizeof(l2cap_flow_t) );

	fConfiguration.flush_timo = L2CAP_FLUSH_TIMO_DEFAULT;
	fConfiguration.link_timo  = L2CAP_LINK_TIMO_DEFAULT;

	// TODO: XXX not for listening endpoints, imtu should be known first
	gStackModule->init_fifo(&fReceivingFifo, "l2cap recvfifo", L2CAP_MTU_DEFAULT);
}


L2capEndpoint::~L2capEndpoint()
{
	CALLED();

	gStackModule->uninit_fifo(&fReceivingFifo);
}


status_t
L2capEndpoint::Init()
{
	CALLED();

	return B_OK;
}


void
L2capEndpoint::Uninit()
{
	CALLED();

}


status_t
L2capEndpoint::Open()
{
	CALLED();

	status_t error = ProtocolSocket::Open();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
L2capEndpoint::Close()
{
	CALLED();

	if (fChannel == NULL) {
		// TODO: Parent socket

	} else {
		// Child Socket
		if (fState == CLOSED) {
			// TODO: Clean needed stuff
			return B_OK;
		} else {
			// Issue Disconnection request over the channel
			MarkClosed();

			bigtime_t timeout = absolute_timeout(300 * 1000 * 1000);

			status_t error = l2cap_upper_dis_req(fChannel);

			if (error != B_OK)
				return error;

			return acquire_sem_etc(fEstablishSemaphore, 1,
				B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);
		}
	}

	if (fEstablishSemaphore != -1) {
		delete_sem(fEstablishSemaphore);
	}

	return B_OK;
}


status_t
L2capEndpoint::Free()
{
	CALLED();

	return B_OK;
}


status_t
L2capEndpoint::Bind(const struct sockaddr* _address)
{
	const sockaddr_l2cap* address
		= reinterpret_cast<const sockaddr_l2cap*>(_address);

	if (_address == NULL)
		return B_ERROR;

	if (address->l2cap_family != AF_BLUETOOTH )
		return EAFNOSUPPORT;

	if (address->l2cap_len != sizeof(struct sockaddr_l2cap))
		return EAFNOSUPPORT;

	// TODO: Check if that PSM is already bound
	// return EADDRINUSE;

	// TODO: Check if the PSM is valid, check assigned numbers document for valid
	// psm available to applications.
	// All PSM values shall be ODD, that is, the least significant bit of the least
	// significant octet must be ’1’. Also, all PSM values shall have the least
	// significant bit of the most significant octet equal to ’0’. This allows
	// the PSM field to be extended beyond 16 bits.
	if ((address->l2cap_psm & 1) == 0)
		return B_ERROR;

	memcpy(&socket->address, _address, sizeof(struct sockaddr_l2cap));
	socket->address.ss_len = sizeof(struct sockaddr_l2cap);

	fState = BOUND;

	return B_OK;
}


status_t
L2capEndpoint::Unbind()
{
	CALLED();

	return B_OK;
}


status_t
L2capEndpoint::Listen(int backlog)
{
	CALLED();

	if (fState != BOUND) {
		ERROR("%s: Invalid State\n", __func__);
		return B_BAD_VALUE;
	}

	fEstablishSemaphore = create_sem(0, "l2cap serv accept");
	if (fEstablishSemaphore < B_OK) {
		ERROR("%s: Semaphore could not be created\n", __func__);
		return ENOBUFS;
	}

	gSocketModule->set_max_backlog(socket, backlog);

	fState = LISTEN;

	return B_OK;
}


status_t
L2capEndpoint::Connect(const struct sockaddr* _address)
{
	const sockaddr_l2cap* address
		= reinterpret_cast<const sockaddr_l2cap*>(_address);

	if (address->l2cap_len != sizeof(*address))
		return EINVAL;

	// Check for any specific status?
	if (fState == CONNECTING)
		return EINPROGRESS;

	// TODO: should not be in the BOUND status first?

	#if 0
	TRACE("%s: [%ld] %p->L2capEndpoint::Connect(\"%s\")\n", __func__,
		find_thread(NULL), this,
		ConstSocketAddress(&gL2cap4AddressModule, _address)
		.AsString().Data());
	#endif

	// TODO: If we were bound to a specific source address

	// Route, we must find a Connection descriptor with address->l2cap_address
	hci_id hid = btCoreData->RouteConnection(address->l2cap_bdaddr);

	#if 0
	TRACE("%s: %" B_PRId32 " for route %s\n", __func__, hid,
		bdaddrUtils::ToString(address->l2cap_bdaddr).String());
	#endif

	if (hid > 0) {
		HciConnection* connection = btCoreData->ConnectionByDestination(
			address->l2cap_bdaddr, hid);

		L2capChannel* channel = btCoreData->AddChannel(connection,
			address->l2cap_psm);

		if (channel == NULL)
			return ENOMEM;

		// Send connection request
		if (l2cap_upper_con_req(channel) == B_OK) {
			fState = CONNECTING;

			BindToChannel(channel);

			fEstablishSemaphore = create_sem(0, "l2cap client");
			if (fEstablishSemaphore < B_OK) {
				ERROR("%s: Semaphore could not be created\n", __func__);
				return ENOBUFS;
			}

			bigtime_t timeout = absolute_timeout(300 * 1000 * 1000);

			return acquire_sem_etc(fEstablishSemaphore, 1,
				B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);

		} else {
			return ECONNREFUSED;
		}
	}

	return ENETUNREACH;
}


status_t
L2capEndpoint::Accept(net_socket** _acceptedSocket)
{
	CALLED();

	// MutexLocker locker(fLock);

	status_t status;
	bigtime_t timeout = absolute_timeout(300 * 1000 * 1000);

	do {
		// locker.Unlock();

		status = acquire_sem_etc(fEstablishSemaphore, 1, B_ABSOLUTE_TIMEOUT
			| B_CAN_INTERRUPT, timeout);

		if (status != B_OK)
			return status;

		// locker.Lock();
		status = gSocketModule->dequeue_connected(socket, _acceptedSocket);

		if (status != B_OK) {
			ERROR("%s: Could not dequeue socket %s\n", __func__,
				strerror(status));
		} else {

			((L2capEndpoint*)((*_acceptedSocket)->first_protocol))->fState = ESTABLISHED;
			// unassign any channel for the parent endpoint
			fChannel = NULL;
			// we are listening again
			fState = LISTEN;
		}

	} while (status != B_OK);

	return status;
}


ssize_t
L2capEndpoint::Send(const iovec* vecs, size_t vecCount,
	ancillary_data_container* ancillaryData)
{
	CALLED();

	return B_OK;
}


ssize_t
L2capEndpoint::Receive(const iovec* vecs, size_t vecCount,
	ancillary_data_container** _ancillaryData, struct sockaddr* _address,
	socklen_t* _addressLength)
{
	CALLED();

	if (fState != ESTABLISHED) {
		ERROR("%s: Invalid State %p\n", __func__, this);
		return B_BAD_VALUE;
	}

	return B_OK;
}


ssize_t
L2capEndpoint::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	CALLED();

	if (fState != ESTABLISHED) {
		ERROR("%s: Invalid State %p\n", __func__, this);
		return B_BAD_VALUE;
	}

	return gStackModule->fifo_dequeue_buffer(&fReceivingFifo, flags,
		B_INFINITE_TIMEOUT, _buffer);
}


ssize_t
L2capEndpoint::SendData(net_buffer* buffer)
{
	CALLED();

	if (fState != ESTABLISHED) {
		ERROR("%s: Invalid State %p\n", __func__, this);
		return B_BAD_VALUE;
	}

	btCoreData->SpawnFrame(fChannel->conn, fChannel, buffer, L2CAP_B_FRAME);

	SchedConnectionPurgeThread(fChannel->conn);

	// TODO: Report bytes sent?
	return B_OK;
}


ssize_t
L2capEndpoint::Sendable()
{
	CALLED();
	return B_OK;
}


ssize_t
L2capEndpoint::Receivable()
{
	CALLED();
	return 0;
}


L2capEndpoint*
L2capEndpoint::ForPsm(uint16 psm)
{
	L2capEndpoint* endpoint;

	DoublyLinkedList<L2capEndpoint>::Iterator iterator
		= EndpointList.GetIterator();

	while (iterator.HasNext()) {

		endpoint = iterator.Next();
		if (((struct sockaddr_l2cap*)&endpoint->socket->address)->l2cap_psm == psm
			&& endpoint->fState == LISTEN) {
			// TODO endpoint ocupied, lock it! define a channel for it
			return endpoint;
		}
	}

	return NULL;
}


void
L2capEndpoint::BindNewEnpointToChannel(L2capChannel* channel)
{
	net_socket* newSocket;
	status_t error = gSocketModule->spawn_pending_socket(socket, &newSocket);
	if (error != B_OK) {
		ERROR("%s: Could not spawn child for Endpoint %p\n", __func__, this);
		// TODO: Handle situation
		return;
	}

	L2capEndpoint* endpoint = (L2capEndpoint*)newSocket->first_protocol;

	endpoint->fChannel = channel;
	endpoint->fPeerEndpoint = this;

	channel->endpoint = endpoint;

	//debugf("new socket %p/e->%p from parent %p/e->%p\n",
	//	newSocket, endpoint, socket, this);

	// Provide the channel the configuration set by the user socket
	channel->configuration = &fConfiguration;

	// It might be used keep the last negotiated channel
	// fChannel = channel;

	//debugf("New endpoint %p for psm %d, schannel %x dchannel %x\n", endpoint,
	//	channel->psm, channel->scid, channel->dcid);
}


void
L2capEndpoint::BindToChannel(L2capChannel* channel)
{
	this->fChannel = channel;
	channel->endpoint = this;

	// Provide the channel the configuration set by the user socket
	channel->configuration = &fConfiguration;

	// no parent to give feedback
	fPeerEndpoint = NULL;
}


status_t
L2capEndpoint::MarkEstablished()
{
	CALLED();

	status_t error = B_OK;
	fChannel->state = L2CAP_CHAN_OPEN;
	fState = ESTABLISHED;

	if (fPeerEndpoint != NULL) {

		error = gSocketModule->set_connected(socket);
		if (error == B_OK) {
			release_sem(fPeerEndpoint->fEstablishSemaphore);
		} else {
			ERROR("%s: Could not set child Endpoint %p %s\n", __func__, this,
				strerror(error));
		}
	} else
		release_sem(fEstablishSemaphore);

	return error;
}


status_t
L2capEndpoint::MarkClosed()
{
	CALLED();

	if (fState == CLOSED)
		release_sem(fEstablishSemaphore);

	fState = CLOSED;

	return B_OK;
}
