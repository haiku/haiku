/*
 * Copyright 2002-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com
 * This file may be used under the terms of the MIT License.
 */


//! File system error checking


#include "CheckVisitor.h"

#include "BlockAllocator.h"
#include "BPlusTree.h"
#include "Inode.h"
#include "Volume.h"


struct check_index {
	check_index()
		:
		inode(NULL)
	{
	}

	char				name[B_FILE_NAME_LENGTH];
	block_run			run;
	Inode*				inode;
};


CheckVisitor::CheckVisitor(Volume* volume)
	:
	FileSystemVisitor(volume),
	fCheckBitmap(NULL)
{
}


CheckVisitor::~CheckVisitor()
{
	free(fCheckBitmap);
}


status_t
CheckVisitor::StartBitmapPass()
{
	if (!_ControlValid())
		return B_BAD_VALUE;

	// Lock the volume's journal and block allocator
	GetVolume()->GetJournal(0)->Lock(NULL, true);
	recursive_lock_lock(&GetVolume()->Allocator().Lock());

	size_t size = _BitmapSize();
	fCheckBitmap = (uint32*)malloc(size);
	if (fCheckBitmap == NULL) {
		recursive_lock_unlock(&GetVolume()->Allocator().Lock());
		GetVolume()->GetJournal(0)->Unlock(NULL, true);
		return B_NO_MEMORY;
	}

	memset(&Control().stats, 0, sizeof(check_control::stats));

	// initialize bitmap
	memset(fCheckBitmap, 0, size);
	for (int32 block = GetVolume()->Log().Start() + GetVolume()->Log().Length();
			block-- > 0;) {
		_SetCheckBitmapAt(block);
	}

	Control().pass = BFS_CHECK_PASS_BITMAP;
	Control().stats.block_size = GetVolume()->BlockSize();

	// TODO: check reserved area in bitmap!

	Start(VISIT_REGULAR | VISIT_INDICES | VISIT_REMOVED
		| VISIT_ATTRIBUTE_DIRECTORIES);

	return B_OK;
}


status_t
CheckVisitor::WriteBackCheckBitmap()
{
	if (GetVolume()->IsReadOnly())
		return B_OK;

	// calculate the number of used blocks in the check bitmap
	size_t size = _BitmapSize();
	off_t usedBlocks = 0LL;

	// TODO: update the allocation groups used blocks info
	for (uint32 i = size >> 2; i-- > 0;) {
		uint32 compare = 1;
		// Count the number of bits set
		for (int16 j = 0; j < 32; j++, compare <<= 1) {
			if ((compare & fCheckBitmap[i]) != 0)
				usedBlocks++;
		}
	}

	Control().stats.freed = GetVolume()->UsedBlocks() - usedBlocks
		+ Control().stats.missing;
	if (Control().stats.freed < 0)
		Control().stats.freed = 0;

	// Should we fix errors? Were there any errors we can fix?
	if ((Control().flags & BFS_FIX_BITMAP_ERRORS) != 0
		&& (Control().stats.freed != 0 || Control().stats.missing != 0)) {
		// If so, write the check bitmap back over the original one,
		// and use transactions here to play safe - we even use several
		// transactions, so that we don't blow the maximum log size
		// on large disks, since we don't need to make this atomic.
#if 0
		// prints the blocks that differ
		off_t block = 0;
		for (int32 i = 0; i < fNumGroups; i++) {
			AllocationBlock cached(fVolume);
			for (uint32 j = 0; j < fGroups[i].NumBlocks(); j++) {
				cached.SetTo(fGroups[i], j);
				for (uint32 k = 0; k < cached.NumBlockBits(); k++) {
					if (cached.IsUsed(k) != _CheckBitmapIsUsedAt(block)) {
						dprintf("differ block %lld (should be %d)\n", block,
							_CheckBitmapIsUsedAt(block));
					}
					block++;
				}
			}
		}
#endif

		GetVolume()->SuperBlock().used_blocks
			= HOST_ENDIAN_TO_BFS_INT64(usedBlocks);

		size_t blockSize = GetVolume()->BlockSize();
		off_t numBitmapBlocks = GetVolume()->NumBitmapBlocks();

		for (uint32 i = 0; i < numBitmapBlocks; i += 512) {
			Transaction transaction(GetVolume(), 1 + i);

			uint32 blocksToWrite = 512;
			if (blocksToWrite + i > numBitmapBlocks)
				blocksToWrite = numBitmapBlocks - i;

			status_t status = transaction.WriteBlocks(1 + i,
				(uint8*)fCheckBitmap + i * blockSize, blocksToWrite);
			if (status < B_OK) {
				FATAL(("error writing bitmap: %s\n", strerror(status)));
				return status;
			}
			transaction.Done();
		}
	}

	return B_OK;
}


