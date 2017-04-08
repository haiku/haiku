/*
 * Copyright 2001-2017, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! Inode access functions


#include "Debug.h"
#include "Inode.h"
#include "BPlusTree.h"
#include "Index.h"


#if BFS_TRACING && !defined(FS_SHELL) && !defined(_BOOT_MODE)
namespace BFSInodeTracing {

class Create : public AbstractTraceEntry {
public:
	Create(Inode* inode, Inode* parent, const char* name, int32 mode,
			int openMode, uint32 type)
		:
		fInode(inode),
		fID(inode->ID()),
		fParent(parent),
		fParentID(parent != NULL ? parent->ID() : 0),
		fMode(mode),
		fOpenMode(openMode),
		fType(type)
	{
		if (name != NULL)
			strlcpy(fName, name, sizeof(fName));
		else
			fName[0] = '\0';

		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("bfs:Create %Ld (%p), parent %Ld (%p), \"%s\", "
			"mode %lx, omode %x, type %lx", fID, fInode, fParentID,
			fParent, fName, fMode, fOpenMode, fType);
	}

private:
	Inode*	fInode;
	ino_t	fID;
	Inode*	fParent;
	ino_t	fParentID;
	char	fName[32];
	int32	fMode;
	int		fOpenMode;
	uint32	fType;
};

class Remove : public AbstractTraceEntry {
public:
	Remove(Inode* inode, const char* name)
		:
		fInode(inode),
		fID(inode->ID())
	{
		strlcpy(fName, name, sizeof(fName));
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("bfs:Remove %Ld (%p), \"%s\"", fID, fInode, fName);
	}

private:
	Inode*	fInode;
	ino_t	fID;
	char	fName[32];
};

class Action : public AbstractTraceEntry {
public:
	Action(const char* action, Inode* inode)
		:
		fInode(inode),
		fID(inode->ID())
	{
		strlcpy(fAction, action, sizeof(fAction));
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("bfs:%s %Ld (%p)\n", fAction, fID, fInode);
	}

private:
	Inode*	fInode;
	ino_t	fID;
	char	fAction[16];
};

class Resize : public AbstractTraceEntry {
public:
	Resize(Inode* inode, off_t oldSize, off_t newSize, bool trim)
		:
		fInode(inode),
		fID(inode->ID()),
		fOldSize(oldSize),
		fNewSize(newSize),
		fTrim(trim)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("bfs:%s %Ld (%p), %Ld -> %Ld", fTrim ? "Trim" : "Resize",
			fID, fInode, fOldSize, fNewSize);
	}

private:
	Inode*	fInode;
	ino_t	fID;
	off_t	fOldSize;
	off_t	fNewSize;
	bool	fTrim;
};

}	// namespace BFSInodeTracing

#	define T(x) new(std::nothrow) BFSInodeTracing::x;
#else
#	define T(x) ;
#endif


/*!	A helper class used by Inode::Create() to keep track of the belongings
	of an inode creation in progress.
	This class will make sure everything is cleaned up properly.
*/
class InodeAllocator {
public:
							InodeAllocator(Transaction& transaction);
							~InodeAllocator();

			status_t		New(block_run* parentRun, mode_t mode, uint32 flags,
								block_run& run, fs_vnode_ops* vnodeOps,
								Inode** _inode);
			status_t		CreateTree();
			status_t		Keep(fs_vnode_ops* vnodeOps, uint32 publishFlags);

private:
	static	void			_TransactionListener(int32 id, int32 event,
								void* _inode);

			Transaction*	fTransaction;
			block_run		fRun;
			Inode*			fInode;
};


InodeAllocator::InodeAllocator(Transaction& transaction)
	:
	fTransaction(&transaction),
	fInode(NULL)
{
}


InodeAllocator::~InodeAllocator()
{
	if (fTransaction != NULL) {
		Volume* volume = fTransaction->GetVolume();

		if (fInode != NULL) {
			fInode->Node().flags &= ~HOST_ENDIAN_TO_BFS_INT32(INODE_IN_USE);
				// this unblocks any pending bfs_read_vnode() calls
			fInode->Free(*fTransaction);

			if (fInode->fTree != NULL)
				fTransaction->RemoveListener(fInode->fTree);
			fTransaction->RemoveListener(fInode);

			remove_vnode(volume->FSVolume(), fInode->ID());
		} else
			volume->Free(*fTransaction, fRun);
	}

	delete fInode;
}


status_t
InodeAllocator::New(block_run* parentRun, mode_t mode, uint32 publishFlags,
	block_run& run, fs_vnode_ops* vnodeOps, Inode** _inode)
{
	Volume* volume = fTransaction->GetVolume();

	status_t status = volume->AllocateForInode(*fTransaction, parentRun, mode,
		fRun);
	if (status < B_OK) {
		// don't free the space in the destructor, because
		// the allocation failed
		fTransaction = NULL;
		RETURN_ERROR(status);
	}

	run = fRun;
	fInode = new(std::nothrow) Inode(volume, *fTransaction,
		volume->ToVnode(run), mode, run);
	if (fInode == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	if (!volume->IsInitializing()
		&& (publishFlags & BFS_DO_NOT_PUBLISH_VNODE) == 0) {
		status = new_vnode(volume->FSVolume(), fInode->ID(), fInode,
			vnodeOps != NULL ? vnodeOps : &gBFSVnodeOps);
		if (status < B_OK) {
			delete fInode;
			fInode = NULL;
			RETURN_ERROR(status);
		}
	}

	fInode->WriteLockInTransaction(*fTransaction);
	*_inode = fInode;
	return B_OK;
}


status_t
InodeAllocator::CreateTree()
{
	Volume* volume = fTransaction->GetVolume();

	// force S_STR_INDEX to be set, if no type is set
	if ((fInode->Mode() & S_INDEX_TYPES) == 0)
		fInode->Node().mode |= HOST_ENDIAN_TO_BFS_INT32(S_STR_INDEX);

	BPlusTree* tree = new(std::nothrow) BPlusTree(*fTransaction, fInode);
	if (tree == NULL)
		return B_ERROR;

	status_t status = tree->InitCheck();
	if (status != B_OK) {
		delete tree;
		return status;
	}

	fInode->fTree = tree;

	if (fInode->IsRegularNode()) {
		if (tree->Insert(*fTransaction, ".", fInode->ID()) < B_OK
			|| tree->Insert(*fTransaction, "..",
					volume->ToVnode(fInode->Parent())) < B_OK)
			return B_ERROR;
	}
	return B_OK;
}


status_t
InodeAllocator::Keep(fs_vnode_ops* vnodeOps, uint32 publishFlags)
{
	ASSERT(fInode != NULL && fTransaction != NULL);
	Volume* volume = fTransaction->GetVolume();

	status_t status = fInode->WriteBack(*fTransaction);
	if (status < B_OK) {
		FATAL(("writing new inode %" B_PRIdINO " failed!\n", fInode->ID()));
		return status;
	}

	// Symbolic links are not published -- the caller needs to do this once
	// the contents have been written.
	if (!fInode->IsSymLink() && !volume->IsInitializing()
		&& (publishFlags & BFS_DO_NOT_PUBLISH_VNODE) == 0) {
		status = publish_vnode(volume->FSVolume(), fInode->ID(), fInode,
			vnodeOps != NULL ? vnodeOps : &gBFSVnodeOps, fInode->Mode(),
			publishFlags);
	}

	if (status == B_OK) {
		cache_add_transaction_listener(volume->BlockCache(), fTransaction->ID(),
			TRANSACTION_ABORTED, &_TransactionListener, fInode);
	}

	fTransaction = NULL;
	fInode = NULL;

	return status;
}


/*static*/ void
InodeAllocator::_TransactionListener(int32 id, int32 event, void* _inode)
{
	Inode* inode = (Inode*)_inode;

	if (event == TRANSACTION_ABORTED)
		panic("transaction %d aborted, inode %p still around!\n", (int)id, inode);
}


//	#pragma mark -


status_t
bfs_inode::InitCheck(Volume* volume) const
{
	if (Magic1() != INODE_MAGIC1
		|| !(Flags() & INODE_IN_USE)
		|| inode_num.Length() != 1
		// matches inode size?
		|| (uint32)InodeSize() != volume->InodeSize()
		// parent resides on disk?
		|| parent.AllocationGroup() > int32(volume->AllocationGroups())
		|| parent.AllocationGroup() < 0
		|| parent.Start() > (1L << volume->AllocationGroupShift())
		|| parent.Length() != 1
		// attributes, too?
		|| attributes.AllocationGroup() > int32(volume->AllocationGroups())
		|| attributes.AllocationGroup() < 0
		|| attributes.Start() > (1L << volume->AllocationGroupShift()))
		RETURN_ERROR(B_BAD_DATA);

	if (Flags() & INODE_DELETED)
		return B_NOT_ALLOWED;

	// TODO: Add some tests to check the integrity of the other stuff here,
	// especially for the data_stream!

	return B_OK;
}


//	#pragma mark - Inode


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fTree(NULL),
	fAttributes(NULL),
	fCache(NULL),
	fMap(NULL)
{
	PRINT(("Inode::Inode(volume = %p, id = %Ld) @ %p\n", volume, id, this));

	rw_lock_init(&fLock, "bfs inode");
	recursive_lock_init(&fSmallDataLock, "bfs inode small data");

	if (UpdateNodeFromDisk() != B_OK) {
		// TODO: the error code gets eaten
		return;
	}

	// these two will help to maintain the indices
	fOldSize = Size();
	fOldLastModified = LastModified();

	if (IsContainer())
		fTree = new(std::nothrow) BPlusTree(this);
	if (NeedsFileCache()) {
		SetFileCache(file_cache_create(fVolume->ID(), ID(), Size()));
		SetMap(file_map_create(volume->ID(), ID(), Size()));
	}
}


Inode::Inode(Volume* volume, Transaction& transaction, ino_t id, mode_t mode,
		block_run& run)
	:
	fVolume(volume),
	fID(id),
	fTree(NULL),
	fAttributes(NULL),
	fCache(NULL),
	fMap(NULL)
{
	PRINT(("Inode::Inode(volume = %p, transaction = %p, id = %Ld) @ %p\n",
		volume, &transaction, id, this));

	rw_lock_init(&fLock, "bfs inode");
	recursive_lock_init(&fSmallDataLock, "bfs inode small data");

	NodeGetter node(volume, transaction, this, true);
	if (node.Node() == NULL) {
		FATAL(("Could not read inode block %" B_PRId64 "!\n", BlockNumber()));
		return;
	}

	memset(&fNode, 0, sizeof(bfs_inode));

	// Initialize the bfs_inode structure -- it's not written back to disk
	// here, because it will be done so already when the inode could be
	// created completely.

	Node().magic1 = HOST_ENDIAN_TO_BFS_INT32(INODE_MAGIC1);
	Node().inode_num = run;
	Node().mode = HOST_ENDIAN_TO_BFS_INT32(mode);
	Node().flags = HOST_ENDIAN_TO_BFS_INT32(INODE_IN_USE);

	Node().create_time = Node().last_modified_time = Node().status_change_time
		= HOST_ENDIAN_TO_BFS_INT64(bfs_inode::ToInode(real_time_clock_usecs()));

	Node().inode_size = HOST_ENDIAN_TO_BFS_INT32(volume->InodeSize());

	// these two will help to maintain the indices
	fOldSize = Size();
	fOldLastModified = LastModified();
}


Inode::~Inode()
{
	PRINT(("Inode::~Inode() @ %p\n", this));

	file_cache_delete(FileCache());
	file_map_delete(Map());
	delete fTree;

	rw_lock_destroy(&fLock);
	recursive_lock_destroy(&fSmallDataLock);
}


status_t
Inode::InitCheck(bool checkNode) const
{
	// test inode magic and flags
	if (checkNode) {
		status_t status = Node().InitCheck(fVolume);
		if (status == B_BUSY)
			return B_BUSY;

		if (status != B_OK) {
			FATAL(("inode at block %" B_PRIdOFF " corrupt!\n", BlockNumber()));
			RETURN_ERROR(B_BAD_DATA);
		}
	}

	if (IsContainer()) {
		// inodes that have a B+tree
		if (fTree == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		status_t status = fTree->InitCheck();
		if (status != B_OK) {
			FATAL(("inode tree at block %" B_PRIdOFF " corrupt!\n",
				BlockNumber()));
			RETURN_ERROR(B_BAD_DATA);
		}
	}

	if (NeedsFileCache() && (fCache == NULL || fMap == NULL))
		return B_NO_MEMORY;

	return B_OK;
}


/*!	Adds this inode to the specified transaction. This means that the inode will
	be write locked until the transaction ended.
	To ensure that the inode will stay valid until that point, an extra reference
	is acquired to it as long as this transaction stays active.
*/
void
Inode::WriteLockInTransaction(Transaction& transaction)
{
	// These flags can only change while holding the transaction lock
	if ((Flags() & INODE_IN_TRANSACTION) != 0)
		return;

	// We share the same list link with the removed list, so we have to remove
	// the inode from that list here (and add it back when we no longer need it)
	if ((Flags() & INODE_DELETED) != 0)
		fVolume->RemovedInodes().Remove(this);

	if (!fVolume->IsInitializing())
		acquire_vnode(fVolume->FSVolume(), ID());

	rw_lock_write_lock(&Lock());
	Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_IN_TRANSACTION);

	transaction.AddListener(this);
}


status_t
Inode::WriteBack(Transaction& transaction)
{
	NodeGetter node(fVolume, transaction, this);
	if (node.WritableNode() == NULL)
		return B_IO_ERROR;

	memcpy(node.WritableNode(), &Node(), sizeof(bfs_inode));
	return B_OK;
}


status_t
Inode::UpdateNodeFromDisk()
{
	NodeGetter node(fVolume, this);
	if (node.Node() == NULL) {
		FATAL(("Failed to read block %" B_PRId64 " from disk!\n",
			BlockNumber()));
		return B_IO_ERROR;
	}

	memcpy(&fNode, node.Node(), sizeof(bfs_inode));
	fNode.flags &= HOST_ENDIAN_TO_BFS_INT32(INODE_PERMANENT_FLAGS);
	return B_OK;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, Mode(), (gid_t)fNode.GroupID(),
		(uid_t)fNode.UserID());
}


