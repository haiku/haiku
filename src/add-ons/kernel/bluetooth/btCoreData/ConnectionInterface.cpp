/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <util/DoublyLinkedList.h>

#include <KernelExport.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/bluetooth_util.h>

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "Connection"
#define SUBMODULE_COLOR 31
#include <btDebug.h>

#include <l2cap.h>

#include "ConnectionInterface.h"

void PurgeChannels(HciConnection* conn);

HciConnection*
AddConnection(uint16 handle, int type, bdaddr_t* dst, hci_id hid)
{
	// Create connection descriptor
	
	HciConnection* conn = ConnectionByHandle(handle, hid);
	if (conn != NULL)	
		goto update;
		
	conn = new (std::nothrow) HciConnection;
	if (conn == NULL)
		goto bail;
		
	memset(conn, 0, sizeof(HciConnection));

	conn->currentRxPacket = NULL;
	conn->currentRxExpectedLength = 0;
update:
	// fill values
	bacpy(&conn->destination, dst);
	conn->type   	= type;
	conn->handle   = handle;
	conn->Hid   	= hid;
	conn->status  	= HCI_CONN_OPEN;
	conn->mtu	 	= L2CAP_MTU_MINIMUM; // TODO: give the mtu to the connection
	conn->lastCid   = L2CAP_FIRST_CID;
	conn->lastIdent = L2CAP_FIRST_IDENT;

	sConnectionList.Add(conn);

bail:
	return conn;
}


status_t
RemoveConnection(bdaddr_t* destination, hci_id hid)
{
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator = sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid && bacmp(&conn->destination, destination)==0) {

			// if the device is still part of the list, remove it
			if (conn->GetDoublyLinkedListLink()->next != NULL
				|| conn->GetDoublyLinkedListLink()->previous != NULL
				|| conn == sConnectionList.Head()) {
				sConnectionList.Remove(conn);

				delete conn;
				return B_OK;
			}
		}
	}
	return B_ERROR;
}


status_t
RemoveConnection(uint16 handle, hci_id hid)
{
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator = sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid && conn->handle == handle) {

			// if the device is still part of the list, remove it
			if (conn->GetDoublyLinkedListLink()->next != NULL
				|| conn->GetDoublyLinkedListLink()->previous != NULL
				|| conn == sConnectionList.Head()) {
				sConnectionList.Remove(conn);

				PurgeChannels(conn);
				delete conn;
				return B_OK;
			}
		}
	}
	return B_ERROR;
}


hci_id
RouteConnection(bdaddr_t* destination) {

	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator = sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (bacmp(&conn->destination, destination)==0) {
			return conn->Hid;
		}
	}

	return -1;
}


HciConnection*
ConnectionByHandle(uint16 handle, hci_id hid)
{
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator = sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid && conn->handle==handle) {
			return conn;
		}
	}

	return NULL;
}


HciConnection*
ConnectionByDestination(bdaddr_t* destination, hci_id hid)
{
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator = sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid && bacmp(&conn->destination, destination)==0) {
			return conn;
		}
	}

	return NULL;
}

#if 0
#pragma mark - ACL helper funcs
#endif

void
SetAclBuffer(HciConnection* conn, net_buffer* nbuf)
{
	conn->currentRxPacket = nbuf;
}


void
SetAclExpectedSize(HciConnection* conn, size_t size)
{
	conn->currentRxExpectedLength = size;
}


void
AclPutting(HciConnection* conn, size_t size)
{
	conn->currentRxExpectedLength -= size;
}


bool
AclComplete(HciConnection* conn)
{
	return conn->currentRxExpectedLength == 0;
}


bool
AclOverFlowed(HciConnection* conn)
{
	return conn->currentRxExpectedLength < 0;
}

#if 0
#pragma mark - private funcs
#endif

void
PurgeChannels(HciConnection* conn)
{

}
