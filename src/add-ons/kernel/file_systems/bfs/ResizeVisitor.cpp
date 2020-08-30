/*
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com
 * Copyright (C) 2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */


//! File system resizing


#include "ResizeVisitor.h"

#include "BPlusTree.h"
#include "Index.h"
#include "Inode.h"
#include "Journal.h"


ResizeVisitor::ResizeVisitor(Volume* volume)
	:
	FileSystemVisitor(volume)
{
}


ResizeVisitor::~ResizeVisitor()
{
	// reset the allocation range
	GetVolume()->Allocator().SetAllowedRange(0, 0);
}


status_t
ResizeVisitor::Resize(off_t size, disk_job_id job)
{
	_CalculateNewSizes(size);

	status_t status = _IsResizePossible(size);
	if (status != B_OK)
		return status;

	if (fNumBlocks == GetVolume()->NumBlocks()) {
		INFORM(("Resize: New size was equal to old.\n"));
		return B_OK;
	}

	// make sure no unsynced transaction causes invalidated blocks to linger
	// in the block cache when we write data with write_pos
	// TODO: how does this work when the file system is mounted?
	status = GetVolume()->GetJournal(0)->FlushLogAndBlocks();
	if (status != B_OK) {
		FATAL(("Resize: Failed to flush log!\n"));
		return status;
	}

	update_disk_device_job_progress(job, 0.0);

	Start(VISIT_REGULAR | VISIT_INDICES | VISIT_REMOVED
		| VISIT_ATTRIBUTE_DIRECTORIES);

	GetVolume()->Allocator().SetAllowedRange(fBeginBlock, fEndBlock);
		// this is reset in our destructor

	// move file system data out of the way
	while (true) {
		fError = false;

		status = Next();
		if (fError)
			return status;

		if (status == B_ENTRY_NOT_FOUND)
			break;
	}

	// move log and change file system size in the superblock
	if (fShrinking) {
		status = _ResizeVolume();
		if (status != B_OK) {
			FATAL(("Resize: Failed to update file system size!\n"));
			return status;
		}

		status = GetVolume()->GetJournal(0)->MoveLog(fNewLog);
		if (status != B_OK) {
			FATAL(("Resize: Failed to move the log area!\n"));
			return status;
		}
	} else {
		status = GetVolume()->GetJournal(0)->MoveLog(fNewLog);
		if (status != B_OK) {
			FATAL(("Resize: Failed to move the log area!\n"));
			return status;
		}

		status = _ResizeVolume();
		if (status != B_OK) {
			FATAL(("Resize: Failed to update file system size!\n"));
			return status;
		}
	}

	update_disk_device_job_progress(job, 1.0);
	return B_OK;
}