//	#pragma mark - attributes


void
Inode::_AddIterator(AttributeIterator* iterator)
{
	RecursiveLocker _(fSmallDataLock);
	fIterators.Add(iterator);
}


void
Inode::_RemoveIterator(AttributeIterator* iterator)
{
	RecursiveLocker _(fSmallDataLock);
	fIterators.Remove(iterator);
}


/*!	Tries to free up "bytes" space in the small_data section by moving
	attributes to real files. Used for system attributes like the name.
	You need to hold the fSmallDataLock when you call this method
*/
status_t
Inode::_MakeSpaceForSmallData(Transaction& transaction, bfs_inode* node,
	const char* name, int32 bytes)
{
	ASSERT_LOCKED_RECURSIVE(&fSmallDataLock);

	while (bytes > 0) {
		small_data* item = node->SmallDataStart();
		small_data* max = NULL;
		int32 index = 0, maxIndex = 0;
		for (; !item->IsLast(node); item = item->Next(), index++) {
			// should not remove those
			if (*item->Name() == FILE_NAME_NAME || !strcmp(name, item->Name()))
				continue;

			if (max == NULL || max->Size() < item->Size()) {
				maxIndex = index;
				max = item;
			}

			// Remove the first one large enough to free the needed amount of
			// bytes
			if (bytes < (int32)item->Size())
				break;
		}

		if (item->IsLast(node) || (int32)item->Size() < bytes)
			return B_ERROR;

		bytes -= max->Size();

		// Move the attribute to a real attribute file
		// Luckily, this doesn't cause any index updates

		Inode* attribute;
		status_t status = CreateAttribute(transaction, item->Name(),
			item->Type(), &attribute);
		if (status != B_OK)
			RETURN_ERROR(status);

		size_t length = item->DataSize();
		status = attribute->WriteAt(transaction, 0, item->Data(), &length);

		ReleaseAttribute(attribute);

		if (status != B_OK) {
			Vnode vnode(fVolume, Attributes());
			Inode* attributes;
			if (vnode.Get(&attributes) < B_OK
				|| attributes->Remove(transaction, name) < B_OK) {
				FATAL(("Could not remove newly created attribute!\n"));
			}

			RETURN_ERROR(status);
		}

		_RemoveSmallData(node, max, maxIndex);
	}
	return B_OK;
}


/*!	Private function which removes the given attribute from the small_data
	section.
	You need to hold the fSmallDataLock when you call this method
*/
status_t
Inode::_RemoveSmallData(bfs_inode* node, small_data* item, int32 index)
{
	ASSERT_LOCKED_RECURSIVE(&fSmallDataLock);

	small_data* next = item->Next();
	if (!next->IsLast(node)) {
		// find the last attribute
		small_data* last = next;
		while (!last->IsLast(node))
			last = last->Next();

		int32 size = (uint8*)last - (uint8*)next;
		if (size < 0
			|| size > (uint8*)node + fVolume->BlockSize() - (uint8*)next)
			return B_BAD_DATA;

		memmove(item, next, size);

		// Move the "last" one to its new location and
		// correctly terminate the small_data section
		last = (small_data*)((uint8*)last - ((uint8*)next - (uint8*)item));
		memset(last, 0, (uint8*)node + fVolume->BlockSize() - (uint8*)last);
	} else
		memset(item, 0, item->Size());

	// update all current iterators
	SinglyLinkedList<AttributeIterator>::Iterator iterator
		= fIterators.GetIterator();
	while (iterator.HasNext()) {
		iterator.Next()->Update(index, -1);
	}

	return B_OK;
}


//!	Removes the given attribute from the small_data section.
status_t
Inode::_RemoveSmallData(Transaction& transaction, NodeGetter& nodeGetter,
	const char* name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	bfs_inode* node = nodeGetter.WritableNode();
	RecursiveLocker locker(fSmallDataLock);

	// search for the small_data item

	small_data* item = node->SmallDataStart();
	int32 index = 0;
	while (!item->IsLast(node) && strcmp(item->Name(), name)) {
		item = item->Next();
		index++;
	}

	if (item->IsLast(node))
		return B_ENTRY_NOT_FOUND;

	nodeGetter.MakeWritable(transaction);

	status_t status = _RemoveSmallData(node, item, index);
	if (status == B_OK) {
		Node().status_change_time = HOST_ENDIAN_TO_BFS_INT64(
			bfs_inode::ToInode(real_time_clock_usecs()));

		status = WriteBack(transaction);
	}

	return status;
}


/*!	Try to place the given attribute in the small_data section - if the
	new attribute is too big to fit in that section, it returns B_DEVICE_FULL.
	In that case, the attribute should be written to a real attribute file;
	it's the caller's responsibility to remove any existing attributes in the
	small data section if that's the case.

	Note that you need to write back the inode yourself after having called that
	method - it's a bad API decision that it needs a transaction but enforces
	you to write back the inode all by yourself, but it's just more efficient
	in most cases...
*/
status_t
Inode::_AddSmallData(Transaction& transaction, NodeGetter& nodeGetter,
	const char* name, uint32 type, off_t pos, const uint8* data, size_t length,
	bool force)
{
	bfs_inode* node = nodeGetter.WritableNode();

	if (node == NULL || name == NULL || data == NULL)
		return B_BAD_VALUE;

	// reject any requests that can't fit into the small_data section
	uint32 nameLength = strlen(name);
	uint32 spaceNeeded = sizeof(small_data) + nameLength + 3 + pos + length + 1;
	if (spaceNeeded > fVolume->InodeSize() - sizeof(bfs_inode))
		return B_DEVICE_FULL;

	nodeGetter.MakeWritable(transaction);
	RecursiveLocker locker(fSmallDataLock);

	// Find the last item or one with the same name we have to add
	small_data* item = node->SmallDataStart();
	int32 index = 0;
	while (!item->IsLast(node) && strcmp(item->Name(), name)) {
		item = item->Next();
		index++;
	}

	// is the attribute already in the small_data section?
	// then just replace the data part of that one
	if (!item->IsLast(node)) {
		// find last attribute
		small_data* last = item;
		while (!last->IsLast(node))
			last = last->Next();

		// try to change the attributes value
		if (item->data_size > pos + length
			|| force
			|| ((uint8*)last + pos + length - item->DataSize())
					<= ((uint8*)node + fVolume->InodeSize())) {
			// Make room for the new attribute if needed (and we are forced
			// to do so)
			if (force && ((uint8*)last + pos + length - item->DataSize())
					> ((uint8*)node + fVolume->InodeSize())) {
				// We also take the free space at the end of the small_data
				// section into account, and request only what's really needed
				uint32 needed = pos + length - item->DataSize() -
					(uint32)((uint8*)node + fVolume->InodeSize()
						- (uint8*)last);

				if (_MakeSpaceForSmallData(transaction, node, name, needed)
						!= B_OK)
					return B_ERROR;

				// reset our pointers
				item = node->SmallDataStart();
				index = 0;
				while (!item->IsLast(node) && strcmp(item->Name(), name)) {
					item = item->Next();
					index++;
				}

				last = item;
				while (!last->IsLast(node))
					last = last->Next();
			}

			size_t oldDataSize = item->DataSize();

			// Normally, we can just overwrite the attribute data as the size
			// is specified by the type and does not change that often
			if (pos + length != item->DataSize()) {
				// move the attributes after the current one
				small_data* next = item->Next();
				if (!next->IsLast(node)) {
					memmove((uint8*)item + spaceNeeded, next,
						(uint8*)last - (uint8*)next);
				}

				// Move the "last" one to its new location and
				// correctly terminate the small_data section
				last = (small_data*)((uint8*)last
					- ((uint8*)next - ((uint8*)item + spaceNeeded)));
				if ((uint8*)last < (uint8*)node + fVolume->BlockSize()) {
					memset(last, 0, (uint8*)node + fVolume->BlockSize()
						- (uint8*)last);
				}

				item->data_size = HOST_ENDIAN_TO_BFS_INT16(pos + length);
			}

			item->type = HOST_ENDIAN_TO_BFS_INT32(type);

			if ((uint64)oldDataSize < (uint64)pos) {
				// Fill gap with zeros
				memset(item->Data() + oldDataSize, 0, pos - oldDataSize);
			}
			memcpy(item->Data() + pos, data, length);
			item->Data()[pos + length] = '\0';

			return B_OK;
		}

		return B_DEVICE_FULL;
	}

	// try to add the new attribute!

	if ((uint8*)item + spaceNeeded > (uint8*)node + fVolume->InodeSize()) {
		// there is not enough space for it!
		if (!force)
			return B_DEVICE_FULL;

		// make room for the new attribute
		if (_MakeSpaceForSmallData(transaction, node, name, spaceNeeded) < B_OK)
			return B_ERROR;

		// get new last item!
		item = node->SmallDataStart();
		index = 0;
		while (!item->IsLast(node)) {
			item = item->Next();
			index++;
		}
	}

	memset(item, 0, spaceNeeded);
	item->type = HOST_ENDIAN_TO_BFS_INT32(type);
	item->name_size = HOST_ENDIAN_TO_BFS_INT16(nameLength);
	item->data_size = HOST_ENDIAN_TO_BFS_INT16(length);
	strcpy(item->Name(), name);
	memcpy(item->Data() + pos, data, length);

	// correctly terminate the small_data section
	item = item->Next();
	if (!item->IsLast(node))
		memset(item, 0, (uint8*)node + fVolume->InodeSize() - (uint8*)item);

	// update all current iterators
	SinglyLinkedList<AttributeIterator>::Iterator iterator
		= fIterators.GetIterator();
	while (iterator.HasNext()) {
		iterator.Next()->Update(index, 1);
	}

	return B_OK;
}


