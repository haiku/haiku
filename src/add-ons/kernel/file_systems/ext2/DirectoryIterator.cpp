/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include <string.h>

#include <AutoDeleter.h>
#include <util/VectorSet.h>

#include "CachedBlock.h"
#include "CRCTable.h"
#include "HTree.h"
#include "Inode.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("ext2: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif


struct HashedEntry
{
	uint8*	position;
	uint32	hash;

	bool	operator<(const HashedEntry& other)	const
	{
		return hash <= other.hash;
	}

	bool	operator>(const HashedEntry& other)	const
	{
		return hash >= other.hash;
	}
};


DirectoryIterator::DirectoryIterator(Inode* directory, off_t start,
	HTreeEntryIterator* parent)
	:
	fDirectory(directory),
	fVolume(directory->GetVolume()),
	fBlockSize(fVolume->BlockSize()),
	fParent(parent),
	fNumBlocks(directory->Size() == 0 ? 0
		: (directory->Size() - 1) / fBlockSize + 1),
	fLogicalBlock(start / fBlockSize),
	fDisplacement(start % fBlockSize),
	fPreviousDisplacement(fDisplacement),
	fStartLogicalBlock(fLogicalBlock),
	fStartDisplacement(fDisplacement)
{
	TRACE("DirectoryIterator::DirectoryIterator() %" B_PRIdINO ": num blocks: "
		"%" B_PRIu32 "\n", fDirectory->ID(), fNumBlocks);
	fIndexing = parent != NULL;
	fInitStatus = fDirectory->FindBlock(start, fPhysicalBlock);
	fStartPhysicalBlock = fPhysicalBlock;
}


DirectoryIterator::~DirectoryIterator()
{
	TRACE("DirectoryIterator::~DirectoryIterator(): %p, parent: %p\n", this,
		fParent);
	delete fParent;
	TRACE("DirectoryIterator::~DirectoryIterator(): Deleted the parent\n");
}


status_t
DirectoryIterator::InitCheck()
{
	return fInitStatus;
}


status_t
DirectoryIterator::Get(char* name, size_t* _nameLength, ino_t* _id)
{
	TRACE("DirectoryIterator::Get() ID %" B_PRIdINO "\n", fDirectory->ID());
	if (_Offset() >= fDirectory->Size()) {
		TRACE("DirectoryIterator::Get() out of entries\n");
		return B_ENTRY_NOT_FOUND;
	}

	CachedBlock cached(fVolume);
	const uint8* block = cached.SetTo(fPhysicalBlock);
	if (block == NULL)
		return B_IO_ERROR;

	ASSERT(_CheckBlock(block));

	TRACE("DirectoryIterator::Get(): Displacement: %" B_PRIu32 "\n",
		fDisplacement);
	const ext2_dir_entry* entry = (const ext2_dir_entry*)&block[fDisplacement];

	if (entry->Length() == 0 || entry->InodeID() == 0)
		return B_BAD_DATA;

	if (entry->NameLength() != 0) {
		size_t length = entry->NameLength();

		TRACE("block %" B_PRIu32 ", displacement %" B_PRIu32 ": entry ino %"
			B_PRIu32 ", length %u, name length %" B_PRIuSIZE ", type %u\n",
			fLogicalBlock, fDisplacement, entry->InodeID(), entry->Length(),
			length,	entry->FileType());

		if (*_nameLength > 0) {
			if (length + 1 > *_nameLength)
				return B_BUFFER_OVERFLOW;

			memcpy(name, entry->name, length);
			name[length] = '\0';

			*_nameLength = length;
		}

		*_id = entry->InodeID();
	} else
		*_nameLength = 0;

	return B_OK;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	status_t status;
	while ((status = Get(name, _nameLength, _id)) == B_BAD_DATA) {
		status = Next();
		if (status != B_OK)
			return status;
	}
	return status;
}


