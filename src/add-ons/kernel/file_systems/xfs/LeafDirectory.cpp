/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LeafDirectory.h"

#include "VerifyHeader.h"


LeafDirectory::LeafDirectory(Inode* inode)
	:
	fInode(inode),
	fDataMap(NULL),
	fLeafMap(NULL),
	fOffset(0),
	fDataBuffer(NULL),
	fLeafBuffer(NULL),
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

	// check data buffer
	status_t status = FillBuffer(DATA, fDataBuffer, 0);
	if (status != B_OK)
		return status;
	ExtentDataHeader* data = CreateDataHeader(fInode, fDataBuffer);
	if (data == NULL)
		return B_NO_MEMORY;
	if (!VerifyHeader<ExtentDataHeader>(data, fDataBuffer, fInode, 0, fDataMap, XFS_LEAF)) {
		ERROR("Invalid data header");
		delete data;
		return B_BAD_VALUE;
	}
	delete data;

	// check leaf buffer
	status = FillBuffer(LEAF, fLeafBuffer, 0);
	if (status != B_OK)
		return status;
	ExtentLeafHeader* leaf = CreateLeafHeader(fInode, fLeafBuffer);
	if (leaf == NULL)
		return B_NO_MEMORY;
	if (!VerifyHeader<ExtentLeafHeader>(leaf, fLeafBuffer, fInode, 0, fLeafMap, XFS_LEAF)) {
		ERROR("Invalid leaf header");
		delete leaf;
		return B_BAD_VALUE;
	}
	delete leaf;

	return B_OK;
}


bool
LeafDirectory::IsLeafType()
{
	bool status = true;
	if (fInode->BlockCount() == 1
		|| fInode->DataExtentsCount() == 1
		|| (uint64) fInode->Size() !=
			(fInode->BlockCount() - 1) * fInode->DirBlockSize())
		status = false;

	if (status == false)
		return status;

	void* directoryFork = DIR_DFORK_PTR(fInode->Buffer(), fInode->CoreInodeSize());

	uint64* pointerToMap = (uint64*)((char*)directoryFork
		+ (fInode->DataExtentsCount() - 1) * EXTENT_SIZE);

	xfs_fileoff_t startoff = (B_BENDIAN_TO_HOST_INT64(*pointerToMap) & MASK(63)) >> 9;

	if (startoff != LEAF_STARTOFFSET(fInode->GetVolume()->BlockLog()))
		status = false;

	return status;
}


void
LeafDirectory::FillMapEntry(int num, ExtentMapEntry* fMap)
{
	void* directoryFork = DIR_DFORK_PTR(fInode->Buffer(), fInode->CoreInodeSize());

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
	TRACE("Extent::Init: startoff:(%" B_PRIu64 "), startblock:(%" B_PRIu64 "),"
		"blockcount:(%" B_PRIu64 "),state:(%" B_PRIu8 ")\n", fMap->br_startoff, fMap->br_startblock,
		fMap->br_blockcount, fMap->br_state);
}