status_t
CheckVisitor::StartIndexPass()
{
	// if we don't have indices to rebuild, this pass is done
	if (indices.IsEmpty())
		return B_ENTRY_NOT_FOUND;

	Control().pass = BFS_CHECK_PASS_INDEX;

	status_t status = _PrepareIndices();
	if (status != B_OK) {
		Control().status = status;
		return status;
	}

	Start(VISIT_REGULAR);
	return Next();
}


status_t
CheckVisitor::StopChecking()
{
	if (GetVolume()->IsReadOnly()) {
		// We can't fix errors on this volume
		Control().flags = 0;
	}

	if (Control().status != B_ENTRY_NOT_FOUND)
		FATAL(("CheckVisitor didn't run through\n"));

	_FreeIndices();

	recursive_lock_unlock(&GetVolume()->Allocator().Lock());
	GetVolume()->GetJournal(0)->Unlock(NULL, true);
	return B_OK;
}


status_t
CheckVisitor::VisitDirectoryEntry(Inode* inode, Inode* parent,
	const char* treeName)
{
	Control().inode = inode->ID();
	Control().mode = inode->Mode();

	if (Pass() != BFS_CHECK_PASS_BITMAP)
		return B_OK;

	// check if the inode's name is the same as in the b+tree
	if (inode->IsRegularNode()) {
		RecursiveLocker locker(inode->SmallDataLock());
		NodeGetter node(GetVolume(), inode);
		if (node.Node() == NULL) {
			Control().errors |= BFS_COULD_NOT_OPEN;
			Control().status = B_IO_ERROR;
			return B_OK;
		}

		const char* localName = inode->Name(node.Node());
		if (localName == NULL || strcmp(localName, treeName)) {
			Control().errors |= BFS_NAMES_DONT_MATCH;
			FATAL(("Names differ: tree \"%s\", inode \"%s\"\n", treeName,
				localName));

			if ((Control().flags & BFS_FIX_NAME_MISMATCHES) != 0) {
				// Rename the inode
				Transaction transaction(GetVolume(), inode->BlockNumber());

				// Note, this may need extra blocks, but the inode will
				// only be checked afterwards, so that it won't be lost
				status_t status = inode->SetName(transaction, treeName);
				if (status == B_OK)
					status = inode->WriteBack(transaction);
				if (status == B_OK)
					status = transaction.Done();
				if (status != B_OK) {
					Control().status = status;
					return B_OK;
				}
			}
		}
	}

	// Check for the correct mode of the node (if the mode of the
	// file don't fit to its parent, there is a serious problem)
	if (((parent->Mode() & S_ATTR_DIR) != 0
			&& !inode->IsAttribute())
		|| ((parent->Mode() & S_INDEX_DIR) != 0
			&& !inode->IsIndex())
		|| (is_directory(parent->Mode())
			&& !inode->IsRegularNode())) {
		FATAL(("inode at %" B_PRIdOFF " is of wrong type: %o (parent "
			"%o at %" B_PRIdOFF ")!\n", inode->BlockNumber(),
			inode->Mode(), parent->Mode(), parent->BlockNumber()));

		// if we are allowed to fix errors, we should remove the file
		if ((Control().flags & BFS_REMOVE_WRONG_TYPES) != 0
			&& (Control().flags & BFS_FIX_BITMAP_ERRORS) != 0) {
			Control().status = _RemoveInvalidNode(parent, NULL, inode,
				treeName);
		} else
			Control().status = B_ERROR;

		Control().errors |= BFS_WRONG_TYPE;
	}
	return B_OK;
}


