/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "NFS4Server.h"
#include "Request.h"


NFS4Server::NFS4Server(RPC::Server* serv)
	:
	fCIDUseCount(0),
	fSequenceId(0),
	fServer(serv)
{
	mutex_init(&fLock, NULL);
}


NFS4Server::~NFS4Server()
{
	mutex_destroy(&fLock);
}


uint64
NFS4Server::ClientId(uint64 prevId, bool forceNew)
{
	mutex_lock(&fLock);
	if ((forceNew && fClientId == prevId) || fCIDUseCount == 0) {
		Request request(fServer);
		request.Builder().SetClientID(fServer);

		status_t result = request.Send();
		if (result != B_OK)
			goto out_unlock;

		uint64 ver;
		result = request.Reply().SetClientID(&fClientId, &ver);
		if (result != B_OK)
			goto out_unlock;

		request.Reset();
		request.Builder().SetClientIDConfirm(fClientId, ver);

		result = request.Send();
		if (result != B_OK)
			goto out_unlock;

		result = request.Reply().SetClientIDConfirm();
		if (result != B_OK)
			goto out_unlock;
	}

	fCIDUseCount++;

out_unlock:
	mutex_unlock(&fLock);
	return fClientId;
}


void
NFS4Server::ReleaseCID(uint64 cid)
{
	mutex_lock(&fLock);
	fCIDUseCount--;
	mutex_unlock(&fLock);
}

