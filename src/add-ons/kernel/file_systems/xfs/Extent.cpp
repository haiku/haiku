/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Extent.h"


Extent::Extent(Inode* inode)
	:
	fInode(inode),
	fOffset(0)
{
}


Extent::~Extent()
{
}


void
Extent::FillMapEntry(void* pointerToMap)
{
	uint64 firstHalf = *((uint64*)pointerToMap);
	uint64 secondHalf = *((uint64*)pointerToMap + 1);
		//dividing the 128 bits into 2 parts.
	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = (firstHalf >> 63);
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = (secondHalf & MASK(21));
	TRACE("Extent::Init: startoff:(%ld), startblock:(%ld), blockcount:(%ld),"
			"state:(%d)\n",
		fMap->br_startoff,
		fMap->br_startblock,
		fMap->br_blockcount,
		fMap->br_state
		);
}


status_t
Extent::FillBlockBuffer()
{
	if (fMap->br_state != 0)
		return B_BAD_VALUE;

	int len = fInode->DirBlockSize();
	fBlockBuffer = new(std::nothrow) char[len];
	if (fBlockBuffer == NULL)
		return B_NO_MEMORY;

	Volume* volume = fInode->GetVolume();
	xfs_agblock_t numberOfBlocksInAg = volume->AgBlocks();

	uint64 agNo = FSBLOCKS_TO_AGNO(fMap->br_startblock, volume);
	uint64 agBlockNo = FSBLOCKS_TO_AGBLOCKNO(fMap->br_startblock, volume);
	xfs_fsblock_t blockToRead = FSBLOCKS_TO_BASICBLOCKS(volume->BlockLog(),
		((uint64)(agNo * numberOfBlocksInAg) + agBlockNo));

	xfs_daddr_t readPos = blockToRead * BASICBLOCKSIZE;

	TRACE("blockToRead: (%ld), readPos: (%ld)\n", blockToRead, readPos);
	if (read_pos(volume->Device(), readPos, fBlockBuffer, len) != len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
Extent::Init()
{
	fMap = new(std::nothrow) ExtentMapEntry;
	if (fMap == NULL)
		return B_NO_MEMORY;

	ASSERT(IsBlockType() == true);
	void* pointerToMap = DIR_DFORK_PTR(fInode->Buffer());
	FillMapEntry(pointerToMap);
	ASSERT(fMap->br_blockcount == 1);
		//TODO: This is always true for block directories
		//If we use this implementation for leaf directories, this is not
		//always true
	status_t status = FillBlockBuffer();
	ExtentDataHeader* header = (ExtentDataHeader*)fBlockBuffer;
	if (B_BENDIAN_TO_HOST_INT32(header->magic) == DIR2_BLOCK_HEADER_MAGIC) {
		status = B_OK;
		TRACE("Extent:Init(): Block read successfully\n");
	} else {
		status = B_BAD_VALUE;
		TRACE("Extent:Init(): Bad Block!\n");
	}

	return status;
}


ExtentBlockTail*
Extent::BlockTail()
{
	return (ExtentBlockTail*)
		(fBlockBuffer + fInode->DirBlockSize() - sizeof(ExtentBlockTail));
}


uint32
Extent::GetOffsetFromAddress(uint32 address)
{
	address = address * 8;
		// block offset in eight bytes, hence multiple with 8
	return address & (fInode->DirBlockSize() - 1);
}


ExtentLeafEntry*
Extent::BlockFirstLeaf(ExtentBlockTail* tail)
{
	return (ExtentLeafEntry*)tail - B_BENDIAN_TO_HOST_INT32(tail->count);
}


bool
Extent::IsBlockType()
{
	bool status = true;
	if (fInode->BlockCount() != 1)
		status = false;
	if (fInode->Size() != fInode->DirBlockSize())
		status = false;
	return status;
	//TODO: Checks: Fileoffset must be 0 and
	//length = directory block size / filesystem block size
}


int
Extent::EntrySize(int len) const
{
	int entrySize= sizeof(xfs_ino_t) + sizeof(uint8) + len + sizeof(uint16);
			// uint16 is for the tag
	if (fInode->HasFileTypeField())
		entrySize += sizeof(uint8);

	return (entrySize + 7) & -8;
			// rounding off to closest multiple of 8
}


status_t
Extent::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	TRACE("Extend::GetNext\n");
	void* entry = (void*)((ExtentDataHeader*)fBlockBuffer + 1);
		// This could be an unused entry so we should check

	int numberOfEntries = B_BENDIAN_TO_HOST_INT32(BlockTail()->count);
	TRACE("numberOfEntries:(%d)\n", numberOfEntries);

	for (int i = 0; i < numberOfEntries; i++) {
		TRACE("EntryNumber:(%d)\n", i);
		ExtentUnusedEntry* unusedEntry = (ExtentUnusedEntry*)entry;

		if (B_BENDIAN_TO_HOST_INT16(unusedEntry->freetag) == DIR2_FREE_TAG) {
			TRACE("Unused entry found\n");
			i--;
			entry = (void*)
				((char*)entry + B_BENDIAN_TO_HOST_INT16(unusedEntry->length));
			continue;
		}
		ExtentDataEntry* dataEntry = (ExtentDataEntry*) entry;

		uint16 currentOffset = (char*)dataEntry - fBlockBuffer;
		TRACE("GetNext: fOffset:(%d), currentOffset:(%d)\n",
			fOffset, currentOffset);

		if (fOffset >= currentOffset) {
			entry = (void*)((char*)entry + EntrySize(dataEntry->namelen));
			continue;
		}

		if (dataEntry->namelen > *length)
				return B_BUFFER_OVERFLOW;

		fOffset = currentOffset;
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
Extent::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	TRACE("Extent: Lookup\n");
	TRACE("Name: %s\n", name);
	uint32 hashValueOfRequest = hashfunction(name, length);
	TRACE("Hashval:(%ld)\n", hashValueOfRequest);
	ExtentBlockTail* blockTail = BlockTail();
	ExtentLeafEntry* leafEntry = BlockFirstLeaf(blockTail);

	int numberOfLeafEntries = B_BENDIAN_TO_HOST_INT32(blockTail->count);
	int left = 0;
	int mid;
	int right = numberOfLeafEntries - 1;

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

		uint32 offset = GetOffsetFromAddress(address);
		TRACE("offset:(%d)\n", offset);
		ExtentDataEntry* entry = (ExtentDataEntry*)(fBlockBuffer + offset);

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

