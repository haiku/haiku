/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Delegation.h"

#include "Inode.h"
#include "Request.h"


Delegation::Delegation(const OpenDelegationData& data, Inode* inode,
	uint64 clientID)
	:
	fClientID(clientID),
	fData(data),
	fInode(inode)
{
}


status_t
Delegation::GiveUp(bool truncate)
{
	if (!truncate)
		fInode->SyncAndCommit(true);

	ReturnDelegation();

	return B_OK;
}


status_t
Delegation::ReturnDelegation()
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.DelegReturn(fData.fStateID, fData.fStateSeq);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		return reply.DelegReturn();
	} while (true);
}