/*!	Iterates through the small_data section of an inode.
	To start at the beginning of this section, you let smallData
	point to NULL, like:
		small_data* data = NULL;
		while (inode->GetNextSmallData(&data) { ... }

	This function is reentrant and doesn't allocate any memory;
	you can safely stop calling it at any point (you don't need
	to iterate through the whole list).
	You need to hold the fSmallDataLock when you call this method
*/
status_t
Inode::_GetNextSmallData(bfs_inode* node, small_data** _smallData) const
{
	if (node == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	ASSERT_LOCKED_RECURSIVE(&fSmallDataLock);

	small_data* data = *_smallData;

	// begin from the start?
	if (data == NULL)
		data = node->SmallDataStart();
	else
		data = data->Next();

	// is already last item?
	if (data->IsLast(node))
		return B_ENTRY_NOT_FOUND;

	*_smallData = data;

	return B_OK;
}


/*!	Finds the attribute "name" in the small data section, and
	returns a pointer to it (or NULL if it doesn't exist).
	You need to hold the fSmallDataLock when you call this method
*/
small_data*
Inode::FindSmallData(const bfs_inode* node, const char* name) const
{
	ASSERT_LOCKED_RECURSIVE(&fSmallDataLock);

	small_data* smallData = NULL;
	while (_GetNextSmallData(const_cast<bfs_inode*>(node), &smallData)
			== B_OK) {
		if (!strcmp(smallData->Name(), name))
			return smallData;
	}
	return NULL;
}


/*!	Returns a pointer to the node's name if present in the small data
	section, NULL otherwise.
	You need to hold the fSmallDataLock when you call this method
*/
const char*
Inode::Name(const bfs_inode* node) const
{
	ASSERT_LOCKED_RECURSIVE(&fSmallDataLock);

	small_data* smallData = NULL;
	while (_GetNextSmallData((bfs_inode*)node, &smallData) == B_OK) {
		if (*smallData->Name() == FILE_NAME_NAME
			&& smallData->NameSize() == FILE_NAME_NAME_LENGTH)
			return (const char*)smallData->Data();
	}
	return NULL;
}


/*!	Copies the node's name into the provided buffer.
	The buffer should be B_FILE_NAME_LENGTH bytes large.
*/
status_t
Inode::GetName(char* buffer, size_t size) const
{
	NodeGetter node(fVolume, this);
	if (node.Node() == NULL)
		return B_IO_ERROR;

	RecursiveLocker locker(fSmallDataLock);

	const char* name = Name(node.Node());
	if (name == NULL)
		return B_ENTRY_NOT_FOUND;

	strlcpy(buffer, name, size);
	return B_OK;
}


/*!	Changes or set the name of a file: in the inode small_data section only, it
	doesn't change it in the parent directory's b+tree.
	Note that you need to write back the inode yourself after having called
	that method. It suffers from the same API decision as AddSmallData() does
	(and for the same reason).
*/
status_t
Inode::SetName(Transaction& transaction, const char* name)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	NodeGetter node(fVolume, transaction, this);
	if (node.Node() == NULL)
		return B_IO_ERROR;

	const char nameTag[2] = {FILE_NAME_NAME, 0};

	return _AddSmallData(transaction, node, nameTag, FILE_NAME_TYPE, 0,
		(uint8*)name, strlen(name), true);
}


status_t
Inode::_RemoveAttribute(Transaction& transaction, const char* name,
	bool hasIndex, Index* index)
{
	// remove the attribute file if it exists
	Vnode vnode(fVolume, Attributes());
	Inode* attributes;
	status_t status = vnode.Get(&attributes);
	if (status < B_OK)
		return status;

	// update index
	if (index != NULL) {
		Inode* attribute;
		if ((hasIndex || fVolume->CheckForLiveQuery(name))
			&& GetAttribute(name, &attribute) == B_OK) {
			uint8 data[MAX_INDEX_KEY_LENGTH];
			size_t length = MAX_INDEX_KEY_LENGTH;
			if (attribute->ReadAt(0, data, &length) == B_OK) {
				index->Update(transaction, name, attribute->Type(), data,
					length, NULL, 0, this);
			}

			ReleaseAttribute(attribute);
		}
	}

	if ((status = attributes->Remove(transaction, name)) < B_OK)
		return status;

	if (attributes->IsEmpty()) {
		attributes->WriteLockInTransaction(transaction);

		// remove attribute directory (don't fail if that can't be done)
		if (remove_vnode(fVolume->FSVolume(), attributes->ID()) == B_OK) {
			// update the inode, so that no one will ever doubt it's deleted :-)
			attributes->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED);
			if (attributes->WriteBack(transaction) == B_OK) {
				Attributes().SetTo(0, 0, 0);
				WriteBack(transaction);
			} else {
				unremove_vnode(fVolume->FSVolume(), attributes->ID());
				attributes->Node().flags
					&= ~HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED);
			}
		}
	}

	return status;
}


/*!	Reads data from the specified attribute.
	This is a high-level attribute function that understands attributes
	in the small_data section as well as real attribute files.
*/
status_t
Inode::ReadAttribute(const char* name, int32 type, off_t pos, uint8* buffer,
	size_t* _length)
{
	if (pos < 0)
		pos = 0;

	// search in the small_data section (which has to be locked first)
	{
		NodeGetter node(fVolume, this);
		if (node.Node() == NULL)
			return B_IO_ERROR;

		RecursiveLocker locker(fSmallDataLock);

		small_data* smallData = FindSmallData(node.Node(), name);
		if (smallData != NULL) {
			size_t length = *_length;
			if (pos >= smallData->data_size) {
				*_length = 0;
				return B_OK;
			}
			if (length + pos > smallData->DataSize())
				length = smallData->DataSize() - pos;

			memcpy(buffer, smallData->Data() + pos, length);
			*_length = length;
			return B_OK;
		}
	}

	// search in the attribute directory
	Inode* attribute;
	status_t status = GetAttribute(name, &attribute);
	if (status == B_OK) {
		status = attribute->ReadAt(pos, (uint8*)buffer, _length);

		ReleaseAttribute(attribute);
	}

	RETURN_ERROR(status);
}


/*!	Writes data to the specified attribute.
	This is a high-level attribute function that understands attributes
	in the small_data section as well as real attribute files.
*/
status_t
Inode::WriteAttribute(Transaction& transaction, const char* name, int32 type,
	off_t pos, const uint8* buffer, size_t* _length, bool* _created)
{
	if (pos < 0)
		return B_BAD_VALUE;

	// needed to maintain the index
	uint8 oldBuffer[MAX_INDEX_KEY_LENGTH];
	uint8* oldData = NULL;
	size_t oldLength = 0;
	bool created = false;

	// TODO: we actually depend on that the contents of "buffer" are constant.
	// If they get changed during the write (hey, user programs), we may mess
	// up our index trees!
	// TODO: for attribute files, we need to log the first
	// MAX_INDEX_KEY_LENGTH bytes of the data stream, or the same as above
	// might happen.

	Index index(fVolume);
	bool hasIndex = index.SetTo(name) == B_OK;

	Inode* attribute = NULL;
	status_t status = B_OK;

	if (GetAttribute(name, &attribute) != B_OK) {
		// No attribute inode exists yet

		// save the old attribute data
		NodeGetter node(fVolume, transaction, this);
		if (node.Node() == NULL)
			return B_IO_ERROR;

		recursive_lock_lock(&fSmallDataLock);

		small_data* smallData = FindSmallData(node.Node(), name);
		if (smallData != NULL) {
			oldLength = smallData->DataSize();
			if (oldLength > 0) {
				if (oldLength > MAX_INDEX_KEY_LENGTH)
					oldLength = MAX_INDEX_KEY_LENGTH;
				memcpy(oldData = oldBuffer, smallData->Data(), oldLength);
			}
		} else
			created = true;

		recursive_lock_unlock(&fSmallDataLock);

		// if the attribute doesn't exist yet (as a file), try to put it in the
		// small_data section first - if that fails (due to insufficent space),
		// create a real attribute file
		status = _AddSmallData(transaction, node, name, type, pos, buffer,
			*_length);
		if (status == B_DEVICE_FULL) {
			if (smallData != NULL) {
				// remove the old attribute from the small data section - there
				// is no space left for the new data
				status = _RemoveSmallData(transaction, node, name);
			} else
				status = B_OK;

			if (status == B_OK)
				status = CreateAttribute(transaction, name, type, &attribute);
			if (status != B_OK)
				RETURN_ERROR(status);

			created = true;
		} else if (status == B_OK) {
			// Update status time on attribute write
			Node().status_change_time = HOST_ENDIAN_TO_BFS_INT64(
				bfs_inode::ToInode(real_time_clock_usecs()));

			status = WriteBack(transaction);
		}
	}

	if (attribute != NULL) {
		WriteLocker writeLocker(attribute->fLock);

		if (hasIndex || fVolume->CheckForLiveQuery(name)) {
			// Save the old attribute data (if this fails, oldLength will
			// reflect it)
			while (attribute->Size() > 0) {
				bigtime_t oldModified = attribute->LastModified();
				writeLocker.Unlock();

				oldLength = MAX_INDEX_KEY_LENGTH;
				if (attribute->ReadAt(0, oldBuffer, &oldLength) == B_OK)
					oldData = oldBuffer;

				writeLocker.Lock();

				// Read until the data hasn't changed in between
				if (oldModified == attribute->LastModified())
					break;

				oldLength = 0;
			}
		}

		// check if the data fits into the small_data section again
		NodeGetter node(fVolume, transaction, this);
		if (node.Node() == NULL)
			return B_IO_ERROR;

		status = _AddSmallData(transaction, node, name, type, pos, buffer,
			*_length);

		if (status == B_OK) {
			// it does - remove its file
			writeLocker.Unlock();
			status = _RemoveAttribute(transaction, name, false, NULL);
		} else {
			// The attribute type might have been changed - we need to
			// adopt the new one
			attribute->Node().type = HOST_ENDIAN_TO_BFS_INT32(type);
			status = attribute->WriteBack(transaction);
			writeLocker.Unlock();

			if (status == B_OK) {
				status = attribute->WriteAt(transaction, pos, buffer,
					_length);
			}
		}

		if (status == B_OK) {
			// Update status time on attribute write
			Node().status_change_time = HOST_ENDIAN_TO_BFS_INT64(
				bfs_inode::ToInode(real_time_clock_usecs()));

			status = WriteBack(transaction);
		}

		attribute->WriteLockInTransaction(transaction);
		ReleaseAttribute(attribute);
	}

	// TODO: find a better way than this "pos" thing (the begin of the old key
	//	must be copied to the start of the new one for a comparison)
	if (status == B_OK && pos == 0) {
		// Index only the first MAX_INDEX_KEY_LENGTH bytes
		uint16 length = *_length;
		if (length > MAX_INDEX_KEY_LENGTH)
			length = MAX_INDEX_KEY_LENGTH;

		// Update index. Note, Index::Update() may be called even if
		// initializing the index failed - it will just update the live
		// queries in this case
		if (pos < length || (uint64)pos < (uint64)oldLength) {
			index.Update(transaction, name, type, oldData, oldLength, buffer,
				length, this);
		}
	}

	if (_created != NULL)
		*_created = created;

	return status;
}


