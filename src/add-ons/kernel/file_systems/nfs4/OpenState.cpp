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


OpenState::OpenState()
	:
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
	OpenDelegationData delegation;

	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		RPC::Server* server = fFileSystem->Server();
		Request request(server);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Open(CLAIM_PREVIOUS, sequence, sModeToAccess(fMode), newClientID,
			OPEN4_NOCREATE, fFileSystem->OpenOwner(), NULL);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(false);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), server))
			continue;

 		fFileSystem->OpenOwnerSequenceUnlock();

		reply.PutFH();

		result = reply.Open(fStateID, &fStateSeq, &confirm, &delegation);
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

	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Close(sequence, fStateID, fStateSeq);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(false);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv, NULL, this))
			continue;
 		fFileSystem->OpenOwnerSequenceUnlock();

		reply.PutFH();

		return reply.Close();
	} while (true);
}

