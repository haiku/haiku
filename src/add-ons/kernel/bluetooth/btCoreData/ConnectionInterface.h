/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _CONNECTION_INTERFACE_H
#define _CONNECTION_INTERFACE_H

#include <net_buffer.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>

#include <btCoreData.h>

extern mutex sConnectionListLock;
extern DoublyLinkedList<HciConnection> sConnectionList;

HciConnection* ConnectionByHandle(uint16 handle, hci_id hid);
HciConnection* ConnectionByDestination(const bdaddr_t& destination,
	hci_id hid);


HciConnection* AddConnection(uint16 handle, int type, const bdaddr_t& dst,
	hci_id hid);
status_t RemoveConnection(const bdaddr_t& destination, hci_id hid);
status_t RemoveConnection(uint16 handle, hci_id hid);

hci_id RouteConnection(const bdaddr_t& destination);

uint8 allocate_command_ident(HciConnection* conn, void* pointer);
void* lookup_command_ident(HciConnection* conn, uint8 ident);
void free_command_ident(HciConnection* conn, uint8 ident);

#endif // _CONNECTION_INTERFACE_H