/*!	Removes the specified attribute from the inode.
	This is a high-level attribute function that understands attributes
	in the small_data section as well as real attribute files.
*/
status_t
Inode::RemoveAttribute(Transaction& transaction, const char* name)
{
	Index index(fVolume);
	bool hasIndex = index.SetTo(name) == B_OK;
	NodeGetter node(fVolume, this);
	if (node.Node() == NULL)
		return B_IO_ERROR;

	// update index for attributes in the small_data section
	{
		RecursiveLocker _(fSmallDataLock);

		small_data* smallData = FindSmallData(node.Node(), name);
		if (smallData != NULL) {
			uint32 length = smallData->DataSize();
			if (length > MAX_INDEX_KEY_LENGTH)
				length = MAX_INDEX_KEY_LENGTH;
			index.Update(transaction, name, smallData->Type(),
				smallData->Data(), length, NULL, 0, this);
		}
	}

	status_t status = _RemoveSmallData(transaction, node, name);
	if (status == B_ENTRY_NOT_FOUND && !Attributes().IsZero()) {
		// remove the attribute file if it exists
		status = _RemoveAttribute(transaction, name, hasIndex, &index);
		if (status == B_OK) {
			Node().status_change_time = HOST_ENDIAN_TO_BFS_INT64(
				bfs_inode::ToInode(real_time_clock_usecs()));
			WriteBack(transaction);
		}
	}

	return status;
}


