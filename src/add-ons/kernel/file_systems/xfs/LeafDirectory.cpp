/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LeafDirectory.h"


LeafDirectory::LeafDirectory(Inode* inode)
	:
	fInode(inode),
	fOffset(0),
	fDataBuffer(NULL),
	fLeafBuffer(NULL),
	fLeafMap(NULL),
	fDataMap(NULL),
	fCurBlockNumber(-1)
{
}


LeafDirectory::~LeafDirectory()
{
	delete fDataMap;
	delete fLeafMap;
	delete fDataBuffer;
	delete fLeafBuffer;
}


status_t
LeafDirectory::Init()
{
	fLeafMap = new(std::nothrow) ExtentMapEntry;
	if (fLeafMap == NULL)
		return B_NO_MEMORY;

	fDataMap = new(std::nothrow) ExtentMapEntry;
	if (fDataMap == NULL)
		return B_NO_MEMORY;

	FillMapEntry(fInode->DataExtentsCount()-1, fLeafMap);
	FillMapEntry(0, fDataMap);
	return B_OK;
}


bool
LeafDirectory::IsLeafType()
{
	bool status = true;
	if (fInode->BlockCount() == 1
		|| fInode->DataExtentsCount() == 1
		|| fInode->Size() !=
			(fInode->BlockCount() - 1) * fInode->DirBlockSize())
		status = false;

	if (status == false)
		return status;

	FillMapEntry(fInode->DataExtentsCount() - 1, fLeafMap);
	TRACE("leaf_Startoffset:(%ld)\n",
		LEAF_STARTOFFSET(fInode->GetVolume()->BlockLog()));

	if (fLeafMap->br_startoff
		!= LEAF_STARTOFFSET(fInode->GetVolume()->BlockLog()))
		status = false;

	return status;
}


void
LeafDirectory::FillMapEntry(int num, ExtentMapEntry* fMap)
{
	void* directoryFork = DIR_DFORK_PTR(fInode->Buffer());
	uint64* pointerToMap = (uint64*)((char*)directoryFork + num * EXTENT_SIZE);
	uint64 firstHalf = pointerToMap[0];
	uint64 secondHalf = pointerToMap[1];
		//dividing the 128 bits into 2 parts.

	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = firstHalf >> 63;
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = secondHalf & MASK(21);
	TRACE("FillMapEntry: startoff:(%ld), startblock:(%ld), blockcount:(%ld),"
			"state:(%d)\n", fMap->br_startoff, fMap->br_startblock,
			fMap->br_blockcount, fMap->br_state);
}


