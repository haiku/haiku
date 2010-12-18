/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "HTreeEntryIterator.h"

#include <new>

#include "CachedBlock.h"
#include "HTree.h"
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
	fDirectory(directory),
	fVolume(directory->GetVolume()),
	fHasCollision(false),
	fBlockSize(directory->GetVolume()->BlockSize()),
	fParent(NULL),
	fChild(NULL)
{
	fInitStatus = fDirectory->FindBlock(offset, fBlockNum);
	
	if (fInitStatus == B_OK) {
		fFirstEntry = offset % fBlockSize / sizeof(HTreeEntry);
		fCurrentEntry = fFirstEntry;
	}

	TRACE("HTreeEntryIterator::HTreeEntryIterator(): created %p, block %llu, "
		"entry no. %lu, parent: %p\n", this, fBlockNum, (uint32)fCurrentEntry,
		fParent);
}


HTreeEntryIterator::HTreeEntryIterator(uint32 block, uint32 blockSize,
	Inode* directory, HTreeEntryIterator* parent, bool hasCollision)
	:
	fDirectory(directory),
	fVolume(directory->GetVolume()),
	fHasCollision(hasCollision),
	fFirstEntry(1),
	fCurrentEntry(1),
	fBlockSize(blockSize),
	fBlockNum(block),
	fParent(parent),
	fChild(NULL)
{
	// fCurrentEntry is initialized to 1 to skip the fake directory entry
	fInitStatus = B_OK;

	TRACE("HTreeEntryIterator::HTreeEntryIterator(): created %p, block %lu, "
		"parent: %p\n", this, block, fParent);
}


status_t
HTreeEntryIterator::Init()
{
	TRACE("HTreeEntryIterator::Init() first entry: %lu\n",
		(uint32)fFirstEntry);
	
	if (fInitStatus != B_OK)
		return fInitStatus;

	CachedBlock cached(fVolume);
	const uint8* block = cached.SetTo(fBlockNum);
	if (block == NULL) {
		ERROR("Failed to read htree entry block.\n");
		fCount = fLimit = 0;
		return B_IO_ERROR;
	}

	HTreeCountLimit* countLimit = (HTreeCountLimit*)(
		&((HTreeEntry*)block)[fFirstEntry]);

	fCount = countLimit->Count();
	fLimit = countLimit->Limit();

	if (fCount > fLimit) {
		ERROR("HTreeEntryIterator::Init() bad fCount %u (fLimit %u)\n",
			fCount, fLimit);
		fCount = fLimit = 0;
		return B_ERROR;
	}

	if (fLimit != fBlockSize / sizeof(HTreeEntry) - fFirstEntry) {
		ERROR("HTreeEntryIterator::Init() bad fLimit %lu should be %lu "
			"at block %llu\n", (uint32)fLimit, fBlockSize / sizeof(HTreeEntry)
				- fFirstEntry, fBlockNum);
		fCount = fLimit = 0;
		return B_ERROR;
	}

	TRACE("HTreeEntryIterator::Init() count %lu limit %lu\n",
		(uint32)fCount, (uint32)fLimit);

	return B_OK;
}


HTreeEntryIterator::~HTreeEntryIterator()
{
	TRACE("HTreeEntryIterator::~HTreeEntryIterator(): %p, parent: %p\n", this,
		fParent);
	delete fParent;
	TRACE("HTreeEntryIterator::~HTreeEntryIterator(): Deleted the parent\n");
}


