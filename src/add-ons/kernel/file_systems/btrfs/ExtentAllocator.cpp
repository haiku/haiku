/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * This file may be used under the terms of the MIT License.
 */


#include "ExtentAllocator.h"
#include "Chunk.h"


CachedExtent*
CachedExtent::Create(uint64 offset, uint64 length, uint64 flags)
{
	CachedExtent* self = new(std::nothrow) CachedExtent();
	if (self == NULL)
		return NULL;

	self->offset = offset;
	self->length = length;
	self->refCount = 0;
	if ((flags & BTRFS_EXTENT_FLAG_ALLOCATED) != 0)
		self->refCount = 1;
	self->flags = flags;
	self->item = NULL;
	return self;
}


void
CachedExtent::Delete()
{
	if (item != NULL)
		free(item);
	item = NULL;
	delete this;
}


bool
CachedExtent::IsAllocated() const
{
	return (flags & BTRFS_EXTENT_FLAG_ALLOCATED) != 0;
}


bool
CachedExtent::IsData() const
{
	return (flags & BTRFS_EXTENT_FLAG_DATA) != 0;
}


void
CachedExtent::Info() const
{
	const char* extentType = "allocated tree block";
	if (IsAllocated() == false && IsData() == false)
		extentType = "free tree block";
	else if (IsAllocated() == false && IsData() == true)
		extentType = "free data extent";
	else if (IsAllocated() == true && IsData() == true)
		extentType = "allocated data extent";

	TRACE("%s at %" B_PRIu64 " length %" B_PRIu64 " refCount %i\n",
		extentType, offset, length, refCount);
}


// ExtentTree Implementation


CachedExtentTree::CachedExtentTree(const TreeDefinition& definition)
	:
	AVLTree<TreeDefinition>(definition)
{
}


CachedExtentTree::~CachedExtentTree()
{
	Delete();
}


/* Find extent that cover or after "offset" and has length >= "size"
 * it must also satisfy the condition "type".
 */
status_t
CachedExtentTree::FindNext(CachedExtent** chosen, uint64 offset, uint64 size,
	uint64 type)
{
	CachedExtent* found = Find(offset);
	while (found != NULL && (found->flags != type || found->length < size))
		found = Next(found);

	if (found == NULL)
		return B_ENTRY_NOT_FOUND;
	*chosen = found;
	return B_OK;
}


/* this will insert all free extents that are holes,
 * created by existed allocated extents in the tree
 * from lowerBound to upperBound
 */
status_t
CachedExtentTree::FillFreeExtents(uint64 lowerBound, uint64 upperBound)
{
	CachedExtent* node = FindClosest(lowerBound, false);
	CachedExtent* hole = NULL;
	status_t status = B_OK;
	uint64 flags = node->flags & (~BTRFS_EXTENT_FLAG_ALLOCATED);
	if (lowerBound < node->offset) {
		hole = CachedExtent::Create(lowerBound, node->offset - lowerBound,
			flags);
		status = _AddFreeExtent(hole);
		if (status != B_OK)
			return status;
	}

	CachedExtent* next = NULL;
	while ((next = Next(node)) != NULL && next->End() < upperBound) {
		if (node->End() == next->offset) {
			node = next;
			continue;
		}

		hole = CachedExtent::Create(node->End(), next->offset - node->End(),
			flags);
		status = _AddFreeExtent(hole);
		if (status != B_OK)
			return status;
		node = next;
	}

	// final node should be a right most node
	if (upperBound > node->End()) {
		hole = CachedExtent::Create(node->End(), upperBound - node->End(),
			flags);
		status = _AddFreeExtent(hole);
	}

	return status;
}


void
CachedExtentTree::_RemoveExtent(CachedExtent* node)
{
	node->refCount--;
	if (node->refCount <= 0) {
		Remove(node);
		node->Delete();
	}
}


status_t
CachedExtentTree::_AddAllocatedExtent(CachedExtent* node)
{
	if (node == NULL || node->IsAllocated() == false)
		return B_BAD_VALUE;

	CachedExtent* found = Find(node->offset);
	if (found == NULL) {
		Insert(node);
		return B_OK;
	}

	if ((found->IsData() && !node->IsData())
		|| (!found->IsData() && node->IsData())) {
		// this shouldn't happen as because different type has
		// its different region for locating.
		node->Delete();
		return B_BAD_VALUE;
	}

	if (found->IsAllocated() == false) {
		// split free extents (found)
		if (node->End() > found->End()) {
			// TODO: when to handle this ?
			node->Delete();
			return B_BAD_VALUE;
		}

		// |----- found ------|
		//    |-- node ---|
		uint64 diff = node->offset - found->offset;
		found->offset += diff + node->length;
		found->length -= diff + node->length;
		// diff < 0 couldn't happen because of the Compare function
		if (diff > 0) {
			CachedExtent* leftEmpty = CachedExtent::Create(
				node->offset - diff, diff, found->flags);
			_AddFreeExtent(leftEmpty);
		}

		if (found->length == 0) {
			// free-extent that has no space
			_RemoveExtent(found);
		}

		Insert(node);
		return B_OK;
	}

	if (found->length == node->length) {
		found->refCount++;
	} else {
		// TODO: What should we do in this case ?
		return B_BAD_VALUE;
	}
	node->Delete();

	return B_OK;
}