/*!	Returns the attribute inode with the specified \a name, in case it exists.
	This method can only return real attribute files; the attributes in the
	small data section are ignored.
*/
status_t
Inode::GetAttribute(const char* name, Inode** _attribute)
{
	// does this inode even have attributes?
	if (Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	Vnode vnode(fVolume, Attributes());
	Inode* attributes;
	if (vnode.Get(&attributes) < B_OK) {
		FATAL(("get_vnode() failed in Inode::GetAttribute(name = \"%s\")\n",
			name));
		return B_ERROR;
	}

	BPlusTree* tree = attributes->Tree();
	if (tree == NULL)
		return B_BAD_VALUE;

	InodeReadLocker locker(attributes);

	ino_t id;
	status_t status = tree->Find((uint8*)name, (uint16)strlen(name), &id);
	if (status == B_OK) {
		Vnode vnode(fVolume, id);
		Inode* inode;
		// Check if the attribute is really an attribute
		if (vnode.Get(&inode) != B_OK || !inode->IsAttribute())
			return B_ERROR;

		*_attribute = inode;
		vnode.Keep();
		return B_OK;
	}

	return status;
}


void
Inode::ReleaseAttribute(Inode* attribute)
{
	if (attribute == NULL)
		return;

	put_vnode(fVolume->FSVolume(), attribute->ID());
}


status_t
Inode::CreateAttribute(Transaction& transaction, const char* name, uint32 type,
	Inode** attribute)
{
	// do we need to create the attribute directory first?
	if (Attributes().IsZero()) {
		status_t status = Inode::Create(transaction, this, NULL,
			S_ATTR_DIR | S_DIRECTORY | 0666, 0, 0, NULL);
		if (status < B_OK)
			RETURN_ERROR(status);
	}
	Vnode vnode(fVolume, Attributes());
	Inode* attributes;
	if (vnode.Get(&attributes) < B_OK)
		return B_ERROR;

	// Inode::Create() locks the inode for us
	return Inode::Create(transaction, attributes, name,
		S_ATTR | S_FILE | 0666, 0, type, NULL, NULL, attribute);
}


//	#pragma mark - directory tree


bool
Inode::IsEmpty()
{
	TreeIterator iterator(fTree);

	uint32 count = 0;
	char name[MAX_INDEX_KEY_LENGTH + 1];
	uint16 length;
	ino_t id;
	while (iterator.GetNextEntry(name, &length, MAX_INDEX_KEY_LENGTH + 1,
			&id) == B_OK) {
		if ((Mode() & (S_ATTR_DIR | S_INDEX_DIR)) != 0)
			return false;

		// Unlike index and attribute directories, directories
		// for standard files always contain ".", and "..", so
		// we need to ignore those two
		if (++count > 2 || (strcmp(".", name) != 0 && strcmp("..", name) != 0))
			return false;
	}
	return true;
}


status_t
Inode::ContainerContentsChanged(Transaction& transaction)
{
	ASSERT(!InLastModifiedIndex());

	Node().last_modified_time = Node().status_change_time
		= HOST_ENDIAN_TO_BFS_INT64(bfs_inode::ToInode(real_time_clock_usecs()));

	return WriteBack(transaction);
}


//	#pragma mark - data stream


/*!	Computes the number of bytes this inode occupies in the file system.
	This includes the file meta blocks used for maintaining its data stream.

	TODO: However, the attributes in extra files are not really accounted for;
	depending on the speed penalty, this should be changed, though (the value
	could be cached in the inode structure or Inode object, though).
*/
off_t
Inode::AllocatedSize() const
{
	if (IsSymLink() && (Flags() & INODE_LONG_SYMLINK) == 0) {
		// This symlink does not have a data stream
		return Node().InodeSize();
	}

	const data_stream& data = Node().data;
	uint32 blockSize = fVolume->BlockSize();
	off_t size = blockSize;

	if (data.MaxDoubleIndirectRange() != 0) {
		off_t doubleIndirectSize = data.MaxDoubleIndirectRange()
			- data.MaxIndirectRange();
		int32 indirectSize = double_indirect_max_indirect_size(
			data.double_indirect.Length(), fVolume->BlockSize());

		size += (2 * data.double_indirect.Length()
				+ doubleIndirectSize / indirectSize)
			* blockSize + data.MaxDoubleIndirectRange();
	} else if (data.MaxIndirectRange() != 0)
		size += data.indirect.Length() + data.MaxIndirectRange();
	else
		size += data.MaxDirectRange();

	if (!Node().attributes.IsZero()) {
		// TODO: to make this exact, we'd had to count all attributes
		size += 2 * blockSize;
			// 2 blocks, one for the attributes inode, one for its B+tree
	}

	return size;
}


/*!	Finds the block_run where "pos" is located in the data_stream of
	the inode.
	If successful, "offset" will then be set to the file offset
	of the block_run returned; so "pos - offset" is for the block_run
	what "pos" is for the whole stream.
	The caller has to make sure that "pos" is inside the stream.
*/
status_t
Inode::FindBlockRun(off_t pos, block_run& run, off_t& offset)
{
	data_stream* data = &Node().data;

	// find matching block run

	if (data->MaxIndirectRange() > 0 && pos >= data->MaxDirectRange()) {
		if (data->MaxDoubleIndirectRange() > 0
			&& pos >= data->MaxIndirectRange()) {
			// access to double indirect blocks

			CachedBlock cached(fVolume);

			int32 runsPerBlock;
			int32 directSize;
			int32 indirectSize;
			get_double_indirect_sizes(data->double_indirect.Length(),
				fVolume->BlockSize(), runsPerBlock, directSize, indirectSize);
			if (directSize <= 0 || indirectSize <= 0)
				RETURN_ERROR(B_BAD_DATA);

			off_t start = pos - data->MaxIndirectRange();
			int32 index = start / indirectSize;

			block_run* indirect = (block_run*)cached.SetTo(
				fVolume->ToBlock(data->double_indirect) + index / runsPerBlock);
			if (indirect == NULL)
				RETURN_ERROR(B_ERROR);

			int32 current = (start % indirectSize) / directSize;

			indirect = (block_run*)cached.SetTo(
				fVolume->ToBlock(indirect[index % runsPerBlock])
				+ current / runsPerBlock);
			if (indirect == NULL)
				RETURN_ERROR(B_ERROR);

			run = indirect[current % runsPerBlock];
			if (run.Length() != data->double_indirect.Length())
				RETURN_ERROR(B_BAD_DATA);

			offset = data->MaxIndirectRange() + (index * indirectSize)
				+ (current * directSize);
		} else {
			// access to indirect blocks

			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
			off_t runBlockEnd = data->MaxDirectRange();

			CachedBlock cached(fVolume);
			off_t block = fVolume->ToBlock(data->indirect);

			for (int32 i = 0; i < data->indirect.Length(); i++) {
				block_run* indirect = (block_run*)cached.SetTo(block + i);
				if (indirect == NULL)
					RETURN_ERROR(B_IO_ERROR);

				int32 current = -1;
				while (++current < runsPerBlock) {
					if (indirect[current].IsZero())
						break;

					runBlockEnd += (uint32)indirect[current].Length()
						<< cached.BlockShift();
					if (runBlockEnd > pos) {
						run = indirect[current];
						offset = runBlockEnd - ((uint32)run.Length()
							<< cached.BlockShift());
						return fVolume->ValidateBlockRun(run);
					}
				}
			}
			RETURN_ERROR(B_ERROR);
		}
	} else {
		// access from direct blocks

		off_t runBlockEnd = 0LL;
		int32 current = -1;

		while (++current < NUM_DIRECT_BLOCKS) {
			if (data->direct[current].IsZero())
				break;

			runBlockEnd += (uint32)data->direct[current].Length()
				<< fVolume->BlockShift();
			if (runBlockEnd > pos) {
				run = data->direct[current];
				offset = runBlockEnd
					- ((uint32)run.Length() << fVolume->BlockShift());
				return fVolume->ValidateBlockRun(run);
			}
		}

		return B_ENTRY_NOT_FOUND;
	}
	return fVolume->ValidateBlockRun(run);
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;

	InodeReadLocker locker(this);

	if (pos >= Size() || length == 0) {
		*_length = 0;
		return B_NO_ERROR;
	}

	locker.Unlock();

	return file_cache_read(FileCache(), NULL, pos, buffer, _length);
}


status_t
Inode::WriteAt(Transaction& transaction, off_t pos, const uint8* buffer,
	size_t* _length)
{
	InodeReadLocker locker(this);

	// update the last modification time in memory, it will be written
	// back to the inode, and the index when the file is closed
	// TODO: should update the internal last modified time only at this point!
	Node().last_modified_time = Node().status_change_time
		= HOST_ENDIAN_TO_BFS_INT64(bfs_inode::ToInode(real_time_clock_usecs()));

	// TODO: support INODE_LOGGED!

	size_t length = *_length;
	bool changeSize = (uint64)pos + (uint64)length > (uint64)Size();

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;

	locker.Unlock();

	// the transaction doesn't have to be started already
	if (changeSize && !transaction.IsStarted())
		transaction.Start(fVolume, BlockNumber());

	WriteLocker writeLocker(fLock);

	// Work around possible race condition: Someone might have shrunken the file
	// while we had no lock.
	if (!transaction.IsStarted()
		&& (uint64)pos + (uint64)length > (uint64)Size()) {
		writeLocker.Unlock();
		transaction.Start(fVolume, BlockNumber());
		writeLocker.Lock();
	}

	off_t oldSize = Size();

	if ((uint64)pos + (uint64)length > (uint64)oldSize) {
		// let's grow the data stream to the size needed
		status_t status = SetFileSize(transaction, pos + length);
		if (status != B_OK) {
			*_length = 0;
			WriteLockInTransaction(transaction);
			RETURN_ERROR(status);
		}
		// TODO: In theory we would need to update the file size
		// index here as part of the current transaction - this might just
		// be a bit too expensive, but worth a try.

		// we need to write back the inode here because it has to
		// go into this transaction (we cannot wait until the file
		// is closed)
		status = WriteBack(transaction);
		if (status != B_OK) {
			WriteLockInTransaction(transaction);
			return status;
		}
	}

	writeLocker.Unlock();

	if (oldSize < pos)
		FillGapWithZeros(oldSize, pos);

	// If we don't want to write anything, we can now return (we may
	// just have changed the file size using the position parameter)
	if (length == 0)
		return B_OK;

	status_t status = file_cache_write(FileCache(), NULL, pos, buffer, _length);

	if (transaction.IsStarted())
		WriteLockInTransaction(transaction);

	return status;
}


/*!	Fills the gap between the old file size and the new file size
	with zeros.
	It's more or less a copy of Inode::WriteAt() but it can handle
	length differences of more than just 4 GB, and it never uses
	the log, even if the INODE_LOGGED flag is set.
*/
status_t
Inode::FillGapWithZeros(off_t pos, off_t newSize)
{
	while (pos < newSize) {
		size_t size;
		if (newSize > pos + 1024 * 1024 * 1024)
			size = 1024 * 1024 * 1024;
		else
			size = newSize - pos;

		status_t status = file_cache_write(FileCache(), NULL, pos, NULL, &size);
		if (status < B_OK)
			return status;

		pos += size;
	}

	return B_OK;
}


/*!	Allocates \a length blocks, and clears their contents. Growing
	the indirect and double indirect range uses this method.
	The allocated block_run is saved in "run"
*/
status_t
Inode::_AllocateBlockArray(Transaction& transaction, block_run& run,
	size_t length, bool variableSize)
{
	if (!run.IsZero())
		return B_BAD_VALUE;

	status_t status = fVolume->Allocate(transaction, this, length, run,
		variableSize ? 1 : length);
	if (status != B_OK)
		return status;

	// make sure those blocks are empty
	CachedBlock cached(fVolume);
	off_t block = fVolume->ToBlock(run);

	for (int32 i = 0; i < run.Length(); i++) {
		block_run* runs = (block_run*)cached.SetToWritable(transaction,
			block + i, true);
		if (runs == NULL)
			return B_IO_ERROR;
	}
	return B_OK;
}


/*!	Grows the stream to \a size, and fills the direct/indirect/double indirect
	ranges with the runs.
	This method will also determine the size of the preallocation, if any.
*/
status_t
Inode::_GrowStream(Transaction& transaction, off_t size)
{
	data_stream* data = &Node().data;

	// is the data stream already large enough to hold the new size?
	// (can be the case with preallocated blocks)
	if (size < data->MaxDirectRange()
		|| size < data->MaxIndirectRange()
		|| size < data->MaxDoubleIndirectRange()) {
		data->size = HOST_ENDIAN_TO_BFS_INT64(size);
		return B_OK;
	}

	// how many bytes are still needed? (unused ranges are always zero)
	uint16 minimum = 1;
	off_t bytes;
	if (data->Size() < data->MaxDoubleIndirectRange()) {
		bytes = size - data->MaxDoubleIndirectRange();
		// The double indirect range can only handle multiples of
		// its base length
		minimum = data->double_indirect.Length();
	} else if (data->Size() < data->MaxIndirectRange())
		bytes = size - data->MaxIndirectRange();
	else if (data->Size() < data->MaxDirectRange())
		bytes = size - data->MaxDirectRange();
	else {
		// no preallocation left to be used
		bytes = size - data->Size();
		if (data->MaxDoubleIndirectRange() > 0)
			minimum = data->double_indirect.Length();
	}

	// do we have enough free blocks on the disk?
	off_t blocksNeeded = (bytes + fVolume->BlockSize() - 1)
		>> fVolume->BlockShift();
	if (blocksNeeded > fVolume->FreeBlocks())
		return B_DEVICE_FULL;

	off_t blocksRequested = blocksNeeded;
		// because of preallocations and partial allocations, the number of
		// blocks we need to allocate may be different from the one we request
		// from the block allocator

	// Should we preallocate some blocks?
	// Attributes, attribute directories, and long symlinks usually won't get
	// that big, and should stay close to the inode - preallocating could be
	// counterproductive.
	// Also, if free disk space is tight, don't preallocate.
	if (!IsAttribute() && !IsAttributeDirectory() && !IsSymLink()
		&& fVolume->FreeBlocks() > 128) {
		off_t roundTo = 0;
		if (IsFile()) {
			// Request preallocated blocks depending on the file size and growth
			if (size < 1 * 1024 * 1024 && bytes < 512 * 1024) {
				// Preallocate 64 KB for file sizes <1 MB and grow rates <512 KB
				roundTo = 65536 >> fVolume->BlockShift();
			} else if (size < 32 * 1024 * 1024 && bytes <= 1 * 1024 * 1024) {
				// Preallocate 512 KB for file sizes between 1 MB and 32 MB, and
				// grow rates smaller than 1 MB
				roundTo = (512 * 1024) >> fVolume->BlockShift();
			} else {
				// Preallocate 1/16 of the file size (ie. 4 MB for 64 MB,
				// 64 MB for 1 GB)
				roundTo = size >> (fVolume->BlockShift() + 4);
			}
		} else if (IsIndex()) {
			// Always preallocate 64 KB for index directories
			roundTo = 65536 >> fVolume->BlockShift();
		} else {
			// Preallocate only 4 KB - directories only get trimmed when their
			// vnode is flushed, which might not happen very often.
			roundTo = 4096 >> fVolume->BlockShift();
		}
		if (roundTo > 1) {
			// Round to next "roundTo" block count
			blocksRequested = ((blocksNeeded + roundTo) / roundTo) * roundTo;
		}
	}

	while (blocksNeeded > 0) {
		// the requested blocks do not need to be returned with a
		// single allocation, so we need to iterate until we have
		// enough blocks allocated
		if (minimum > 1) {
			// make sure that "blocks" is a multiple of minimum
			blocksRequested = round_up(blocksRequested, minimum);
		}

		block_run run;
		status_t status = fVolume->Allocate(transaction, this, blocksRequested,
			run, minimum);
		if (status != B_OK)
			return status;

		// okay, we have the needed blocks, so just distribute them to the
		// different ranges of the stream (direct, indirect & double indirect)

		blocksNeeded -= run.Length();
		// don't preallocate if the first allocation was already too small
		blocksRequested = blocksNeeded;

		// Direct block range

		if (data->Size() <= data->MaxDirectRange()) {
			// let's try to put them into the direct block range
			int32 free = 0;
			for (; free < NUM_DIRECT_BLOCKS; free++) {
				if (data->direct[free].IsZero())
					break;
			}

			if (free < NUM_DIRECT_BLOCKS) {
				// can we merge the last allocated run with the new one?
				int32 last = free - 1;
				if (free > 0 && data->direct[last].MergeableWith(run)) {
					data->direct[last].length = HOST_ENDIAN_TO_BFS_INT16(
						data->direct[last].Length() + run.Length());
				} else
					data->direct[free] = run;

				data->max_direct_range = HOST_ENDIAN_TO_BFS_INT64(
					data->MaxDirectRange()
					+ run.Length() * fVolume->BlockSize());
				data->size = HOST_ENDIAN_TO_BFS_INT64(blocksNeeded > 0
					? data->max_direct_range : size);
				continue;
			}
		}

		// Indirect block range

		if (data->Size() <= data->MaxIndirectRange()
			|| !data->MaxIndirectRange()) {
			CachedBlock cached(fVolume);
			block_run* runs = NULL;
			uint32 free = 0;
			off_t block;

			// if there is no indirect block yet, create one
			if (data->indirect.IsZero()) {
				status = _AllocateBlockArray(transaction, data->indirect,
					NUM_ARRAY_BLOCKS, true);
				if (status != B_OK)
					return status;

				data->max_indirect_range = HOST_ENDIAN_TO_BFS_INT64(
					data->MaxDirectRange());
				// insert the block_run in the first block
				runs = (block_run*)cached.SetTo(data->indirect);
			} else {
				uint32 numberOfRuns = fVolume->BlockSize() / sizeof(block_run);
				block = fVolume->ToBlock(data->indirect);

				// search first empty entry
				int32 i = 0;
				for (; i < data->indirect.Length(); i++) {
					if ((runs = (block_run*)cached.SetTo(block + i)) == NULL)
						return B_IO_ERROR;

					for (free = 0; free < numberOfRuns; free++)
						if (runs[free].IsZero())
							break;

					if (free < numberOfRuns)
						break;
				}
				if (i == data->indirect.Length())
					runs = NULL;
			}

			if (runs != NULL) {
				// try to insert the run to the last one - note that this
				// doesn't take block borders into account, so it could be
				// further optimized
				cached.MakeWritable(transaction);

				int32 last = free - 1;
				if (free > 0 && runs[last].MergeableWith(run)) {
					runs[last].length = HOST_ENDIAN_TO_BFS_INT16(
						runs[last].Length() + run.Length());
				} else
					runs[free] = run;

				data->max_indirect_range = HOST_ENDIAN_TO_BFS_INT64(
					data->MaxIndirectRange()
					+ ((uint32)run.Length() << fVolume->BlockShift()));
				data->size = HOST_ENDIAN_TO_BFS_INT64(blocksNeeded > 0
					? data->MaxIndirectRange() : size);
				continue;
			}
		}

		// Double indirect block range

		if (data->Size() <= data->MaxDoubleIndirectRange()
			|| !data->max_double_indirect_range) {
			// We make sure here that we have this minimum allocated, so if
			// the allocation succeeds, we don't run into an endless loop.
			if (!data->max_double_indirect_range)
				minimum = _DoubleIndirectBlockLength();
			else
				minimum = data->double_indirect.Length();

			if ((run.Length() % minimum) != 0) {
				// The number of allocated blocks isn't a multiple of 'minimum',
				// so we have to change this. This can happen the first time the
				// stream grows into the double indirect range.
				// First, free the remaining blocks that don't fit into this
				// multiple.
				int32 rest = run.Length() % minimum;
				run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - rest);

				status = fVolume->Free(transaction,
					block_run::Run(run.AllocationGroup(),
					run.Start() + run.Length(), rest));
				if (status != B_OK)
					return status;

				blocksNeeded += rest;
				blocksRequested = round_up(blocksNeeded, minimum);

				// Are there any blocks left in the run? If not, allocate
				// a new one
				if (run.length == 0)
					continue;
			}

			// if there is no double indirect block yet, create one
			if (data->double_indirect.IsZero()) {
				status = _AllocateBlockArray(transaction,
					data->double_indirect, _DoubleIndirectBlockLength());
				if (status != B_OK)
					return status;

				data->max_double_indirect_range = data->max_indirect_range;
			}

			// calculate the index where to insert the new blocks

			int32 runsPerBlock;
			int32 directSize;
			int32 indirectSize;
			get_double_indirect_sizes(data->double_indirect.Length(),
				fVolume->BlockSize(), runsPerBlock, directSize, indirectSize);
			if (directSize <= 0 || indirectSize <= 0)
				return B_BAD_DATA;

			off_t start = data->MaxDoubleIndirectRange()
				- data->MaxIndirectRange();
			int32 indirectIndex = start / indirectSize;
			int32 index = (start % indirectSize) / directSize;
			int32 runsPerArray = runsPerBlock * minimum;

			// distribute the blocks to the array and allocate
			// new array blocks when needed

			CachedBlock cached(fVolume);
			CachedBlock cachedDirect(fVolume);
			block_run* array = NULL;
			uint32 runLength = run.Length();

			while (run.length != 0) {
				// get the indirect array block
				if (array == NULL) {
					uint32 block = indirectIndex / runsPerBlock;
					if (block >= minimum)
						return EFBIG;

					array = (block_run*)cached.SetTo(fVolume->ToBlock(
						data->double_indirect) + block);
					if (array == NULL)
						return B_IO_ERROR;
				}

				do {
					// do we need a new array block?
					if (array[indirectIndex % runsPerBlock].IsZero()) {
						cached.MakeWritable(transaction);

						status = _AllocateBlockArray(transaction,
							array[indirectIndex % runsPerBlock],
							data->double_indirect.Length());
						if (status != B_OK)
							return status;
					}

					block_run* runs = (block_run*)cachedDirect.SetToWritable(
						transaction, fVolume->ToBlock(array[indirectIndex
							% runsPerBlock]) + index / runsPerBlock);
					if (runs == NULL)
						return B_IO_ERROR;

					do {
						// insert the block_run into the array
						runs[index % runsPerBlock] = run;
						runs[index % runsPerBlock].length
							= HOST_ENDIAN_TO_BFS_INT16(minimum);

						// alter the remaining block_run
						run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start()
							+ minimum);
						run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length()
							- minimum);
					} while ((++index % runsPerBlock) != 0 && run.length);
				} while ((index % runsPerArray) != 0 && run.length);

				if (index == runsPerArray)
					index = 0;
				if (++indirectIndex % runsPerBlock == 0) {
					array = NULL;
					index = 0;
				}
			}

			data->max_double_indirect_range = HOST_ENDIAN_TO_BFS_INT64(
				data->MaxDoubleIndirectRange()
				+ (runLength << fVolume->BlockShift()));
			data->size = blocksNeeded > 0 ? HOST_ENDIAN_TO_BFS_INT64(
				data->max_double_indirect_range) : size;

			continue;
		}

		RETURN_ERROR(EFBIG);
	}
	// update the size of the data stream
	data->size = HOST_ENDIAN_TO_BFS_INT64(size);

	return B_OK;
}


