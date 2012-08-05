/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "OpenState.h"

#include <util/AutoLock.h>

#include "FileSystem.h"
#include "Request.h"


vint64 OpenState::fLastOwnerID = 0;

OpenState::OpenState()
	:
	fSequence(0),
	fOpened(false)
{
	mutex_init(&fLock, NULL);
}


OpenState::~OpenState()
{
	Close();
	mutex_destroy(&fLock);
}


status_t
OpenState::Reclaim(uint64 newClientID)
{
	if (!fOpened)
		return B_OK;

	MutexLocker _(fLock);

	if (fClientID == newClientID)
		return B_OK;
	fClientID = newClientID;

	bool confirm;
	do {
		RPC::Server* server = fFileSystem->Server();
		Request request(server);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Open(CLAIM_PREVIOUS, fSequence++, sModeToAccess(fMode), newClientID,
			OPEN4_NOCREATE, fOwnerID, NULL);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), server))
			continue;

		reply.PutFH();

		result = reply.Open(fStateID, &fStateSeq, &confirm);
		if (result != B_OK)
			return result;
	} while (true);

	if (confirm)
		return ConfirmOpen(fInfo.fHandle, this);

	return B_OK;
}


status_t
OpenState::Close()
{
	if (!fOpened)
		return B_OK;

	MutexLocker _(fLock);
	fOpened = false;

	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Close(fSequence++, fStateID, fStateSeq);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv, NULL, this))
			continue;

		reply.PutFH();
		return reply.Close();
	} while (true);
}