status_t
LeafDirectory::FillBuffer(int type, char* blockBuffer, int howManyBlocksFurthur)
{
	TRACE("FILLBUFFER\n");
	ExtentMapEntry* map;
	if (type == DATA)
		map = fDataMap;
	else if (type == LEAF)
		map = fLeafMap;
	else
		return B_BAD_VALUE;

	if (map->br_state !=0)
		return B_BAD_VALUE;

	int len = fInode->DirBlockSize();
	if (blockBuffer == NULL) {
		blockBuffer = new(std::nothrow) char[len];
		if (blockBuffer == NULL)
			return B_NO_MEMORY;
	}

	xfs_daddr_t readPos =
		fInode->FileSystemBlockToAddr(map->br_startblock + howManyBlocksFurthur);

	if (read_pos(fInode->GetVolume()->Device(), readPos, blockBuffer, len)
		!= len) {
		ERROR("LeafDirectory::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	if (type == DATA) {
		fDataBuffer = blockBuffer;
	} else if (type == LEAF) {
		fLeafBuffer = blockBuffer;
		ExtentLeafHeader* header = CreateLeafHeader(fInode, fLeafBuffer);
		if (header == NULL)
			return B_NO_MEMORY;
		TRACE("NumberOfEntries in leaf: (%" B_PRIu16 ")\n", header->Count());
		delete header;
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
	return (ExtentLeafEntry*)((char*)fLeafBuffer + SizeOfLeafHeader(fInode));
}


int
LeafDirectory::EntrySize(int len) const
{
	int entrySize = sizeof(xfs_ino_t) + sizeof(uint8) + len + sizeof(uint16);
		// uint16 is for the tag
	if (fInode->HasFileTypeField())
		entrySize += sizeof(uint8);

	return (entrySize + 7) & -8;
		// rounding off to closest multiple of 8
}


void
LeafDirectory::SearchAndFillDataMap(uint64 blockNo)
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

	void* entry; // This could be unused entry so we should check

	entry = (void*)(fDataBuffer + SizeOfDataHeader(fInode));

	uint32 blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);
	if (fOffset != 0 && blockNoFromAddress == fCurBlockNumber)
		entry = (void*)(fDataBuffer
				+ BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode));
		// This gets us a little faster to the next entry

	uint32 curDirectorySize = fInode->Size();

	while (fOffset != curDirectorySize) {
		blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);

		TRACE("fOffset:(%" B_PRIu32 "), blockNoFromAddress:(%" B_PRIu32 ")\n",
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
			entry = (void*)(fDataBuffer + SizeOfDataHeader(fInode));
			fOffset = fOffset + SizeOfDataHeader(fInode);
			fCurBlockNumber = blockNoFromAddress;
		} else if (fCurBlockNumber != blockNoFromAddress) {
			// When the block isn't mapped in the current data map entry
			SearchAndFillDataMap(blockNoFromAddress);
			status = FillBuffer(DATA, fDataBuffer,
				blockNoFromAddress - fDataMap->br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)(fDataBuffer + SizeOfDataHeader(fInode));
			fOffset = fOffset + SizeOfDataHeader(fInode);
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
		TRACE("GetNext: fOffset:(%" B_PRIu32 "), currentOffset:(%" B_PRIu16 ")\n",
			BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode), currentOffset);

		if (BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode) > currentOffset) {
			entry = (void*)((char*)entry + EntrySize(dataEntry->namelen));
			continue;
		}

		if ((size_t)(dataEntry->namelen) >= *length)
			return B_BUFFER_OVERFLOW;

		fOffset = fOffset + EntrySize(dataEntry->namelen);
		memcpy(name, dataEntry->name, dataEntry->namelen);
		name[dataEntry->namelen] = '\0';
		*length = dataEntry->namelen + 1;
		*ino = B_BENDIAN_TO_HOST_INT64(dataEntry->inumber);

		TRACE("Entry found. Name: (%s), Length: (%" B_PRIuSIZE "),ino: (%" B_PRIu64 ")\n",
			name, *length, *ino);
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
	TRACE("Hashval:(%" B_PRIu32 ")\n", hashValueOfRequest);

	status_t status = B_OK;
	if (fLeafBuffer == NULL)
		status = FillBuffer(LEAF, fLeafBuffer, 0);
	if (status != B_OK)
		return status;

	ExtentLeafHeader* leafHeader = CreateLeafHeader(fInode, fLeafBuffer);
	if (leafHeader == NULL)
		return B_NO_MEMORY;

	ExtentLeafEntry* leafEntry = FirstLeaf();
	if (leafEntry == NULL)
		return B_NO_MEMORY;

	int numberOfLeafEntries = leafHeader->Count();
	TRACE("numberOfLeafEntries:(%" B_PRId32 ")\n", numberOfLeafEntries);
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
	TRACE("left:(%" B_PRId32 "), right:(%" B_PRId32 ")\n", left, right);

	while (B_BENDIAN_TO_HOST_INT32(leafEntry[left].hashval)
			== hashValueOfRequest) {

		uint32 address = B_BENDIAN_TO_HOST_INT32(leafEntry[left].address);
		if (address == 0) {
			left++;
			continue;
		}

		uint32 dataBlockNumber = BLOCKNO_FROM_ADDRESS(address * 8, volume);
		uint32 offset = BLOCKOFFSET_FROM_ADDRESS(address * 8, fInode);

		TRACE("DataBlockNumber:(%" B_PRIu32 "), offset:(%" B_PRIu32 ")\n", dataBlockNumber, offset);
		if (dataBlockNumber != fCurBlockNumber) {
			fCurBlockNumber = dataBlockNumber;
			SearchAndFillDataMap(dataBlockNumber);
			status = FillBuffer(DATA, fDataBuffer,
				dataBlockNumber - fDataMap->br_startoff);
			if (status != B_OK)
				return status;
		}

		TRACE("offset:(%" B_PRIu32 ")\n", offset);
		ExtentDataEntry* entry = (ExtentDataEntry*)(fDataBuffer + offset);

		int retVal = strncmp(name, (char*)entry->name, entry->namelen);
		if (retVal == 0) {
			*ino = B_BENDIAN_TO_HOST_INT64(entry->inumber);
			TRACE("ino:(%" B_PRIu64 ")\n", *ino);
			return B_OK;
		}
		left++;
	}

	delete leafHeader;

	return B_ENTRY_NOT_FOUND;
}


ExtentLeafHeader::~ExtentLeafHeader()
{
}


/*
	First see which type of directory we reading then
	return magic number as per Inode Version.
*/
uint32
ExtentLeafHeader::ExpectedMagic(int8 WhichDirectory, Inode* inode)
{
	if (WhichDirectory == XFS_LEAF) {
		if (inode->Version() == 1 || inode->Version() == 2)
			return V4_LEAF_HEADER_MAGIC;
		else
			return V5_LEAF_HEADER_MAGIC;
	} else {
		if (inode->Version() == 1 || inode->Version() == 2)
			return XFS_DIR2_LEAFN_MAGIC;
		else
			return XFS_DIR3_LEAFN_MAGIC;
	}
}