status_t
CheckVisitor::VisitInode(Inode* inode, const char* treeName)
{
	Control().inode = inode->ID();
	Control().mode = inode->Mode();
		// (we might have set these in VisitDirectoryEntry already)

	// set name
	if (treeName == NULL) {
		if (inode->GetName(Control().name) < B_OK) {
			if (inode->IsContainer())
				strcpy(Control().name, "(dir has no name)");
			else
				strcpy(Control().name, "(node has no name)");
		}
	} else
		strcpy(Control().name, treeName);

	status_t status = B_OK;

	switch (Pass()) {
		case BFS_CHECK_PASS_BITMAP:
		{
			status = _CheckInodeBlocks(inode, NULL);
			if (status != B_OK)
				return status;

			// Check the B+tree as well
			if (inode->IsContainer()) {
				bool repairErrors = (Control().flags & BFS_FIX_BPLUSTREES) != 0;
				bool errorsFound = false;

				status = inode->Tree()->Validate(repairErrors, errorsFound);

				if (errorsFound) {
					Control().errors |= BFS_INVALID_BPLUSTREE;
					if (inode->IsIndex() && treeName != NULL && repairErrors) {
						// We completely rebuild corrupt indices
						check_index* index = new(std::nothrow) check_index;
						if (index == NULL)
							return B_NO_MEMORY;

						strlcpy(index->name, treeName, sizeof(index->name));
						index->run = inode->BlockRun();
						Indices().Push(index);
					}
				}
			}

			break;
		}

		case BFS_CHECK_PASS_INDEX:
			status = _AddInodeToIndex(inode);
			break;
	}

	Control().status = status;
	return B_OK;
}


status_t
CheckVisitor::OpenInodeFailed(status_t reason, ino_t id, Inode* parent,
		char* treeName, TreeIterator* iterator)
{
	FATAL(("Could not open inode at %" B_PRIdOFF "\n", id));

	if (treeName != NULL)
		strlcpy(Control().name, treeName, B_FILE_NAME_LENGTH);
	else
		strcpy(Control().name, "(node has no name)");

	Control().inode = id;
	Control().errors = BFS_COULD_NOT_OPEN;

	// remove inode from the tree if we can
	if (parent != NULL && iterator != NULL
		&& (Control().flags & BFS_REMOVE_INVALID) != 0) {
		Control().status = _RemoveInvalidNode(parent, iterator->Tree(), NULL,
			treeName);
	} else
		Control().status = B_ERROR;

	return B_OK;
}


status_t
CheckVisitor::OpenBPlusTreeFailed(Inode* inode)
{
	FATAL(("Could not open b+tree from inode at %" B_PRIdOFF "\n",
		inode->ID()));
	return B_OK;
}


status_t
CheckVisitor::TreeIterationFailed(status_t reason, Inode* parent)
{
	// Iterating over the B+tree failed - we let the checkfs run
	// fail completely, as we would delete all files we cannot
	// access.
	// TODO: maybe have a force parameter that actually does that.
	// TODO: we also need to be able to repair broken B+trees!
	return reason;
}


status_t
CheckVisitor::_RemoveInvalidNode(Inode* parent, BPlusTree* tree,
	Inode* inode, const char* name)
{
	// It's safe to start a transaction, because Inode::Remove()
	// won't touch the block bitmap (which we hold the lock for)
	// if we set the INODE_DONT_FREE_SPACE flag - since we fix
	// the bitmap anyway.
	Transaction transaction(GetVolume(), parent->BlockNumber());
	status_t status;

	if (inode != NULL) {
		inode->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_DONT_FREE_SPACE);

		status = parent->Remove(transaction, name, NULL, false, true);
	} else {
		parent->WriteLockInTransaction(transaction);

		// does the file even exist?
		off_t id;
		status = tree->Find((uint8*)name, (uint16)strlen(name), &id);
		if (status == B_OK)
			status = tree->Remove(transaction, name, id);
	}

	if (status == B_OK) {
		entry_cache_remove(GetVolume()->ID(), parent->ID(), name);
		transaction.Done();
	}

	return status;
}


bool
CheckVisitor::_ControlValid()
{
	if (Control().magic != BFS_IOCTL_CHECK_MAGIC) {
		FATAL(("invalid check_control!\n"));
		return false;
	}

	return true;
}


bool
CheckVisitor::_CheckBitmapIsUsedAt(off_t block) const
{
	size_t size = _BitmapSize();
	uint32 index = block / 32;	// 32bit resolution
	if (index > size / 4)
		return false;

	return BFS_ENDIAN_TO_HOST_INT32(fCheckBitmap[index])
		& (1UL << (block & 0x1f));
}


void
CheckVisitor::_SetCheckBitmapAt(off_t block)
{
	size_t size = _BitmapSize();
	uint32 index = block / 32;	// 32bit resolution
	if (index > size / 4)
		return;

	fCheckBitmap[index] |= HOST_ENDIAN_TO_BFS_INT32(1UL << (block & 0x1f));
}


size_t
CheckVisitor::_BitmapSize() const
{
	return GetVolume()->BlockSize() * GetVolume()->NumBitmapBlocks();
}


