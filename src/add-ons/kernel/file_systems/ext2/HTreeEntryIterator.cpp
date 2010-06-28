/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "HTreeEntryIterator.h"

#include <new>

#include "HTree.h"
#include "IndexedDirectoryIterator.h"
#include "Inode.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)


HTreeEntryIterator::HTreeEntryIterator(off_t offset, Inode* directory)
	:
	fHasCollision(false),
	fDirectory(directory),
	fOffset(offset),
	fParent(NULL),
	fChild(NULL)
{
	fBlockSize = fDirectory->GetVolume()->BlockSize();
}


HTreeEntryIterator::HTreeEntryIterator(uint32 block, uint32 blockSize,
	Inode* directory, HTreeEntryIterator* parent, bool hasCollision)
	:
	fHasCollision(hasCollision),
	fBlockSize(blockSize),
	fDirectory(directory),
	fOffset(block * blockSize + sizeof(HTreeFakeDirEntry)),
	fParent(parent),
	fChild(NULL)
{
	TRACE("HTreeEntryIterator::HTreeEntryIterator() block %ld offset %Lx\n",
		block, fOffset);
}


status_t
HTreeEntryIterator::Init()
{
	size_t length = sizeof(HTreeCountLimit);
	HTreeCountLimit countLimit;
	
	status_t status = fDirectory->ReadAt(fOffset, (uint8*)&countLimit,
		&length);
	
	if (status != B_OK)
		return status;
	
	if (length != sizeof(HTreeCountLimit)) {
		ERROR("HTreeEntryIterator::Init() bad length %ld fOffset 0x%Lx\n",
			length, fOffset);
		fCount = fLimit = 0;
		return B_ERROR;
	}
	
	fCount = countLimit.Count();
	fLimit = countLimit.Limit();

	if (fCount >= fLimit) {
		ERROR("HTreeEntryIterator::Init() bad fCount %d fOffset 0x%Lx\n",
			fCount, fOffset);
		fCount = fLimit = 0;
		return B_ERROR;
	}

	if (fParent != NULL && 
		fLimit != ((fBlockSize - sizeof(HTreeFakeDirEntry))
			/ sizeof(HTreeEntry)) ) {
		ERROR("HTreeEntryIterator::Init() bad fLimit %d should be %ld "
			"fOffset 0x%Lx\n", fLimit, (fBlockSize - sizeof(HTreeFakeDirEntry))
				/ sizeof(HTreeEntry), fOffset);
		fCount = fLimit = 0;
		return B_ERROR;
	} 

	TRACE("HTreeEntryIterator::Init() count 0x%x limit 0x%x\n", fCount,
		fLimit);

	fMaxOffset = fOffset + (fCount - 1) * sizeof(HTreeEntry);
	
	return B_OK;
}


HTreeEntryIterator::~HTreeEntryIterator()
{
}


status_t
HTreeEntryIterator::Lookup(uint32 hash, int indirections,
	DirectoryIterator** directoryIterator)
{
	off_t start = fOffset + sizeof(HTreeEntry);
	off_t end = fMaxOffset;
	off_t middle = start;
	size_t entrySize = sizeof(HTreeEntry);
	HTreeEntry entry;
	
	while (start <= end) {
		middle = (end - start) / 2;
		middle -= middle % entrySize; // Alignment
		middle += start;

		TRACE("HTreeEntryIterator::Lookup() %d 0x%Lx 0x%Lx 0x%Lx\n",
			indirections, start, end, middle);
		
		status_t status = fDirectory->ReadAt(middle, (uint8*)&entry,
			&entrySize);

		TRACE("HTreeEntryIterator::Lookup() %lx %lx\n", hash, entry.Hash());
		
		if (status != B_OK)
			return status;
		else if (entrySize != sizeof(entry)) {
			// Fallback to linear search
			*directoryIterator = new(std::nothrow)
				DirectoryIterator(fDirectory);
			
			if (*directoryIterator == NULL)
				return B_NO_MEMORY;
			
			return B_OK;
		}
		
		if (hash >= entry.Hash())
			start = middle + entrySize;
		else
			end = middle - entrySize;
	}

	status_t status = fDirectory->ReadAt(start - entrySize, (uint8*)&entry,
		&entrySize);
	if (status != B_OK)
		return status;
	
	if (indirections == 0) {
		*directoryIterator = new(std::nothrow)
			IndexedDirectoryIterator(entry.Block() * fBlockSize, fBlockSize,
			fDirectory, this);
		
		if (*directoryIterator == NULL)
			return B_NO_MEMORY;

		return B_OK;
	}

	delete fChild;

	fChild = new(std::nothrow) HTreeEntryIterator(entry.Block(), fBlockSize,
		fDirectory, this, entry.Hash() & 1 == 1);
	
	if (fChild == NULL)
		return B_NO_MEMORY;
	fChildDeleter.SetTo(fChild);
	
	status = fChild->Init();
	if (status != B_OK)
		return status;
	
	return fChild->Lookup(hash, indirections - 1, directoryIterator);
}


status_t
HTreeEntryIterator::GetNext(off_t& childOffset)
{
	size_t entrySize = sizeof(HTreeEntry);
	fOffset += entrySize;
	bool firstEntry = fOffset >= fMaxOffset;
	
	if (firstEntry) {
		if (fParent == NULL)
			return B_ENTRY_NOT_FOUND;
		
		status_t status = fParent->GetNext(fOffset);
		if (status != B_OK)
			return status;
		
		status = Init();
		if (status != B_OK)
			return status;

		fHasCollision = fParent->HasCollision();
	}
	
	HTreeEntry entry;
	status_t status = fDirectory->ReadAt(fOffset, (uint8*)&entry, &entrySize);
	if (status != B_OK)
		return status;
	else if (entrySize != sizeof(entry)) {
		// Weird error, try to skip it
		return GetNext(childOffset);
	}
	
	if (!firstEntry)
		fHasCollision = (entry.Hash() & 1) == 1;
	
	childOffset = entry.Block() * fBlockSize;
	
	return B_OK;
}
