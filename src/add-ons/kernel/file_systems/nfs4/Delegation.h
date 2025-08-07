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
							uint64 clientID, bool attr = false);

	status_t			GiveUp(bool truncate = false);
			status_t	PrepareGiveUp(bool truncate);
			status_t	DoGiveUp(bool truncate, bool wait = true);

	inline	void		SetData(const OpenDelegationData& data);
	inline	Inode*		GetInode() const;
			void		GetStateIDandSeq(uint32* stateID, uint32& stateSeq) const;
	inline	OpenDelegation Type();
	inline	void		MarkRecalled();
	inline	bool		RecallInitiated() const;

	void				Dump(void (*xprintf)(const char*, ...) = dprintf) const;

protected:
	status_t			ReturnDelegation();

private:
	uint64				fClientID;
	OpenDelegationData	fData;
	Inode*				fInode;
	bool				fAttribute;
	uint32				fStateID[3];
	uint32				fStateSeq;
	uid_t				fUid;
	gid_t				fGid;
	bool				fRecalled;
};


inline void
Delegation::SetData(const OpenDelegationData& data)
{
	fData = data;
}


inline Inode*
Delegation::GetInode() const
{
	return fInode;
}


inline OpenDelegation
Delegation::Type()
{
	return fData.fType;
}


inline void
Delegation::MarkRecalled()
{
	fRecalled = true;
}


inline bool
Delegation::RecallInitiated() const
{
	return fRecalled;
}


#endif	// DELEGATION_H

