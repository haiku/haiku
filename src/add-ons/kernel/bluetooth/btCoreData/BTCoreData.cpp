/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ConnectionInterface.h"


#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <bluetooth/HCI/btHCI_event.h>

#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>


int32 api_version = B_CUR_DRIVER_API_VERSION;


mutex sConnectionListLock = MUTEX_INITIALIZER("bt connection list");
DoublyLinkedList<HciConnection> sConnectionList;
net_buffer_module_info* gBufferModule = NULL;


inline bool
ExistConnectionByDestination(const bdaddr_t& destination, hci_id hid = -1)
{
	return ConnectionByDestination(destination, hid) != NULL;
}


inline bool
ExistConnectionByHandle(uint16 handle, hci_id hid)
{
	return ConnectionByHandle(handle, hid);
}


status_t
PostEvent(bluetooth_device* ndev, void* event, size_t size)
{
	struct hci_event_header* outgoingEvent = (struct hci_event_header*) event;
	status_t err;

	// Take actions on certain type of events.
	switch (outgoingEvent->ecode) {
		case HCI_EVENT_CONN_COMPLETE:
		{
			struct hci_ev_conn_complete* data
				= (struct hci_ev_conn_complete*)(outgoingEvent + 1);

			// TODO: XXX parse handle field
			HciConnection* conn = AddConnection(data->handle, BT_ACL,
				data->bdaddr, ndev->index);

			if (conn == NULL)
				panic("no mem for conn desc");
			conn->ndevice = ndev;
			TRACE("%s: Registered connection handle=%#x\n", __func__,
				data->handle);
			break;
		}

		case HCI_EVENT_DISCONNECTION_COMPLETE:
		{
			struct hci_ev_disconnection_complete_reply* data;

			data = (struct hci_ev_disconnection_complete_reply*)
				(outgoingEvent + 1);

			RemoveConnection(data->handle, ndev->index);
			TRACE("%s: unRegistered connection handle=%#x\n", __func__,
				data->handle);
			break;
		}

	}

	// forward to bluetooth server
	port_id port = find_port(BT_USERLAND_PORT_NAME);
	if (port != B_NAME_NOT_FOUND) {

		err = write_port_etc(port, PACK_PORTCODE(BT_EVENT, ndev->index, -1),
			event, size, B_TIMEOUT, 1 * 1000 * 1000);

		if (err != B_OK)
			ERROR("%s: Error posting userland %s\n", __func__, strerror(err));

	} else {
		ERROR("%s: bluetooth_server not found for posting!\n", __func__);
		err = B_NAME_NOT_FOUND;
	}

	return err;
}


static status_t
bcd_std_ops(int32 op, ...)
{
	status_t status;

	switch (op) {
		case B_MODULE_INIT:
			new (&sConnectionList) DoublyLinkedList<HciConnection>;

			status = get_module(NET_BUFFER_MODULE_NAME,
				(module_info **)&gBufferModule);
			if (status != B_OK)
				return status;

			return B_OK;

		case B_MODULE_UNINIT:
			put_module(NET_BUFFER_MODULE_NAME);
			return B_OK;
	}

	return B_ERROR;
}


bluetooth_core_data_module_info sBCDModule = {
	{
		BT_CORE_DATA_MODULE_NAME,
		0,
		bcd_std_ops
	},
	PostEvent,
	AddConnection,
	// RemoveConnection,
	RemoveConnection,

	RouteConnection,

	ConnectionByHandle,
	ConnectionByDestination,

	allocate_command_ident,
	lookup_command_ident,
	free_command_ident,
};


module_info* modules[] = {
	(module_info*)&sBCDModule,
	NULL
};
