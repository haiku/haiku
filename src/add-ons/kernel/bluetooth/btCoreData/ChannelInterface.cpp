/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <string.h>

#include <l2cap.h>
#include <bluetooth/HCI/btHCI_command.h>

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "Channel"
#define SUBMODULE_COLOR 31
#include <btDebug.h>

#include "ChannelInterface.h"
#include "FrameInterface.h"

L2capChannel*
ChannelBySourceID(HciConnection *conn, uint16 scid)
{
	L2capChannel*	channel = NULL;

	DoublyLinkedList<L2capChannel>::Iterator iterator = conn->ChannelList.GetIterator();

	while (iterator.HasNext()) {
		channel = iterator.Next();
		if (channel->scid == scid)
			return channel;
	}

	return NULL;
}


uint16
ChannelAllocateCid(HciConnection* conn)
{
	uint16 cid = conn->lastCid ;
	debugf("Starting search cid %d\n",cid);
	do {
		cid = (cid == L2CAP_LAST_CID)?L2CAP_FIRST_CID : cid + 1;

		if(ChannelBySourceID(conn, cid) == NULL) {
			conn->lastCid = cid;
			return cid;
		}

	} while (cid != conn->lastCid);

	return L2CAP_NULL_CID;
}


uint16
ChannelAllocateIdent(HciConnection* conn)
{
	uint8	ident = conn->lastIdent + 1;

	if (ident < L2CAP_FIRST_IDENT)
		ident = L2CAP_FIRST_IDENT;

	while (ident != conn->lastIdent) {
		if (SignalByIdent(conn, ident) == NULL) {
			conn->lastIdent = ident;

			return ident;
		}

		ident ++;
		if (ident < L2CAP_FIRST_IDENT)
			ident = L2CAP_FIRST_IDENT;
	}

	return L2CAP_NULL_IDENT;
}


L2capChannel*
AddChannel(HciConnection* conn, uint16 psm)
{
	L2capChannel* channel = new (std::nothrow) L2capChannel;

	if (channel == NULL) {
		flowf ("no mem for channel");
		return (NULL);
	}

	/* Get a new Source CID */
	channel->scid = ChannelAllocateCid(conn);

	if (channel->scid != L2CAP_NULL_CID) {
		/* Initialize channel */
		channel->psm = psm;
		channel->conn = conn;
		channel->state = L2CAP_CHAN_CLOSED;
		channel->cfgState = 0;
		channel->endpoint = NULL;

 		// the last assigned CID should be the last in the list
		// Think if keeping an ordered list will improve the search method
		// as it is called in every reception
		conn->ChannelList.Add(channel);
		//conn->num_channels++;

		// Any constance of the new channel created ...? ng_l2cap_con_ref(con);

	} else {
		flowf("no CID available\n");
		delete channel;
		channel = NULL;
	}

	return (channel);
}


void
RemoveChannel(HciConnection* conn, uint16 scid)
{
	//TODO make it safer
	conn->ChannelList.Remove(ChannelBySourceID(conn, scid));
}