status_t
LeafDirectory::FillBuffer(int type, char* blockBuffer, int howManyBlocksFurthur)
{
	TRACE("FILLBUFFER\n");
	ExtentMapEntry* map;
	if (type == DATA)
		map = fDataMap;

	if (type == LEAF)
		map = fLeafMap;

	if (map->br_state !=0)
		return B_BAD_VALUE;

	int len = fInode->DirBlockSize();
	if (blockBuffer == NULL) {
		blockBuffer = new(std::nothrow) char[len];
		if (blockBuffer == NULL)
			return B_NO_MEMORY;
	}

	Volume* volume = fInode->GetVolume();
	xfs_agblock_t numberOfBlocksInAg = volume->AgBlocks();

	uint64 agNo =
		FSBLOCKS_TO_AGNO(map->br_startblock + howManyBlocksFurthur, volume);
	uint64 agBlockNo =
		FSBLOCKS_TO_AGBLOCKNO(map->br_startblock + howManyBlocksFurthur, volume);

	xfs_fsblock_t blockToRead = FSBLOCKS_TO_BASICBLOCKS(volume->BlockLog(),
		(agNo * numberOfBlocksInAg + agBlockNo));

	xfs_daddr_t readPos = blockToRead * BASICBLOCKSIZE;

	TRACE("blockToRead: (%ld), readPos: (%ld)\n", blockToRead, readPos);
	if (read_pos(volume->Device(), readPos, blockBuffer, len) != len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	if (type == DATA)
		fDataBuffer = blockBuffer;

	if (type == LEAF)
		fLeafBuffer = blockBuffer;

	if (type == LEAF) {
		ExtentLeafHeader* header = (ExtentLeafHeader*) fLeafBuffer;
		TRACE("NumberOfEntries in leaf: (%d)\n",
			B_BENDIAN_TO_HOST_INT16(header->count));
	}
	if (type == DATA) {
		ExtentDataHeader* header = (ExtentDataHeader*) fDataBuffer;
		if (B_BENDIAN_TO_HOST_INT32(header->magic) == HEADER_MAGIC)
			TRACE("DATA BLOCK VALID\n");
		else {
			TRACE("DATA BLOCK INVALID\n");
			return B_BAD_VALUE;
		}
	}
	return B_OK;
}


uint32
LeafDirectory::GetOffsetFromAddress(uint32 address)
{
	address = address * 8;
		// block offset in eight bytes, hence multiply with 8
	return address & (fInode->DirBlockSize() - 1);
}


ExtentLeafEntry*
LeafDirectory::FirstLeaf()
{
	TRACE("LeafDirectory: FirstLeaf\n");
	if (fLeafBuffer == NULL) {
		ASSERT(fLeafMap != NULL);
		status_t status = FillBuffer(LEAF, fLeafBuffer, 0);
		if (status != B_OK)
			return NULL;
	}
	return (ExtentLeafEntry*)((char*)fLeafBuffer + sizeof(ExtentLeafHeader));
}


int
LeafDirectory::EntrySize(int len) const
{
	int entrySize= sizeof(xfs_ino_t) + sizeof(uint8) + len + sizeof(uint16);
		// uint16 is for the tag
	if (fInode->HasFileTypeField())
		entrySize += sizeof(uint8);

	return (entrySize + 7) & -8;
		// rounding off to closest multiple of 8
}


void
LeafDirectory::SearchAndFillDataMap(int blockNo)
{
	int len = fInode->DataExtentsCount();

	for(int i = 0; i < len - 1; i++) {
		FillMapEntry(i, fDataMap);
		if (fDataMap->br_startoff <= blockNo
			&& (blockNo <= fDataMap->br_startoff + fDataMap->br_blockcount - 1))
			// Map found
			return;
	}
}


status_t
LeafDirectory::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	TRACE("LeafDirectory::GetNext\n");
	status_t status;

	if (fDataBuffer == NULL) {
		status = FillBuffer(DATA, fDataBuffer, 0);
		if (status != B_OK)
			return status;
	}

	Volume* volume = fInode->GetVolume();
	void* entry = (void*)((ExtentDataHeader*)fDataBuffer + 1);
		// This could be an unused entry so we should check

	uint32 blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);
	if (fOffset != 0 && blockNoFromAddress == fCurBlockNumber)
		entry = (void*)(fDataBuffer
				+ BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode));
		// This gets us a little faster to the next entry

	uint32 curDirectorySize = fInode->Size();

	while (fOffset != curDirectorySize) {
		blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);

		TRACE("fOffset:(%d), blockNoFromAddress:(%d)\n",
			fOffset, blockNoFromAddress);
		if (fCurBlockNumber != blockNoFromAddress
			&& blockNoFromAddress > fDataMap->br_startoff
			&& blockNoFromAddress
				<= fDataMap->br_startoff + fDataMap->br_blockcount - 1) {
			// When the block is mapped in the same data
			// map entry but is not the first block
			status = FillBuffer(DATA, fDataBuffer,
				blockNoFromAddress - fDataMap->br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)((ExtentDataHeader*)fDataBuffer + 1);
			fOffset = fOffset + sizeof(ExtentDataHeader);
			fCurBlockNumber = blockNoFromAddress;
		} else if (fCurBlockNumber != blockNoFromAddress) {
			// When the block isn't mapped in the current data map entry
			SearchAndFillDataMap(blockNoFromAddress);
			status = FillBuffer(DATA, fDataBuffer,
				blockNoFromAddress - fDataMap->br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)((ExtentDataHeader*)fDataBuffer + 1);
			fOffset = fOffset + sizeof(ExtentDataHeader);
			fCurBlockNumber = blockNoFromAddress;
		}

		ExtentUnusedEntry* unusedEntry = (ExtentUnusedEntry*)entry;

		if (B_BENDIAN_TO_HOST_INT16(unusedEntry->freetag) == DIR2_FREE_TAG) {
			TRACE("Unused entry found\n");
			fOffset = fOffset + B_BENDIAN_TO_HOST_INT16(unusedEntry->length);
			entry = (void*)
				((char*)entry + B_BENDIAN_TO_HOST_INT16(unusedEntry->length));
			continue;
		}
		ExtentDataEntry* dataEntry = (ExtentDataEntry*) entry;

		uint16 currentOffset = (char*)dataEntry - fDataBuffer;
		TRACE("GetNext: fOffset:(%d), currentOffset:(%d)\n",
			BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode), currentOffset);

		if (BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode) > currentOffset) {
			entry = (void*)((char*)entry + EntrySize(dataEntry->namelen));
			continue;
		}

		if (dataEntry->namelen > *length)
			return B_BUFFER_OVERFLOW;

		fOffset = fOffset + EntrySize(dataEntry->namelen);
		memcpy(name, dataEntry->name, dataEntry->namelen);
		name[dataEntry->namelen] = '\0';
		*length = dataEntry->namelen + 1;
		*ino = B_BENDIAN_TO_HOST_INT64(dataEntry->inumber);

		TRACE("Entry found. Name: (%s), Length: (%ld),ino: (%ld)\n", name,
			*length, *ino);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