status_t
HTreeEntryIterator::Lookup(uint32 hash, int indirections,
	DirectoryIterator** directoryIterator, bool& detachRoot)
{
	TRACE("HTreeEntryIterator::Lookup()\n");

	if (fCount == 0)
		return B_ENTRY_NOT_FOUND;

	CachedBlock cached(fVolume);
	const uint8* block = cached.SetTo(fBlockNum);
	if (block == NULL) {
		ERROR("Failed to read htree entry block.\n");
		// Fallback to linear search
		*directoryIterator = new(std::nothrow)
			DirectoryIterator(fDirectory);

		if (*directoryIterator == NULL)
			return B_NO_MEMORY;

		return B_OK;
	}

	HTreeEntry* start = (HTreeEntry*)block + fCurrentEntry + 1;
	HTreeEntry* end = (HTreeEntry*)block + fCount + fFirstEntry - 1;
	HTreeEntry* middle = start;
	
	TRACE("HTreeEntryIterator::Lookup() current entry: %lu\n",
		(uint32)fCurrentEntry);
	TRACE("HTreeEntryIterator::Lookup() indirections: %d s:%p m:%p e:%p\n",
		indirections, start, middle, end);

	while (start <= end) {
		middle = (HTreeEntry*)((end - start) / 2
			+ start);

		TRACE("HTreeEntryIterator::Lookup() indirections: %d s:%p m:%p e:%p\n",
			indirections, start, middle, end);

		TRACE("HTreeEntryIterator::Lookup() %lx %lx\n", hash, middle->Hash());

		if (hash >= middle->Hash())
			start = middle + 1;
		else
			end = middle - 1;
	}

	--start;

	fCurrentEntry = ((uint8*)start - block) / sizeof(HTreeEntry);

	if (indirections == 0) {
		TRACE("HTreeEntryIterator::Lookup(): Creating an indexed directory "
			"iterator starting at block: %lu, hash: 0x%lX\n", start->Block(), 
			start->Hash());
		*directoryIterator = new(std::nothrow)
			DirectoryIterator(fDirectory, start->Block() * fBlockSize, this);

		if (*directoryIterator == NULL)
			return B_NO_MEMORY;

		detachRoot = true;
		return B_OK;
	}

	TRACE("HTreeEntryIterator::Lookup(): Creating a HTree entry iterator "
		"starting at block: %lu, hash: 0x%lX\n", start->Block(), start->Hash());
	fsblock_t blockNum;
	status_t status = fDirectory->FindBlock(start->Block() * fBlockSize,
		blockNum);
	if (status != B_OK)
		return status;

	delete fChild;

	fChild = new(std::nothrow) HTreeEntryIterator(blockNum, fBlockSize,
		fDirectory, this, (start->Hash() & 1) == 1);
	if (fChild == NULL)
		return B_NO_MEMORY;
	
	status = fChild->Init();
	if (status != B_OK)
		return status;
	
	return fChild->Lookup(hash, indirections - 1, directoryIterator,
		detachRoot);
}


status_t
HTreeEntryIterator::GetNext(uint32& childBlock)
{
	fCurrentEntry++;
	TRACE("HTreeEntryIterator::GetNext(): current entry: %lu count: %lu, "
		"limit: %lu\n", (uint32)fCurrentEntry, (uint32)fCount, (uint32)fLimit);
	bool endOfBlock = fCurrentEntry >= (fCount + fFirstEntry);

	if (endOfBlock) {
		TRACE("HTreeEntryIterator::GetNext(): end of entries in the block\n");
		if (fParent == NULL) {
			TRACE("HTreeEntryIterator::GetNext(): block was the root block\n");
			return B_ENTRY_NOT_FOUND;
		}
		
		uint32 logicalBlock;
		status_t status = fParent->GetNext(logicalBlock);
		if (status != B_OK)
			return status;
		
		TRACE("HTreeEntryIterator::GetNext(): moving to next block: %lu\n",
			logicalBlock);
		
		status = fDirectory->FindBlock(logicalBlock * fBlockSize, fBlockNum);
		if (status != B_OK)
			return status;
		
		fFirstEntry = 1; // Skip fake directory entry
		fCurrentEntry = 1;
		status = Init();
		if (status != B_OK)
			return status;

		fHasCollision = fParent->HasCollision();
	}

	CachedBlock cached(fVolume);
	const uint8* block = cached.SetTo(fBlockNum);
	if (block == NULL)
		return B_IO_ERROR;

	HTreeEntry* entry = &((HTreeEntry*)block)[fCurrentEntry];

	if (!endOfBlock)
		fHasCollision = (entry[fCurrentEntry].Hash() & 1) == 1;
	
	TRACE("HTreeEntryIterator::GetNext(): next block: %lu\n",
		entry->Block());

	childBlock = entry->Block();
	
	return B_OK;
}