status_t
ResizeVisitor::VisitInode(Inode* inode, const char* treeName)
{
	// get the name so we can show it to the user if something goes wrong
	// TODO: we don't really have to do this for every inode we visit
	char name[B_FILE_NAME_LENGTH];

	if (treeName == NULL) {
		if (inode->GetName(name) < B_OK) {
			if (inode->IsContainer())
				strcpy(name, "(dir has no name)");
			else
				strcpy(name, "(node has no name)");
		}
	} else
		strcpy(name, treeName);

	off_t inodeBlock = inode->BlockNumber();

	// start by moving the inode so we can place the stream close to it
	// if possible
	if (inodeBlock < fBeginBlock || inodeBlock >= fEndBlock) {
// 		TODO: this kernel API does not exist anymore/yet
//		status_t status = mark_vnode_busy(GetVolume()->FSVolume(), inode->ID(), true);

//		ino_t oldInodeID = inode->ID();
		off_t newInodeID;

		status_t status = _MoveInode(inode, newInodeID, treeName);
		if (status != B_OK) {
//			mark_vnode_busy(GetVolume()->FSVolume(), inode->ID(), false);
			FATAL(("Resize: Failed to move inode %" B_PRIdINO ", \"%s\"!\n", inode->ID(), name));
			fError = true;
			return status;
		}

//		status = change_vnode_id(GetVolume()->FSVolume(), oldInodeID,
//			newInodeID);
//		if (status != B_OK) {
//			mark_vnode_busy(GetVolume()->FSVolume(), inode->ID(), false);
//			FATAL(("Resize: Failed to change ID in vnode, inode %" B_PRIdINO
//				", \"%s\"!\n", inode->ID(), name));
//			fError = true;
//			return status;
//		}

		inode->SetID(newInodeID);

		// accessing the inode with the new ID
//		mark_vnode_busy(GetVolume()->FSVolume(), inode->ID(), false);
	}

	// move the stream if necessary
	bool inRange;
	status_t status = inode->StreamCompletelyInsideAllowedRange(inRange);
	if (status != B_OK) {
		FATAL(("Resize: Failed to check file stream, inode %" B_PRIdINO
			", \"%s\"!\n", inode->ID(), name));
		fError = true;
		return status;
	}

	if (!inRange) {
		status = inode->MoveStream();
		if (status != B_OK) {
			FATAL(("Resize: Failed to move file stream, inode %" B_PRIdINO
				", \"%s\"!\n", inode->ID(), name));
			fError = true;
			return status;
		}
	}

	return B_OK;
}


status_t
ResizeVisitor::OpenInodeFailed(status_t reason, ino_t id, Inode* parent,
	char* treeName, TreeIterator* iterator)
{
	FATAL(("Could not open inode at %" B_PRIdOFF "\n", id));

	fError = true;
	return reason;
}


status_t
ResizeVisitor::OpenBPlusTreeFailed(Inode* inode)
{
	FATAL(("Could not open b+tree from inode at %" B_PRIdOFF "\n",
		inode->ID()));

	fError = true;
	return B_ERROR;
}


status_t
ResizeVisitor::TreeIterationFailed(status_t reason, Inode* parent)
{
	FATAL(("Tree iteration failed in parent at %" B_PRIdOFF "\n",
		parent->ID()));

	fError = true;
	return reason;
}


void
ResizeVisitor::_CalculateNewSizes(off_t size)
{
	uint32 blockSize = GetVolume()->BlockSize();
	uint32 blockShift = GetVolume()->BlockShift();

	fNumBlocks = size >> blockShift;

	fBitmapBlocks = (fNumBlocks + blockSize * 8 - 1) / (blockSize * 8);
		// divide total number of blocks by the number of bits in a bitmap
		// block, rounding up

	off_t logLength = Volume::CalculateLogSize(fNumBlocks, size);

	fNewLog.SetTo(0, 1 + fBitmapBlocks, logLength);
	fReservedLength = 1 + fBitmapBlocks + logLength;

	fBeginBlock = fReservedLength;

	if (fNumBlocks < GetVolume()->NumBlocks()) {
		fShrinking = true;
		fEndBlock = fNumBlocks;
	} else {
		fShrinking = false;
		fEndBlock = GetVolume()->NumBlocks();
	}
}


