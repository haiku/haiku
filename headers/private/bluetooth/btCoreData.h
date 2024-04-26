/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTCOREDATA_H
#define _BTCOREDATA_H


#include <module.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/VectorMap.h>
#include <net_datalink.h>
#include <net/if_dl.h>


#include <net_buffer.h>
#include <net_device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <l2cap.h>


#define BT_CORE_DATA_MODULE_NAME "bluetooth/btCoreData/v1"


typedef enum _connection_status {
	HCI_CONN_CLOSED,
	HCI_CONN_OPEN,
} connection_status;


#ifdef __cplusplus

struct HciConnection : DoublyLinkedListLinkImpl<HciConnection> {
	HciConnection(hci_id hid);
	virtual ~HciConnection();

	hci_id				Hid;
	bluetooth_device*	ndevice;
	bdaddr_t			destination;
	uint16				handle;
	int					type;
	uint16				mtu;
	connection_status	status;

	net_buffer*			currentRxPacket;
	ssize_t				currentRxExpectedLength;

	struct net_interface_address interface_address;
	struct sockaddr_dl address_dl;
	struct sockaddr_storage address_dest;

	void (*disconnect_hook)(HciConnection*);

public:
	mutex			fLock;
	uint8			fNextIdent;
	VectorMap<uint8, void*> fInUseIdents;
};

#else

struct HciConnection;

#endif


struct bluetooth_core_data_module_info {
	module_info info;

	status_t				(*PostEvent)(bluetooth_device* ndev, void* event,
								size_t size);

	// FIXME: We really shouldn't be passing out connection pointers at all...
	struct HciConnection*	(*AddConnection)(uint16 handle, int type,
								const bdaddr_t& dst, hci_id hid);

	// status_t				(*RemoveConnection)(bdaddr_t destination, hci_id hid);
	status_t				(*RemoveConnection)(uint16 handle, hci_id hid);

	hci_id					(*RouteConnection)(const bdaddr_t& destination);

	struct HciConnection*	(*ConnectionByHandle)(uint16 handle, hci_id hid);
	struct HciConnection*	(*ConnectionByDestination)(
								const bdaddr_t& destination, hci_id hid);

	uint8					(*allocate_command_ident)(struct HciConnection* conn, void* associated);
	void*					(*lookup_command_ident)(struct HciConnection* conn, uint8 ident);
	void					(*free_command_ident)(struct HciConnection* conn, uint8 ident);
};


inline bool ExistConnectionByDestination(const bdaddr_t& destination,
				hci_id hid);
inline bool ExistConnectionByHandle(uint16 handle, hci_id hid);


#endif // _BTCOREDATA_H
