/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BPlusTree.h"

TreeDirectory::TreeDirectory(Inode* inode)
	:
	fInode(inode),
	fInitStatus(B_OK),
	fRoot(NULL),
	fExtents(NULL),
	fSingleDirBlock(NULL),
	fOffsetOfSingleDirBlock(-1),
	fCountOfFilledExtents(0),
	fCurMapIndex(0),
	fOffset(0),
	fCurBlockNumber(0)
{
	fRoot = new(std::nothrow) BlockInDataFork;
	if (fRoot == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fSingleDirBlock = new(std::nothrow) char[fInode->DirBlockSize()];
	if (fSingleDirBlock == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	memcpy((void*)fRoot,
		DIR_DFORK_PTR(fInode->Buffer()), sizeof(BlockInDataFork));

	for (int i = 0; i < MAX_TREE_DEPTH; i++) {
		fPathForLeaves[i].blockData = NULL;
		fPathForData[i].blockData = NULL;
	}
}


TreeDirectory::~TreeDirectory()
{
	delete fRoot;
	delete[] fExtents;
	delete[] fSingleDirBlock;
}


status_t
TreeDirectory::InitCheck()
{
	return fInitStatus;
}


int
TreeDirectory::BlockLen()
{
	return XFS_BTREE_LBLOCK_SIZE;
}


size_t
TreeDirectory::KeySize()
{
	return XFS_KEY_SIZE;
}


size_t
TreeDirectory::PtrSize()
{
	return XFS_PTR_SIZE;
}


size_t
TreeDirectory::MaxRecordsPossibleRoot()
{
	size_t lengthOfDataFork;
	if (fInode->ForkOffset() != 0)
		lengthOfDataFork = fInode->ForkOffset() << 3;
	if (fInode->ForkOffset() == 0) {
		lengthOfDataFork = fInode->GetVolume()->InodeSize()
			- INODE_CORE_UNLINKED_SIZE;
	}

	lengthOfDataFork -= sizeof(BlockInDataFork);
	return lengthOfDataFork / (KeySize() + PtrSize());
}


size_t
TreeDirectory::GetPtrOffsetIntoRoot(int pos)
{
	size_t maxRecords = MaxRecordsPossibleRoot();
	return sizeof(BlockInDataFork) + maxRecords * KeySize()
		+ (pos - 1) * PtrSize();
}


size_t
TreeDirectory::MaxRecordsPossibleNode()
{
	size_t availableSpace = fInode->GetVolume()->BlockSize() - BlockLen();
	return availableSpace / (KeySize() + PtrSize());
}


size_t
TreeDirectory::GetPtrOffsetIntoNode(int pos)
{
	size_t maxRecords = MaxRecordsPossibleNode();
	return BlockLen() + maxRecords * KeySize() + (pos - 1) * PtrSize();
}


TreePointer*
TreeDirectory::GetPtrFromRoot(int pos)
{
	return (TreePointer*)
		((char*)DIR_DFORK_PTR(fInode->Buffer()) + GetPtrOffsetIntoRoot(pos));
}


TreePointer*
TreeDirectory::GetPtrFromNode(int pos, void* buffer)
{
	size_t offsetIntoNode = GetPtrOffsetIntoNode(pos);
	return (TreePointer*)((char*)buffer + offsetIntoNode);
}


TreeKey*
TreeDirectory::GetKeyFromNode(int pos, void* buffer)
{
	return (TreeKey*)
		((char*)buffer + BlockLen() + (pos - 1) * KeySize());
}


TreeKey*
TreeDirectory::GetKeyFromRoot(int pos)
{
	off_t offset = (pos - 1) * KeySize();
	char* base = (char*)DIR_DFORK_PTR(fInode->Buffer())
		+ sizeof(BlockInDataFork);
	return (TreeKey*) (base + offset);
}


status_t
TreeDirectory::SearchOffsetInTreeNode(uint32 offset,
	TreePointer** pointer, int pathIndex)
{
	// This is O(MaxRecords). Next is to implement this using upper bound
	// binary search and get O(log(MaxRecords))
	*pointer = NULL;
	TreeKey* offsetKey = NULL;
	size_t maxRecords = MaxRecordsPossibleNode();
	for (int i = maxRecords - 1; i >= 0; i--) {
		offsetKey
			= GetKeyFromNode(i + 1, (void*)fPathForLeaves[pathIndex].blockData);
		if (B_BENDIAN_TO_HOST_INT64(*offsetKey) <= offset) {
			*pointer = GetPtrFromNode(i + 1, (void*)
				fPathForLeaves[pathIndex].blockData);

			break;
		}
	}

	if (offsetKey == NULL || *pointer == NULL)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
TreeDirectory::SearchAndFillPath(uint32 offset, int type)
{
	TRACE("SearchAndFillPath:\n");
	PathNode* path;
	if (type == DATA)
		path = fPathForData;
	else if (type == LEAF)
		path = fPathForLeaves;
	else
		return B_BAD_VALUE;

	TreePointer* ptrToNode = NULL;
	TreeKey* offsetKey = NULL;
	// Go down the root of the tree first
	for (int i = fRoot->NumRecords() - 1; i >= 0; i--) {
		offsetKey = GetKeyFromRoot(i + 1);
		if (B_BENDIAN_TO_HOST_INT64(*offsetKey) <= offset) {
			ptrToNode = GetPtrFromRoot(i + 1);
			break;
		}
	}
	if (ptrToNode == NULL || offsetKey == NULL) {
		//Corrupt tree
		return B_BAD_VALUE;
	}

	// We now have gone down the root and save path if not saved.
	int level = fRoot->Levels() - 1;
	Volume* volume = fInode->GetVolume();
	status_t status;
	for (int i = 0; i < MAX_TREE_DEPTH && level >= 0; i++, level--) {
		uint64 requiredBlock = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
		TRACE("requiredBlock:(%d)\n", requiredBlock);
		if (path[i].blockNumber == requiredBlock) {
			// This block already has what we need
			if (path[i].type == 2)
				break;
			status = SearchOffsetInTreeNode(offset, &ptrToNode, i);
			if (status != B_OK)
				return status;
			continue;
		}
		// We do not have the block we need
		size_t len;
		if (level == 0) {
			// The size of buffer should be the directory block size
			len = fInode->DirBlockSize();
			TRACE("path node type:(%d)\n", path[i].type);
			if (path[i].type != 2) {
				// Size is not directory block size.
				delete path[i].blockData;
				path[i].type = 0;
				path[i].blockData = new(std::nothrow) char[len];
				if (path[i].blockData == NULL)
					return B_NO_MEMORY;
				path[i].type = 2;
			}
			uint64 readPos = fInode->FileSystemBlockToAddr(requiredBlock);
			if (read_pos(volume->Device(), readPos, path[i].blockData, len)
				!= len) {
				ERROR("FillPath::FillBlockBuffer(): IO Error");
				return B_IO_ERROR;
			}
			path[i].blockNumber = requiredBlock;
			break;
		}
		// The size of buffer should be the block size
		len = volume->BlockSize();
		if (path[i].type != 1) {
			delete path[i].blockData;
			path[i].type = 0;
			path[i].blockData = new(std::nothrow) char[len];
			if (path[i].blockData == NULL)
				return B_NO_MEMORY;
			path[i].type = 1;
		}
		uint64 readPos = fInode->FileSystemBlockToAddr(requiredBlock);
		if (read_pos(volume->Device(), readPos, path[i].blockData, len)
			!= len) {
			ERROR("FillPath::FillBlockBuffer(): IO Error");
			return B_IO_ERROR;
		}
		path[i].blockNumber = requiredBlock;
		status = SearchOffsetInTreeNode(offset, &ptrToNode, i);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
TreeDirectory::GetAllExtents()
{
	xfs_extnum_t noOfExtents = fInode->DataExtentsCount();

	ExtentMapUnwrap* extentsWrapped
		= new(std::nothrow) ExtentMapUnwrap[noOfExtents];
	if (extentsWrapped == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<ExtentMapUnwrap> extentsWrappedDeleter(extentsWrapped);

	size_t maxRecords = MaxRecordsPossibleRoot();
	TRACE("Maxrecords: (%d)\n", maxRecords);

	Volume* volume = fInode->GetVolume();
	size_t len = volume->BlockSize();

	uint16 levelsInTree = fRoot->Levels();
	status_t status = fInode->GetNodefromTree(levelsInTree, volume, len,
		fInode->DirBlockSize(), fSingleDirBlock);
	if (status != B_OK)
		return status;

	// We should be at the left most leaf node.
	// This could be a multilevel node type directory
	uint64 fileSystemBlockNo;
	while (1) {
		// Run till you have leaf blocks to checkout
		char* leafBuffer = fSingleDirBlock;
		ASSERT(((LongBlock*)leafBuffer)->Magic() == XFS_BMAP_MAGIC);
		uint32 offset = sizeof(LongBlock);
		int numRecs = ((LongBlock*)leafBuffer)->NumRecs();

		for (int i = 0; i < numRecs; i++) {
			extentsWrapped[fCountOfFilledExtents].first =
				*(uint64*)(leafBuffer + offset);
			extentsWrapped[fCountOfFilledExtents].second =
				*(uint64*)(leafBuffer + offset + sizeof(uint64));
			offset += sizeof(ExtentMapUnwrap);
			fCountOfFilledExtents++;
		}

		fileSystemBlockNo = ((LongBlock*)leafBuffer)->Right();
		TRACE("Next leaf is at: (%d)\n", fileSystemBlockNo);
		if (fileSystemBlockNo == -1)
			break;
		uint64 readPos = fInode->FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, fSingleDirBlock, len)
				!= len) {
				ERROR("Extent::FillBlockBuffer(): IO Error");
				return B_IO_ERROR;
		}
	}
	TRACE("Total covered: (%d)\n", fCountOfFilledExtents);

	status = UnWrapExtents(extentsWrapped);

	return status;
}


status_t
TreeDirectory::FillBuffer(char* blockBuffer, int howManyBlocksFurther,
	ExtentMapEntry* targetMap)
{
	TRACE("FILLBUFFER\n");
	ExtentMapEntry map;
	if (targetMap == NULL)
		map = fExtents[fCurMapIndex];
	if (targetMap != NULL)
		map = *targetMap;

	if (map.br_state != 0)
		return B_BAD_VALUE;

	size_t len = fInode->DirBlockSize();
	if (blockBuffer == NULL) {
		blockBuffer = new(std::nothrow) char[len];
		if (blockBuffer == NULL)
			return B_NO_MEMORY;
	}

	xfs_daddr_t readPos = fInode->FileSystemBlockToAddr(
		map.br_startblock + howManyBlocksFurther);

	if (read_pos(fInode->GetVolume()->Device(), readPos, blockBuffer, len)
		!= len) {
		ERROR("TreeDirectory::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	if (targetMap == NULL) {
		fSingleDirBlock = blockBuffer;
		ExtentDataHeader* header = (ExtentDataHeader*) fSingleDirBlock;
		if (B_BENDIAN_TO_HOST_INT32(header->magic) == DATA_HEADER_MAGIC) {
			TRACE("DATA BLOCK VALID\n");
		} else {
			TRACE("DATA BLOCK INVALID\n");
			return B_BAD_VALUE;
		}
	}
	if (targetMap != NULL) {
		fSingleDirBlock = blockBuffer;
		ExtentLeafHeader* header = (ExtentLeafHeader*) fSingleDirBlock;
		if (B_BENDIAN_TO_HOST_INT16(header->info.magic) == XFS_DA_NODE_MAGIC
			|| B_BENDIAN_TO_HOST_INT16(header->info.magic)
				== XFS_DIR2_LEAFN_MAGIC) {
			TRACE("LEAF/NODE VALID\n");
		} else {
			TRACE("LEAF/NODE INVALID\n");
			return B_BAD_VALUE;
		}
	}
	return B_OK;
}


int
TreeDirectory::EntrySize(int len) const
{
	int entrySize = sizeof(xfs_ino_t) + sizeof(uint8) + len + sizeof(uint16);
		// uint16 is for the tag
	if (fInode->HasFileTypeField())
		entrySize += sizeof(uint8);

	return ROUNDUP(entrySize, 8);
		// rounding off to next greatest multiple of 8
}


/*
 * Throw in the desired block number and get the index of it
 */
status_t
TreeDirectory::SearchMapInAllExtent(int blockNo, uint32& mapIndex)
{
	ExtentMapEntry map;
	for (uint32 i = 0; i < fCountOfFilledExtents; i++) {
		map = fExtents[i];
		if (map.br_startoff <= blockNo
			&& (blockNo <= map.br_startoff + map.br_blockcount - 1)) {
			// Map found
			mapIndex = i;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
TreeDirectory::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	TRACE("TreeDirectory::GetNext\n");
	status_t status;
	if (fExtents == NULL) {
		status = GetAllExtents();
		if (status != B_OK)
			return status;
	}
	status = FillBuffer(fSingleDirBlock, 0);
	if (status != B_OK)
		return status;

	Volume* volume = fInode->GetVolume();
	void* entry = (void*)((ExtentDataHeader*)fSingleDirBlock + 1);
		// This could be an unused entry so we should check

	uint32 blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);
	if (fOffset != 0 && blockNoFromAddress == fCurBlockNumber) {
		entry = (void*)(fSingleDirBlock
			+ BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode));
		// This gets us a little faster to the next entry
	}

	uint32 curDirectorySize = fInode->Size();
	ExtentMapEntry& map = fExtents[fCurMapIndex];
	while (fOffset != curDirectorySize) {
		blockNoFromAddress = BLOCKNO_FROM_ADDRESS(fOffset, volume);

		TRACE("fOffset:(%d), blockNoFromAddress:(%d)\n",
			fOffset, blockNoFromAddress);
		if (fCurBlockNumber != blockNoFromAddress
			&& blockNoFromAddress > map.br_startoff
			&& blockNoFromAddress
				<= map.br_startoff + map.br_blockcount - 1) {
			// When the block is mapped in the same data
			// map entry but is not the first block
			status = FillBuffer(fSingleDirBlock,
				blockNoFromAddress - map.br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)((ExtentDataHeader*)fSingleDirBlock + 1);
			fOffset = fOffset + sizeof(ExtentDataHeader);
			fCurBlockNumber = blockNoFromAddress;
		} else if (fCurBlockNumber != blockNoFromAddress) {
			// When the block isn't mapped in the current data map entry
			uint32 curMapIndex;
			status = SearchMapInAllExtent(blockNoFromAddress, curMapIndex);
			if (status != B_OK)
				return status;
			fCurMapIndex = curMapIndex;
			map = fExtents[fCurMapIndex];
			status = FillBuffer(fSingleDirBlock,
				blockNoFromAddress - map.br_startoff);
			if (status != B_OK)
				return status;
			entry = (void*)((ExtentDataHeader*)fSingleDirBlock + 1);
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

		uint16 currentOffset = (char*)dataEntry - fSingleDirBlock;
		TRACE("GetNext: fOffset:(%d), currentOffset:(%d)\n",
			BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode), currentOffset);

		if (BLOCKOFFSET_FROM_ADDRESS(fOffset, fInode) > currentOffset) {
			entry = (void*)((char*)entry + EntrySize(dataEntry->namelen));
			continue;
		}

		if (dataEntry->namelen + 1 > *length)
			return B_BUFFER_OVERFLOW;

		fOffset = fOffset + EntrySize(dataEntry->namelen);
		memcpy(name, dataEntry->name, dataEntry->namelen);
		name[dataEntry->namelen] = '\0';
		*length = dataEntry->namelen + 1;
		*ino = B_BENDIAN_TO_HOST_INT64(dataEntry->inumber);

		TRACE("Entry found. Name: (%s), Length: (%ld), ino: (%ld)\n", name,
			*length, *ino);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
TreeDirectory::UnWrapExtents(ExtentMapUnwrap* extentsWrapped)
{
	fExtents = new(std::nothrow) ExtentMapEntry[fCountOfFilledExtents];
	if (fExtents == NULL)
		return B_NO_MEMORY;
	uint64 first, second;

	for (int i = 0; i < fCountOfFilledExtents; i++) {
		first = B_BENDIAN_TO_HOST_INT64(extentsWrapped[i].first);
		second = B_BENDIAN_TO_HOST_INT64(extentsWrapped[i].second);
		fExtents[i].br_state = first >> 63;
		fExtents[i].br_startoff = (first & MASK(63)) >> 9;
		fExtents[i].br_startblock = ((first & MASK(9)) << 43) | (second >> 21);
		fExtents[i].br_blockcount = second & MASK(21);
	}

	return B_OK;
}


void
TreeDirectory::FillMapEntry(int num, ExtentMapEntry** fMap,
	int type, int pathIndex)
{
	void* pointerToMap;
	if (type == DATA) {
		char* base = fPathForData[pathIndex].blockData + BlockLen();
		off_t offset = num * EXTENT_SIZE;
		pointerToMap = (void*)(base + offset);
	} else {
		char* base = fPathForLeaves[pathIndex].blockData + BlockLen();
		off_t offset = num * EXTENT_SIZE;
		pointerToMap = (void*)(base + offset);
	}
	uint64 firstHalf = *((uint64*)pointerToMap);
	uint64 secondHalf = *((uint64*)pointerToMap + 1);
		//dividing the 128 bits into 2 parts.

	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	(*fMap)->br_state = firstHalf >> 63;
	(*fMap)->br_startoff = (firstHalf & MASK(63)) >> 9;
	(*fMap)->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	(*fMap)->br_blockcount = secondHalf & MASK(21);
	TRACE("FillMapEntry: startoff:(%ld), startblock:(%ld), blockcount:(%ld),"
		"state:(%d)\n", (*fMap)->br_startoff, (*fMap)->br_startblock,
		(*fMap)->br_blockcount, (*fMap)->br_state);
}


void
TreeDirectory::SearchForMapInDirectoryBlock(int blockNo,
	int entries, ExtentMapEntry** map, int type, int pathIndex)
{
	TRACE("SearchForMapInDirectoryBlock: blockNo:(%d)\n", blockNo);
	for (int i = 0; i < entries; i++) {
		FillMapEntry(i, map, type, pathIndex);
		if ((*map)->br_startoff <= blockNo
			&& (blockNo <= (*map)->br_startoff + (*map)->br_blockcount - 1)) {
			// Map found
			return;
		}
	}
	// Map wasn't found. Some kind of corruption. This is checked by caller.
	*map = NULL;
}


uint32
TreeDirectory::SearchForHashInNodeBlock(uint32 hashVal)
{
	NodeHeader* header = (NodeHeader*)(fSingleDirBlock);
	NodeEntry* entry = (NodeEntry*)(fSingleDirBlock + sizeof(NodeHeader));
	int count = B_BENDIAN_TO_HOST_INT16(header->count);

	for (int i = 0; i < count; i++) {
		if (hashVal <= B_BENDIAN_TO_HOST_INT32(entry[i].hashval))
			return B_BENDIAN_TO_HOST_INT32(entry[i].before);
	}
	return 0;
}


status_t
TreeDirectory::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	TRACE("TreeDirectory: Lookup\n");
	TRACE("Name: %s\n", name);
	uint32 hashValueOfRequest = hashfunction(name, length);
	TRACE("Hashval:(%ld)\n", hashValueOfRequest);

	Volume* volume = fInode->GetVolume();
	status_t status;

	ExtentMapEntry* targetMap = new(std::nothrow) ExtentMapEntry;
	if (targetMap == NULL)
		return B_NO_MEMORY;
	int pathIndex = -1;
	uint32 rightOffset = LEAF_STARTOFFSET(volume->BlockLog());

	// In node directories, the "node blocks" had a single level
	// Here we could have multiple levels. With each iteration of
	// the loop we go a level lower.
	while (rightOffset != fOffsetOfSingleDirBlock && 1) {
		status = SearchAndFillPath(rightOffset, LEAF);
		if (status != B_OK)
			return status;

		// The path should now have the Tree Leaf at appropriate level
		// Find the directory block in the path
		for (int i = 0; i < MAX_TREE_DEPTH; i++) {
			if (fPathForLeaves[i].type == 2) {
				pathIndex = i;
				break;
			}
		}
		if (pathIndex == -1) {
			// corrupt tree
			return B_BAD_VALUE;
		}

		// Get the node block from directory block
		// If level is non-zero, reiterate with new "rightOffset"
		// Else, we are at leaf block, then break
		LongBlock* curDirBlock
			= (LongBlock*)fPathForLeaves[pathIndex].blockData;

		if (curDirBlock->Magic() != XFS_BMAP_MAGIC)
			return B_BAD_VALUE;

		SearchForMapInDirectoryBlock(rightOffset, curDirBlock->NumRecs(),
			&targetMap, LEAF, pathIndex);
		if (targetMap == NULL)
			return B_BAD_VALUE;

		FillBuffer(fSingleDirBlock, rightOffset - targetMap->br_startoff,
			targetMap);
		fOffsetOfSingleDirBlock = rightOffset;
		ExtentLeafHeader* dirBlock = (ExtentLeafHeader*)fSingleDirBlock;
		if (B_BENDIAN_TO_HOST_INT16(dirBlock->info.magic)
			== XFS_DIR2_LEAFN_MAGIC) {
			// Got the potential leaf. Break.
			break;
		}
		if (B_BENDIAN_TO_HOST_INT16(dirBlock->info.magic)
			== XFS_DA_NODE_MAGIC) {
			rightOffset = SearchForHashInNodeBlock(hashValueOfRequest);
			if (rightOffset == 0)
				return B_ENTRY_NOT_FOUND;
			continue;
		}
	}
	// We now have the leaf block that might contain the entry we need.
	// Else go to the right subling if it might contain it. Else break.
	while (1) {
		ExtentLeafHeader* leafHeader
			= (ExtentLeafHeader*)fSingleDirBlock;
		ExtentLeafEntry* leafEntry
			= (ExtentLeafEntry*)(fSingleDirBlock + sizeof(ExtentLeafHeader));

		int numberOfLeafEntries = B_BENDIAN_TO_HOST_INT16(leafHeader->count);
		TRACE("numberOfLeafEntries:(%d)\n", numberOfLeafEntries);
		int left = 0;
		int mid;
		int right = numberOfLeafEntries - 1;

		// Trying to find the lowerbound of hashValueOfRequest
		// This is slightly different from bsearch(), as we want the first
		// instance of hashValueOfRequest and not any instance.
		while (left < right) {
			mid = (left + right) / 2;
			uint32 hashval = B_BENDIAN_TO_HOST_INT32(leafEntry[mid].hashval);
			if (hashval >= hashValueOfRequest) {
				right = mid;
				continue;
			}
			if (hashval < hashValueOfRequest) {
				left = mid + 1;
			}
		}
		TRACE("left:(%d), right:(%d)\n", left, right);
		uint32 nextLeaf = B_BENDIAN_TO_HOST_INT32(leafHeader->info.forw);
		uint32 lastHashVal = B_BENDIAN_TO_HOST_INT32(
			leafEntry[numberOfLeafEntries - 1].hashval);

		while (B_BENDIAN_TO_HOST_INT32(leafEntry[left].hashval)
				== hashValueOfRequest) {
			uint32 address = B_BENDIAN_TO_HOST_INT32(leafEntry[left].address);
			if (address == 0) {
				left++;
				continue;
			}

			uint32 dataBlockNumber = BLOCKNO_FROM_ADDRESS(address * 8, volume);
			uint32 offset = BLOCKOFFSET_FROM_ADDRESS(address * 8, fInode);

			TRACE("BlockNumber:(%d), offset:(%d)\n", dataBlockNumber, offset);

			status = SearchAndFillPath(dataBlockNumber, DATA);
			int pathIndex = -1;
			for (int i = 0; i < MAX_TREE_DEPTH; i++) {
				if (fPathForData[i].type == 2) {
					pathIndex = i;
					break;
				}
			}
			if (pathIndex == -1)
				return B_BAD_VALUE;

			LongBlock* curDirBlock
				= (LongBlock*)fPathForData[pathIndex].blockData;

			SearchForMapInDirectoryBlock(dataBlockNumber,
				curDirBlock->NumRecs(), &targetMap, DATA, pathIndex);
			if (targetMap == NULL)
				return B_BAD_VALUE;

			FillBuffer(fSingleDirBlock,
				dataBlockNumber - targetMap->br_startoff, targetMap);
			fOffsetOfSingleDirBlock = dataBlockNumber;

			TRACE("offset:(%d)\n", offset);
			ExtentDataEntry* entry
				= (ExtentDataEntry*)(fSingleDirBlock + offset);

			int retVal = strncmp(name, (char*)entry->name, entry->namelen);
			if (retVal == 0) {
				*ino = B_BENDIAN_TO_HOST_INT64(entry->inumber);
				TRACE("ino:(%d)\n", *ino);
				return B_OK;
			}
			left++;
		}
		if (lastHashVal == hashValueOfRequest && nextLeaf != -1) {
			// Go to forward neighbor. We might find an entry there.
			status = SearchAndFillPath(nextLeaf, LEAF);
			if (status != B_OK)
				return status;

			pathIndex = -1;
			for (int i = 0; i < MAX_TREE_DEPTH; i++) {
				if (fPathForLeaves[i].type == 2) {
					pathIndex = i;
					break;
				}
			}
			if (pathIndex == -1)
				return B_BAD_VALUE;

			LongBlock* curDirBlock
				= (LongBlock*)fPathForLeaves[pathIndex].blockData;

			SearchForMapInDirectoryBlock(nextLeaf, curDirBlock->NumRecs(),
				&targetMap, LEAF, pathIndex);
			if (targetMap == NULL)
				return B_BAD_VALUE;

			FillBuffer(fSingleDirBlock,
				nextLeaf - targetMap->br_startoff, targetMap);
			fOffsetOfSingleDirBlock = nextLeaf;

			continue;
		} else {
			break;
		}
	}
	return B_ENTRY_NOT_FOUND;
}