status_t
CachedExtentTree::_AddFreeExtent(CachedExtent* node)
{
	if (node == NULL || node->IsAllocated() == true)
		return B_BAD_VALUE;

	CachedExtent* found = Find(node->offset);
	if (found == NULL) {
		Insert(node);
		_CombineFreeExtent(node);
		return B_OK;
	}

	if ((found->IsData() && !node->IsData())
		|| (!found->IsData() && node->IsData())) {
		// this shouldn't happen as because different type has
		// its different region for locating.
		node->Delete();
		return B_BAD_VALUE;
	}

	if (found->End() < node->End()) {
		// |---- found-----|
		//      |--- node ------|
		CachedExtent* rightEmpty = CachedExtent::Create(found->End(),
			node->End() - found->End(), node->flags);
		_AddFreeExtent(rightEmpty);
		node->length -= node->End() - found->End();
	}

	if (found->IsAllocated() == true) {
		// free part of the allocated extents(found)
		// |----- found ------|
		//    |-- node ---|
		uint64 diff = node->offset - found->offset;
		found->offset += diff + node->length;
		found->length -= diff + node->length;
		// diff < 0 couldn't happen because of the Compare function
		if (diff > 0){
			CachedExtent* left = CachedExtent::Create(node->offset - diff,
				diff, found->flags);
			_AddAllocatedExtent(left);
		}

		if (found->length == 0)
			_RemoveExtent(found);

		Insert(node);
		_CombineFreeExtent(node);
	}

	return B_OK;
}


status_t
CachedExtentTree::AddExtent(CachedExtent* extent)
{
	if (extent->IsAllocated() == true)
		return _AddAllocatedExtent(extent);
	return _AddFreeExtent(extent);
}


void
CachedExtentTree::_CombineFreeExtent(CachedExtent* node)
{
	// node should be inserted first to call this function,
	// otherwise it will overdelete.
	if (node->IsAllocated() == true)
		return;

	CachedExtent* other = Next(node);
	if (other != NULL) {
		if (node->End() == other->offset && node->flags == other->flags) {
			node->length += other->length;
			_RemoveExtent(other);
		}
	}

	other = Previous(node);
	if (other != NULL) {
		if (other->End() == node->offset && node->flags == other->flags) {
			other->length += node->length;
			_RemoveExtent(node);
		}
	}
}


void
CachedExtentTree::_DumpInOrder(CachedExtent* node) const
{
	if (node == NULL)
		return;
	_DumpInOrder(_GetValue(node->left));
	node->Info();
	_DumpInOrder(_GetValue(node->right));
}


void
CachedExtentTree::DumpInOrder() const
{
	TRACE("\n");
	_DumpInOrder(RootNode());
	TRACE("\n");
}


void
CachedExtentTree::Delete()
{
	_Delete(RootNode());
	Clear();
}


void
CachedExtentTree::_Delete(CachedExtent* node)
{
	if (node == NULL)
		return;
	_Delete(_GetValue(node->left));
	_Delete(_GetValue(node->right));
	node->Delete();
}


// BlockGroup


BlockGroup::BlockGroup(BTree* extentTree)
	:
	fItem(NULL)
{
	fCurrentExtentTree = new(std::nothrow) BTree(extentTree->SystemVolume(),
		extentTree->RootBlock());
}


BlockGroup::~BlockGroup()
{
	if (fItem != NULL) {
		free(fItem);
		fItem = NULL;
	}

	delete fCurrentExtentTree;
	fCurrentExtentTree = NULL;
}


status_t
BlockGroup::SetExtentTree(off_t rootAddress)
{
	status_t status = fCurrentExtentTree->SetRoot(rootAddress, NULL);
	if (status != B_OK)
		return status;

	if (fItem != NULL) {
		// re-allocate BlockGroup;
		uint64 flags = Flags();
		return Initialize(flags);
	}

	return B_OK;
}


/* Initialize BlockGroup that has flag
 */