status_t
DirectoryIterator::Next()
{
	TRACE("DirectoryIterator::Next() fDirectory->ID() %" B_PRIdINO "\n",
		fDirectory->ID());

	if (_Offset() >= fDirectory->Size()) {
		TRACE("DirectoryIterator::Next() out of entries\n");
		return B_ENTRY_NOT_FOUND;
	}

	TRACE("DirectoryIterator::Next(): Creating cached block\n");

	CachedBlock cached(fVolume);
	ext2_dir_entry* entry;

	TRACE("DirectoryIterator::Next(): Loading cached block\n");
	const uint8* block = cached.SetTo(fPhysicalBlock);
	if (block == NULL)
		return B_IO_ERROR;

	ASSERT(_CheckBlock(block));
	uint32 maxSize = _MaxSize();

	entry = (ext2_dir_entry*)(block + fDisplacement);

	do {
		TRACE("Checking entry at block %" B_PRIu64 ", displacement %" B_PRIu32
			" entry inodeid %" B_PRIu32 "\n", fPhysicalBlock, fDisplacement,
			entry->InodeID());

		if (entry->Length() != 0) {
			if (!entry->IsValid()) {
				TRACE("invalid entry.\n");
				return B_BAD_DATA;
			}
			fPreviousDisplacement = fDisplacement;
			fDisplacement += entry->Length();
		} else 
			fDisplacement = maxSize;

		if (fDisplacement == maxSize) {
			TRACE("Reached end of block\n");

			fDisplacement = 0;

			status_t status = _NextBlock();
			if (status != B_OK)
				return status;
						
			if ((off_t)(_Offset() + ext2_dir_entry::MinimumSize())
					>= fDirectory->Size()) {
				TRACE("DirectoryIterator::Next() end of directory file\n");
				return B_ENTRY_NOT_FOUND;
			}
			status = fDirectory->FindBlock(_Offset(), fPhysicalBlock);
			if (status != B_OK)
				return status;

			block = cached.SetTo(fPhysicalBlock);
			if (block == NULL)
				return B_IO_ERROR;
			ASSERT(_CheckBlock(block));

		} else if (fDisplacement > maxSize) {
			TRACE("The entry isn't block aligned.\n");
			// TODO: Is block alignment obligatory?
			return B_BAD_DATA;
		}

		entry = (ext2_dir_entry*)(block + fDisplacement);

		TRACE("DirectoryIterator::Next() skipping entry %d %" B_PRIu32 "\n",
			entry->Length(), entry->InodeID());
	} while (entry->Length() == 0 || entry->InodeID() == 0);

	TRACE("DirectoryIterator::Next() entry->Length() %d entry->name %*s\n",
			entry->Length(), entry->NameLength(), entry->name);

	return B_OK;
}


status_t
DirectoryIterator::Rewind()
{
	fDisplacement = 0;
	fPreviousDisplacement = 0;
	fLogicalBlock = 0;

	return fDirectory->FindBlock(0, fPhysicalBlock);
}


void
DirectoryIterator::Restart()
{
	TRACE("DirectoryIterator::Restart(): (logical, physical, displacement): "
		"current: (%" B_PRIu32 ", %" B_PRIu64 ", %" B_PRIu32 "), start: (%"
		B_PRIu32 ", %" B_PRIu64 ", %" B_PRIu32 ")\n", fLogicalBlock,
		fPhysicalBlock, fDisplacement, fStartLogicalBlock, fStartPhysicalBlock,
		fStartDisplacement);
	fLogicalBlock = fStartLogicalBlock;
	fPhysicalBlock = fStartPhysicalBlock;
	fDisplacement = fPreviousDisplacement = fStartDisplacement;
}


status_t
DirectoryIterator::AddEntry(Transaction& transaction, const char* name,
	size_t _nameLength, ino_t id, uint8 type)
{
	TRACE("DirectoryIterator::AddEntry(%s, ...)\n", name);

	uint8 nameLength = _nameLength > EXT2_NAME_LENGTH ? EXT2_NAME_LENGTH
		: _nameLength;

	status_t status = B_OK;
	while (status == B_OK) {
		uint16 pos = 0;
		uint16 newLength;

		status = _AllocateBestEntryInBlock(nameLength, pos, newLength);
		if (status == B_OK) {
			return _AddEntry(transaction, name, nameLength, id, type, newLength,
				pos);
		} else if (status != B_DEVICE_FULL)
			return status;
		
		fDisplacement = 0;
		status = _NextBlock();
		if (status == B_OK)
			status = fDirectory->FindBlock(_Offset(), fPhysicalBlock);
	}

	if (status != B_ENTRY_NOT_FOUND)
		return status;

	bool firstSplit = fNumBlocks == 1 && fVolume->IndexedDirectories();

	fNumBlocks++;

	if (fIndexing) {
		TRACE("DirectoryIterator::AddEntry(): Adding another HTree leaf\n");
		fNumBlocks += fParent->BlocksNeededForNewEntry();
	} else if (firstSplit) {
		// Allocate another block (fNumBlocks should become 3)
		TRACE("DirectoryIterator::AddEntry(): Creating index for directory\n");
		fNumBlocks++;
	} else
		TRACE("DirectoryIterator::AddEntry(): Enlarging directory\n");

	status = fDirectory->Resize(transaction, fNumBlocks * fBlockSize);
	if (status != B_OK)
		return status;

	if (firstSplit || fIndexing) {
		// firstSplit and fIndexing are mutually exclusive
		return _SplitIndexedBlock(transaction, name, nameLength, id, type,
			fNumBlocks - 1, firstSplit);
	}

	fLogicalBlock = fNumBlocks - 1;
	status = fDirectory->FindBlock(fLogicalBlock * fBlockSize, fPhysicalBlock);
	if (status != B_OK)
		return status;

	return _AddEntry(transaction, name, nameLength, id, type, fBlockSize, 0,
		false);
}


