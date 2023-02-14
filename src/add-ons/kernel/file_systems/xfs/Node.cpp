/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Node.h"

#include "VerifyHeader.h"


NodeDirectory::NodeDirectory(Inode* inode)
	:
	fInode(inode),
	fDataMap(NULL),
	fLeafMap(NULL),
	fOffset(0),
	fDataBuffer(NULL),
	fLeafBuffer(NULL),
	fCurBlockNumber(-1)
{
	fFirstLeafMapIndex = FirstLeafMapIndex();
}


NodeDirectory::~NodeDirectory()
{
	delete fDataMap;
	delete fLeafMap;
	delete fDataBuffer;
	delete fLeafBuffer;
}


xfs_extnum_t
NodeDirectory::FirstLeafMapIndex()
{
	int len = fInode->DataExtentsCount();

	for (int i = 0; i < len; i++) {
		void* directoryFork = DIR_DFORK_PTR(fInode->Buffer(), fInode->CoreInodeSize());
		void* pointerToMap = (void*)((char*)directoryFork + i * EXTENT_SIZE);

		uint64 startoff = *((uint64*)pointerToMap);
		startoff = B_BENDIAN_TO_HOST_INT64(startoff);
		startoff = (startoff & MASK(63)) >> 9;

		// First Leaf Map found
		if (startoff == LEAF_STARTOFFSET(fInode->GetVolume()->BlockLog()))
			return i;
	}

	// No Leaf
	return -1;
}


status_t
NodeDirectory::Init()
{
	TRACE("Node directory : init()\n");
	fLeafMap = new(std::nothrow) ExtentMapEntry;
	if (fLeafMap == NULL)
		return B_NO_MEMORY;

	fDataMap = new(std::nothrow) ExtentMapEntry;
	if (fDataMap == NULL)
		return B_NO_MEMORY;

	FillMapEntry(fFirstLeafMapIndex, fLeafMap);
	fCurLeafMapNumber = 1;
	FillMapEntry(0, fDataMap);
	return B_OK;
}


bool
NodeDirectory::IsNodeType()
{
	if (fFirstLeafMapIndex == -1)
		return false;
	return true;
}