status_t
BlockGroup::Initialize(uint64 flag)
{
	if (fCurrentExtentTree == NULL)
		return B_NO_MEMORY;

	if (fItem != NULL)
		free(fItem);

	TRACE("BlockGroup::Initialize() find block group has flag: %"
		B_PRIu64 "\n", flag);
	BTree::Path path(fCurrentExtentTree);
	fKey.SetObjectID(0);
	// find first item in extent to get the ObjectID
	// ignore type because block_group is not always the first item
	fKey.SetType(BTRFS_KEY_TYPE_ANY);
	fCurrentExtentTree->FindNext(&path, fKey, NULL);

	fKey.SetType(BTRFS_KEY_TYPE_BLOCKGROUP_ITEM);
	fKey.SetOffset(0);
	status_t status;

	while (true) {
		status = fCurrentExtentTree->FindNext(&path, fKey, (void**)&fItem);
		if ((Flags() & flag) == flag || status != B_OK)
			break;
		fKey.SetObjectID(End());
		fKey.SetOffset(0);
	}

	if (status != B_OK)
		ERROR("BlockGroup::Initialize() cannot find block group\n");

	return status;
}


status_t
BlockGroup::LoadExtent(CachedExtentTree* tree, bool inverse)
{
	if (fItem == NULL)
		return B_NO_INIT;

	uint64 flags = (Flags() & BTRFS_BLOCKGROUP_FLAG_DATA) != 0 ?
		BTRFS_EXTENT_FLAG_DATA : BTRFS_EXTENT_FLAG_TREE_BLOCK;
	if (inverse == false)
		flags |= BTRFS_EXTENT_FLAG_ALLOCATED;

	uint64 start = Start();
	CachedExtent* insert;
	void* data;
	uint64 extentSize = fCurrentExtentTree->SystemVolume()->BlockSize();

	btrfs_key key;
	key.SetType(BTRFS_KEY_TYPE_METADATA_ITEM);
	if ((flags & BTRFS_EXTENT_FLAG_DATA) != 0)
		key.SetType(BTRFS_KEY_TYPE_EXTENT_ITEM);
	key.SetObjectID(start);
	key.SetOffset(0);

	TreeIterator iterator(fCurrentExtentTree, key);
	status_t status;
	while (true) {
		status = iterator.GetNextEntry(&data);
		key = iterator.Key();
		if (status != B_OK) {
			if (key.ObjectID() != Start())
				break;
			// When we couldn't find the item and key has
			// objectid == BlockGroup's objectID, because
			// key's type < BLOCKGROUP_ITEM
			continue;
		}

		if (inverse == false) {
			if ((flags & BTRFS_EXTENT_FLAG_DATA) != 0)
				extentSize = key.Offset();
			insert = CachedExtent::Create(key.ObjectID(), extentSize, flags);
			insert->item = (btrfs_extent*)data;
		} else {
			extentSize = key.ObjectID() - start;
			insert = CachedExtent::Create(start, extentSize, flags);
			free(data);		// free extent doesn't have data;
		}
		_InsertExtent(tree, insert);
		start = key.ObjectID() + extentSize;
	}

	if (inverse == true)
		_InsertExtent(tree, start, End() - start, flags);

	return B_OK;
}


status_t
BlockGroup::_InsertExtent(CachedExtentTree* tree, uint64 start, uint64 length,
	uint64 flags)
{
	CachedExtent* extent = CachedExtent::Create(start, length, flags);
	return _InsertExtent(tree, extent);
}


status_t
BlockGroup::_InsertExtent(CachedExtentTree* tree, CachedExtent* extent)
{
	if (extent->length == 0)
		return B_BAD_VALUE;

	status_t status = tree->AddExtent(extent);
	if (status != B_OK) {
		return status;
	}
	return B_OK;
}


// ExtentAllocator


ExtentAllocator::ExtentAllocator(Volume* volume)
	:
	fVolume(volume),
	fTree(NULL),
	fStart((uint64)-1),
	fEnd(0)
{
	fTree = new(std::nothrow) CachedExtentTree(TreeDefinition());
}


ExtentAllocator::~ExtentAllocator()
{
	delete fTree;
	fTree = NULL;
}