status_t
DirectoryIterator::FindEntry(const char* name, ino_t* _id)
{
	TRACE("DirectoryIterator::FindEntry(): %p %p\n", this, name);
	char buffer[EXT2_NAME_LENGTH + 1];
	ino_t id;

	status_t status = B_OK;
	size_t length = strlen(name);
	while (status == B_OK) {
		size_t nameLength = EXT2_NAME_LENGTH;
		status = Get(buffer, &nameLength, &id);
		if (status == B_OK) {
			if (length == nameLength 
				&& strncmp(name, buffer, nameLength) == 0) {
				if (_id != NULL)
					*_id = id;
				return B_OK;
			}
		} else if (status != B_BAD_DATA)
			break;
		status = Next();
	}

	return status;
}


status_t
DirectoryIterator::RemoveEntry(Transaction& transaction)
{
	TRACE("DirectoryIterator::RemoveEntry()\n");
	ext2_dir_entry* previousEntry;
	ext2_dir_entry* dirEntry;
	CachedBlock cached(fVolume);

	uint8* block = cached.SetToWritable(transaction, fPhysicalBlock);
	uint32 maxSize = _MaxSize();

	if (fDisplacement == 0) {
		previousEntry = (ext2_dir_entry*)&block[fDisplacement];

		fPreviousDisplacement = fDisplacement;
		fDisplacement += previousEntry->Length();

		if (fDisplacement == maxSize) {
			previousEntry->SetInodeID(0);
			fDisplacement = 0;
			return B_OK;
		} else if (fDisplacement > maxSize) {
			TRACE("DirectoryIterator::RemoveEntry(): Entry isn't aligned to "
				"block entry.");
			return B_BAD_DATA;
		}

		dirEntry = (ext2_dir_entry*)&block[fDisplacement];
		memcpy(&block[fPreviousDisplacement], &block[fDisplacement],
			dirEntry->Length());

		previousEntry->SetLength(fDisplacement - fPreviousDisplacement
			+ previousEntry->Length());

		fDirectory->SetDirEntryChecksum(block);

		ASSERT(_CheckBlock(block));

		return B_OK;
	}

	TRACE("DirectoryIterator::RemoveEntry() fDisplacement %" B_PRIu32 "\n",
		fDisplacement);

	if (fPreviousDisplacement == fDisplacement) {
		char buffer[EXT2_NAME_LENGTH + 1];

		dirEntry = (ext2_dir_entry*)&block[fDisplacement];

		memcpy(buffer, dirEntry->name, (uint32)dirEntry->name_length);

		fDisplacement = 0;
		status_t status = FindEntry(dirEntry->name);
		if (status == B_ENTRY_NOT_FOUND)
			return B_BAD_DATA;
		if (status != B_OK)
			return status;
	}

	previousEntry = (ext2_dir_entry*)&block[fPreviousDisplacement];
	dirEntry = (ext2_dir_entry*)&block[fDisplacement];

	previousEntry->SetLength(previousEntry->Length() + dirEntry->Length());

	memset(&block[fDisplacement], 0, 
		fPreviousDisplacement + previousEntry->Length() - fDisplacement);

	fDirectory->SetDirEntryChecksum(block);

	ASSERT(_CheckBlock(block));

	return B_OK;
}


