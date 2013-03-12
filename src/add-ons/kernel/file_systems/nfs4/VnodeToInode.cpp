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
	if (fInode != NULL && !IsRoot()) {
		fInode->GetFileSystem()->InoIdMap()->MarkRemoved(fID);
		delete fInode;
	}

	fInode = newInode;
	if (fInode != NULL) {
		ASSERT(fFileSystem == fInode->GetFileSystem());
		fInode->GetFileSystem()->InoIdMap()->AddEntry(fInode->fInfo, fID);
	}
}