status_t
CheckVisitor::_CheckInodeBlocks(Inode* inode, const char* name)
{
	status_t status = _CheckAllocated(inode->BlockRun(), "inode");
	if (status != B_OK)
		return status;

	if (inode->IsSymLink() && (inode->Flags() & INODE_LONG_SYMLINK) == 0) {
		// symlinks may not have a valid data stream
		if (strlen(inode->Node().short_symlink) >= SHORT_SYMLINK_NAME_LENGTH)
			return B_BAD_DATA;

		return B_OK;
	}

	data_stream* data = &inode->Node().data;

	// check the direct range

	if (data->max_direct_range) {
		for (int32 i = 0; i < NUM_DIRECT_BLOCKS; i++) {
			if (data->direct[i].IsZero())
				break;

			status = _CheckAllocated(data->direct[i], "direct");
			if (status < B_OK)
				return status;

			Control().stats.direct_block_runs++;
			Control().stats.blocks_in_direct
				+= data->direct[i].Length();
		}
	}

	CachedBlock cached(GetVolume());

	// check the indirect range

	if (data->max_indirect_range) {
		status = _CheckAllocated(data->indirect, "indirect");
		if (status < B_OK)
			return status;

		off_t block = GetVolume()->ToBlock(data->indirect);

		for (int32 i = 0; i < data->indirect.Length(); i++) {
			block_run* runs = (block_run*)cached.SetTo(block + i);
			if (runs == NULL)
				RETURN_ERROR(B_IO_ERROR);

			int32 runsPerBlock = GetVolume()->BlockSize() / sizeof(block_run);
			int32 index = 0;
			for (; index < runsPerBlock; index++) {
				if (runs[index].IsZero())
					break;

				status = _CheckAllocated(runs[index], "indirect->run");
				if (status < B_OK)
					return status;

				Control().stats.indirect_block_runs++;
				Control().stats.blocks_in_indirect
					+= runs[index].Length();
			}
			Control().stats.indirect_array_blocks++;

			if (index < runsPerBlock)
				break;
		}
	}

	// check the double indirect range

	if (data->max_double_indirect_range) {
		status = _CheckAllocated(data->double_indirect, "double indirect");
		if (status != B_OK)
			return status;

		int32 runsPerBlock = runs_per_block(GetVolume()->BlockSize());
		int32 runsPerArray = runsPerBlock * data->double_indirect.Length();

		CachedBlock cachedDirect(GetVolume());

		for (int32 indirectIndex = 0; indirectIndex < runsPerArray;
				indirectIndex++) {
			// get the indirect array block
			block_run* array = (block_run*)cached.SetTo(
				GetVolume()->ToBlock(data->double_indirect)
					+ indirectIndex / runsPerBlock);
			if (array == NULL)
				return B_IO_ERROR;

			block_run indirect = array[indirectIndex % runsPerBlock];
			// are we finished yet?
			if (indirect.IsZero())
				return B_OK;

			status = _CheckAllocated(indirect, "double indirect->runs");
			if (status != B_OK)
				return status;

			int32 maxIndex
				= ((uint32)indirect.Length() << GetVolume()->BlockShift())
					/ sizeof(block_run);

			for (int32 index = 0; index < maxIndex; ) {
				block_run* runs = (block_run*)cachedDirect.SetTo(
					GetVolume()->ToBlock(indirect) + index / runsPerBlock);
				if (runs == NULL)
					return B_IO_ERROR;

				do {
					// are we finished yet?
					if (runs[index % runsPerBlock].IsZero())
						return B_OK;

					status = _CheckAllocated(runs[index % runsPerBlock],
						"double indirect->runs->run");
					if (status != B_OK)
						return status;

					Control().stats.double_indirect_block_runs++;
					Control().stats.blocks_in_double_indirect
						+= runs[index % runsPerBlock].Length();
				} while ((++index % runsPerBlock) != 0);
			}

			Control().stats.double_indirect_array_blocks++;
		}
	}

	return B_OK;
}


