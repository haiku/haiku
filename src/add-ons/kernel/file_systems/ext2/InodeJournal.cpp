/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "InodeJournal.h"

#include <new>

#include <fs_cache.h>

#include "HashRevokeManager.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


InodeJournal::InodeJournal(Inode* inode)
	:
	Journal(),
	fInode(inode)
{
	if (inode == NULL)
		fInitStatus = B_BAD_DATA;
	else {
		Volume* volume = inode->GetVolume();

		fFilesystemVolume = volume;
		fFilesystemBlockCache = volume->BlockCache();
		fJournalVolume = volume;
		fJournalBlockCache = volume->BlockCache();

		if (inode->HasFileCache())
			inode->DeleteFileCache();

		fInitStatus = B_OK;

		TRACE("InodeJournal::InodeJournal(): Inode's file cache disabled "
			"successfully\n");
		HashRevokeManager* revokeManager = new(std::nothrow)
			HashRevokeManager(volume->Has64bitFeature());
		TRACE("InodeJournal::InodeJournal(): Allocated a hash revoke "
			"manager at %p\n", revokeManager);

		if (revokeManager == NULL) {
			TRACE("InodeJournal::InodeJournal(): Insufficient memory to "
				"create the hash revoke manager\n");
			fInitStatus = B_NO_MEMORY;
		} else {
			fInitStatus = revokeManager->Init();

			if (fInitStatus == B_OK) {
				fRevokeManager = revokeManager;
				fInitStatus = _LoadSuperBlock();
			} else
				delete revokeManager;
		}
	}
}


InodeJournal::~InodeJournal()
{
}


status_t
InodeJournal::InitCheck()
{
	if (fInitStatus != B_OK)
		TRACE("InodeJournal: Initialization error\n");
	return fInitStatus;
}


status_t
InodeJournal::MapBlock(off_t logical, fsblock_t& physical)
{
	TRACE("InodeJournal::MapBlock()\n");
	return fInode->FindBlock(logical * fBlockSize, physical);
}