size_t
Inode::_DoubleIndirectBlockLength() const
{
	if (fVolume->BlockSize() > DOUBLE_INDIRECT_ARRAY_SIZE)
		return 1;

	return DOUBLE_INDIRECT_ARRAY_SIZE / fVolume->BlockSize();
}


/*!	Frees the statically sized array of the double indirect part of a data
	stream.
*/
status_t
Inode::_FreeStaticStreamArray(Transaction& transaction, int32 level,
	block_run run, off_t size, off_t offset, off_t& max)
{
	int32 indirectSize;
	if (level == 0) {
		indirectSize = double_indirect_max_indirect_size(run.Length(),
			fVolume->BlockSize());
	} else {
		indirectSize = double_indirect_max_direct_size(run.Length(),
			fVolume->BlockSize());
	}
	if (indirectSize <= 0)
		return B_BAD_DATA;

	off_t start;
	if (size > offset)
		start = size - offset;
	else
		start = 0;

	int32 index = start / indirectSize;
	int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);

	CachedBlock cached(fVolume);
	off_t blockNumber = fVolume->ToBlock(run);

	// set the file offset to the current block run
	offset += (off_t)index * indirectSize;

	for (int32 i = index / runsPerBlock; i < run.Length(); i++) {
		block_run* array = (block_run*)cached.SetToWritable(transaction,
			blockNumber + i);
		if (array == NULL)
			RETURN_ERROR(B_ERROR);

		for (index = index % runsPerBlock; index < runsPerBlock; index++) {
			if (array[index].IsZero()) {
				// we also want to break out of the outer loop
				i = run.Length();
				break;
			}

			status_t status = B_OK;
			if (level == 0) {
				status = _FreeStaticStreamArray(transaction, 1, array[index],
					size, offset, max);
			} else if (offset >= size)
				status = fVolume->Free(transaction, array[index]);
			else
				max = HOST_ENDIAN_TO_BFS_INT64(offset + indirectSize);

			if (status < B_OK)
				RETURN_ERROR(status);

			if (offset >= size)
				array[index].SetTo(0, 0, 0);

			offset += indirectSize;
		}
		index = 0;
	}
	return B_OK;
}


/*!	Frees all block_runs in the array which come after the specified size.
	It also trims the last block_run that contain the size.
	"offset" and "max" are maintained until the last block_run that doesn't
	have to be freed - after this, the values won't be correct anymore, but
	will still assure correct function for all subsequent calls.
	"max" is considered to be in file system byte order.
*/
status_t
Inode::_FreeStreamArray(Transaction& transaction, block_run* array,
	uint32 arrayLength, off_t size, off_t& offset, off_t& max)
{
	PRINT(("FreeStreamArray: arrayLength %lu, size %Ld, offset %Ld, max %Ld\n",
		arrayLength, size, offset, max));

	off_t newOffset = offset;
	uint32 i = 0;
	for (; i < arrayLength; i++, offset = newOffset) {
		if (array[i].IsZero())
			break;

		newOffset += (off_t)array[i].Length() << fVolume->BlockShift();
		if (newOffset <= size)
			continue;

		block_run run = array[i];

		// determine the block_run to be freed
		if (newOffset > size && offset < size) {
			// free partial block_run (and update the original block_run)
			run.start = HOST_ENDIAN_TO_BFS_INT16(array[i].Start()
				+ ((size + fVolume->BlockSize() - 1 - offset)
					>> fVolume->BlockShift()));
				// round to next block
			array[i].length = HOST_ENDIAN_TO_BFS_INT16(run.Start()
				- array[i].Start());
			run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length()
				- array[i].Length());

			if (run.length == 0)
				continue;

			// update maximum range
			max = HOST_ENDIAN_TO_BFS_INT64(offset + ((off_t)array[i].Length()
				<< fVolume->BlockShift()));
		} else {
			// free the whole block_run
			array[i].SetTo(0, 0, 0);

			if ((off_t)BFS_ENDIAN_TO_HOST_INT64(max) > offset)
				max = HOST_ENDIAN_TO_BFS_INT64(offset);
		}

		if (fVolume->Free(transaction, run) < B_OK)
			return B_IO_ERROR;
	}
	return B_OK;
}


status_t
Inode::_ShrinkStream(Transaction& transaction, off_t size)
{
	data_stream* data = &Node().data;
	status_t status;

	if (data->MaxDoubleIndirectRange() > size) {
		off_t* maxDoubleIndirect = &data->max_double_indirect_range;
			// gcc 4 work-around: "error: cannot bind packed field
			// 'data->data_stream::max_double_indirect_range' to 'off_t&'"
		status = _FreeStaticStreamArray(transaction, 0, data->double_indirect,
			size, data->MaxIndirectRange(), *maxDoubleIndirect);
		if (status != B_OK)
			return status;

		if (size <= data->MaxIndirectRange()) {
			fVolume->Free(transaction, data->double_indirect);
			data->double_indirect.SetTo(0, 0, 0);
			data->max_double_indirect_range = 0;
		}
	}

	if (data->MaxIndirectRange() > size) {
		CachedBlock cached(fVolume);
		off_t block = fVolume->ToBlock(data->indirect);
		off_t offset = data->MaxDirectRange();

		for (int32 i = 0; i < data->indirect.Length(); i++) {
			block_run* array = (block_run*)cached.SetToWritable(transaction,
				block + i);
			if (array == NULL)
				break;

			off_t* maxIndirect = &data->max_indirect_range;
				// gcc 4 work-around: "error: cannot bind packed field
				// 'data->data_stream::max_indirect_range' to 'off_t&'"
			if (_FreeStreamArray(transaction, array, fVolume->BlockSize()
					/ sizeof(block_run), size, offset, *maxIndirect) != B_OK)
				return B_IO_ERROR;
		}
		if (data->max_direct_range == data->max_indirect_range) {
			fVolume->Free(transaction, data->indirect);
			data->indirect.SetTo(0, 0, 0);
			data->max_indirect_range = 0;
		}
	}

	if (data->MaxDirectRange() > size) {
		off_t offset = 0;
		off_t *maxDirect = &data->max_direct_range;
			// gcc 4 work-around: "error: cannot bind packed field
			// 'data->data_stream::max_direct_range' to 'off_t&'"
		status = _FreeStreamArray(transaction, data->direct, NUM_DIRECT_BLOCKS,
			size, offset, *maxDirect);
		if (status < B_OK)
			return status;
	}

	data->size = HOST_ENDIAN_TO_BFS_INT64(size);
	return B_OK;
}


