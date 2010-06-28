/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "IndexedDirectoryIterator.h"

#include "ext2.h"
#include "HTree.h"
#include "HTreeEntryIterator.h"
#include "Inode.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


IndexedDirectoryIterator::IndexedDirectoryIterator(off_t start,
	uint32 blockSize, Inode* directory, HTreeEntryIterator* parent)
	:
	DirectoryIterator(directory),
	fIndexing(true),
	
	fParent(parent),
	fMaxOffset(start + blockSize),
	fBlockSize(blockSize),
	fMaxAttempts(0)
{
	fOffset = start;
}


IndexedDirectoryIterator::~IndexedDirectoryIterator()
{
}


status_t
IndexedDirectoryIterator::GetNext(char* name, size_t* nameLength, ino_t* id)
{
	if (fIndexing && fOffset + sizeof(HTreeFakeDirEntry) >= fMaxOffset) {
		TRACE("IndexedDirectoryIterator::GetNext() calling next block\n");
		status_t status = fParent->GetNext(fOffset);
		if (status != B_OK)
			return status;

		if (fMaxAttempts++ > 4)
			return B_ERROR;
		
		fMaxOffset = fOffset + fBlockSize;
	}
	
	return DirectoryIterator::GetNext(name, nameLength, id);
}


status_t
IndexedDirectoryIterator::Rewind()
{
	// The only way to rewind it is too loose indexing
	
	fOffset = 0;
	fMaxOffset = fInode->Size();
	fIndexing = false;

	return B_OK;
}