status_t
ResizeVisitor::_IsResizePossible(off_t size)
{
	if ((size % GetVolume()->BlockSize()) != 0) {
		FATAL(("Resize: New size not multiple of block size!\n"));
		return B_BAD_VALUE;
	}

	// the new size is limited by what we can fit into the first allocation
	// group
	if (fReservedLength > (off_t)(1UL << GetVolume()->AllocationGroupShift())) {
		FATAL(("Resize: Reserved area is too large for allocation group!\n"));
		return B_BAD_VALUE;
	}

	if (GetVolume()->UsedBlocks() > fNumBlocks) {
		FATAL(("Resize: Not enough free space for resize!\n"));
		return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
ResizeVisitor::_ResizeVolume()
{
	status_t status;
	BlockAllocator& allocator = GetVolume()->Allocator();

	// check that the end blocks are free
	if (fNumBlocks < GetVolume()->NumBlocks()) {
		status = allocator.CheckBlocks(fNumBlocks,
			GetVolume()->NumBlocks() - fNumBlocks, false);
		if (status != B_OK)
			return status;
	}

	// make sure we have space to resize the bitmap
	if (1 + fBitmapBlocks > GetVolume()->Log().Start())
		return B_ERROR;

	// clear bitmap blocks - aTODO maybe not use a transaction?
	Transaction transaction(GetVolume(), 0);

	CachedBlock cached(GetVolume());
	for (off_t block = 1 + GetVolume()->NumBitmapBlocks();
		block < 1 + fBitmapBlocks; block++) {
		status = cached.SetToWritable(transaction, block);
		if (status != B_OK)
			return status;
		uint8* buffer = cached.WritableBlock();

		memset(buffer, 0, GetVolume()->BlockSize());
	}
	cached.Unset();

	status = transaction.Done();
	if (status != B_OK)
		return status;

	// change size of block cache
	status = GetVolume()->ResizeBlockCache(fNumBlocks);
	if (status != B_OK)
		return status;

	// we flush the log before updating file system size, so nothing
	// on the disk remains unmoved
	status = GetVolume()->GetJournal(0)->FlushLogAndBlocks();
	if (status != B_OK)
		return status;

	// update superblock and volume information
	disk_super_block& superBlock = GetVolume()->SuperBlock();

	uint32 groupShift = GetVolume()->AllocationGroupShift();
	int32 blocksPerGroup = 1L << groupShift;

	// used to revert the volume super_block changes if we can't write them
	// to disk, so we don't have to convert to and from host endianess
	int64 oldNumBlocks = superBlock.num_blocks;
	int32 oldNumAgs = superBlock.num_ags;

	superBlock.num_blocks = HOST_ENDIAN_TO_BFS_INT64(fNumBlocks);
	superBlock.num_ags = HOST_ENDIAN_TO_BFS_INT32(
			(fNumBlocks + blocksPerGroup - 1) >> groupShift);

	status = GetVolume()->WriteSuperBlock();
	if (status != B_OK) {
		superBlock.num_blocks = oldNumBlocks;
		superBlock.num_ags = oldNumAgs;
		return status;
	}

	// reinitialize block allocator
	status = GetVolume()->Allocator().Reinitialize();
	if (status != B_OK)
		return status;

	return B_OK;
}


status_t
ResizeVisitor::_UpdateParent(Transaction& transaction, Inode* inode,
	off_t newInodeID, const char* treeName)
{
	// get Inode of parent
	Vnode parentVnode(GetVolume(), inode->Parent());
	Inode* parent;
	status_t status = parentVnode.Get(&parent);
	if (status != B_OK)
		return status;

	parent->WriteLockInTransaction(transaction);

	// update inode id in parent
	if (inode->IsAttributeDirectory()) {
		parent->Attributes() = GetVolume()->ToBlockRun(newInodeID);
		return parent->WriteBack(transaction);
	} else {
		// get name of this inode
		const char* name;
		char smallDataName[B_FILE_NAME_LENGTH];

		if (treeName == NULL) {
			status = inode->GetName(smallDataName, B_FILE_NAME_LENGTH);
			if (status != B_OK)
				return status;

			name = smallDataName;
		}
		else
			name = treeName;

		BPlusTree* tree = parent->Tree();
		return tree->Replace(transaction, (const uint8*)name,
			(uint16)strlen(name), newInodeID);
	}
}


status_t
ResizeVisitor::_UpdateAttributeDirectory(Transaction& transaction, Inode* inode,
	block_run newInodeRun)
{
	Vnode vnode(GetVolume(), inode->Attributes());
	Inode* attributeDirectory;

	status_t status = vnode.Get(&attributeDirectory);
	if (status != B_OK)
		return status;

	attributeDirectory->WriteLockInTransaction(transaction);

	attributeDirectory->Parent() = newInodeRun;
	return attributeDirectory->WriteBack(transaction);
}


status_t
ResizeVisitor::_UpdateIndexReferences(Transaction& transaction, Inode* inode,
	off_t newInodeID, bool rootOrIndexDir)
{
	// update user file attributes
	AttributeIterator iterator(inode);

	char attributeName[B_FILE_NAME_LENGTH];
	size_t nameLength;
	uint32 attributeType;
	ino_t attributeID;

	uint8 key[BPLUSTREE_MAX_KEY_LENGTH];
	size_t keyLength;

	status_t status;
	Index index(GetVolume());

	while (iterator.GetNext(attributeName, &nameLength, &attributeType,
			&attributeID) == B_OK) {
		// ignore attribute if not in index
		if (index.SetTo(attributeName) != B_OK)
			continue;

		keyLength = BPLUSTREE_MAX_KEY_LENGTH;
		status = inode->ReadAttribute(attributeName, attributeType, 0, key,
			&keyLength);
		if (status != B_OK)
			return status;

		status = index.UpdateNodeID(transaction, key, (uint16)keyLength,
			inode->ID(), newInodeID);
		if (status != B_OK)
			return status;
	}

	// update built-in attributes

	// the root node is not in the name index even though InNameIndex() is true
	if (inode->InNameIndex() && !rootOrIndexDir) {
		status = index.SetTo("name");
		if (status != B_OK)
			return status;

		status = inode->GetName((char*)key, BPLUSTREE_MAX_KEY_LENGTH);
		if (status != B_OK)
			return status;

		status = index.UpdateNodeID(transaction, key, (uint16)strlen((char*)key),
			inode->ID(), newInodeID);
		if (status != B_OK)
			return status;
	}
	if (inode->InSizeIndex()) {
		status = index.SetTo("size");
		if (status != B_OK)
			return status;

		off_t size = inode->Size();
		status = index.UpdateNodeID(transaction, (uint8*)&size, sizeof(int64),
			inode->ID(), newInodeID);
		if (status != B_OK)
			return status;
	}
	if (inode->InLastModifiedIndex()) {
		status = index.SetTo("last_modified");
		if (status != B_OK)
			return status;

		off_t modified = inode->LastModified();
		status = index.UpdateNodeID(transaction, (uint8*)&modified,
			sizeof(int64), inode->ID(), newInodeID);
		if (status != B_OK)
			return status;
	}
	return B_OK;

}


status_t
ResizeVisitor::_UpdateTree(Transaction& transaction, Inode* inode,
	off_t newInodeID)
{
	BPlusTree* tree = inode->Tree();
	if (tree == NULL)
		return B_ERROR;

	// update "." entry
	status_t status = tree->Replace(transaction, (const uint8*)".", strlen("."),
		newInodeID);
	if (status != B_OK)
		return status;

	// update ".." entry if we are the root node
	if (inode->Parent() == inode->BlockRun()) {
		status = tree->Replace(transaction, (const uint8*)"..",
			strlen(".."), newInodeID);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
ResizeVisitor::_UpdateChildren(Transaction& transaction, Inode* inode,
	off_t newInodeID)
{
	BPlusTree* tree = inode->Tree();
	if (tree == NULL)
		return B_ERROR;

	TreeIterator iterator(tree);
	while (true) {
		char name[B_FILE_NAME_LENGTH];
		uint16 length;
		ino_t id;

		status_t status = iterator.GetNextEntry(name, &length,
			B_FILE_NAME_LENGTH, &id);
		if (status == B_ENTRY_NOT_FOUND)
			break;
		else if (status != B_OK)
			return status;

		if (!strcmp(name, ".") || !strcmp(name, ".."))
			continue;

		Vnode childVnode(GetVolume(), id);
		Inode* childInode;
		status = childVnode.Get(&childInode);
		if (status != B_OK)
			return status;

		childInode->WriteLockInTransaction(transaction);

		childInode->Node().parent = GetVolume()->ToBlockRun(newInodeID);
		childInode->WriteBack(transaction);

		if (childInode->IsDirectory()) {
			// update ".." entry
			BPlusTree* childTree = childInode->Tree();
			if (childTree == NULL)
				return B_ERROR;

			status = childTree->Replace(transaction, (const uint8*)"..",
				strlen(".."), newInodeID);
			if (status != B_OK)
				return status;
		}
	}

	return B_OK;
}


status_t
ResizeVisitor::_UpdateSuperBlock(Inode* inode, off_t newInodeID)
{
	// we need to have our transaction written to disk before we can write
	// the updated super block, otherwise we might end up with a dangling
	// reference if something fails later on.
	status_t status = GetVolume()->GetJournal(0)->FlushLogAndBlocks();
	if (status != B_OK) {
		FATAL(("Resize: Failed to flush log before updating super block!\n"));
		return status;
	}

	MutexLocker locker(GetVolume()->Lock());
	disk_super_block& superBlock = GetVolume()->SuperBlock();

	if (inode->BlockRun() == superBlock.root_dir) {
		INFORM(("New root directory: block %" B_PRIdOFF "\n", newInodeID));
		superBlock.root_dir = GetVolume()->ToBlockRun(newInodeID);
	} else if (inode->BlockRun() == superBlock.indices) {
		INFORM(("New index directory: block %" B_PRIdOFF "\n", newInodeID));
		superBlock.indices = GetVolume()->ToBlockRun(newInodeID);
	} else {
		FATAL(("_UpdateSuperBlock: Expected inode %" B_PRIdINO
			" to be root or index directory!\n", inode->ID()));
		return B_BAD_VALUE;
	}

	return GetVolume()->WriteSuperBlock();
}


status_t
ResizeVisitor::_MoveInode(Inode* inode, off_t& newInodeID, const char* treeName)
{
	Transaction transaction(GetVolume(), 0);
	inode->WriteLockInTransaction(transaction);

	bool rootOrIndexDir = inode->BlockRun() == inode->Parent();

	// TODO: Apply allocation policies!

	block_run run;
	status_t status = GetVolume()->Allocator().AllocateBlocks(transaction, 0, 0, 1, 1, run);
	if (status != B_OK)
		RETURN_ERROR(status);

	newInodeID = GetVolume()->ToBlock(run);

	status = inode->CopyBlockTo(transaction, newInodeID);
	if (status != B_OK)
		RETURN_ERROR(status);

	if (!rootOrIndexDir && !inode->IsDeleted()) {
		status = _UpdateParent(transaction, inode, newInodeID, treeName);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	// update parent reference in attribute directory if we have one
	if (!inode->Attributes().IsZero()) {
		status = _UpdateAttributeDirectory(transaction, inode, run);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	if (!inode->IsDeleted()) {
		status = _UpdateIndexReferences(transaction, inode, newInodeID,
			rootOrIndexDir);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	// update "." and ".." tree entries if we are a directory
	if (inode->IsDirectory()) {
		status = _UpdateTree(transaction, inode, newInodeID);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	// update the parent reference if we have children, this applies to
	// regular directories, attribute directories and the index directory
	// (not the indices themselves though)
	if (inode->IsContainer() && !inode->IsIndex()) {
		status = _UpdateChildren(transaction, inode, newInodeID);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	status = GetVolume()->Free(transaction, inode->BlockRun());
	if (status != B_OK)
		RETURN_ERROR(status);

	status = transaction.Done();
	if (status != B_OK)
		RETURN_ERROR(status);

	if (rootOrIndexDir) {
		status = _UpdateSuperBlock(inode, newInodeID);
		if (status != B_OK) {
			// we've already completed the transaction, this is very bad
			FATAL(("_MoveInode: Could not write super block!\n"));
			return status;
		}
	}

	return B_OK;
}
