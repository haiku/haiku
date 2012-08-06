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


class Inode;

class Delegation : public NFS4Object,
	public DoublyLinkedListLinkImpl<Delegation> {
public:
						Delegation(const OpenDelegationData& data, Inode* inode,
							uint64 clientID);

	// TODO: locks

	status_t			GiveUp(bool truncate = false);

	inline	Inode*		GetInode();
	inline	OpenDelegation Type();

protected:
	status_t			ReturnDelegation();

private:
	uint64				fClientID;
	OpenDelegationData	fData;
	Inode*				fInode;
};


inline Inode*
Delegation::GetInode()
{
	return fInode;
}


inline OpenDelegation
Delegation::Type()
{
	return fData.fType;
}


#endif	// DELEGATION_H