uint32
HTreeEntryIterator::BlocksNeededForNewEntry()
{
	TRACE("HTreeEntryIterator::BlocksNeededForNewEntry(): block num: %llu, "
		"volume: %p\n", fBlockNum, fVolume);
	CachedBlock cached(fVolume);

	const uint8* blockData = cached.SetTo(fBlockNum);
	const HTreeEntry* entries = (const HTreeEntry*)blockData;
	const HTreeCountLimit* countLimit =
		(const HTreeCountLimit*)&entries[fFirstEntry];

	uint32 newBlocks = 0;
	if (countLimit->IsFull()) {
		newBlocks++;

		if (fParent != NULL)
			newBlocks += fParent->BlocksNeededForNewEntry();
		else {
			// Need a new level
			HTreeRoot* root = (HTreeRoot*)entries;

			if (root->indirection_levels == 1) {
				// Maximum supported indirection levels reached
				return B_DEVICE_FULL;
			}

			newBlocks++;
		}
	}

	return newBlocks;
}


status_t
HTreeEntryIterator::InsertEntry(Transaction& transaction, uint32 hash,
	off_t blockNum, off_t newBlocksPos, bool hasCollision)
{
	TRACE("HTreeEntryIterator::InsertEntry(): block num: %llu\n", fBlockNum);
	CachedBlock cached(fVolume);

	uint8* blockData = cached.SetToWritable(transaction, fBlockNum);
	if (blockData == NULL)
		return B_IO_ERROR;

	HTreeEntry* entries = (HTreeEntry*)blockData;

	HTreeCountLimit* countLimit = (HTreeCountLimit*)&entries[fFirstEntry];
	uint16 count = countLimit->Count();

	if (count == countLimit->Limit()) {
		TRACE("HTreeEntryIterator::InsertEntry(): Splitting the node\n");
		panic("Splitting a HTree node required, but isn't yet fully "
			"supported\n");

		fsblock_t physicalBlock;
		status_t status = fDirectory->FindBlock(newBlocksPos, physicalBlock);
		if (status != B_OK)
			return status;

		CachedBlock secondCached(fVolume);
		uint8* secondBlockData = secondCached.SetToWritable(transaction,
			physicalBlock);
		if (secondBlockData == NULL)
			return B_IO_ERROR;

		HTreeFakeDirEntry* fakeEntry = (HTreeFakeDirEntry*)secondBlockData;
		fakeEntry->inode_id = 0; // ?
		fakeEntry->SetEntryLength(fBlockSize);
		fakeEntry->name_length = 0;
		fakeEntry->file_type = 0; // ?

		HTreeEntry* secondBlockEntries = (HTreeEntry*)secondBlockData;
		memmove(&entries[fFirstEntry + count / 2], &secondBlockEntries[1],
			(count - count / 2) * sizeof(HTreeEntry));
	}

	TRACE("HTreeEntryIterator::InsertEntry(): Inserting node. Count: %u, "
		"current entry: %u\n", count, fCurrentEntry);

	if (count > 0) {
		TRACE("HTreeEntryIterator::InsertEntry(): memmove(%u, %u, %u)\n",
			fCurrentEntry + 2, fCurrentEntry + 1, count + fFirstEntry
				- fCurrentEntry - 1);
		memmove(&entries[fCurrentEntry + 2], &entries[fCurrentEntry + 1],
			(count + fFirstEntry - fCurrentEntry - 1) * sizeof(HTreeEntry));
	}

	uint32 oldHash = entries[fCurrentEntry].Hash();
	entries[fCurrentEntry].SetHash(hasCollision ? oldHash | 1 : oldHash & ~1);
	entries[fCurrentEntry + 1].SetHash((oldHash & 1) == 0 ? hash & ~1
		: hash | 1);
	entries[fCurrentEntry + 1].SetBlock(blockNum);

	countLimit->SetCount(count + 1);

	return B_OK;
}
