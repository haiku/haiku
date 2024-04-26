/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "L2capEndpointManager.h"

#include <AutoDeleter.h>

#include <bluetooth/bdaddrUtils.h>


L2capEndpointManager gL2capEndpointManager;


L2capEndpointManager::L2capEndpointManager()
	:
	fNextChannelID(L2CAP_FIRST_CID)
{
	rw_lock_init(&fBoundEndpointsLock, "l2cap bound endpoints");
	rw_lock_init(&fChannelEndpointsLock, "l2cap channel endpoints");
}


L2capEndpointManager::~L2capEndpointManager()
{
	rw_lock_destroy(&fBoundEndpointsLock);
	rw_lock_destroy(&fChannelEndpointsLock);
}


status_t
L2capEndpointManager::Bind(L2capEndpoint* endpoint, const sockaddr_l2cap& address)
{
	// TODO: Support binding to specific addresses?
	if (!Bluetooth::bdaddrUtils::Compare(address.l2cap_bdaddr, BDADDR_ANY))
		return EINVAL;

	// PSM values must be odd.
	if ((address.l2cap_psm & 1) == 0)
		return EINVAL;

	WriteLocker _(fBoundEndpointsLock);

	if (fBoundEndpoints.Find(address.l2cap_psm) != NULL)
		return EADDRINUSE;

	memcpy(*endpoint->LocalAddress(), &address, sizeof(struct sockaddr_l2cap));
	fBoundEndpoints.Insert(endpoint);
	gSocketModule->acquire_socket(endpoint->socket);

	return B_OK;
}


status_t
L2capEndpointManager::Unbind(L2capEndpoint* endpoint)
{
	WriteLocker _(fBoundEndpointsLock);

	fBoundEndpoints.Remove(endpoint);
	(*endpoint->LocalAddress())->sa_len = 0;
	gSocketModule->release_socket(endpoint->socket);

	return B_OK;
}


L2capEndpoint*
L2capEndpointManager::ForPSM(uint16 psm)
{
	ReadLocker _(fBoundEndpointsLock);
	// TODO: Acquire reference?
	return fBoundEndpoints.Find(psm);
}


status_t
L2capEndpointManager::BindToChannel(L2capEndpoint* endpoint)
{
	WriteLocker _(fChannelEndpointsLock);

	for (uint16 i = 0; i < (L2CAP_LAST_CID - L2CAP_FIRST_CID); i++) {
		const uint16 cid = fNextChannelID;
		fNextChannelID++;
		if (fNextChannelID < L2CAP_FIRST_CID)
			fNextChannelID = L2CAP_FIRST_CID;

		if (fChannelEndpoints.Find(cid) != NULL)
			continue;

		endpoint->fChannelID = cid;
		fChannelEndpoints.Insert(endpoint);
		gSocketModule->acquire_socket(endpoint->socket);
		return B_OK;
	}

	return EADDRINUSE;
}


status_t
L2capEndpointManager::UnbindFromChannel(L2capEndpoint* endpoint)
{
	WriteLocker _(fChannelEndpointsLock);

	fChannelEndpoints.Remove(endpoint);
	endpoint->fChannelID = 0;
	gSocketModule->release_socket(endpoint->socket);

	return B_OK;
}


L2capEndpoint*
L2capEndpointManager::ForChannel(uint16 cid)
{
	ReadLocker _(fChannelEndpointsLock);
	// TODO: Acquire reference?
	return fChannelEndpoints.Find(cid);
}


void
L2capEndpointManager::Disconnected(HciConnection* connection)
{
	ReadLocker _(fChannelEndpointsLock);
	auto iter = fChannelEndpoints.GetIterator();
	while (iter.HasNext()) {
		L2capEndpoint* endpoint = iter.Next();
		if (endpoint->fConnection != connection)
			continue;

		endpoint->fConnection = NULL;
		endpoint->fState = L2capEndpoint::CLOSED;
	}
}