status_t
CheckVisitor::_CheckAllocated(block_run run, const char* type)
{
	BlockAllocator& allocator = GetVolume()->Allocator();

	// make sure the block run is valid
	if (!allocator.IsValidBlockRun(run)) {
		Control().errors |= BFS_INVALID_BLOCK_RUN;
		return B_OK;
	}

	status_t status;

	off_t start = GetVolume()->ToBlock(run);
	off_t end = start + run.Length();

	// check if the run is allocated in the block bitmap on disk
	off_t block = start;

	while (block < end) {
		off_t firstMissing;
		status = allocator.CheckBlocks(block, end - block, true, &firstMissing);
		if (status == B_OK)
			break;
		else if (status != B_BAD_DATA)
			return status;

		off_t afterLastMissing;
		status = allocator.CheckBlocks(firstMissing, end - firstMissing, false,
			&afterLastMissing);
		if (status == B_OK)
			afterLastMissing = end;
		else if (status != B_BAD_DATA)
			return status;

		PRINT(("%s: block_run(%ld, %u, %u): blocks %Ld - %Ld are "
			"not allocated!\n", type, run.AllocationGroup(), run.Start(),
			run.Length(), firstMissing, afterLastMissing - 1));

		Control().stats.missing += afterLastMissing - firstMissing;

		block = afterLastMissing;
	}

	// set bits in check bitmap, while checking if they're already set
	off_t firstSet = -1;

	for (block = start; block < end; block++) {
		if (_CheckBitmapIsUsedAt(block)) {
			if (firstSet == -1) {
				firstSet = block;
				Control().errors |= BFS_BLOCKS_ALREADY_SET;
			}
			Control().stats.already_set++;
		} else {
			if (firstSet != -1) {
				FATAL(("%s: block_run(%d, %u, %u): blocks %" B_PRIdOFF
					" - %" B_PRIdOFF " are already set!\n", type,
					(int)run.AllocationGroup(), run.Start(), run.Length(),
					firstSet, block - 1));
				firstSet = -1;
			}
			_SetCheckBitmapAt(block);
		}
	}

	return B_OK;
}


status_t
CheckVisitor::_PrepareIndices()
{
	int32 count = 0;

	for (int32 i = 0; i < Indices().CountItems(); i++) {
		check_index* index = Indices().Array()[i];
		Vnode vnode(GetVolume(), index->run);
		Inode* inode;
		status_t status = vnode.Get(&inode);
		if (status != B_OK) {
			FATAL(("check: Could not open index at %" B_PRIdOFF "\n",
				GetVolume()->ToBlock(index->run)));
			return status;
		}

		BPlusTree* tree = inode->Tree();
		if (tree == NULL) {
			// TODO: We can't yet repair those
			continue;
		}

		status = tree->MakeEmpty();
		if (status != B_OK)
			return status;

		index->inode = inode;
		vnode.Keep();
		count++;
	}

	return count == 0 ? B_ENTRY_NOT_FOUND : B_OK;
}


void
CheckVisitor::_FreeIndices()
{
	for (int32 i = 0; i < Indices().CountItems(); i++) {
		check_index* index = Indices().Array()[i];
		if (index->inode != NULL) {
			put_vnode(GetVolume()->FSVolume(),
				GetVolume()->ToVnode(index->inode->BlockRun()));
		}
		delete index;
	}
	Indices().MakeEmpty();
}


status_t
CheckVisitor::_AddInodeToIndex(Inode* inode)
{
	Transaction transaction(GetVolume(), inode->BlockNumber());

	for (int32 i = 0; i < Indices().CountItems(); i++) {
		check_index* index = Indices().Array()[i];
		if (index->inode == NULL)
			continue;

		index->inode->WriteLockInTransaction(transaction);

		BPlusTree* tree = index->inode->Tree();
		if (tree == NULL)
			return B_ERROR;

		status_t status = B_OK;

		if (!strcmp(index->name, "name")) {
			if (inode->InNameIndex()) {
				char name[B_FILE_NAME_LENGTH];
				if (inode->GetName(name, B_FILE_NAME_LENGTH) != B_OK)
					return B_ERROR;

				status = tree->Insert(transaction, name, inode->ID());
			}
		} else if (!strcmp(index->name, "last_modified")) {
			if (inode->InLastModifiedIndex()) {
				status = tree->Insert(transaction, inode->OldLastModified(),
					inode->ID());
			}
		} else if (!strcmp(index->name, "size")) {
			if (inode->InSizeIndex())
				status = tree->Insert(transaction, inode->Size(), inode->ID());
		} else {
			uint8 key[MAX_INDEX_KEY_LENGTH];
			size_t keyLength = sizeof(key);
			if (inode->ReadAttribute(index->name, B_ANY_TYPE, 0, key,
					&keyLength) == B_OK) {
				status = tree->Insert(transaction, key, keyLength, inode->ID());
			}
		}

		if (status != B_OK)
			return status;
	}

	return transaction.Done();
}