status_t
DirectoryIterator::ChangeEntry(Transaction& transaction, ino_t id,
	uint8 fileType)
{
	CachedBlock cached(fVolume);

	uint8* block = cached.SetToWritable(transaction, fPhysicalBlock);
	if (block == NULL)
		return B_IO_ERROR;

	ext2_dir_entry* dirEntry = (ext2_dir_entry*)&block[fDisplacement];
	dirEntry->SetInodeID(id);
	dirEntry->file_type = fileType;

	fDirectory->SetDirEntryChecksum(block);

	ASSERT(_CheckBlock(block));

	return B_OK;
}


status_t
DirectoryIterator::_AllocateBestEntryInBlock(uint8 nameLength, uint16& pos,
	uint16& newLength)
{
	TRACE("DirectoryIterator::_AllocateBestEntryInBlock()\n");
	CachedBlock cached(fVolume);
	const uint8* block = cached.SetTo(fPhysicalBlock);

	ASSERT(_CheckBlock(block));

	uint16 requiredLength = EXT2_DIR_REC_LEN(nameLength);
	uint32 maxSize = _MaxSize();
	
	uint16 bestPos = maxSize;
	uint16 bestLength = maxSize;
	uint16 bestRealLength = maxSize;
	ext2_dir_entry* dirEntry;
	
	while (pos < maxSize) {
		dirEntry = (ext2_dir_entry*)&block[pos];
		if (!_CheckDirEntry(dirEntry, block)) {
			TRACE("DirectoryIterator::_AllocateBestEntryInBlock(): invalid "
				"dirEntry->Length() pos %d\n", pos);
			return B_BAD_DATA;
		}

		uint16 realLength = EXT2_DIR_REC_LEN(dirEntry->NameLength());
		uint16 emptySpace = dirEntry->Length();
		if (dirEntry->InodeID() != 0)
			emptySpace -= realLength;
		if (emptySpace == requiredLength) {
			// Found an exact match
			TRACE("DirectoryIterator::_AllocateBestEntryInBlock(): Found an "
				"exact length match\n");
			newLength = realLength;

			return B_OK;
		} else if (emptySpace > requiredLength && emptySpace < bestLength) {
			bestPos = pos;
			bestLength = emptySpace;
			bestRealLength = realLength;
		}

		pos += dirEntry->Length();
	}
	
	if (bestPos == maxSize)
		return B_DEVICE_FULL;

	TRACE("DirectoryIterator::_AllocateBestEntryInBlock(): Found a suitable "
		"location: %u\n", bestPos);
	pos = bestPos;
	newLength = bestRealLength;

	return B_OK;
}


status_t
DirectoryIterator::_AddEntry(Transaction& transaction, const char* name,
	uint8 nameLength, ino_t id, uint8 type, uint16 newLength, uint16 pos,
	bool hasPrevious)
{
	TRACE("DirectoryIterator::_AddEntry(%s, %d, %" B_PRIdINO ", %d, %d, %d, "
		"%c)\n", name, nameLength, id, type, newLength, pos,
		hasPrevious ? 't' : 'f');
	CachedBlock cached(fVolume);

	uint8* block = cached.SetToWritable(transaction, fPhysicalBlock);
	if (block == NULL)
		return B_IO_ERROR;

	ext2_dir_entry* dirEntry = (ext2_dir_entry*)&block[pos];

	if (hasPrevious) {
		uint16 previousLength = dirEntry->Length();
		dirEntry->SetLength(newLength);

		dirEntry = (ext2_dir_entry*)&block[pos + newLength];
		newLength = previousLength - newLength;
	}

	dirEntry->SetLength(newLength);
	dirEntry->name_length = nameLength;
	dirEntry->SetInodeID(id);
	dirEntry->file_type = type;
	memcpy(dirEntry->name, name, nameLength);

	fDirectory->SetDirEntryChecksum(block);

	ASSERT(_CheckBlock(block));

	TRACE("DirectoryIterator::_AddEntry(): Done\n");

	return B_OK;
}