status_t
ExtentAllocator::_LoadExtentTree(uint64 flags)
{
	TRACE("ExtentAllocator::_LoadExtentTree() flags: %" B_PRIu64 "\n", flags);
	BlockGroup blockGroup(fVolume->ExtentTree());
	status_t status = blockGroup.Initialize(flags);
	if (status != B_OK)
		return status;

	for (int i = 0; i < BTRFS_NUM_ROOT_BACKUPS; i++) {
		uint64 extentRootAddr =
			fVolume->SuperBlock().backup_roots[i].ExtentRoot();
		if (extentRootAddr == 0)
			continue;	// new device has 0 root address

		status = blockGroup.SetExtentTree(extentRootAddr);
		if (status != B_OK)
			return status;
		status = blockGroup.LoadExtent(fTree, false);
		if (status != B_OK)
			return status;
	}

	if (fTree->IsEmpty())	// 4 backup roots is 0
		return B_OK;

	uint64 lowerBound = blockGroup.Start();
	uint64 upperBound = blockGroup.End();
	status = fTree->FillFreeExtents(lowerBound, upperBound);
	if (status != B_OK) {
		ERROR("ExtentAllocator::_LoadExtentTree() could not fill free extents"
			"start %" B_PRIu64 " end %" B_PRIu64 "\n", lowerBound,
			upperBound);
		return status;
	}

	if (fStart > lowerBound)
		fStart = lowerBound;
	if (fEnd < upperBound)
		fEnd = upperBound;
	return B_OK;
}


status_t
ExtentAllocator::Initialize()
{
	TRACE("ExtentAllocator::Initialize()\n");
	if (fTree == NULL)
		return B_NO_MEMORY;

	status_t status = _LoadExtentTree(BTRFS_BLOCKGROUP_FLAG_DATA);
	if (status != B_OK) {
		ERROR("ExtentAllocator:: could not load extent tree (data)\n");
		return status;
	}

	status = _LoadExtentTree(BTRFS_BLOCKGROUP_FLAG_SYSTEM);
	if (status != B_OK) {
		ERROR("ExtentAllocator:: could not load extent tree (system)\n");
		return status;
	}

	status = _LoadExtentTree(BTRFS_BLOCKGROUP_FLAG_METADATA);
	if (status != B_OK) {
		ERROR("ExtentAllocator:: could not load extent tree (metadata)\n");
		return status;
	}

	fTree->DumpInOrder();
	return B_OK;
}


/* Allocate extent that
 * startwith or after "start" and has size >= "size"
 * For now the allocating strategy is "first fit"
 */
status_t
ExtentAllocator::_Allocate(uint64& found, uint64 start, uint64 size,
	uint64 type)
{
	TRACE("ExtentAllocator::_Allocate() start %" B_PRIu64 " size %" B_PRIu64
		" type %" B_PRIu64 "\n", start, size, type);
	CachedExtent* chosen;
	status_t status;
	while (true) {
		status = fTree->FindNext(&chosen, start, size, type);
		if (status != B_OK)
			return status;

		if (TreeDefinition().Compare(start, chosen) == 0) {
			if (chosen->End() - start >= size) {
				// if covered and have enough space for allocating
				break;
			}
			start = chosen->End();	// set new start and retry
		} else
			start = chosen->offset;
	}

	TRACE("ExtentAllocator::_Allocate() found %" B_PRIu64 "\n", start);
	chosen = CachedExtent::Create(start, size,
		type | BTRFS_EXTENT_FLAG_ALLOCATED);
	status = fTree->AddExtent(chosen);
	if (status != B_OK)
		return status;

	found = start;
	return B_OK;
}


// Allocate block for metadata
// flags is BLOCKGROUP's flags
status_t
ExtentAllocator::AllocateTreeBlock(uint64& found, uint64 start, uint64 flags)
{
	// TODO: implement more features here with flags, e.g DUP, RAID, etc

	BlockGroup blockGroup(fVolume->ExtentTree());
	status_t status = blockGroup.Initialize(flags);
	if (status != B_OK)
		return status;
	if (start == (uint64)-1)
		start = blockGroup.Start();

	// normalize inputs
	uint64 remainder = start % fVolume->BlockSize();
	if (remainder != 0)
		start += fVolume->BlockSize() - remainder;

	status = _Allocate(found, start, fVolume->BlockSize(),
		BTRFS_EXTENT_FLAG_TREE_BLOCK);
	if (status != B_OK)
		return status;

	// check here because tree block locate in 2 blockgroups (system and
	// metadata), and there might be a case one can get over the limit.
	if (found >= blockGroup.End())
		return B_BAD_DATA;
	return B_OK;
}


// Allocate block for file data
status_t
ExtentAllocator::AllocateDataBlock(uint64& found, uint64 size, uint64 start,
	uint64 flags)
{
	// TODO: implement more features here with flags, e.g DUP, RAID, etc

	if (start == (uint64)-1) {
		BlockGroup blockGroup(fVolume->ExtentTree());
		status_t status = blockGroup.Initialize(flags);
		if (status != B_OK)
			return status;
		start = blockGroup.Start();
	}

	// normalize inputs
	uint64 remainder = start % fVolume->SectorSize();
	if (remainder != 0)
		start += fVolume->SectorSize() - remainder;
	size = size / fVolume->SectorSize() * fVolume->SectorSize();

	return _Allocate(found, start, size, BTRFS_EXTENT_FLAG_DATA);
}
