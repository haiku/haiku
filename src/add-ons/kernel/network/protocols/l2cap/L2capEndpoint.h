/*
 * Copyright 2008, Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_ENDPOINT_H
#define L2CAP_ENDPOINT_H


#include <lock.h>
#include <condition_variable.h>
#include <util/AVLTree.h>

#include <net_protocol.h>
#include <net_socket.h>
#include <ProtocolUtilities.h>

#include <bluetooth/L2CAP/btL2CAP.h>
#include "l2cap_internal.h"


class L2capEndpoint;


class L2capEndpoint : public net_protocol, public ProtocolSocket, public AVLTreeNode {
public:
						L2capEndpoint(net_socket* socket);
	virtual				~L2capEndpoint();

			status_t	Open();
			status_t	Shutdown();
			status_t	Close();
			status_t	Free();

			status_t	Bind(const struct sockaddr* _address);
			status_t	Unbind();
			status_t	Listen(int backlog);
			status_t	Connect(const struct sockaddr* address);
			status_t	Accept(net_socket** _acceptedSocket);

			uint16		ChannelID() const { return fChannelID; }

			ssize_t		ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer);
			ssize_t		SendData(net_buffer* buffer);
			status_t	ReceiveData(net_buffer* buffer);
			ssize_t		Sendable();
			ssize_t		Receivable();

public:
			void		_HandleCommandRejected(uint8 ident, uint16 reason,
							const l2cap_command_reject_data& data);
			void 		_HandleConnectionReq(HciConnection* connection,
							uint8 ident, uint16 psm, uint16 scid);
			void		_HandleConnectionRsp(uint8 ident, const l2cap_connection_rsp& response);
			void		_HandleConfigurationReq(uint8 ident, uint16 flags,
							uint16* mtu, uint16* flush_timeout, l2cap_qos* flow);
			void		_HandleConfigurationRsp(uint8 ident, uint16 scid, uint16 flags,
							uint16 result, uint16* mtu, uint16* flush_timeout, l2cap_qos* flow);
			void		_HandleDisconnectionReq(uint8 ident, uint16 scid);
			void		_HandleDisconnectionRsp(uint8 ident, uint16 dcid, uint16 scid);

private:
	inline bool
	_IsEstablishing()
	{
		return fState > LISTEN && fState < OPEN;
	}

			status_t	_WaitForStateChange(bigtime_t absoluteTimeout);

	static	void		_SendTimer(net_timer* timer, void* _endpoint);
			void		_SendQueued();

			void		_SendChannelConfig();
			status_t	_MarkEstablished();
			void		_MarkClosed();

private:
	friend class L2capEndpointManager;

	sem_id			fAcceptSemaphore;
	mutex			fLock;

	enum channel_status {
		CLOSED,
		BOUND,
		LISTEN,

		WAIT_FOR_CONNECTION_RSP,
		CONFIGURATION,
		OPEN,
		WAIT_FOR_DISCONNECTION_RSP,
		RECEIVED_DISCONNECTION_REQ,
	} fState;

	HciConnection*	fConnection;

	ConditionVariable fCommandWait;

	uint16			fChannelID, fDestinationChannelID;

	net_fifo		fReceiveQueue, fSendQueue;
	net_timer		fSendTimer;

	struct ConfigState {
		enum : uint8 {
			NONE,
			SENT,
			DONE,
		} in, out;
	} fConfigState;
	struct ChannelConfiguration {
		uint16		incoming_mtu;
		l2cap_qos	incoming_flow;
		uint16		outgoing_mtu;
		l2cap_qos	outgoing_flow;

		uint16		flush_timeout;
		uint16		link_timeout;
	} fChannelConfig;
};


#endif	// L2CAP_ENDPOINT_H
