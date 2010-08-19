/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ConnectionInterface.h"
#include "ChannelInterface.h"
#include "FrameInterface.h"


#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <bluetooth/HCI/btHCI_event.h>

#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>

DoublyLinkedList<HciConnection> sConnectionList;
net_buffer_module_info* gBufferModule = NULL;


inline bool
ExistConnectionByDestination(const bdaddr_t& destination, hci_id hid = -1)
{
	return (ConnectionByDestination(destination, hid) != NULL);
}


inline bool
ExistConnectionByHandle(uint16 handle, hci_id hid)
{
	return (ConnectionByHandle(handle, hid));
}


static int
DumpHciConnections(int argc, char** argv)
{
	HciConnection* conn;
	L2capChannel* chan;
	L2capFrame* frame;
	DoublyLinkedList<HciConnection>::Iterator iterator
		= sConnectionList.GetIterator();

	while (iterator.HasNext()) {
		conn = iterator.Next();
		kprintf("LocalDevice=%lx Destination=%s handle=%#x type=%d"
			"outqueue=%ld expected=%ld\n", conn->Hid,
			bdaddrUtils::ToString(conn->destination), conn->handle,	conn->type,
			conn->OutGoingFrames.Count() , conn->ExpectedResponses.Count());

			// each channel
			kprintf("\tChannels\n");
			DoublyLinkedList<L2capChannel>::Iterator channelIterator
				= conn->ChannelList.GetIterator();

			while (channelIterator.HasNext()) {
				chan = channelIterator.Next();
				kprintf("\t\tscid=%x dcid=%x state=%x cfg=%x\n", chan->scid,
					chan->dcid, chan->state, chan->cfgState);
			}

			// Each outgoing
			kprintf("\n\tOutGoingFrames\n");
			DoublyLinkedList<L2capFrame>::Iterator frameIterator
				= conn->OutGoingFrames.GetIterator();
			while (frameIterator.HasNext()) {
				frame = frameIterator.Next();
				kprintf("\t\tscid=%x code=%x ident=%x type=%x, buffer=%p\n",
					frame->channel->scid, frame->code, frame->ident,
					frame->type, frame->buffer);
			}

			// Each expected
			kprintf("\n\tExpectedFrames\n");
			DoublyLinkedList<L2capFrame>::Iterator frameExpectedIterator
				= conn->ExpectedResponses.GetIterator();

			while (frameExpectedIterator.HasNext()) {
				frame = frameExpectedIterator.Next();
				kprintf("\t\tscid=%x code=%x ident=%x type=%x, buffer=%p\n",
					frame->channel->scid, frame->code, frame->ident,
					frame->type, frame->buffer);
			}
	}

	return 0;
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
			debugf("Registered connection handle=%#x\n",data->handle);
			break;
		}

		case HCI_EVENT_DISCONNECTION_COMPLETE:
		{
			struct hci_ev_disconnection_complete_reply* data;

			data = (struct hci_ev_disconnection_complete_reply*)
				(outgoingEvent + 1);

			RemoveConnection(data->handle, ndev->index);
			debugf("unRegistered connection handle=%#x\n",data->handle);
			break;
		}

	}

	// forward to bluetooth server
	port_id port = find_port(BT_USERLAND_PORT_NAME);
	if (port != B_NAME_NOT_FOUND) {

		err = write_port_etc(port, PACK_PORTCODE(BT_EVENT, ndev->index, -1),
			event, size, B_TIMEOUT, 1 * 1000 * 1000);

		if (err != B_OK)
			debugf("Error posting userland %s\n", strerror(err));

	} else {
		flowf("ERROR:bluetooth_server not found for posting\n");
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
			add_debugger_command("btConnections", &DumpHciConnections,
				"Lists Bluetooth Connections with RemoteDevices & channels");

			status = get_module(NET_BUFFER_MODULE_NAME,
				(module_info **)&gBufferModule);
			if (status < B_OK)
				return status;

			return B_OK;

		break;

		case B_MODULE_UNINIT:

			remove_debugger_command("btConnections", &DumpHciConnections);
			put_module(NET_BUFFER_MODULE_NAME);

			return B_OK;
		break;
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

	SetAclBuffer,
	SetAclExpectedSize,
	AclPutting,
	AclComplete,
	AclOverFlowed,

	ConnectionByHandle,
	ConnectionByDestination,

	AddChannel,
	RemoveChannel,
	ChannelBySourceID,
	ChannelAllocateCid,
	ChannelAllocateIdent,

	SignalByIdent,
	TimeoutSignal,
	unTimeoutSignal,
	SpawmFrame,
	SpawmSignal,
	AcknowledgeSignal,
	QueueSignal,

};


module_info* modules[] = {
	(module_info*)&sBCDModule,
	NULL
};
