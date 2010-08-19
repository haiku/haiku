/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTCOREDATA_H
#define _BTCOREDATA_H


#include <module.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/DoublyLinkedQueue.h>

#include <net_buffer.h>
#include <net_device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <l2cap.h>


#define BT_CORE_DATA_MODULE_NAME "bluetooth/btCoreData/v1"


struct L2capChannel;
struct L2capFrame;
struct L2capEndpoint;

typedef enum _connection_status {
	HCI_CONN_CLOSED,
	HCI_CONN_OPEN,
} connection_status;


#ifdef __cplusplus

struct HciConnection : DoublyLinkedListLinkImpl<HciConnection> {
	HciConnection();
	virtual ~HciConnection();

	hci_id				Hid;
	bluetooth_device*	ndevice;
	net_buffer*			currentRxPacket;
	ssize_t				currentRxExpectedLength;
	bdaddr_t			destination;
	uint16				handle;
	int					type;
	uint16				mtu;
	connection_status	status;
	uint16				lastCid;
	uint8				lastIdent;
	DoublyLinkedList<L2capChannel> ChannelList;
	DoublyLinkedList<L2capFrame> ExpectedResponses;
	DoublyLinkedList<L2capFrame> OutGoingFrames;
	mutex				fLock;
	mutex				fLockExpected;
};

#else

struct HciConnection;

#endif

typedef enum _channel_status {
	L2CAP_CHAN_CLOSED,				/* channel closed */
	L2CAP_CHAN_W4_L2CAP_CON_RSP,	/* wait for L2CAP resp. */
	L2CAP_CHAN_W4_L2CA_CON_RSP,		/* wait for upper resp. */
	L2CAP_CHAN_CONFIG,				/* L2CAP configuration */
	L2CAP_CHAN_OPEN,				/* channel open */
	L2CAP_CHAN_W4_L2CAP_DISCON_RSP,	/* wait for L2CAP discon. */
	L2CAP_CHAN_W4_L2CA_DISCON_RSP	/* wait for upper discon. */
} channel_status;


#ifdef __cplusplus

typedef struct _ChannelConfiguration {

	uint16			imtu;	/* incoming channel MTU */
	l2cap_flow_t	iflow;	/* incoming flow control */
	uint16			omtu;	/* outgoing channel MTU */
	l2cap_flow_t	oflow;	/* outgoing flow control */

	uint16			flush_timo;	/* flush timeout */
	uint16			link_timo;	/* link timeout */

} ChannelConfiguration;

struct L2capChannel : DoublyLinkedListLinkImpl<L2capChannel> {
	HciConnection*	conn;
	uint16			scid;
	uint16			dcid;
	uint16			psm;
	uint8			ident;
	uint8			cfgState;

	channel_status		state;
	ChannelConfiguration*	configuration;
	L2capEndpoint*		endpoint;
};

#endif


typedef enum _frametype {
	L2CAP_C_FRAME, // signals
	L2CAP_G_FRAME, // CL packets
	L2CAP_B_FRAME, // CO packets

	L2CAP_I_FRAME,
	L2CAP_S_FRAME
} frame_type;

#ifdef __cplusplus

struct L2capFrame : DoublyLinkedListLinkImpl<L2capFrame> {

	HciConnection*	conn;
	L2capChannel*	channel;

	uint16			flags;	/* command flags */
	#define L2CAP_CMD_PENDING	(1 << 0)	/* command is pending */

	uint8			code;	/* L2CAP command opcode */
	uint8			ident;	/* L2CAP command ident */

	frame_type		type;

	net_buffer*		buffer; // contains 1 l2cap / mutliple acls

    // TODO :struct callout	timo;		/* RTX/ERTX timeout */
};

#endif


struct bluetooth_core_data_module_info {
	module_info info;

	status_t				(*PostEvent)(bluetooth_device* ndev, void* event,
								size_t size);
	struct HciConnection*	(*AddConnection)(uint16 handle, int type,
								const bdaddr_t& dst, hci_id hid);

	// status_t				(*RemoveConnection)(bdaddr_t destination, hci_id hid);
	status_t				(*RemoveConnection)(uint16 handle, hci_id hid);

	hci_id					(*RouteConnection)(const bdaddr_t& destination);

	void					(*SetAclBuffer)(struct HciConnection* conn,
								net_buffer* nbuf);
	void					(*SetAclExpectedSize)(struct HciConnection* conn,
								size_t size);
	void					(*AclPutting)(struct HciConnection* conn,
								size_t size);
	bool					(*AclComplete)(struct HciConnection* conn);
	bool					(*AclOverFlowed)(struct HciConnection* conn);

	struct HciConnection*	(*ConnectionByHandle)(uint16 handle, hci_id hid);
	struct HciConnection*	(*ConnectionByDestination)(
								const bdaddr_t& destination, hci_id hid);

	struct L2capChannel*	(*AddChannel)(struct HciConnection* conn,
								uint16 psm);
	void					(*RemoveChannel)(struct HciConnection* conn,
								uint16 scid);
	struct L2capChannel*	(*ChannelBySourceID)(struct HciConnection* conn,
								uint16 sid);
	uint16					(*ChannelAllocateCid)(struct HciConnection* conn);
	uint16					(*ChannelAllocateIdent)(struct HciConnection* conn);

	struct L2capFrame*		(*SignalByIdent)(struct HciConnection* conn,
								uint8 ident);
	status_t				(*TimeoutSignal)(struct L2capFrame* frame,
								uint32 timeo);
	status_t				(*UnTimeoutSignal)(struct L2capFrame* frame);
	struct L2capFrame*		(*SpawnFrame)(struct HciConnection* conn,
								struct L2capChannel* channel,
								net_buffer* buffer, frame_type frame);
	struct L2capFrame*		(*SpawnSignal)(struct HciConnection* conn,
								struct L2capChannel* channel,
								net_buffer* buffer, uint8 ident, uint8 code);
	status_t				(*AcknowledgeSignal)(struct L2capFrame* frame);
	status_t				(*QueueSignal)(struct L2capFrame* frame);

};


inline bool ExistConnectionByDestination(const bdaddr_t& destination,
				hci_id hid);
inline bool ExistConnectionByHandle(uint16 handle, hci_id hid);


#endif // _BTCOREDATA_H