void
NodeDirectory::FillMapEntry(int num, ExtentMapEntry* fMap)
{
	void* directoryFork = DIR_DFORK_PTR(fInode->Buffer(), fInode->CoreInodeSize());
	void* pointerToMap = (void*)((char*)directoryFork + num * EXTENT_SIZE);
	uint64 firstHalf = *((uint64*)pointerToMap);
	uint64 secondHalf = *((uint64*)pointerToMap + 1);
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
NodeDirectory::FillBuffer(int type, char* blockBuffer, int howManyBlocksFurthur)
{
	TRACE("FILLBUFFER\n");
	ExtentMapEntry* map;
	if (type == DATA)
		map = fDataMap;
	else if (type == LEAF)
		map = fLeafMap;
	else
		return B_BAD_VALUE;

	if (map->br_state != 0)
		return B_BAD_VALUE;

	ssize_t len = fInode->DirBlockSize();
	if (blockBuffer == NULL) {
		blockBuffer = new(std::nothrow) char[len];
		if (blockBuffer == NULL)
			return B_NO_MEMORY;
	}

	xfs_daddr_t readPos = fInode->FileSystemBlockToAddr(map->br_startblock
		+ howManyBlocksFurthur);

	if (read_pos(fInode->GetVolume()->Device(), readPos, blockBuffer, len)
		!= len) {
		ERROR("NodeDirectory::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	if (type == DATA) {
		fDataBuffer = blockBuffer;
		ExtentDataHeader* header = ExtentDataHeader::Create(fInode, fDataBuffer);
		if(header == NULL)
			return B_NO_MEMORY;
		if (!VerifyHeader<ExtentDataHeader>(header, fDataBuffer, fInode,
				howManyBlocksFurthur, fDataMap, XFS_NODE)) {
			ERROR("DATA BLOCK INVALID\n");
			delete header;
			return B_BAD_VALUE;
		}
		delete header;

	} else if (type == LEAF) {
		fLeafBuffer = blockBuffer;
		/*
			This could be leaf or node block perform check for both
			based on magic number found.
		*/
		ExtentLeafHeader* leaf = ExtentLeafHeader::Create(fInode, fLeafBuffer);
		if (leaf == NULL)
			return B_NO_MEMORY;

		if ((leaf->Magic() == XFS_DIR2_LEAFN_MAGIC
				|| leaf->Magic() == XFS_DIR3_LEAFN_MAGIC)
			&& !VerifyHeader<ExtentLeafHeader>(leaf, fLeafBuffer, fInode,
				howManyBlocksFurthur, fLeafMap, XFS_NODE)) {
			TRACE("Leaf block invalid");
			delete leaf;
			return B_BAD_VALUE;
		}
		delete leaf;
		leaf = NULL;

		NodeHeader* node = NodeHeader::Create(fInode, fLeafBuffer);
		if (node == NULL)
			return B_NO_MEMORY;

		if ((node->Magic() == XFS_DA_NODE_MAGIC
				|| node->Magic() == XFS_DA3_NODE_MAGIC)
			&& !VerifyHeader<NodeHeader>(node, fLeafBuffer, fInode,
				howManyBlocksFurthur, fLeafMap, XFS_NODE)) {
			TRACE("Node block invalid");
			delete node;
			return B_BAD_VALUE;
		}
		delete node;
	}

	return B_OK;
}


uint32
NodeDirectory::GetOffsetFromAddress(uint32 address)
{
	address = address * 8;
		// block offset in eight bytes, hence multiple with 8
	return address & (fInode->DirBlockSize() - 1);
}


status_t
NodeDirectory::FindHashInNode(uint32 hashVal, uint32* rightMapOffset)
{
	NodeHeader* header = NodeHeader::Create(fInode, fLeafBuffer);
	if (header == NULL)
		return B_NO_MEMORY;

	NodeEntry* entry = (NodeEntry*)(void*)(fLeafBuffer + NodeHeader::Size(fInode));
	int count = header->Count();
	delete header;
	if ((NodeEntry*)(void*)fLeafBuffer + fInode->DirBlockSize()
		< &entry[count]) {
			return B_BAD_VALUE;
	}

	for (int i = 0; i < count; i++) {
		if (hashVal <= B_BENDIAN_TO_HOST_INT32(entry[i].hashval)) {
			*rightMapOffset = B_BENDIAN_TO_HOST_INT32(entry[i].before);
			return B_OK;
		}
	}

	*rightMapOffset = 1;
	return B_OK;
}


int
NodeDirectory::EntrySize(int len) const
{
	int entrySize = sizeof(xfs_ino_t) + sizeof(uint8) + len + sizeof(uint16);
			// uint16 is for the tag
	if (fInode->HasFileTypeField())
		entrySize += sizeof(uint8);

	return ROUNDUP(entrySize, 8);
			// rounding off to closest multiple of 8
}


void
NodeDirectory::SearchAndFillDataMap(uint64 blockNo)
{
	int len = fInode->DataExtentsCount();

	for (int i = 0; i <= len - fFirstLeafMapIndex; i++) {
		FillMapEntry(i, fDataMap);
		if (fDataMap->br_startoff <= blockNo
			&& (blockNo <= fDataMap->br_startoff + fDataMap->br_blockcount - 1))
			// Map found
			return;
	}
}


status_t
NodeDirectory::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	TRACE("NodeDirectory::GetNext\n");
	status_t status;

	if (fDataBuffer == NULL) {
		status = FillBuffer(DATA, fDataBuffer, 0);
		if (status != B_OK)
			return status;
	}

	Volume* volume = fInode->GetVolume();

	void* entry; // This could be unused entry so we should check

	entry = (void*)(fDataBuffer + ExtentDataHeader::Size(fInode));

	uint32 blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);
	if (fOffset != 0 && blockNoFromAddress == fCurBlockNumber) {
		entry = (void*)(fDataBuffer
			+ BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode));
		// This gets us a little faster to the next entry
	}

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
			entry = (void*)(fDataBuffer + ExtentDataHeader::Size(fInode));
			fOffset = fOffset + ExtentDataHeader::Size(fInode);
			fCurBlockNumber = blockNoFromAddress;
		} else if (fCurBlockNumber != blockNoFromAddress) {
			// When the block isn't mapped in the current data map entry
			SearchAndFillDataMap(blockNoFromAddress);
			status = FillBuffer(DATA, fDataBuffer,
				blockNoFromAddress - fDataMap->br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)(fDataBuffer + ExtentDataHeader::Size(fInode));
			fOffset = fOffset + ExtentDataHeader::Size(fInode);
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
			name,*length, *ino);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
