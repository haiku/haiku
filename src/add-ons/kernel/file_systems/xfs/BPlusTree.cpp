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
	fCountOfFilledExtents(0)
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
}


TreeDirectory::~TreeDirectory()
{
	delete fRoot;
	delete[] fExtents;
	delete fSingleDirBlock;
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


TreeKey
TreeDirectory::GetKey(int pos)
{
	return BlockLen() + (pos - 1) * XFS_KEY_SIZE;
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


TreePointer*
TreeDirectory::GetPtr(int pos, LongBlock* curLongBlock)
{
	size_t availableSpace = fInode->GetVolume()->BlockSize() - BlockLen();
	size_t maxRecords = availableSpace / (KeySize() + PtrSize());
	size_t offsetIntoNode =
		BlockLen() + maxRecords * KeySize() + (pos - 1) * PtrSize();
	return (TreePointer*)((char*)curLongBlock + offsetIntoNode);
}


size_t
TreeDirectory::MaxRecordsPossible(size_t len)
{
	len -= sizeof(BlockInDataFork);
	return len / (KeySize() + PtrSize());
}


status_t
TreeDirectory::GetAllExtents()
{
	xfs_extnum_t noOfExtents = fInode->DataExtentsCount();
	ExtentMapUnwrap* extentsWrapped
		= new(std::nothrow) ExtentMapUnwrap[noOfExtents];
	if (extentsWrapped == NULL)
		return B_NO_MEMORY;

	Volume* volume = fInode->GetVolume();
	uint16 levelsInTree = fRoot->Levels();

	size_t lengthOfDataFork;
	if (fInode->ForkOffset() != 0)
		lengthOfDataFork = fInode->ForkOffset() << 3;
	else
		lengthOfDataFork = volume->InodeSize() - INODE_CORE_UNLINKED_SIZE;

	TRACE("Length Of Data Fork: (%d)\n", lengthOfDataFork);
	size_t maxRecords = MaxRecordsPossible(lengthOfDataFork);
	TRACE("Maxrecords: (%d)\n", maxRecords);
	TreePointer* ptrToNode = (TreePointer*)
		((char*)DIR_DFORK_PTR(fInode->Buffer())
			+ sizeof(BlockInDataFork) + maxRecords*KeySize());

	size_t len = volume->BlockSize();
	char node[len];
		// This isn't for a directory block but for one of the tree nodes

	TRACE("levels:(%d)\n", levelsInTree);
	TRACE("Numrecs:(%d)\n", fRoot->NumRecords());

	// Go down the tree by taking the leftest pointer to go to the first leaf
	uint64 fileSystemBlockNo = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
	uint64 readPos = fInode->FileSystemBlockToAddr(fileSystemBlockNo);
	while (levelsInTree != 1) {
		fileSystemBlockNo = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
			// The fs block that contains node at next lower level. Now read.
		readPos = fInode->FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, node, len) != len) {
			ERROR("Extent::FillBlockBuffer(): IO Error");

			delete[] extentsWrapped;
			extentsWrapped = NULL;
			return B_IO_ERROR;
		}
		LongBlock* curLongBlock = (LongBlock*)node;
		ASSERT(curLongBlock->Magic() == XFS_BMAP_MAGIC);
		ptrToNode = GetPtr(1, curLongBlock);
			// Get's the first pointer. This points to next node.
		levelsInTree--;
	}

	// Next level wil contain leaf nodes. Now Read Directory Buffer
	len = fInode->DirBlockSize();
	if (read_pos(volume->Device(), readPos, fSingleDirBlock, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");

		delete[] extentsWrapped;
		extentsWrapped = NULL;
		return B_IO_ERROR;
	}
	levelsInTree--;
	ASSERT(levelsInTree == 0);

	// We should be at the left most leaf node.
	// This could be a multilevel node type directory
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

				delete[] extentsWrapped;
				extentsWrapped = NULL;
				return B_IO_ERROR;
		}
	}
	TRACE("Total covered: (%d)\n", fCountOfFilledExtents);

	status_t status = UnWrapExtents(extentsWrapped);

	delete[] extentsWrapped;
	extentsWrapped = NULL;

	return status;
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
