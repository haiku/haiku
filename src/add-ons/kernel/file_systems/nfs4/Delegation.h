/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef DELEGATION_H
#define DELEGATION_H


#include <lock.h>
#include <SupportDefs.h>

#include "NFS4Object.h"


class Delegation : public NFS4Object {
public:
						Delegation(OpenDelegationData data, Inode* inode,
							uint64 clientID);
						~Delegation();

	status_t			Write(void* buffer, uint32* size);
	status_t			Read(void* buffer, uint32* size);

	// TODO: locks

	status_t			Reclaim(uint64 newClientID);

	status_t			GiveUp(bool flush);

private:
	uint64				fClientID;
	OpenDelegationData	fData;
	Inode*				fInode;

	rw_lock				fLock;
};


#endif	// DELEGATION_H