LeafDirectory::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	TRACE("LeafDirectory: Lookup\n");
	TRACE("Name: %s\n", name);
	uint32 hashValueOfRequest = hashfunction(name, length);
	TRACE("Hashval:(%ld)\n", hashValueOfRequest);

	status_t status;
	if (fLeafBuffer == NULL)
		status = FillBuffer(LEAF, fLeafBuffer, 0);
	if (status != B_OK)
		return status;

	ExtentLeafHeader* leafHeader = (ExtentLeafHeader*)(void*)fLeafBuffer;
	ExtentLeafEntry* leafEntry = FirstLeaf();
	if (leafEntry == NULL)
		return B_NO_MEMORY;

	int numberOfLeafEntries = B_BENDIAN_TO_HOST_INT16(leafHeader->count);
	TRACE("numberOfLeafEntries:(%d)\n", numberOfLeafEntries);
	int left = 0;
	int mid;
	int right = numberOfLeafEntries - 1;
	Volume* volume = fInode->GetVolume();

	/*
	* Trying to find the lowerbound of hashValueOfRequest
	* This is slightly different from bsearch(), as we want the first
	* instance of hashValueOfRequest and not any instance.
	*/
	while (left < right) {
		mid = (left+right)/2;
		uint32 hashval = B_BENDIAN_TO_HOST_INT32(leafEntry[mid].hashval);
		if (hashval >= hashValueOfRequest) {
			right = mid;
			continue;
		}
		if (hashval < hashValueOfRequest) {
			left = mid+1;
		}
	}
	TRACE("left:(%d), right:(%d)\n", left, right);

	while (B_BENDIAN_TO_HOST_INT32(leafEntry[left].hashval)
			== hashValueOfRequest) {

		uint32 address = B_BENDIAN_TO_HOST_INT32(leafEntry[left].address);
		if (address == 0) {
			left++;
			continue;
		}

		uint32 dataBlockNumber = BLOCKNO_FROM_ADDRESS(address * 8, volume);
		uint32 offset = BLOCKOFFSET_FROM_ADDRESS(address * 8, fInode);

		TRACE("DataBlockNumber:(%d), offset:(%d)\n", dataBlockNumber, offset);
		if (dataBlockNumber != fCurBlockNumber) {
			fCurBlockNumber = dataBlockNumber;
			SearchAndFillDataMap(dataBlockNumber);
			status = FillBuffer(DATA, fDataBuffer,
				dataBlockNumber - fDataMap->br_startoff);
			if (status != B_OK)
				return B_OK;
		}

		TRACE("offset:(%d)\n", offset);
		ExtentDataEntry* entry = (ExtentDataEntry*)(fDataBuffer + offset);

		int retVal = strncmp(name, (char*)entry->name, entry->namelen);
		if (retVal == 0) {
			*ino = B_BENDIAN_TO_HOST_INT64(entry->inumber);
			TRACE("ino:(%d)\n", *ino);
			return B_OK;
		}
		left++;
	}

	return B_ENTRY_NOT_FOUND;
}