status_t
Inode::SetFileSize(Transaction& transaction, off_t size)
{
	if (size < 0)
		return B_BAD_VALUE;

	off_t oldSize = Size();

	if (size == oldSize)
		return B_OK;

	T(Resize(this, oldSize, size, false));

	// should the data stream grow or shrink?
	status_t status;
	if (size > oldSize) {
		status = _GrowStream(transaction, size);
		if (status < B_OK) {
			// if the growing of the stream fails, the whole operation
			// fails, so we should shrink the stream to its former size
			_ShrinkStream(transaction, oldSize);
		}
	} else
		status = _ShrinkStream(transaction, size);

	if (status < B_OK)
		return status;

	file_cache_set_size(FileCache(), size);
	file_map_set_size(Map(), size);

	return WriteBack(transaction);
}


status_t
Inode::Append(Transaction& transaction, off_t bytes)
{
	return SetFileSize(transaction, Size() + bytes);
}


/*!	Checks whether or not this inode's data stream needs to be trimmed
	because of an earlier preallocation.
	Returns true if there are any blocks to be trimmed.
*/
bool
Inode::NeedsTrimming() const
{
	// We never trim preallocated index blocks to make them grow as smooth as
	// possible. There are only few indices anyway, so this doesn't hurt.
	// Also, if an inode is already in deleted state, we don't bother trimming
	// it.
	if (IsIndex() || IsDeleted()
		|| (IsSymLink() && (Flags() & INODE_LONG_SYMLINK) == 0))
		return false;

	off_t roundedSize = round_up(Size(), fVolume->BlockSize());

	return Node().data.MaxDirectRange() > roundedSize
		|| Node().data.MaxIndirectRange() > roundedSize
		|| Node().data.MaxDoubleIndirectRange() > roundedSize;
}


status_t
Inode::TrimPreallocation(Transaction& transaction)
{
	T(Resize(this, max_c(Node().data.MaxDirectRange(),
		Node().data.MaxIndirectRange()), Size(), true));

	status_t status = _ShrinkStream(transaction, Size());
	if (status < B_OK)
		return status;

	return WriteBack(transaction);
}


//!	Frees the file's data stream and removes all attributes
status_t
Inode::Free(Transaction& transaction)
{
	FUNCTION();

	// Perhaps there should be an implementation of Inode::ShrinkStream() that
	// just frees the data_stream, but doesn't change the inode (since it is
	// freed anyway) - that would make an undelete command possible
	if (!IsSymLink() || (Flags() & INODE_LONG_SYMLINK) != 0) {
		status_t status = SetFileSize(transaction, 0);
		if (status < B_OK)
			return status;
	}

	// Free all attributes, and remove their indices
	{
		// We have to limit the scope of AttributeIterator, so that its
		// destructor is not called after the inode is deleted
		AttributeIterator iterator(this);

		char name[B_FILE_NAME_LENGTH];
		uint32 type;
		size_t length;
		ino_t id;
		while (iterator.GetNext(name, &length, &type, &id) == B_OK) {
			RemoveAttribute(transaction, name);
		}
	}

	if (WriteBack(transaction) < B_OK)
		return B_IO_ERROR;

	return fVolume->Free(transaction, BlockRun());
}


//!	Synchronizes (writes back to disk) the file stream of the inode.
status_t
Inode::Sync()
{
	if (FileCache())
		return file_cache_sync(FileCache());

	// We may also want to flush the attribute's data stream to
	// disk here... (do we?)

	if (IsSymLink() && (Flags() & INODE_LONG_SYMLINK) == 0)
		return B_OK;

	InodeReadLocker locker(this);

	data_stream* data = &Node().data;
	status_t status = B_OK;

	// flush direct range

	for (int32 i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (data->direct[i].IsZero())
			return B_OK;

		status = block_cache_sync_etc(fVolume->BlockCache(),
			fVolume->ToBlock(data->direct[i]), data->direct[i].Length());
		if (status != B_OK)
			return status;
	}

	// flush indirect range

	if (data->max_indirect_range == 0)
		return B_OK;

	CachedBlock cached(fVolume);
	off_t block = fVolume->ToBlock(data->indirect);
	int32 count = fVolume->BlockSize() / sizeof(block_run);

	for (int32 j = 0; j < data->indirect.Length(); j++) {
		block_run* runs = (block_run*)cached.SetTo(block + j);
		if (runs == NULL)
			break;

		for (int32 i = 0; i < count; i++) {
			if (runs[i].IsZero())
				return B_OK;

			status = block_cache_sync_etc(fVolume->BlockCache(),
				fVolume->ToBlock(runs[i]), runs[i].Length());
			if (status != B_OK)
				return status;
		}
	}

	// flush double indirect range

	if (data->max_double_indirect_range == 0)
		return B_OK;

	off_t indirectBlock = fVolume->ToBlock(data->double_indirect);

	for (int32 l = 0; l < data->double_indirect.Length(); l++) {
		block_run* indirectRuns = (block_run*)cached.SetTo(indirectBlock + l);
		if (indirectRuns == NULL)
			return B_FILE_ERROR;

		CachedBlock directCached(fVolume);

		for (int32 k = 0; k < count; k++) {
			if (indirectRuns[k].IsZero())
				return B_OK;

			block = fVolume->ToBlock(indirectRuns[k]);
			for (int32 j = 0; j < indirectRuns[k].Length(); j++) {
				block_run* runs = (block_run*)directCached.SetTo(block + j);
				if (runs == NULL)
					return B_FILE_ERROR;

				for (int32 i = 0; i < count; i++) {
					if (runs[i].IsZero())
						return B_OK;

					// TODO: combine single block_runs to bigger ones when
					// they are adjacent
					status = block_cache_sync_etc(fVolume->BlockCache(),
						fVolume->ToBlock(runs[i]), runs[i].Length());
					if (status != B_OK)
						return status;
				}
			}
		}
	}
	return B_OK;
}


//	#pragma mark - TransactionListener implementation


void
Inode::TransactionDone(bool success)
{
	if (!success) {
		// Revert any changes made to the cached bfs_inode
		// TODO: return code gets eaten
		UpdateNodeFromDisk();
	}
}


void
Inode::RemovedFromTransaction()
{
	Node().flags &= ~HOST_ENDIAN_TO_BFS_INT32(INODE_IN_TRANSACTION);

	// See AddInode() why we do this here
	if ((Flags() & INODE_DELETED) != 0)
		fVolume->RemovedInodes().Add(this);

	rw_lock_write_unlock(&Lock());

	if (!fVolume->IsInitializing())
		put_vnode(fVolume->FSVolume(), ID());
}


//	#pragma mark - creation/deletion


status_t
Inode::Remove(Transaction& transaction, const char* name, ino_t* _id,
	bool isDirectory, bool force)
{
	if (fTree == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	WriteLockInTransaction(transaction);

	// does the file even exist?
	off_t id;
	if (fTree->Find((uint8*)name, (uint16)strlen(name), &id) < B_OK)
		return B_ENTRY_NOT_FOUND;

	if (_id)
		*_id = id;

	Vnode vnode(fVolume, id);
	Inode* inode;
	status_t status = vnode.Get(&inode);
	if (status < B_OK) {
		REPORT_ERROR(status);
		return B_ENTRY_NOT_FOUND;
	}

	T(Remove(inode, name));
	inode->WriteLockInTransaction(transaction);

	// Inode::IsContainer() is true also for indices (furthermore, the S_IFDIR
	// bit is set for indices in BFS, not for attribute directories) - but you
	// should really be able to do whatever you want with your indices
	// without having to remove all files first :)
	if (!inode->IsIndex() && !force) {
		// if it's not of the correct type, don't delete it!
		if (inode->IsContainer() != isDirectory)
			return isDirectory ? B_NOT_A_DIRECTORY : B_IS_A_DIRECTORY;

		// only delete empty directories
		if (isDirectory && !inode->IsEmpty())
			return B_DIRECTORY_NOT_EMPTY;
	}

	// remove_vnode() allows the inode to be accessed until the last put_vnode()
	status = remove_vnode(fVolume->FSVolume(), id);
	if (status != B_OK)
		return status;

	if (fTree->Remove(transaction, name, id) != B_OK && !force) {
		unremove_vnode(fVolume->FSVolume(), id);
		RETURN_ERROR(B_ERROR);
	}

#ifdef DEBUG
	if (fTree->Find((uint8*)name, (uint16)strlen(name), &id) == B_OK) {
		DIE(("deleted entry still there"));
	}
#endif

	ContainerContentsChanged(transaction);

	// update the inode, so that no one will ever doubt it's deleted :-)
	inode->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED);
	inode->Node().flags &= ~HOST_ENDIAN_TO_BFS_INT32(INODE_IN_USE);

	// In balance to the Inode::Create() method, the main indices
	// are updated here (name, size, & last_modified)

	Index index(fVolume);
	if (inode->InNameIndex()) {
		index.RemoveName(transaction, name, inode);
			// If removing from the index fails, it is not regarded as a
			// fatal error and will not be reported back!
			// Deleted inodes won't be visible in queries anyway.
	}

	if (inode->InSizeIndex())
		index.RemoveSize(transaction, inode);
	if (inode->InLastModifiedIndex())
		index.RemoveLastModified(transaction, inode);

	return inode->WriteBack(transaction);
}


