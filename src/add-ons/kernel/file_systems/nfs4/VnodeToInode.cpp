/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "VnodeToInode.h"


Inode*
VnodeToInode::Get()
{
	if (fInode == NULL) {
		status_t result = fFileSystem->GetInode(fID, &fInode);
		if (result != B_OK)
			fInode = NULL;
	}

	return fInode;
}


void
VnodeToInode::Replace(Inode* newInode)
{
	WriteLocker _(fLock);
	if (!IsRoot())
		delete fInode;

	fInode = newInode;
}


bool
VnodeToInode::Unlink(InodeNames* parent, const char* name)
{
	WriteLocker _(fLock);
	if (fInode != NULL && !IsRoot()) {
		return fInode->GetFileSystem()->InoIdMap()->RemoveName(fID, parent,
			name);
	}

	return false;
}


void
VnodeToInode::Dump(void (*xprintf)(const char*, ...))
{
	xprintf("VTI\t%" B_PRIdINO " at %p\n", fID);

	ReadLocker locker;
	if (xprintf != kprintf)
		locker.SetTo(fLock, false);

	if (fInode != NULL)
		fInode->Dump(xprintf);
	else
		xprintf("NULL Inode");

	return;
}