status_t
DirectoryIterator::_SplitIndexedBlock(Transaction& transaction,
	const char* name, uint8 nameLength, ino_t id, uint8 type,
	uint32 newBlocksPos, bool firstSplit)
{
	// Block is full, split required
	TRACE("DirectoryIterator::_SplitIndexedBlock(.., %s, %u, %" B_PRIdINO ", %"
		B_PRIu32 ", %c)\n", name, nameLength, id, newBlocksPos,
		firstSplit ? 't' : 'f');

	// Allocate a buffer for the entries in the block
	uint8* buffer = new(std::nothrow) uint8[fBlockSize];
	if (buffer == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<uint8> bufferDeleter(buffer);

	fsblock_t firstPhysicalBlock = 0;
	uint32 maxSize = _MaxSize();

	// Prepare block to hold the first half of the entries and fill the buffer
	CachedBlock cachedFirst(fVolume);

	if (firstSplit) {
		// Save all entries to the buffer
		status_t status = fDirectory->FindBlock(0, firstPhysicalBlock);
		if (status != B_OK)
			return status;

		const uint8* srcBlock = cachedFirst.SetTo(firstPhysicalBlock);
		if (srcBlock == NULL)
			return B_IO_ERROR;

		memcpy(buffer, srcBlock, fBlockSize);

		status = fDirectory->FindBlock(fBlockSize, fPhysicalBlock);
		if (status != B_OK)
			return status;
	}

	uint8* firstBlock = cachedFirst.SetToWritable(transaction, fPhysicalBlock);
	uint8* secondBlock = NULL;
	if (firstBlock == NULL)
		return B_IO_ERROR;

	status_t status;

	if (!firstSplit) {
		// Save all entries to the buffer
		memcpy(buffer, firstBlock, fBlockSize);
	} else {
		// Initialize the root node
		fDirectory->Node().SetFlag(EXT2_INODE_INDEXED);
		HTreeRoot* root;

		secondBlock = cachedFirst.SetToWritable(transaction,
			firstPhysicalBlock, true);
		if (secondBlock == NULL)
			return B_IO_ERROR;

		status = fDirectory->WriteBack(transaction);
		if (status != B_OK)
			return status;

		memcpy(secondBlock, buffer, 2 * (sizeof(HTreeFakeDirEntry) + 4));

		root = (HTreeRoot*)secondBlock;

		HTreeFakeDirEntry* dotdot = &root->dotdot;
		dotdot->SetEntryLength(maxSize);

		root->hash_version = fVolume->DefaultHashVersion();
		root->root_info_length = 8;
		root->indirection_levels = 0;

		root->count_limit->SetLimit((maxSize
			- ((uint8*)root->count_limit - secondBlock)) / sizeof(HTreeEntry));
		root->count_limit->SetCount(2);
	}

	// Sort entries
	VectorSet<HashedEntry> entrySet;

	HTree htree(fVolume, fDirectory);
	status = htree.PrepareForHash();
	if (status != B_OK)
		return status;

	uint32 displacement = firstSplit ? 2 * (sizeof(HTreeFakeDirEntry) + 4) : 0;

	HashedEntry entry;
	ext2_dir_entry* dirEntry = NULL;

	while (displacement < maxSize) {
		entry.position = &buffer[displacement];
		dirEntry = (ext2_dir_entry*)entry.position;

		TRACE("DirectoryIterator::_SplitIndexedBlock(): pos: %p, name "
			"length: %u, entry length: %u\n", entry.position,
			dirEntry->name_length,
			dirEntry->Length());

		char cbuffer[256];
		memcpy(cbuffer, dirEntry->name, dirEntry->name_length);
		cbuffer[dirEntry->name_length] = '\0';
		entry.hash = htree.Hash(dirEntry->name, dirEntry->name_length);
		TRACE("DirectoryIterator::_SplitIndexedBlock(): %s -> %" B_PRIu32 "\n",
			cbuffer, entry.hash);

		status = entrySet.Insert(entry);
		if (status != B_OK)
			return status;

		displacement += dirEntry->Length();
	}

	// Prepare the new entry to be included as well
	ext2_dir_entry newEntry;

	uint16 newLength = EXT2_DIR_REC_LEN(nameLength);

	newEntry.name_length = nameLength;
	newEntry.SetLength(newLength);
	newEntry.SetInodeID(id);
	newEntry.file_type = type;
	memcpy(newEntry.name, name, nameLength);

	entry.position = (uint8*)&newEntry;
	entry.hash = htree.Hash(name, nameLength);
	TRACE("DirectoryIterator::_SplitIndexedBlock(): %s -> %" B_PRIu32 "\n",
		name, entry.hash);

	entrySet.Insert(entry);

	// Move first half of entries to the first block
	VectorSet<HashedEntry>::Iterator iterator = entrySet.Begin();
	int32 median = entrySet.Count() / 2;
	displacement = 0;
	TRACE("DirectoryIterator::_SplitIndexedBlock(): Count: %" B_PRId32
		", median: %" B_PRId32 "\n", entrySet.Count(), median);

	uint32 previousHash = (*iterator).hash;

	for (int32 i = 0; i < median; ++i) {
		dirEntry = (ext2_dir_entry*)(*iterator).position;
		previousHash = (*iterator).hash;

		uint32 realLength = EXT2_DIR_REC_LEN(dirEntry->name_length);

		dirEntry->SetLength((uint16)realLength);
		memcpy(&firstBlock[displacement], dirEntry, realLength);

		displacement += realLength;
		iterator++;
	}

	// Update last entry in the block
	uint16 oldLength = dirEntry->Length();
	dirEntry = (ext2_dir_entry*)&firstBlock[displacement - oldLength];
	dirEntry->SetLength(maxSize - displacement + oldLength);

	fDirectory->SetDirEntryChecksum(firstBlock);

	bool collision = false;

	while (iterator != entrySet.End() && (*iterator).hash == previousHash) {
		// Keep collisions on the same block
		TRACE("DirectoryIterator::_SplitIndexedBlock(): Handling collisions\n");

		// This isn't the ideal solution, but it is a rare occurance
		dirEntry = (ext2_dir_entry*)(*iterator).position;

		if (displacement + dirEntry->Length() > maxSize) {
			// Doesn't fit on the block
			collision = true;
			break;
		}

		memcpy(&firstBlock[displacement], dirEntry, dirEntry->Length());

		displacement += dirEntry->Length();
		iterator++;
	}

	// Save the hash to store in the parent
	uint32 medianHash = (*iterator).hash;

	// Update parent
	if (firstSplit) {
		TRACE("DirectoryIterator::_SplitIndexedBlock(): Updating root\n");
		HTreeRoot* root = (HTreeRoot*)secondBlock;
		HTreeEntry* htreeEntry = (HTreeEntry*)root->count_limit;
		htreeEntry->SetBlock(1);
		
		++htreeEntry;
		htreeEntry->SetBlock(2);
		htreeEntry->SetHash(medianHash);


		off_t start = (off_t)root->root_info_length
			+ 2 * (sizeof(HTreeFakeDirEntry) + 4);
		_SetHTreeEntryChecksum(secondBlock, start, 2);
		fParent = new(std::nothrow) HTreeEntryIterator(start, fDirectory);
		if (fParent == NULL)
			return B_NO_MEMORY;
		
		fLogicalBlock = 1;
		status = fDirectory->FindBlock(fLogicalBlock * fBlockSize,
			fPhysicalBlock);

		fPreviousDisplacement = fDisplacement = 0;

		status = fParent->Init();
	}
	else {
		status = fParent->InsertEntry(transaction, medianHash, fNumBlocks - 1,
			newBlocksPos, collision);
	}
	if (status != B_OK)
		return status;

	// Prepare last block to hold the second half of the entries
	TRACE("DirectoryIterator::_SplitIndexedBlock(): Preparing second leaf "
		"block\n");
	fDisplacement = 0;

	status = fDirectory->FindBlock(fDirectory->Size() - 1, fPhysicalBlock);
	if (status != B_OK)
		return status;

	CachedBlock cachedSecond(fVolume);
	secondBlock = cachedSecond.SetToWritable(transaction,
		fPhysicalBlock);
	if (secondBlock == NULL)
		return B_IO_ERROR;

	// Move the second half of the entries to the second block
	VectorSet<HashedEntry>::Iterator end = entrySet.End();
	displacement = 0;

	while (iterator != end) {
		dirEntry = (ext2_dir_entry*)(*iterator).position;

		uint32 realLength = EXT2_DIR_REC_LEN(dirEntry->name_length);

		dirEntry->SetLength((uint16)realLength);
		memcpy(&secondBlock[displacement], dirEntry, realLength);

		displacement += realLength;
		iterator++;
	}

	// Update last entry in the block
	oldLength = dirEntry->Length();
	dirEntry = (ext2_dir_entry*)&secondBlock[displacement - oldLength];
	dirEntry->SetLength(maxSize - displacement + oldLength);

	fDirectory->SetDirEntryChecksum(secondBlock);

	ASSERT(_CheckBlock(firstBlock));
	ASSERT(_CheckBlock(secondBlock));

	TRACE("DirectoryIterator::_SplitIndexedBlock(): Done\n");
	return B_OK;
}


status_t
DirectoryIterator::_NextBlock()
{
	TRACE("DirectoryIterator::_NextBlock()\n");
	if (fIndexing) {
		TRACE("DirectoryIterator::_NextBlock(): Indexing\n");
		if (!fParent->HasCollision()) {
			TRACE("DirectoryIterator::_NextBlock(): next block doesn't "
				"contain collisions from previous block\n");
#ifndef COLLISION_TEST
			return B_ENTRY_NOT_FOUND;
#endif
		}

		return fParent->GetNext(fLogicalBlock);
	}

	if (++fLogicalBlock > fNumBlocks)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


bool
DirectoryIterator::_CheckDirEntry(const ext2_dir_entry* dirEntry, const uint8* buffer)
{
	const char *errmsg = NULL;
	if (dirEntry->Length() < EXT2_DIR_REC_LEN(1))
		errmsg = "Length is too small";
	else if (dirEntry->Length() % 4 != 0)
		errmsg = "Length is not a multiple of 4";
	else if (dirEntry->Length() < EXT2_DIR_REC_LEN(dirEntry->NameLength()))
		errmsg = "Length is too short for the name";
	else if (((uint8*)dirEntry - buffer) + dirEntry->Length() > fBlockSize)
		errmsg = "Length is too big for the blocksize";

	TRACE("DirectoryIterator::_CheckDirEntry() %s\n", errmsg);
	return errmsg == NULL;
}


status_t
DirectoryIterator::_CheckBlock(const uint8* buffer)
{
	uint32 maxSize = fBlockSize;
	if (fVolume->HasMetaGroupChecksumFeature())
		maxSize -= sizeof(ext2_dir_entry_tail);

	status_t err = B_OK;
	uint16 pos = 0;
	while (pos < maxSize) {
		const ext2_dir_entry *dirEntry = (const ext2_dir_entry*)&buffer[pos];

		if (!_CheckDirEntry(dirEntry, buffer)) {
			TRACE("DirectoryIterator::_CheckBlock(): invalid "
				"dirEntry pos %d\n", pos);
			err = B_BAD_DATA;
		}

		pos += dirEntry->Length();
	}
	return err;
}


ext2_htree_tail*
DirectoryIterator::_HTreeEntryTail(uint8* block, uint16 offset) const
{
	HTreeEntry* entries = (HTreeEntry*)block;
	uint16 firstEntry = offset % fBlockSize / sizeof(HTreeEntry);
	HTreeCountLimit* countLimit = (HTreeCountLimit*)&entries[firstEntry];
	uint16 limit = countLimit->Limit();
	TRACE("HTreeEntryIterator::_HTreeEntryTail() %p %p\n", block, &entries[firstEntry + limit]);
	return (ext2_htree_tail*)(&entries[firstEntry + limit]);
}


uint32
DirectoryIterator::_HTreeRootChecksum(uint8* block, uint16 offset, uint16 count) const
{
	uint32 number = fDirectory->ID();
	uint32 checksum = calculate_crc32c(fVolume->ChecksumSeed(),
		(uint8*)&number, sizeof(number));
	uint32 gen = fDirectory->Node().generation;
	checksum = calculate_crc32c(checksum, (uint8*)&gen, sizeof(gen));
	checksum = calculate_crc32c(checksum, block,
		offset + count * sizeof(HTreeEntry));
	TRACE("HTreeEntryIterator::_HTreeRootChecksum() size %u\n", offset + count * sizeof(HTreeEntry));
	ext2_htree_tail dummy;
	dummy.reserved = 0;
	checksum = calculate_crc32c(checksum, (uint8*)&dummy, sizeof(dummy));
	return checksum;
}


void
DirectoryIterator::_SetHTreeEntryChecksum(uint8* block, uint16 offset, uint16 count)
{
	TRACE("DirectoryIterator::_SetHTreeEntryChecksum()\n");
	if (fVolume->HasMetaGroupChecksumFeature()) {
		ext2_htree_tail* tail = _HTreeEntryTail(block, offset);
		tail->reserved = 0x0;
		tail->checksum = _HTreeRootChecksum(block, offset, count);
	}
}


uint32
DirectoryIterator::_MaxSize()
{
	uint32 maxSize = fBlockSize;
	if (fVolume->HasMetaGroupChecksumFeature())
		maxSize -= sizeof(ext2_dir_entry_tail);
	return maxSize;
}

