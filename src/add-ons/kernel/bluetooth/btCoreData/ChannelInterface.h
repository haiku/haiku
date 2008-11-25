/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _CHANNEL_INTERFACE_H
#define _CHANNEL_INTERFACE_H

#include <util/DoublyLinkedList.h>

#include <l2cap.h>
#include <btCoreData.h>

L2capChannel* AddChannel(HciConnection* conn, uint16 psm);
void RemoveChannel(HciConnection* conn, uint16 scid);

L2capChannel* ChannelBySourceID(HciConnection* conn, uint16 scid);
uint16	ChannelAllocateCid(HciConnection* conn);
uint16	ChannelAllocateIdent(HciConnection* conn);


#endif // _CHANNELINTERFACE_H
