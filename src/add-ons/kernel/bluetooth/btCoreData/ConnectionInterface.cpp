/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>

#include <net_protocol.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/L2CAP/btL2CAP.h>

#include <btDebug.h>
#include <btModules.h>

#include <l2cap.h>

#include "ConnectionInterface.h"

struct net_protocol_module_info* L2cap = NULL;
extern net_buffer_module_info* gBufferModule;


HciConnection::HciConnection(hci_id hid)
{
	mutex_init(&fLock, "HciConnection");
	Hid = hid;
	fNextIdent = L2CAP_FIRST_CID;

	// TODO: This doesn't really belong here...
	interface_address = {};
	address_dl = {};
	interface_address.local = (struct sockaddr*)&address_dl;
	interface_address.destination = (struct sockaddr*)&address_dest;
	address_dl.sdl_index = Hid;
}


HciConnection::~HciConnection()
{
	if (L2cap == NULL)
	if (get_module(NET_BLUETOOTH_L2CAP_NAME, (module_info**)&L2cap) != B_OK) {
		ERROR("%s: cannot get module \"%s\"\n", __func__,
			NET_BLUETOOTH_L2CAP_NAME);
	} // TODO: someone put it

	// Inform the L2CAP module this connection is about to be gone.
	if (L2cap != NULL) {
		net_buffer* error = gBufferModule->create(128);
		error->interface_address = &interface_address;
		if (L2cap->error_received(B_NET_ERROR_UNREACH_HOST, error) != B_OK) {
			error->interface_address = NULL;
			gBufferModule->free(error);
		}
	}

	mutex_destroy(&fLock);
}


HciConnection*
AddConnection(uint16 handle, int type, const bdaddr_t& dst, hci_id hid)
{
	// Create connection descriptor

	HciConnection* conn = ConnectionByHandle(handle, hid);
	if (conn != NULL)
		goto update;

	conn = new (std::nothrow) HciConnection(hid);
	if (conn == NULL)
		goto bail;

	// memset(conn, 0, sizeof(HciConnection));

	conn->currentRxPacket = NULL;
	conn->currentRxExpectedLength = 0;
update:
	// fill values
	bdaddrUtils::Copy(conn->destination, dst);
	{
	sockaddr_l2cap* destination = (sockaddr_l2cap*)&conn->address_dest;
	destination->l2cap_len = sizeof(sockaddr_l2cap);
	destination->l2cap_family = AF_BLUETOOTH;
	destination->l2cap_bdaddr = dst;
	}
	conn->type = type;
	conn->handle = handle;
	conn->status = HCI_CONN_OPEN;
	conn->mtu = L2CAP_MTU_MINIMUM; // TODO: give the mtu to the connection

	{
	MutexLocker _(&sConnectionListLock);
	sConnectionList.Add(conn);
	}

bail:
	return conn;
}


status_t
RemoveConnection(const bdaddr_t& destination, hci_id hid)
{
	MutexLocker locker(&sConnectionListLock);
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator
		= sConnectionList.GetIterator();

	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid
			&& bdaddrUtils::Compare(conn->destination, destination)) {

			// if the device is still part of the list, remove it
			if (conn->GetDoublyLinkedListLink()->next != NULL
				|| conn->GetDoublyLinkedListLink()->previous != NULL
				|| conn == sConnectionList.Head()) {
				sConnectionList.Remove(conn);

				locker.Unlock();
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
	MutexLocker locker(&sConnectionListLock);
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator
		= sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid && conn->handle == handle) {

			// if the device is still part of the list, remove it
			if (conn->GetDoublyLinkedListLink()->next != NULL
				|| conn->GetDoublyLinkedListLink()->previous != NULL
				|| conn == sConnectionList.Head()) {
				sConnectionList.Remove(conn);

				locker.Unlock();
				delete conn;
				return B_OK;
			}
		}
	}
	return B_ERROR;
}


hci_id
RouteConnection(const bdaddr_t& destination)
{
	MutexLocker _(&sConnectionListLock);
	HciConnection* conn;

	DoublyLinkedList<HciConnection>::Iterator iterator
		= sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (bdaddrUtils::Compare(conn->destination, destination)) {
			return conn->Hid;
		}
	}

	return -1;
}


HciConnection*
ConnectionByHandle(uint16 handle, hci_id hid)
{
	MutexLocker _(&sConnectionListLock);
	HciConnection*	conn;

	DoublyLinkedList<HciConnection>::Iterator iterator
		= sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		conn = iterator.Next();
		if (conn->Hid == hid && conn->handle == handle) {
			return conn;
		}
	}

	return NULL;
}


HciConnection*
ConnectionByDestination(const bdaddr_t& destination, hci_id hid)
{
	MutexLocker _(&sConnectionListLock);

	DoublyLinkedList<HciConnection>::Iterator iterator
		= sConnectionList.GetIterator();
	while (iterator.HasNext()) {

		HciConnection* conn = iterator.Next();
		if (conn->Hid == hid
			&& bdaddrUtils::Compare(conn->destination, destination)) {
			return conn;
		}
	}

	return NULL;
}


uint8
allocate_command_ident(HciConnection* conn, void* pointer)
{
	MutexLocker _(&conn->fLock);

	uint8 ident = conn->fNextIdent + 1;

	if (ident < L2CAP_FIRST_IDENT)
		ident = L2CAP_FIRST_IDENT;

	while (ident != conn->fNextIdent) {
		if (conn->fInUseIdents.Find(ident) == conn->fInUseIdents.End()) {
			conn->fInUseIdents.Insert(ident, pointer);
			return ident;
		}

		ident++;
		if (ident < L2CAP_FIRST_IDENT)
			ident = L2CAP_FIRST_IDENT;
	}

	return L2CAP_NULL_IDENT;
}


void*
lookup_command_ident(HciConnection* conn, uint8 ident)
{
	MutexLocker _(&conn->fLock);

	auto iter = conn->fInUseIdents.Find(ident);
	if (iter == conn->fInUseIdents.End())
		return NULL;

	return iter->Value();
}


void
free_command_ident(HciConnection* conn, uint8 ident)
{
	MutexLocker _(&conn->fLock);
	conn->fInUseIdents.Remove(ident);
}