NodeDirectory::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	TRACE("NodeDirectory: Lookup\n");
	TRACE("Name: %s\n", name);
	uint32 hashValueOfRequest = hashfunction(name, length);
	TRACE("Hashval:(%" B_PRIu32 ")\n", hashValueOfRequest);

	status_t status;
	if (fCurLeafBufferNumber != 1) {
		if (fCurLeafMapNumber != 1) {
			FillMapEntry(fFirstLeafMapIndex, fLeafMap);
			fCurLeafMapNumber = 1;
		}
		status = FillBuffer(LEAF, fLeafBuffer, 0);
		if (status != B_OK)
			return status;
		fCurLeafBufferNumber = 1;
	}
	/* Leaf now has the nodes. */
	uint32 rightMapOffset;
	status = FindHashInNode(hashValueOfRequest, &rightMapOffset);
	if(status != B_OK)
		return status;

	if (rightMapOffset == 1) {
		TRACE("Not in this directory.\n");
		return B_ENTRY_NOT_FOUND;
	}

	TRACE("rightMapOffset:(%" B_PRIu32 ")\n", rightMapOffset);

	for(int i = fFirstLeafMapIndex; i < fInode->DataExtentsCount(); i++)
	{
		FillMapEntry(i, fLeafMap);
		fCurLeafMapNumber = 2;
		status = FillBuffer(LEAF, fLeafBuffer,
			rightMapOffset - fLeafMap->br_startoff);
		if (status != B_OK)
			return status;
		fCurLeafBufferNumber = 2;
		ExtentLeafHeader* leafHeader = ExtentLeafHeader::Create(fInode, fLeafBuffer);
		if(leafHeader == NULL)
			return B_NO_MEMORY;
		ExtentLeafEntry* leafEntry =
			(ExtentLeafEntry*)(void*)(fLeafBuffer + ExtentLeafHeader::Size(fInode));
		if (leafEntry == NULL)
			return B_NO_MEMORY;

		int numberOfLeafEntries = leafHeader->Count();
		TRACE("numberOfLeafEntries:(%" B_PRId32 ")\n", numberOfLeafEntries);
		int left = 0;
		int right = numberOfLeafEntries - 1;
		Volume* volume = fInode->GetVolume();

		hashLowerBound<ExtentLeafEntry>(leafEntry, left, right, hashValueOfRequest);

		while (B_BENDIAN_TO_HOST_INT32(leafEntry[left].hashval)
				== hashValueOfRequest) {

			uint32 address = B_BENDIAN_TO_HOST_INT32(leafEntry[left].address);
			if (address == 0) {
				left++;
				continue;
			}

			uint32 dataBlockNumber = BLOCKNO_FROM_ADDRESS(address * 8, volume);
			uint32 offset = BLOCKOFFSET_FROM_ADDRESS(address * 8, fInode);

			TRACE("DataBlockNumber:(%" B_PRIu32 "), offset:(%" B_PRIu32 ")\n",
				dataBlockNumber, offset);
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
	}

	return B_ENTRY_NOT_FOUND;
}


NodeHeader::~NodeHeader()
{
}


/*
	In NodeHeader we have same magic number for all forms of
	directory so return magic number as per Inode Version.
*/
uint32
NodeHeader::ExpectedMagic(int8 WhichDirectory, Inode* inode)
{
	if (inode->Version() == 1 || inode->Version() == 2)
		return XFS_DA_NODE_MAGIC;
	else
		return XFS_DA3_NODE_MAGIC;
}


uint32
NodeHeader::CRCOffset()
{
	return offsetof(NodeHeaderV5::OnDiskData, info.crc);
}


