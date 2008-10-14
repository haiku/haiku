/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BCOREDATA_H
#define _BCOREDATA_H

#include <module.h>
#include <util/DoublyLinkedList.h>
#include <util/DoublyLinkedQueue.h>
#include <net_buffer.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>
#include <l2cap.h>

#define BT_CORE_DATA_MODULE_NAME "generic/btCoreData/v1"

struct L2capChannel;
struct L2capFrame;
struct L2capEndpoint;

typedef enum _connection_status {
	HCI_CONN_CLOSED,
	HCI_CONN_OPEN,
} connection_status;

struct HciConnection : DoublyLinkedListLinkImpl<HciConnection> {
	hci_id				Hid;
	net_buffer*			currentRxPacket;
	ssize_t				currentRxExpectedLength;
	bdaddr_t			destination;
	uint16        		handle;
	int			  		type;
	uint16				mtu;
	connection_status   status;      /* ACL connection state */
	uint16				lastCid;
	DoublyLinkedList<L2capChannel> ChannelList;
	DoublyLinkedQueue<L2capFrame> ExpectedResponses;
	DoublyLinkedQueue<L2capFrame> OutGoingFrames;
};

typedef enum _channel_status {
	L2CAP_CHAN_CLOSED,				/* channel closed */
	L2CAP_CHAN_W4_L2CAP_CON_RSP,	/* wait for L2CAP resp. */
	L2CAP_CHAN_W4_L2CA_CON_RSP,		/* wait for upper resp. */
	L2CAP_CHAN_CONFIG,				/* L2CAP configuration */
	L2CAP_CHAN_OPEN,				/* channel open */
	L2CAP_CHAN_W4_L2CAP_DISCON_RSP,	/* wait for L2CAP discon. */
	L2CAP_CHAN_W4_L2CA_DISCON_RSP	/* wait for upper discon. */
} channel_status;

struct L2capChannel : DoublyLinkedListLinkImpl<L2capChannel> {
	HciConnection*		conn;
	uint16        		scid;
	uint16        		dcid;
	uint16				psm;
	uint8				ident;

	channel_status		state;
	uint16				imtu;       /* incoming channel MTU */
	l2cap_flow_t		iflow;      /* incoming flow control */
	uint16				omtu;       /* outgoing channel MTU */
	l2cap_flow_t		oflow;      /* outgoing flow control */

	uint16				flush_timo; /* flush timeout */
	uint16				link_timo;  /* link timeout */

	int					cfgState;
	L2capEndpoint*		endpoint;
};

typedef enum _frametype {
	L2CAP_C_FRAME, // signals
	L2CAP_G_FRAME, // CL packets
	L2CAP_B_FRAME, // CO packets

	L2CAP_I_FRAME,
	L2CAP_S_FRAME
} frame_type;

struct L2capFrame : DoublyLinkedListLinkImpl<L2capFrame> {

	HciConnection*	conn;
	L2capChannel*	channel;

	uint16		flags;     /* command flags */
    #define L2CAP_CMD_PENDING   (1 << 0)   /* command is pending */

	uint8	code;	/* L2CAP command opcode */
	uint8	ident;	/* L2CAP command ident */

    frame_type	type;

	net_buffer*	buffer; // contains 1 l2cap / mutliple acls

    //TODO :struct callout			 timo;		/* RTX/ERTX timeout */
};


struct bluetooth_core_data_module_info {
	module_info info;
	HciConnection*	(*AddConnection)(uint16 handle, int type, bdaddr_t *dst, hci_id hid);

	/*status_t	(*RemoveConnection)(bdaddr_t destination, hci_id hid);*/
	status_t	(*RemoveConnection)(uint16 handle, hci_id hid);

	hci_id		(*RouteConnection)(bdaddr_t* destination);

	void		(*SetAclBuffer)(HciConnection* conn, net_buffer* nbuf);
	void		(*SetAclExpectedSize)(HciConnection* conn, size_t size);
	void 		(*AclPutting)(HciConnection* conn, size_t size);
	bool		(*AclComplete)(HciConnection* conn);
	bool		(*AclOverFlowed)(HciConnection* conn);

	HciConnection*	(*ConnectionByHandle)(uint16 handle, hci_id hid);
	HciConnection*	(*ConnectionByDestination)(bdaddr_t* destination, hci_id hid);

	L2capChannel*		(*AddChannel)(HciConnection* conn, uint16 psm);
	void 				(*RemoveChannel)(HciConnection* conn, uint16 scid);
	L2capChannel*		(*ChannelBySourceID)(HciConnection* conn, uint16 sid);
	uint16			(*ChannelAllocateCid)(HciConnection* conn);
	uint16			(*ChannelAllocateIdent)(HciConnection* conn);

	L2capFrame*		(*SignalByIdent)(HciConnection* conn, uint8 ident);
	status_t		(*TimeoutSignal)(L2capFrame* frame, uint32 timeo);
	status_t		(*UnTimeoutSignal)(L2capFrame* frame);
	L2capFrame*		(*SpawnFrame)(HciConnection* conn, net_buffer* buffer, frame_type frame);
	L2capFrame*		(*SpawnSignal)(HciConnection* conn, L2capChannel* channel, net_buffer* buffer, uint8 ident, uint8 code);
	status_t		(*AcknowledgeSignal)(L2capFrame* frame);

};

inline bool ExistConnectionByDestination(bdaddr_t* destination, hci_id hid = -1);
inline bool ExistConnectionByHandle(uint16 handle, hci_id hid);

#endif // _BCOREDATA_H