uint32
ExtentLeafHeader::CRCOffset()
{
	return XFS_LEAF_CRC_OFF - XFS_LEAF_V5_VPTR_OFF;
}


void
ExtentLeafHeaderV4::SwapEndian()
{
	info.forw	=	B_BENDIAN_TO_HOST_INT32(info.forw);
	info.back	=	B_BENDIAN_TO_HOST_INT32(info.back);
	info.magic	= 	B_BENDIAN_TO_HOST_INT16(info.magic);
	info.pad	=	B_BENDIAN_TO_HOST_INT16(info.pad);
	count		=	B_BENDIAN_TO_HOST_INT16(count);
	stale		=	B_BENDIAN_TO_HOST_INT16(stale);
}


ExtentLeafHeaderV4::ExtentLeafHeaderV4(const char* buffer)
{
	uint32 offset = 0;

	info = *(BlockInfo*)(buffer + offset);
	offset += sizeof(BlockInfo);

	count = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	stale = *(uint16*)(buffer + offset);

	SwapEndian();
}


ExtentLeafHeaderV4::~ExtentLeafHeaderV4()
{
}


uint16
ExtentLeafHeaderV4::Magic()
{
	return info.magic;
}


uint64
ExtentLeafHeaderV4::Blockno()
{
	return B_BAD_VALUE;
}


uint64
ExtentLeafHeaderV4::Lsn()
{
	return B_BAD_VALUE;
}


uint64
ExtentLeafHeaderV4::Owner()
{
	return B_BAD_VALUE;
}


uuid_t*
ExtentLeafHeaderV4::Uuid()
{
	return NULL;
}


uint16
ExtentLeafHeaderV4::Count()
{
	return count;
}


uint32
ExtentLeafHeaderV4::Forw()
{
	return info.forw;
}


void
ExtentLeafHeaderV5::SwapEndian()
{
	info.forw	=	B_BENDIAN_TO_HOST_INT32(info.forw);
	info.back	=	B_BENDIAN_TO_HOST_INT32(info.back);
	info.magic	= 	B_BENDIAN_TO_HOST_INT16(info.magic);
	info.pad	=	B_BENDIAN_TO_HOST_INT16(info.pad);
	info.blkno	=	B_BENDIAN_TO_HOST_INT64(info.blkno);
	info.lsn	=	B_BENDIAN_TO_HOST_INT64(info.lsn);
	info.owner	=	B_BENDIAN_TO_HOST_INT64(info.owner);
	count		=	B_BENDIAN_TO_HOST_INT16(count);
	stale		=	B_BENDIAN_TO_HOST_INT16(stale);
	pad			=	B_BENDIAN_TO_HOST_INT32(pad);
}


ExtentLeafHeaderV5::ExtentLeafHeaderV5(const char* buffer)
{
	uint32 offset = 0;

	info = *(BlockInfoV5*)(buffer + offset);
	offset += sizeof(BlockInfoV5);

	count = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	stale = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	pad = *(uint32*)(buffer + offset);

	SwapEndian();
}


ExtentLeafHeaderV5::~ExtentLeafHeaderV5()
{
}


uint16
ExtentLeafHeaderV5::Magic()
{
	return info.magic;
}


uint64
ExtentLeafHeaderV5::Blockno()
{
	return info.blkno;
}


uint64
ExtentLeafHeaderV5::Lsn()
{
	return info.lsn;
}


uint64
ExtentLeafHeaderV5::Owner()
{
	return info.owner;
}


uuid_t*
ExtentLeafHeaderV5::Uuid()
{
	return &info.uuid;
}


uint16
ExtentLeafHeaderV5::Count()
{
	return count;
}


uint32
ExtentLeafHeaderV5::Forw()
{
	return info.forw;
}


//Function to get V4 or V5 leaf header instance
ExtentLeafHeader*
CreateLeafHeader(Inode* inode, const char* buffer)
{
	if (inode->Version() == 1 || inode->Version() == 2) {
		ExtentLeafHeaderV4* header = new (std::nothrow) ExtentLeafHeaderV4(buffer);
		return header;
	} else {
		ExtentLeafHeaderV5* header = new (std::nothrow) ExtentLeafHeaderV5(buffer);
		return header;
	}
}


/*
	This Function returns Actual size of leaf header
	in all forms of directory.
	Never use sizeof() operator because we now have
	vtable as well and it will give wrong results
*/
uint32
SizeOfLeafHeader(Inode* inode)
{
	if (inode->Version() == 1 || inode->Version() == 2)
		return sizeof(ExtentLeafHeaderV4) - XFS_LEAF_V4_VPTR_OFF;
	else
		return sizeof(ExtentLeafHeaderV5) - XFS_LEAF_V5_VPTR_OFF;
}