NodeHeader*
NodeHeader::Create(Inode* inode, const char* buffer)
{
	if (inode->Version() == 1 || inode->Version() == 2) {
		NodeHeaderV4* header = new (std::nothrow) NodeHeaderV4(buffer);
		return header;
	} else {
		NodeHeaderV5* header = new (std::nothrow) NodeHeaderV5(buffer);
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
NodeHeader::Size(Inode* inode)
{
	if (inode->Version() == 1 || inode->Version() == 2)
		return sizeof(NodeHeaderV4::OnDiskData);
	else
		return sizeof(NodeHeaderV5::OnDiskData);
}


void
NodeHeaderV4::SwapEndian()
{
	fData.info.forw	=	B_BENDIAN_TO_HOST_INT32(fData.info.forw);
	fData.info.back	=	B_BENDIAN_TO_HOST_INT32(fData.info.back);
	fData.info.magic	= 	B_BENDIAN_TO_HOST_INT16(fData.info.magic);
	fData.info.pad	=	B_BENDIAN_TO_HOST_INT16(fData.info.pad);
	fData.count		=	B_BENDIAN_TO_HOST_INT16(fData.count);
	fData.level		=	B_BENDIAN_TO_HOST_INT16(fData.level);
}


NodeHeaderV4::NodeHeaderV4(const char* buffer)
{
	uint32 offset = 0;

	fData.info = *(BlockInfo*)(buffer + offset);
	offset += sizeof(BlockInfo);

	fData.count = *(uint16*)(buffer + offset);
	offset += sizeof(uint16);

	fData.level = *(uint16*)(buffer + offset);

	SwapEndian();
}


NodeHeaderV4::~NodeHeaderV4()
{
}


uint16
NodeHeaderV4::Magic()
{
	return fData.info.magic;
}


uint64
NodeHeaderV4::Blockno()
{
	return B_BAD_VALUE;
}


uint64
NodeHeaderV4::Lsn()
{
	return B_BAD_VALUE;
}


uint64
NodeHeaderV4::Owner()
{
	return B_BAD_VALUE;
}


const uuid_t&
NodeHeaderV4::Uuid()
{
	static uuid_t nullUuid = {0};
	return nullUuid;
}


uint16
NodeHeaderV4::Count()
{
	return fData.count;
}


void
NodeHeaderV5::SwapEndian()
{
	fData.info.forw		=	B_BENDIAN_TO_HOST_INT32(fData.info.forw);
	fData.info.back		=	B_BENDIAN_TO_HOST_INT32(fData.info.back);
	fData.info.magic	= 	B_BENDIAN_TO_HOST_INT16(fData.info.magic);
	fData.info.pad		=	B_BENDIAN_TO_HOST_INT16(fData.info.pad);
	fData.info.blkno	=	B_BENDIAN_TO_HOST_INT64(fData.info.blkno);
	fData.info.lsn		=	B_BENDIAN_TO_HOST_INT64(fData.info.lsn);
	fData.info.owner	=	B_BENDIAN_TO_HOST_INT64(fData.info.owner);
	fData.count			=	B_BENDIAN_TO_HOST_INT16(fData.count);
	fData.level			=	B_BENDIAN_TO_HOST_INT16(fData.level);
	fData.pad32			=	B_BENDIAN_TO_HOST_INT32(fData.pad32);
}


NodeHeaderV5::NodeHeaderV5(const char* buffer)
{
	memcpy(&fData, buffer, sizeof(fData));
	SwapEndian();
}


NodeHeaderV5::~NodeHeaderV5()
{
}


uint16
NodeHeaderV5::Magic()
{
	return fData.info.magic;
}


uint64
NodeHeaderV5::Blockno()
{
	return fData.info.blkno;
}


uint64
NodeHeaderV5::Lsn()
{
	return fData.info.lsn;
}


uint64
NodeHeaderV5::Owner()
{
	return fData.info.owner;
}


const uuid_t&
NodeHeaderV5::Uuid()
{
	return fData.info.uuid;
}


uint16
NodeHeaderV5::Count()
{
	return fData.count;
}
