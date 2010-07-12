/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_ENDPOINT_H
#define L2CAP_ENDPOINT_H

#include <sys/stat.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <net_protocol.h>
#include <net_socket.h>
#include <ProtocolUtilities.h>

#include "l2cap_internal.h"

extern net_stack_module_info* gStackModule;

class L2capEndpoint : public net_protocol,
	public ProtocolSocket,
	public DoublyLinkedListLinkImpl<L2capEndpoint> {

public:
	L2capEndpoint(net_socket* socket);
	virtual ~L2capEndpoint();

	status_t Init();
	void Uninit();

	status_t Open();
	status_t Close();
	status_t Free();

	bool Lock()
	{
		return mutex_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		mutex_unlock(&fLock);
	}

	status_t	Bind(const struct sockaddr* _address);
	status_t	Unbind();
	status_t	Listen(int backlog);
	status_t	Connect(const struct sockaddr* address);
	status_t	Accept(net_socket** _acceptedSocket);

	ssize_t		Send(const iovec* vecs, size_t vecCount,
					ancillary_data_container* ancillaryData);
	ssize_t		Receive(const iovec* vecs, size_t vecCount,
					ancillary_data_container** _ancillaryData,
					struct sockaddr* _address, socklen_t* _addressLength);

	ssize_t ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer);
	ssize_t SendData(net_buffer* buffer);
	ssize_t Sendable();
	ssize_t Receivable();

	status_t SetReceiveBufferSize(size_t size);
	status_t GetPeerCredentials(ucred* credentials);

	status_t Shutdown(int direction);

	void 		BindNewEnpointToChannel(L2capChannel* channel);
	void 		BindToChannel(L2capChannel* channel);
	status_t	MarkEstablished();
	status_t	MarkClosed();

	static L2capEndpoint* ForPsm(uint16 psm);

	bool RequiresConfiguration()
	{
		return fConfigurationSet;
	}

	ChannelConfiguration 	fConfiguration;
	bool 					fConfigurationSet;
	net_fifo		fReceivingFifo;

private:
	typedef enum {
		// establishing a connection
		CLOSED,
		BOUND,
		LISTEN,
		CONNECTING,
		ESTABLISHED,

		// peer closes the connection
		FINISH_RECEIVED,
		WAIT_FOR_FINISH_ACKNOWLEDGE,

		// we close the connection
		FINISH_SENT,
		FINISH_ACKNOWLEDGED,
		CLOSING,
		TIME_WAIT
	} State;

	mutex			fLock;
	State    		fState;
	sem_id			fEstablishSemaphore;
	L2capEndpoint*	fPeerEndpoint;
	L2capChannel* 	fChannel;

};


extern DoublyLinkedList<L2capEndpoint> EndpointList;

#endif	// L2CAP_ENDPOINT_H
