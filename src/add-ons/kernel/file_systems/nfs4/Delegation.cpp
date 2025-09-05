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
	uint64 clientID, bool attribute)
	:
	fClientID(clientID),
	fData(data),
	fInode(inode),
	fAttribute(attribute),
	fUid(geteuid()),
	fGid(getegid())
{
	ASSERT(inode != NULL);
}


status_t
Delegation::GiveUp(bool truncate)
{
	if (!fAttribute && !truncate)
		fInode->SyncAndCommit(true);

	ReturnDelegation();

	return B_OK;
}


void
Delegation::Dump(void (*xprintf)(const char*, ...)) const
{
	xprintf("Delegation at %p for Inode at %p (ino %" B_PRIdINO ")\n", this, fInode, fInode->ID());
	if (fData.fType == OPEN_DELEGATE_READ)
		xprintf("\ttype OPEN_DELEGATE_READ, ");
	else if (fData.fType == OPEN_DELEGATE_WRITE)
		xprintf("\ttype OPEN_DELEGATE_WRITE, ");
	xprintf("attribute %d, uid %" B_PRIu32 ", gid %" B_PRIu32 "\n", fAttribute, fUid, fGid);
	xprintf("\tstate id and sequence %" B_PRIu32 " %" B_PRIu32 " %" B_PRIu32 " %" B_PRIu32 "\n",
		fData.fStateID[0], fData.fStateID[1], fData.fStateID[2], fData.fStateSeq);

	return;
}


status_t
Delegation::ReturnDelegation()
{
	uint32 attempt = 0;
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem, fUid, fGid);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.DelegReturn(fData.fStateID, fData.fStateSeq);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(attempt, reply.NFS4Error(), serv, NULL,
				fInode->GetOpenState())) {
			continue;
		}

		reply.PutFH();

		return reply.DelegReturn();
	} while (true);
}