/*!	Creates the inode with the specified \a parent directory, and automatically
	adds the created inode to that parent directory. If an attribute directory
	is created, it will also automatically  be added to the \a parent inode as
	such. However, the indices root node, and the regular root node won't be
	added to the superblock.
	It will also create the initial B+tree for the inode if it's a directory
	of any kind.
	\a name may be \c NULL, but only if no \a parent is given.
	If the "_id" or "_inode" variable is given and non-NULL to store the
	inode's ID, the inode stays locked - you have to call put_vnode() if you
	don't use it anymore.
	If the node already exists, this method will fail if \c O_EXCL is set, or
	it's a directory or a symlink. Otherwise, it will just be returned.
	If \c O_TRUNC has been specified, the file will also be truncated.
*/
status_t
Inode::Create(Transaction& transaction, Inode* parent, const char* name,
	int32 mode, int openMode, uint32 type, bool* _created, ino_t* _id,
	Inode** _inode, fs_vnode_ops* vnodeOps, uint32 publishFlags)
{
	FUNCTION_START(("name = %s, mode = %ld\n", name, mode));

	block_run parentRun = parent ? parent->BlockRun() : block_run::Run(0, 0, 0);
	Volume* volume = transaction.GetVolume();
	BPlusTree* tree = NULL;

	if (parent != NULL && (mode & S_ATTR_DIR) == 0 && parent->IsContainer()) {
		// check if the file already exists in the directory
		tree = parent->Tree();
	}

	if (parent != NULL) {
		// the parent directory is locked during the whole inode creation
		parent->WriteLockInTransaction(transaction);
	}

	if (parent != NULL && !volume->IsInitializing() && parent->IsContainer()) {
		// don't create anything in removed directories
		bool removed;
		if (get_vnode_removed(volume->FSVolume(), parent->ID(), &removed)
				== B_OK && removed) {
			RETURN_ERROR(B_ENTRY_NOT_FOUND);
		}
	}

	if (tree != NULL) {
		// Does the file already exist?
		off_t offset;
		if (tree->Find((uint8*)name, (uint16)strlen(name), &offset) == B_OK) {
			// Return if the file should be a directory/link or opened in
			// exclusive mode
			if (S_ISDIR(mode) || S_ISLNK(mode) || (openMode & O_EXCL) != 0)
				return B_FILE_EXISTS;

			Vnode vnode(volume, offset);
			Inode* inode;
			status_t status = vnode.Get(&inode);
			if (status != B_OK) {
				REPORT_ERROR(status);
				return B_ENTRY_NOT_FOUND;
			}

			if (inode->IsDirectory() && (openMode & O_RWMASK) != O_RDONLY)
				return B_IS_A_DIRECTORY;
			if ((openMode & O_DIRECTORY) != 0 && !inode->IsDirectory())
				return B_NOT_A_DIRECTORY;

			// we want to open the file, so we should have the rights to do so
			if (inode->CheckPermissions(open_mode_to_access(openMode)
					| ((openMode & O_TRUNC) != 0 ? W_OK : 0)) != B_OK)
				return B_NOT_ALLOWED;

			if ((openMode & O_TRUNC) != 0) {
				// truncate the existing file
				inode->WriteLockInTransaction(transaction);

				status_t status = inode->SetFileSize(transaction, 0);
				if (status == B_OK)
					status = inode->WriteBack(transaction);

				if (status != B_OK)
					return status;
			}

			if (_created)
				*_created = false;
			if (_id)
				*_id = inode->ID();
			if (_inode)
				*_inode = inode;

			// Only keep the vnode in memory if the _id or _inode pointer is
			// provided
			if (_id != NULL || _inode != NULL)
				vnode.Keep();

			return B_OK;
		}
	} else if (parent != NULL && (mode & S_ATTR_DIR) == 0) {
		return B_BAD_VALUE;
	} else if ((openMode & O_DIRECTORY) != 0) {
		// TODO: we might need to return B_NOT_A_DIRECTORY here
		return B_ENTRY_NOT_FOUND;
	}

	status_t status;

	// do we have the power to create new files at all?
	if (parent != NULL && (status = parent->CheckPermissions(W_OK)) != B_OK)
		RETURN_ERROR(status);

	// allocate space for the new inode
	InodeAllocator allocator(transaction);
	block_run run;
	Inode* inode;
	status = allocator.New(&parentRun, mode, publishFlags, run, vnodeOps,
		&inode);
	if (status < B_OK)
		return status;

	T(Create(inode, parent, name, mode, openMode, type));

	// Initialize the parts of the bfs_inode structure that
	// InodeAllocator::New() hasn't touched yet

	bfs_inode* node = &inode->Node();

	if (parent == NULL) {
		// we set the parent to itself in this case
		// (only happens for the root and indices node)
		node->parent = run;
	} else
		node->parent = parentRun;

	node->uid = HOST_ENDIAN_TO_BFS_INT32(geteuid());
	node->gid = HOST_ENDIAN_TO_BFS_INT32(parent
		? parent->Node().GroupID() : getegid());
		// the group ID is inherited from the parent, if available

	node->type = HOST_ENDIAN_TO_BFS_INT32(type);

	inode->WriteBack(transaction);
		// make sure the initialized node is available to others

	// only add the name to regular files, directories, or symlinks
	// don't add it to attributes, or indices
	if (tree && inode->IsRegularNode()
		&& inode->SetName(transaction, name) != B_OK)
		return B_ERROR;

	// Initialize b+tree if it's a directory (and add "." & ".." if it's
	// a standard directory for files - not for attributes or indices)
	if (inode->IsContainer()) {
		status = allocator.CreateTree();
		if (status != B_OK)
			return status;
	}

	// Add a link to the inode from the parent, depending on its type
	// (the vnode is not published yet, so it is safe to make the inode
	// accessable to the file system here)
	if (tree != NULL) {
		status = tree->Insert(transaction, name, inode->ID());
	} else if (parent != NULL && (mode & S_ATTR_DIR) != 0) {
		parent->Attributes() = run;
		status = parent->WriteBack(transaction);
	}

	// Note, we only care if the inode could be made accessable for the
	// two cases above; the root node or the indices root node must
	// handle this case on their own (or other cases where "parent" is
	// NULL)
	if (status != B_OK)
		RETURN_ERROR(status);

	// Update the main indices (name, size & last_modified)
	// (live queries might want to access us after this)

	Index index(volume);
	if (inode->InNameIndex() && name != NULL) {
		// the name index only contains regular files
		// (but not the root node where name == NULL)
		status = index.InsertName(transaction, name, inode);
		if (status != B_OK && status != B_BAD_INDEX) {
			// We have to remove the node from the parent at this point,
			// because the InodeAllocator destructor can't handle this
			// case (and if it fails, we can't do anything about it...)
			if (tree)
				tree->Remove(transaction, name, inode->ID());
			else if (parent != NULL && (mode & S_ATTR_DIR) != 0)
				parent->Node().attributes.SetTo(0, 0, 0);

			RETURN_ERROR(status);
		}
	}

	if (parent != NULL && parent->IsContainer())
		parent->ContainerContentsChanged(transaction);

	inode->UpdateOldLastModified();

	// The "size" & "last_modified" indices don't contain directories.
	// If adding to these indices fails, the inode creation will not be
	// harmed; they are considered less important than the "name" index.
	if (inode->InSizeIndex())
		index.InsertSize(transaction, inode);
	if (inode->InLastModifiedIndex())
		index.InsertLastModified(transaction, inode);

	if (inode->NeedsFileCache()) {
		inode->SetFileCache(file_cache_create(volume->ID(), inode->ID(),
			inode->Size()));
		inode->SetMap(file_map_create(volume->ID(), inode->ID(),
			inode->Size()));

		if (inode->FileCache() == NULL || inode->Map() == NULL)
			return B_NO_MEMORY;
	}

	// Everything worked well until this point, we have a fully
	// initialized inode, and we want to keep it
	allocator.Keep(vnodeOps, publishFlags);

	if (_created)
		*_created = true;
	if (_id != NULL)
		*_id = inode->ID();
	if (_inode != NULL)
		*_inode = inode;

	// if either _id or _inode is passed, we will keep the inode locked
	if (_id == NULL && _inode == NULL)
		put_vnode(volume->FSVolume(), inode->ID());

	return B_OK;
}


//	#pragma mark - AttributeIterator


AttributeIterator::AttributeIterator(Inode* inode)
	:
	fCurrentSmallData(0),
	fInode(inode),
	fAttributes(NULL),
	fIterator(NULL),
	fBuffer(NULL)
{
	inode->_AddIterator(this);
}


AttributeIterator::~AttributeIterator()
{
	if (fAttributes)
		put_vnode(fAttributes->GetVolume()->FSVolume(), fAttributes->ID());

	delete fIterator;
	fInode->_RemoveIterator(this);
}


status_t
AttributeIterator::Rewind()
{
	fCurrentSmallData = 0;

	if (fIterator != NULL)
		fIterator->Rewind();

	return B_OK;
}


status_t
AttributeIterator::GetNext(char* name, size_t* _length, uint32* _type,
	ino_t* _id)
{
	// read attributes out of the small data section

	if (fCurrentSmallData >= 0) {
		NodeGetter nodeGetter(fInode->GetVolume(), fInode);
		if (nodeGetter.Node() == NULL)
			return B_IO_ERROR;

		const bfs_inode* node = nodeGetter.Node();
		const small_data* item = ((bfs_inode*)node)->SmallDataStart();

		RecursiveLocker _(&fInode->SmallDataLock());

		int32 index = 0;
		for (; !item->IsLast(node); item = item->Next(), index++) {
			if (item->NameSize() == FILE_NAME_NAME_LENGTH
				&& *item->Name() == FILE_NAME_NAME)
				continue;

			if (index >= fCurrentSmallData)
				break;
		}

		if (!item->IsLast(node)) {
			strncpy(name, item->Name(), B_FILE_NAME_LENGTH);
			*_type = item->Type();
			*_length = item->NameSize();
			*_id = (ino_t)index;

			fCurrentSmallData = index + 1;
			return B_OK;
		}

		// stop traversing the small_data section
		fCurrentSmallData = -1;
	}

	// read attributes out of the attribute directory

	if (fInode->Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	Volume* volume = fInode->GetVolume();

	// if you haven't yet access to the attributes directory, get it
	if (fAttributes == NULL) {
		if (get_vnode(volume->FSVolume(), volume->ToVnode(fInode->Attributes()),
				(void**)&fAttributes) != B_OK) {
			FATAL(("get_vnode() failed in AttributeIterator::GetNext(ino_t"
				" = %" B_PRIdINO ",name = \"%s\")\n", fInode->ID(), name));
			return B_ENTRY_NOT_FOUND;
		}

		BPlusTree* tree = fAttributes->Tree();
		if (tree == NULL
			|| (fIterator = new(std::nothrow) TreeIterator(tree)) == NULL) {
			FATAL(("could not get tree in AttributeIterator::GetNext(ino_t"
				" = %" B_PRIdINO ",name = \"%s\")\n", fInode->ID(), name));
			return B_ENTRY_NOT_FOUND;
		}
	}

	uint16 length;
	ino_t id;
	status_t status = fIterator->GetNextEntry(name, &length,
		B_FILE_NAME_LENGTH, &id);
	if (status != B_OK)
		return status;

	Vnode vnode(volume, id);
	Inode* attribute;
	if ((status = vnode.Get(&attribute)) == B_OK) {
		*_type = attribute->Type();
		*_length = length;
		*_id = id;
	}

	return status;
}


void
AttributeIterator::Update(uint16 index, int8 change)
{
	// fCurrentSmallData points already to the next item
	if (index < fCurrentSmallData)
		fCurrentSmallData += change;
}

