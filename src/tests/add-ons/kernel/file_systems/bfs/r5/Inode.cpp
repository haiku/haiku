/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de
 * This file may be used under the terms of the MIT License.
 */

//! inode access functions
 

#include "Debug.h"
#include "Inode.h"
#include "BPlusTree.h"
#include "Stream.h"
#include "Index.h"

#include <util/kernel_cpp.h>

#include <string.h>
#include <stdio.h>


class InodeAllocator {
	public:
		InodeAllocator(Transaction *transaction);
		~InodeAllocator();

		status_t New(block_run *parentRun, mode_t mode, block_run &run, Inode **_inode);
		status_t CreateTree();
		status_t Keep();

	private:
		Transaction *fTransaction;
		block_run fRun;
		Inode *fInode;
};


InodeAllocator::InodeAllocator(Transaction *transaction)
	:
	fTransaction(transaction),
	fInode(NULL)
{
}


InodeAllocator::~InodeAllocator()
{
	if (fTransaction != NULL) {
		if (fInode != NULL) {
			fInode->Node()->flags &= ~HOST_ENDIAN_TO_BFS_INT32(INODE_IN_USE | INODE_NOT_READY);
				// this unblocks any pending bfs_read_vnode() calls
			fInode->Free(fTransaction);			
		} else
			fTransaction->GetVolume()->Free(fTransaction, fRun);
	}

	delete fInode;
}


status_t 
InodeAllocator::New(block_run *parentRun, mode_t mode, block_run &run, Inode **_inode)
{
	Volume *volume = fTransaction->GetVolume();

	status_t status = volume->AllocateForInode(fTransaction, parentRun, mode, fRun);
	if (status < B_OK) {
		// don't free the space in the destructor, because
		// the allocation failed
		fTransaction = NULL;
		RETURN_ERROR(status);
	}

	run = fRun;
	fInode = new Inode(volume, volume->ToVnode(run), true);
	if (fInode == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	// initialize the on-disk bfs_inode structure 

	bfs_inode *node = fInode->Node();

	node->magic1 = HOST_ENDIAN_TO_BFS_INT32(INODE_MAGIC1);
	node->inode_num = run;
	node->mode = HOST_ENDIAN_TO_BFS_INT32(mode);
	node->flags = HOST_ENDIAN_TO_BFS_INT32(INODE_IN_USE | INODE_NOT_READY);
		// INODE_NOT_READY prevents the inode from being opened - it is
		// cleared in InodeAllocator::Keep()
	node->etc = (uint32)fInode;
		// this is temporarily set along INODE_NOT_READY and lets bfs_read_vnode()
		// find the associated Inode object

	node->create_time = HOST_ENDIAN_TO_BFS_INT64((bigtime_t)time(NULL) << INODE_TIME_SHIFT);
	node->last_modified_time = HOST_ENDIAN_TO_BFS_INT64(node->create_time
		| (volume->GetUniqueID() & INODE_TIME_MASK));
		// we use Volume::GetUniqueID() to avoid having too many duplicates in the
		// last_modified index

	node->inode_size = HOST_ENDIAN_TO_BFS_INT32(volume->InodeSize());

	*_inode = fInode;
	return B_OK;
}


status_t
InodeAllocator::CreateTree()
{
	Volume *volume = fTransaction->GetVolume();

	// force S_STR_INDEX to be set, if no type is set
	if ((fInode->Mode() & S_INDEX_TYPES) == 0)
		fInode->Node()->mode |= HOST_ENDIAN_TO_BFS_INT32(S_STR_INDEX);

	BPlusTree *tree = fInode->fTree = new BPlusTree(fTransaction, fInode);
	if (tree == NULL || tree->InitCheck() < B_OK)
		return B_ERROR;

	if (fInode->IsRegularNode()) {
		if (tree->Insert(fTransaction, ".", fInode->ID()) < B_OK
			|| tree->Insert(fTransaction, "..", volume->ToVnode(fInode->Parent())) < B_OK)
			return B_ERROR;
	}
	return B_OK;
}


status_t
InodeAllocator::Keep()
{
	ASSERT(fInode != NULL && fTransaction != NULL);

	fInode->Node()->flags &= ~HOST_ENDIAN_TO_BFS_INT32(INODE_NOT_READY);
	status_t status = fInode->WriteBack(fTransaction);

	fTransaction = NULL;
	fInode = NULL;

	return status;
}


//	#pragma mark -


status_t 
bfs_inode::InitCheck(Volume *volume)
{
	if (Flags() & INODE_NOT_READY) {
		// the other fields may not yet contain valid values
		return B_BUSY;
	}
	if (Flags() & INODE_DELETED)
		return B_NOT_ALLOWED;

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

	// ToDo: Add some tests to check the integrity of the other stuff here,
	// especially for the data_stream!

	return B_OK;
}


//	#pragma mark -


Inode::Inode(Volume *volume, vnode_id id, bool empty, uint8 reenter)
	: CachedBlock(volume, volume->VnodeToBlock(id), empty),
	fTree(NULL),
	fLock()
{
	PRINT(("Inode::Inode(%p, %Ld, %s, %s) @ %p\n",
		volume, id, empty ? "empty" : "not-empty", reenter ? "reenter":"not-reenter", this));

	Initialize();
}


Inode::Inode(CachedBlock *cached)
	: CachedBlock(cached),
	fTree(NULL),
	fLock()
{
	PRINT(("Inode::Inode(%p) @ %p\n", cached, this));

	Initialize();
}


Inode::~Inode()
{
	PRINT(("Inode::~Inode() @ %p\n", this));

	delete fTree;
}


void
Inode::Initialize()
{
	char lockName[32];
	sprintf(lockName, "bfs inode %ld.%d", BlockRun().AllocationGroup(), BlockRun().Start());
	fLock.Initialize(lockName);

	Node()->flags &= HOST_ENDIAN_TO_BFS_INT32(INODE_PERMANENT_FLAGS);

	// these two will help to maintain the indices
	fOldSize = Size();
	fOldLastModified = LastModified();

	if (IsContainer())
		fTree = new BPlusTree(this);

	fCache = NULL;
}


status_t 
Inode::InitCheck(bool checkNode)
{
	if (!Node())
		RETURN_ERROR(B_IO_ERROR);

	// test inode magic and flags
	if (checkNode) {
		status_t status = Node()->InitCheck(fVolume);
		if (status == B_BUSY)
			return B_BUSY;
	
		if (status < B_OK) {
			FATAL(("inode at block %Ld corrupt!\n", fBlockNumber));
			RETURN_ERROR(B_BAD_DATA);
		}
	}

	if (IsContainer()) {
		// inodes that have a 
		if (fTree == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		status_t status = fTree->InitCheck();
		if (status < B_OK) {
			FATAL(("inode tree at block %Ld corrupt!\n", fBlockNumber));
			RETURN_ERROR(B_BAD_DATA);
		}
	}

	// it's more important to know that the inode is corrupt
	// so we check for the lock not until here
	return fLock.InitCheck();
}


status_t 
Inode::CheckPermissions(int accessMode) const
{
	uid_t user = geteuid();
	gid_t group = getegid();

	// you never have write access to a read-only volume
	if (accessMode & W_OK && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// root users always have full access (but they can't execute anything)
	if (user == 0 && !((accessMode & X_OK) && (Mode() & S_IXUSR) == 0))
		return B_OK;

	// shift mode bits, to check directly against accessMode
	mode_t mode = Mode();
	if (user == (uid_t)Node()->UserID())
		mode >>= 6;
	else if (group == (gid_t)Node()->GroupID())
		mode >>= 3;

	if (accessMode & ~(mode & S_IRWXO))
		return B_NOT_ALLOWED;

	return B_OK;
}


//	#pragma mark -


void 
Inode::AddIterator(AttributeIterator *iterator)
{
	if (fSmallDataLock.Lock() < B_OK)
		return;

	fIterators.Add(iterator);

	fSmallDataLock.Unlock();
}


void 
Inode::RemoveIterator(AttributeIterator *iterator)
{
	if (fSmallDataLock.Lock() < B_OK)
		return;

	fIterators.Remove(iterator);

	fSmallDataLock.Unlock();
}


/**	Tries to free up "bytes" space in the small_data section by moving
 *	attributes to real files. Used for system attributes like the name.
 *	You need to hold the fSmallDataLock when you call this method
 */

status_t
Inode::MakeSpaceForSmallData(Transaction *transaction, const char *name, int32 bytes)
{
	ASSERT(fSmallDataLock.IsLocked());

	while (bytes > 0) {
		small_data *item = Node()->SmallDataStart(), *max = NULL;
		int32 index = 0, maxIndex = 0;
		for (; !item->IsLast(Node()); item = item->Next(), index++) {
			// should not remove those
			if (*item->Name() == FILE_NAME_NAME || !strcmp(name, item->Name()))
				continue;

			if (max == NULL || max->Size() < item->Size()) {
				maxIndex = index;
				max = item;
			}

			// remove the first one large enough to free the needed amount of bytes
			if (bytes < (int32)item->Size())
				break;
		}

		if (item->IsLast(Node()) || (int32)item->Size() < bytes)
			return B_ERROR;

		bytes -= max->Size();

		// Move the attribute to a real attribute file
		// Luckily, this doesn't cause any index updates

		Inode *attribute;
		status_t status = CreateAttribute(transaction, item->Name(), item->Type(), &attribute);
		if (status < B_OK)
			RETURN_ERROR(status);

		size_t length = item->DataSize();
		status = attribute->WriteAt(transaction, 0, item->Data(), &length);

		ReleaseAttribute(attribute);

		if (status < B_OK) {
			Vnode vnode(fVolume,Attributes());
			Inode *attributes;
			if (vnode.Get(&attributes) < B_OK
				|| attributes->Remove(transaction, name) < B_OK) {
				FATAL(("Could not remove newly created attribute!\n"));
			}

			RETURN_ERROR(status);
		}

		RemoveSmallData(max, maxIndex);
	}
	return B_OK;
}


/**	Private function which removes the given attribute from the small_data
 *	section.
 *	You need to hold the fSmallDataLock when you call this method
 */

status_t 
Inode::RemoveSmallData(small_data *item, int32 index)
{
	ASSERT(fSmallDataLock.IsLocked());

	small_data *next = item->Next();
	if (!next->IsLast(Node())) {
		// find the last attribute
		small_data *last = next;
		while (!last->IsLast(Node()))
			last = last->Next();

		int32 size = (uint8 *)last - (uint8 *)next;
		if (size < 0 || size > (uint8 *)Node() + fVolume->BlockSize() - (uint8 *)next)
			return B_BAD_DATA;

		memmove(item, next, size);

		// Move the "last" one to its new location and
		// correctly terminate the small_data section
		last = (small_data *)((uint8 *)last - ((uint8 *)next - (uint8 *)item));
		memset(last, 0, (uint8 *)Node() + fVolume->BlockSize() - (uint8 *)last);
	} else
		memset(item, 0, item->Size());

	// update all current iterators
	AttributeIterator *iterator = NULL;
	while ((iterator = fIterators.Next(iterator)) != NULL)
		iterator->Update(index, -1);

	return B_OK;
}


/**	Removes the given attribute from the small_data section.
 *	Note that you need to write back the inode yourself after having called
 *	that method.
 */

status_t
Inode::RemoveSmallData(Transaction *transaction, const char *name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	SimpleLocker locker(fSmallDataLock);

	// search for the small_data item

	small_data *item = Node()->SmallDataStart();
	int32 index = 0;
	while (!item->IsLast(Node()) && strcmp(item->Name(), name)) {
		item = item->Next();
		index++;
	}

	if (item->IsLast(Node()))
		return B_ENTRY_NOT_FOUND;

	return RemoveSmallData(item, index);
}


/**	Try to place the given attribute in the small_data section - if the
 *	new attribute is too big to fit in that section, it returns B_DEVICE_FULL.
 *	In that case, the attribute should be written to a real attribute file;
 *	if the attribute was already part of the small_data section, but the new
 *	one wouldn't fit, the old one is automatically removed from the small_data
 *	section.
 *	Note that you need to write back the inode yourself after having called that
 *	method - it's a bad API decision that it needs a transaction but enforces you
 *	to write back the inode all by yourself, but it's just more efficient in most
 *	cases...
 */

status_t
Inode::AddSmallData(Transaction *transaction, const char *name, uint32 type,
	const uint8 *data, size_t length, bool force)
{
	if (name == NULL || data == NULL || type == 0)
		return B_BAD_VALUE;

	// reject any requests that can't fit into the small_data section
	uint32 nameLength = strlen(name);
	uint32 spaceNeeded = sizeof(small_data) + nameLength + 3 + length + 1;
	if (spaceNeeded > fVolume->InodeSize() - sizeof(bfs_inode))
		return B_DEVICE_FULL;

	SimpleLocker locker(fSmallDataLock);

	small_data *item = Node()->SmallDataStart();
	int32 index = 0;
	while (!item->IsLast(Node()) && strcmp(item->Name(), name)) {
		item = item->Next();
		index++;
	}

	// is the attribute already in the small_data section?
	// then just replace the data part of that one
	if (!item->IsLast(Node())) {
		// find last attribute
		small_data *last = item;
		while (!last->IsLast(Node()))
			last = last->Next();

		// try to change the attributes value
		if (item->data_size > length
			|| force
			|| ((uint8 *)last + length - item->DataSize()) <= ((uint8 *)Node() + fVolume->InodeSize())) {
			// make room for the new attribute if needed (and we are forced to do so)
			if (force
				&& ((uint8 *)last + length - item->DataSize()) > ((uint8 *)Node() + fVolume->InodeSize())) {
				// We also take the free space at the end of the small_data section
				// into account, and request only what's really needed
				uint32 needed = length - item->DataSize() -
						(uint32)((uint8 *)Node() + fVolume->InodeSize() - (uint8 *)last);

				if (MakeSpaceForSmallData(transaction, name, needed) < B_OK)
					return B_ERROR;

				// reset our pointers
				item = Node()->SmallDataStart();
				index = 0;
				while (!item->IsLast(Node()) && strcmp(item->Name(), name)) {
					item = item->Next();
					index++;
				}

				last = item;
				while (!last->IsLast(Node()))
					last = last->Next();
			}

			// Normally, we can just overwrite the attribute data as the size
			// is specified by the type and does not change that often
			if (length != item->DataSize()) {
				// move the attributes after the current one
				small_data *next = item->Next();
				if (!next->IsLast(Node()))
					memmove((uint8 *)item + spaceNeeded, next, (uint8 *)last - (uint8 *)next);
	
				// Move the "last" one to its new location and
				// correctly terminate the small_data section
				last = (small_data *)((uint8 *)last - ((uint8 *)next - ((uint8 *)item + spaceNeeded)));
				if ((uint8 *)last < (uint8 *)Node() + fVolume->BlockSize())
					memset(last, 0, (uint8 *)Node() + fVolume->BlockSize() - (uint8 *)last);

				item->data_size = HOST_ENDIAN_TO_BFS_INT16(length);
			}

			item->type = HOST_ENDIAN_TO_BFS_INT32(type);
			memcpy(item->Data(), data, length);
			item->Data()[length] = '\0';

			return B_OK;
		}

		// Could not replace the old attribute, so remove it to let
		// let the calling function create an attribute file for it
		if (RemoveSmallData(item, index) < B_OK)
			return B_ERROR;

		return B_DEVICE_FULL;
	}

	// try to add the new attribute!

	if ((uint8 *)item + spaceNeeded > (uint8 *)Node() + fVolume->InodeSize()) {
		// there is not enough space for it!
		if (!force)
			return B_DEVICE_FULL;

		// make room for the new attribute
		if (MakeSpaceForSmallData(transaction, name, spaceNeeded) < B_OK)
			return B_ERROR;

		// get new last item!
		item = Node()->SmallDataStart();
		index = 0;
		while (!item->IsLast(Node())) {
			item = item->Next();
			index++;
		}
	}

	memset(item, 0, spaceNeeded);
	item->type = HOST_ENDIAN_TO_BFS_INT32(type);
	item->name_size = HOST_ENDIAN_TO_BFS_INT16(nameLength);
	item->data_size = HOST_ENDIAN_TO_BFS_INT16(length);
	strcpy(item->Name(), name);
	memcpy(item->Data(), data, length);

	// correctly terminate the small_data section
	item = item->Next();
	if (!item->IsLast(Node()))
		memset(item, 0, (uint8 *)Node() + fVolume->InodeSize() - (uint8 *)item);

	// update all current iterators
	AttributeIterator *iterator = NULL;
	while ((iterator = fIterators.Next(iterator)) != NULL)
		iterator->Update(index, 1);

	return B_OK;
}


/**	Iterates through the small_data section of an inode.
 *	To start at the beginning of this section, you let smallData
 *	point to NULL, like:
 *		small_data *data = NULL;
 *		while (inode->GetNextSmallData(&data) { ... }
 *
 *	This function is reentrant and doesn't allocate any memory;
 *	you can safely stop calling it at any point (you don't need
 *	to iterate through the whole list).
 *	You need to hold the fSmallDataLock when you call this method
 */

status_t
Inode::GetNextSmallData(small_data **_smallData) const
{
	if (!Node())
		RETURN_ERROR(B_ERROR);

	ASSERT(fSmallDataLock.IsLocked());

	small_data *data = *_smallData;

	// begin from the start?
	if (data == NULL)
		data = Node()->SmallDataStart();
	else
		data = data->Next();

	// is already last item?
	if (data->IsLast(Node()))
		return B_ENTRY_NOT_FOUND;

	*_smallData = data;

	return B_OK;
}


/**	Finds the attribute "name" in the small data section, and
 *	returns a pointer to it (or NULL if it doesn't exist).
 *	You need to hold the fSmallDataLock when you call this method
 */

small_data *
Inode::FindSmallData(const char *name) const
{
	ASSERT(fSmallDataLock.IsLocked());

	small_data *smallData = NULL;
	while (GetNextSmallData(&smallData) == B_OK) {
		if (!strcmp(smallData->Name(), name))
			return smallData;
	}
	return NULL;
}


/** Returns a pointer to the node's name if present in the small data
 *	section, NULL otherwise.
 *	You need to hold the fSmallDataLock when you call this method
 */

const char *
Inode::Name() const
{
	ASSERT(fSmallDataLock.IsLocked());

	small_data *smallData = NULL;
	while (GetNextSmallData(&smallData) == B_OK) {
		if (*smallData->Name() == FILE_NAME_NAME && smallData->NameSize() == FILE_NAME_NAME_LENGTH)
			return (const char *)smallData->Data();
	}
	return NULL;
}


/** Copies the node's name into the provided buffer.
 *	The buffer must be B_FILE_NAME_LENGTH bytes large.
 */

status_t
Inode::GetName(char *buffer) const
{
	SimpleLocker locker(fSmallDataLock);

	const char *name = Name();
	if (name == NULL)
		return B_ENTRY_NOT_FOUND;

	strlcpy(buffer, name, B_FILE_NAME_LENGTH);
	return B_OK;
}


/**	Changes or set the name of a file: in the inode small_data section only, it
 *	doesn't change it in the parent directory's b+tree.
 *	Note that you need to write back the inode yourself after having called
 *	that method. It suffers from the same API decision as AddSmallData() does
 *	(and for the same reason).
 */

status_t 
Inode::SetName(Transaction *transaction, const char *name)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	const char nameTag[2] = {FILE_NAME_NAME, 0};

	return AddSmallData(transaction, nameTag, FILE_NAME_TYPE, (uint8 *)name, strlen(name), true);
}


/**	Reads data from the specified attribute.
 *	This is a high-level attribute function that understands attributes
 *	in the small_data section as well as real attribute files.
 */

status_t
Inode::ReadAttribute(const char *name, int32 type, off_t pos, uint8 *buffer, size_t *_length)
{
	if (pos < 0)
		pos = 0;

	// search in the small_data section (which has to be locked first)
	{
		SimpleLocker locker(fSmallDataLock);

		small_data *smallData = FindSmallData(name);
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
	Inode *attribute;
	status_t status = GetAttribute(name, &attribute);
	if (status == B_OK) {
		if (attribute->Lock().Lock() == B_OK) {
			status = attribute->ReadAt(pos, (uint8 *)buffer, _length);
			attribute->Lock().Unlock();
		} else
			status = B_ERROR;

		ReleaseAttribute(attribute);
	}

	RETURN_ERROR(status);
}


/**	Writes data to the specified attribute.
 *	This is a high-level attribute function that understands attributes
 *	in the small_data section as well as real attribute files.
 */

status_t
Inode::WriteAttribute(Transaction *transaction, const char *name, int32 type, off_t pos,
	const uint8 *buffer, size_t *_length)
{
	// needed to maintain the index
	uint8 oldBuffer[BPLUSTREE_MAX_KEY_LENGTH], *oldData = NULL;
	size_t oldLength = 0;

	// ToDo: we actually depend on that the contents of "buffer" are constant.
	// If they get changed during the write (hey, user programs), we may mess
	// up our index trees!

	Index index(fVolume);
	index.SetTo(name);

	Inode *attribute = NULL;
	status_t status = B_OK;

	if (GetAttribute(name, &attribute) < B_OK) {
		// save the old attribute data
		fSmallDataLock.Lock();

		small_data *smallData = FindSmallData(name);
		if (smallData != NULL) {
			oldLength = smallData->DataSize();
			if (oldLength > 0) {
				if (oldLength > BPLUSTREE_MAX_KEY_LENGTH)
					oldLength = BPLUSTREE_MAX_KEY_LENGTH;
				memcpy(oldData = oldBuffer, smallData->Data(), oldLength);
			}
		}
		fSmallDataLock.Unlock();

		// if the attribute doesn't exist yet (as a file), try to put it in the
		// small_data section first - if that fails (due to insufficent space),
		// create a real attribute file
		status = AddSmallData(transaction, name, type, buffer, *_length);
		if (status == B_DEVICE_FULL) {
			status = CreateAttribute(transaction, name, type, &attribute);
			if (status < B_OK)
				RETURN_ERROR(status);
		} else if (status == B_OK)
			status = WriteBack(transaction);
	}

	if (attribute != NULL) {
		if (attribute->Lock().LockWrite() == B_OK) {
			// save the old attribute data (if this fails, oldLength will reflect it)
			if (fVolume->CheckForLiveQuery(name) && attribute->Size() > 0) {
				oldLength = BPLUSTREE_MAX_KEY_LENGTH;
				if (attribute->ReadAt(0, oldBuffer, &oldLength) == B_OK)
					oldData = oldBuffer;
			}
			// ToDo: check if the data fits in the inode now and delete the attribute file if so
			status = attribute->WriteAt(transaction, pos, buffer, _length);
			if (status == B_OK) {
				// The attribute type might have been changed - we need to adopt
				// the new one
				attribute->Node()->type = HOST_ENDIAN_TO_BFS_INT32(type);
				status = attribute->WriteBack(transaction);
			}

			attribute->Lock().UnlockWrite();
		} else
			status = B_ERROR;

		ReleaseAttribute(attribute);
	}

	// ToDo: find a better way than this "pos" thing (the begin of the old key
	//	must be copied to the start of the new one for a comparison)
	if (status == B_OK && pos == 0) {
		// index only the first BPLUSTREE_MAX_KEY_LENGTH bytes
		uint16 length = *_length;
		if (length > BPLUSTREE_MAX_KEY_LENGTH)
			length = BPLUSTREE_MAX_KEY_LENGTH;

		// Update index. Note, Index::Update() may be called even if initializing
		// the index failed - it will just update the live queries in this case
		if (pos < length || pos < oldLength)
			index.Update(transaction, name, type, oldData, oldLength, buffer, length, this);
	}
	return status;
}


/**	Removes the specified attribute from the inode.
 *	This is a high-level attribute function that understands attributes
 *	in the small_data section as well as real attribute files.
 */

status_t
Inode::RemoveAttribute(Transaction *transaction, const char *name)
{
	Index index(fVolume);
	bool hasIndex = index.SetTo(name) == B_OK;

	// update index for attributes in the small_data section
	{
		fSmallDataLock.Lock();

		small_data *smallData = FindSmallData(name);
		if (smallData != NULL) {
			uint32 length = smallData->DataSize();
			if (length > BPLUSTREE_MAX_KEY_LENGTH)
				length = BPLUSTREE_MAX_KEY_LENGTH;
			index.Update(transaction, name, smallData->Type(), smallData->Data(), length, NULL, 0, this);
		}
		fSmallDataLock.Unlock();
	}

	status_t status = RemoveSmallData(transaction, name);
	if (status == B_OK) {
		status = WriteBack(transaction);
	} else if (status == B_ENTRY_NOT_FOUND && !Attributes().IsZero()) {
		// remove the attribute file if it exists
		Vnode vnode(fVolume, Attributes());
		Inode *attributes;
		if ((status = vnode.Get(&attributes)) < B_OK)
			return status;

		// update index
		Inode *attribute;
		if ((hasIndex || fVolume->CheckForLiveQuery(name))
			&& GetAttribute(name, &attribute) == B_OK) {
			uint8 data[BPLUSTREE_MAX_KEY_LENGTH];
			size_t length = BPLUSTREE_MAX_KEY_LENGTH;
			if (attribute->ReadAt(0, data, &length) == B_OK)
				index.Update(transaction, name, attribute->Type(), data, length, NULL, 0, this);

			ReleaseAttribute(attribute);
		}

		if ((status = attributes->Remove(transaction, name)) < B_OK)
			return status;

		if (attributes->IsEmpty()) {
			// remove attribute directory (don't fail if that can't be done)
			if (remove_vnode(fVolume->ID(), attributes->ID()) == B_OK) {
				// update the inode, so that no one will ever doubt it's deleted :-)
				attributes->Node()->flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED);
				if (attributes->WriteBack(transaction) == B_OK) {
					Attributes().SetTo(0, 0, 0);
					WriteBack(transaction);
				} else
					unremove_vnode(fVolume->ID(), attributes->ID());
			}
		}
	}
	return status;
}


status_t
Inode::GetAttribute(const char *name, Inode **_attribute)
{
	// does this inode even have attributes?
	if (Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	Vnode vnode(fVolume, Attributes());
	Inode *attributes;
	if (vnode.Get(&attributes) < B_OK) {
		FATAL(("get_vnode() failed in Inode::GetAttribute(name = \"%s\")\n", name));
		return B_ERROR;
	}

	BPlusTree *tree;
	status_t status = attributes->GetTree(&tree);
	if (status == B_OK) {
		vnode_id id;
		if ((status = tree->Find((uint8 *)name, (uint16)strlen(name), &id)) == B_OK) {
			Vnode vnode(fVolume, id);
			Inode *inode;
			// Check if the attribute is really an attribute
			if (vnode.Get(&inode) < B_OK || !inode->IsAttribute())
				return B_ERROR;

			*_attribute = inode;
			vnode.Keep();
			return B_OK;
		}
	}
	return status;
}


void
Inode::ReleaseAttribute(Inode *attribute)
{
	if (attribute == NULL)
		return;

	put_vnode(fVolume->ID(), attribute->ID());
}


status_t
Inode::CreateAttribute(Transaction *transaction, const char *name, uint32 type, Inode **attribute)
{
	// do we need to create the attribute directory first?
	if (Attributes().IsZero()) {
		status_t status = Inode::Create(transaction, this, NULL, 
			S_ATTR_DIR | S_DIRECTORY | 0666, 0, 0, NULL);
		if (status < B_OK)
			RETURN_ERROR(status);
	}
	Vnode vnode(fVolume, Attributes());
	Inode *attributes;
	if (vnode.Get(&attributes) < B_OK)
		return B_ERROR;

	// Inode::Create() locks the inode for us
	return Inode::Create(transaction, attributes, name, 
		S_ATTR | S_FILE | 0666, 0, type, NULL, attribute);
}


//	#pragma mark -


/**	Gives the caller direct access to the b+tree for a given directory.
 *	The tree is no longer created on demand, but when the inode is first
 *	created. That will report any potential errors upfront, saves locking,
 *	and should work as good (though a bit slower).
 */

status_t
Inode::GetTree(BPlusTree **tree)
{
	if (fTree) {
		*tree = fTree;
		return B_OK;
	}

	RETURN_ERROR(B_BAD_VALUE);
}


bool 
Inode::IsEmpty()
{
	BPlusTree *tree;
	status_t status = GetTree(&tree);
	if (status < B_OK)
		return status;

	TreeIterator iterator(tree);

	// index and attribute directories are really empty when they are
	// empty - directories for standard files always contain ".", and
	// "..", so we need to ignore those two

	uint32 count = 0;
	char name[BPLUSTREE_MAX_KEY_LENGTH];
	uint16 length;
	vnode_id id;
	while (iterator.GetNextEntry(name, &length, B_FILE_NAME_LENGTH, &id) == B_OK) {
		if (Mode() & (S_ATTR_DIR | S_INDEX_DIR))
			return false;

		if (++count > 2 || strcmp(".", name) && strcmp("..", name))
			return false;
	}
	return true;
}


/** Finds the block_run where "pos" is located in the data_stream of
 *	the inode.
 *	If successful, "offset" will then be set to the file offset
 *	of the block_run returned; so "pos - offset" is for the block_run
 *	what "pos" is for the whole stream.
 *	The caller has to make sure that "pos" is inside the stream.
 */

status_t 
Inode::FindBlockRun(off_t pos, block_run &run, off_t &offset)
{
	// The BPlusTree class will call this function, we'll provide
	// standard cached access only from here
	return ((Stream<Access::Cached> *)this)->FindBlockRun(pos, run, offset);
}


status_t
Inode::ReadAt(off_t pos, uint8 *buffer, size_t *_length)
{
	// call the right ReadAt() method, depending on the inode flags

	if (Flags() & INODE_NO_CACHE)
		return ((Stream<Access::Uncached> *)this)->ReadAt(pos, buffer, _length);

	if (Flags() & INODE_LOGGED)
		return ((Stream<Access::Logged> *)this)->ReadAt(pos, buffer, _length);

	return ((Stream<Access::Cached> *)this)->ReadAt(pos, buffer, _length);
}


status_t 
Inode::WriteAt(Transaction *transaction, off_t pos, const uint8 *buffer, size_t *_length)
{
	// call the right WriteAt() method, depending on the inode flags

	// update the last modification time in memory, it will be written
	// back to the inode, and the index when the file is closed
	// ToDo: should update the internal last modified time only at this point!
	Node()->last_modified_time = (bigtime_t)time(NULL) << INODE_TIME_SHIFT;

	if (Flags() & INODE_NO_CACHE)
		return ((Stream<Access::Uncached> *)this)->WriteAt(transaction, pos, buffer, _length);

	if (Flags() & INODE_LOGGED)
		return ((Stream<Access::Logged> *)this)->WriteAt(transaction, pos, buffer, _length);

	return ((Stream<Access::Cached> *)this)->WriteAt(transaction, pos, buffer, _length);
}


/**	Fills the gap between the old file size and the new file size
 *	with zeros.
 *	It's more or less a copy of Inode::WriteAt() but it can handle
 *	length differences of more than just 4 GB, and it never uses
 *	the log, even if the INODE_LOGGED flag is set.
 */

status_t
Inode::FillGapWithZeros(off_t pos, off_t newSize)
{
	// ToDo: we currently do anything here, same as original BFS!
	//if (pos >= newSize)
		return B_OK;

	block_run run;
	off_t offset;
	if (FindBlockRun(pos, run, offset) < B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	off_t length = newSize - pos;
	uint32 bytesWritten = 0;
	uint32 blockSize = fVolume->BlockSize();
	uint32 blockShift = fVolume->BlockShift();
	uint8 *block;

	// the first block_run we write could not be aligned to the block_size boundary
	// (write partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		run.start += (pos - offset) / blockSize;
		run.length -= (pos - offset) / blockSize;

		CachedBlock cached(fVolume,run);
		if ((block = cached.Block()) == NULL)
			RETURN_ERROR(B_BAD_VALUE);

		bytesWritten = blockSize - (pos % blockSize);
		if (length < bytesWritten)
			bytesWritten = length;

		memset(block + (pos % blockSize), 0, bytesWritten);
		if (fVolume->WriteBlocks(cached.BlockNumber(), block, 1) < B_OK)
			RETURN_ERROR(B_IO_ERROR);

		pos += bytesWritten;

		length -= bytesWritten;
		if (length == 0)
			return B_OK;

		if (FindBlockRun(pos, run, offset) < B_OK)
			RETURN_ERROR(B_BAD_VALUE);
	}

	while (length > 0) {
		// offset is the offset to the current pos in the block_run
		run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + ((pos - offset) >> blockShift));
		run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - ((pos - offset) >> blockShift));

		CachedBlock cached(fVolume);
		off_t blockNumber = fVolume->ToBlock(run);
		for (int32 i = 0; i < run.Length(); i++) {
			if ((block = cached.SetTo(blockNumber + i, true)) == NULL)
				RETURN_ERROR(B_IO_ERROR);

			if (fVolume->WriteBlocks(cached.BlockNumber(), block, 1) < B_OK)
				RETURN_ERROR(B_IO_ERROR);
		}

		int32 bytes = run.Length() << blockShift;
		length -= bytes;
		bytesWritten += bytes;

		// since we don't respect a last partial block, length can be lower
		if (length <= 0)
			break;

		pos += bytes;

		if (FindBlockRun(pos, run, offset) < B_OK)
			RETURN_ERROR(B_BAD_VALUE);
	}
	return B_OK;
}


/** Allocates NUM_ARRAY_BLOCKS blocks, and clears their contents. Growing
 *	the indirect and double indirect range uses this method.
 *	The allocated block_run is saved in "run"
 */

status_t
Inode::AllocateBlockArray(Transaction *transaction, block_run &run)
{
	if (!run.IsZero())
		return B_BAD_VALUE;

	status_t status = fVolume->Allocate(transaction, this, NUM_ARRAY_BLOCKS, run, NUM_ARRAY_BLOCKS);
	if (status < B_OK)
		return status;

	// make sure those blocks are empty
	CachedBlock cached(fVolume);
	off_t block = fVolume->ToBlock(run);

	for (int32 i = 0; i < run.Length(); i++) {
		block_run *runs = (block_run *)cached.SetTo(block + i, true);
		if (runs == NULL)
			return B_IO_ERROR;

		if (cached.WriteBack(transaction) < B_OK)
			return B_IO_ERROR;
	}
	return B_OK;
}


status_t 
Inode::GrowStream(Transaction *transaction, off_t size)
{
	data_stream *data = &Node()->data;

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
		// the double indirect range can only handle multiple of NUM_ARRAY_BLOCKS
		minimum = NUM_ARRAY_BLOCKS;
	} else if (data->Size() < data->MaxIndirectRange())
		bytes = size - data->MaxIndirectRange();
	else if (data->Size() < data->MaxDirectRange())
		bytes = size - data->MaxDirectRange();
	else
		bytes = size - data->Size();

	// do we have enough free blocks on the disk?
	off_t blocksRequested = (bytes + fVolume->BlockSize() - 1) >> fVolume->BlockShift();
	if (blocksRequested > fVolume->FreeBlocks())
		return B_DEVICE_FULL;

	off_t blocksNeeded = blocksRequested;
		// because of preallocations and partial allocations, the number of
		// blocks we need to allocate may be different from the one we request
		// from the block allocator

	// Should we preallocate some blocks (currently, always 64k)?
	// Attributes, attribute directories, and long symlinks usually won't get that big,
	// and should stay close to the inode - preallocating could be counterproductive.
	// Also, if free disk space is tight, we probably don't want to do this as well.
	if (!IsAttribute() && !IsAttributeDirectory() && !IsSymLink()
		&& blocksRequested < (65536 >> fVolume->BlockShift())
		&& fVolume->FreeBlocks() > 128)
		blocksRequested = 65536 >> fVolume->BlockShift();

	while (blocksNeeded > 0) {
		// the requested blocks do not need to be returned with a
		// single allocation, so we need to iterate until we have
		// enough blocks allocated
		block_run run;
		status_t status = fVolume->Allocate(transaction, this, blocksRequested, run, minimum);
		if (status < B_OK)
			return status;

		// okay, we have the needed blocks, so just distribute them to the
		// different ranges of the stream (direct, indirect & double indirect)

		// ToDo: if anything goes wrong here, we probably want to free the
		// blocks that couldn't be distributed into the stream!

		blocksNeeded -= run.Length();
		// don't preallocate if the first allocation was already too small
		blocksRequested = blocksNeeded;
		if (minimum > 1) {
			// make sure that "blocks" is a multiple of minimum
			blocksRequested = (blocksRequested + minimum - 1) & ~(minimum - 1);
		}

		// Direct block range

		if (data->Size() <= data->MaxDirectRange()) {
			// let's try to put them into the direct block range
			int32 free = 0;
			for (; free < NUM_DIRECT_BLOCKS; free++)
				if (data->direct[free].IsZero())
					break;

			if (free < NUM_DIRECT_BLOCKS) {
				// can we merge the last allocated run with the new one?
				int32 last = free - 1;
				if (free > 0 && data->direct[last].MergeableWith(run))
					data->direct[last].length = HOST_ENDIAN_TO_BFS_INT16(data->direct[last].Length() + run.Length());
				else
					data->direct[free] = run;

				data->max_direct_range = HOST_ENDIAN_TO_BFS_INT64(data->MaxDirectRange() + run.Length() * fVolume->BlockSize());
				data->size = HOST_ENDIAN_TO_BFS_INT64(blocksNeeded > 0 ? data->max_direct_range : size);
				continue;
			}
		}

		// Indirect block range

		if (data->Size() <= data->MaxIndirectRange() || !data->MaxIndirectRange()) {
			CachedBlock cached(fVolume);
			block_run *runs = NULL;
			uint32 free = 0;
			off_t block;

			// if there is no indirect block yet, create one
			if (data->indirect.IsZero()) {
				status = AllocateBlockArray(transaction, data->indirect);
				if (status < B_OK)
					return status;

				data->max_indirect_range = HOST_ENDIAN_TO_BFS_INT64(data->MaxDirectRange());
				// insert the block_run in the first block
				runs = (block_run *)cached.SetTo(data->indirect);
			} else {
				uint32 numberOfRuns = fVolume->BlockSize() / sizeof(block_run);
				block = fVolume->ToBlock(data->indirect);

				// search first empty entry
				int32 i = 0;
				for (; i < data->indirect.Length(); i++) {
					if ((runs = (block_run *)cached.SetTo(block + i)) == NULL)
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
				// try to insert the run to the last one - note that this doesn't
				// take block borders into account, so it could be further optimized
				int32 last = free - 1;
				if (free > 0 && runs[last].MergeableWith(run))
					runs[last].length = HOST_ENDIAN_TO_BFS_INT16(runs[last].Length() + run.Length());
				else
					runs[free] = run;

				data->max_indirect_range = HOST_ENDIAN_TO_BFS_INT64(data->MaxIndirectRange() + (run.Length() << fVolume->BlockShift()));
				data->size = HOST_ENDIAN_TO_BFS_INT64(blocksNeeded > 0 ? data->MaxIndirectRange() : size);

				cached.WriteBack(transaction);
				continue;
			}
		}

		// Double indirect block range

		if (data->Size() <= data->MaxDoubleIndirectRange() || !data->max_double_indirect_range) {
			while ((run.Length() % NUM_ARRAY_BLOCKS) != 0) {
				// The number of allocated blocks isn't a multiple of NUM_ARRAY_BLOCKS,
				// so we have to change this. This can happen the first time the stream
				// grows into the double indirect range.
				// First, free the remaining blocks that don't fit into a multiple
				// of NUM_ARRAY_BLOCKS
				int32 rest = run.Length() % NUM_ARRAY_BLOCKS;
				run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - rest);

				status = fVolume->Free(transaction, block_run::Run(run.AllocationGroup(),
					run.Start() + run.Length(), rest));
				if (status < B_OK)
					return status;

				blocksNeeded += rest;
				blocksRequested = (blocksNeeded + NUM_ARRAY_BLOCKS - 1) & ~(NUM_ARRAY_BLOCKS - 1);
				minimum = NUM_ARRAY_BLOCKS;
					// we make sure here that we have at minimum NUM_ARRAY_BLOCKS allocated,
					// so if the allocation succeeds, we don't run into an endless loop

				// Are there any blocks left in the run? If not, allocate a new one
				if (run.length == 0)
					continue;
			}

			// if there is no double indirect block yet, create one
			if (data->double_indirect.IsZero()) {
				status = AllocateBlockArray(transaction, data->double_indirect);
				if (status < B_OK)
					return status;

				data->max_double_indirect_range = data->max_indirect_range;
			}

			// calculate the index where to insert the new blocks

			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
			int32 indirectSize = ((1L << INDIRECT_BLOCKS_SHIFT) << fVolume->BlockShift())
				* runsPerBlock;
			int32 directSize = NUM_ARRAY_BLOCKS << fVolume->BlockShift();
			int32 runsPerArray = runsPerBlock << ARRAY_BLOCKS_SHIFT;

			off_t start = data->MaxDoubleIndirectRange() - data->MaxIndirectRange();
			int32 indirectIndex = start / indirectSize;
			int32 index = start / directSize;

			// distribute the blocks to the array and allocate
			// new array blocks when needed

			CachedBlock cached(fVolume);
			CachedBlock cachedDirect(fVolume);
			block_run *array = NULL;
			uint32 runLength = run.Length();

			// ToDo: the following code is commented - it could be used to
			// preallocate all needed block arrays to see in advance if the
			// allocation will succeed.
			// I will probably remove it later, because it's no perfect solution
			// either: if the allocation was broken up before (blocksNeeded != 0),
			// it doesn't guarantee anything.
			// And since failing in this case is not that common, it doesn't have
			// to be optimized in that way.
			// Anyway, I wanted to have it in CVS - all those lines, and they will
			// be removed soon :-)
/*
			// allocate new block arrays if needed

			off_t block = -1;

			for (int32 i = 0;i < needed;i++) {
				// get the block to insert the run into
				block = fVolume->ToBlock(data->double_indirect) + i + indirectIndex / runsPerBlock;
				if (cached.BlockNumber() != block)
					array = (block_run *)cached.SetTo(block);

				if (array == NULL)
					return B_ERROR;

				status = AllocateBlockArray(transaction, array[i + indirectIndex % runsPerBlock]);
				if (status < B_OK)
					return status;
			}
*/

			while (run.length != 0) {
				// get the indirect array block
				if (array == NULL) {
					if (cached.Block() != NULL
						&& cached.WriteBack(transaction) < B_OK)
						return B_IO_ERROR;

					array = (block_run *)cached.SetTo(fVolume->ToBlock(data->double_indirect)
						+ indirectIndex / runsPerBlock);
					if (array == NULL)
						return B_IO_ERROR;
				}

				do {
					// do we need a new array block?
					if (array[indirectIndex % runsPerBlock].IsZero()) {
						status = AllocateBlockArray(transaction, array[indirectIndex % runsPerBlock]);
						if (status < B_OK)
							return status;
					}

					block_run *runs = (block_run *)cachedDirect.SetTo(
						fVolume->ToBlock(array[indirectIndex % runsPerBlock])
						+ index / runsPerBlock);
					if (runs == NULL)
						return B_IO_ERROR;

					do {
						// insert the block_run into the array
						runs[index % runsPerBlock] = run;
						runs[index % runsPerBlock].length = HOST_ENDIAN_TO_BFS_INT16(NUM_ARRAY_BLOCKS);

						// alter the remaining block_run
						run.start = HOST_ENDIAN_TO_BFS_INT16(run.Start() + NUM_ARRAY_BLOCKS);
						run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - NUM_ARRAY_BLOCKS);
					} while ((++index % runsPerBlock) != 0 && run.length);

					if (cachedDirect.WriteBack(transaction) < B_OK)
						return B_IO_ERROR;
				} while ((index % runsPerArray) != 0 && run.length);

				if (++indirectIndex % runsPerBlock == 0) {
					array = NULL;
					index = 0;
				}
			}

			data->max_double_indirect_range = HOST_ENDIAN_TO_BFS_INT64(data->MaxDoubleIndirectRange() + (runLength << fVolume->BlockShift()));
			data->size = blocksNeeded > 0 ? HOST_ENDIAN_TO_BFS_INT64(data->max_double_indirect_range) : size;

			continue;
		}

		RETURN_ERROR(EFBIG);
	}
	// update the size of the data stream
	data->size = HOST_ENDIAN_TO_BFS_INT64(size);

	return B_OK;
}


status_t
Inode::FreeStaticStreamArray(Transaction *transaction, int32 level, block_run run,
	off_t size, off_t offset, off_t &max)
{
	int32 indirectSize = 0;
	if (level == 0)
		indirectSize = (1L << (INDIRECT_BLOCKS_SHIFT + fVolume->BlockShift()))
			* (fVolume->BlockSize() / sizeof(block_run));
	else if (level == 1)
		indirectSize = 4 << fVolume->BlockShift();

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
		block_run *array = (block_run *)cached.SetTo(blockNumber + i);
		if (array == NULL)
			RETURN_ERROR(B_ERROR);

		for (index = index % runsPerBlock; index < runsPerBlock; index++) {
			if (array[index].IsZero()) {
				// we also want to break out of the outer loop
				i = run.Length();
				break;
			}

			status_t status = B_OK;
			if (level == 0)
				status = FreeStaticStreamArray(transaction, 1, array[index], size, offset, max);
			else if (offset >= size)
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

		cached.WriteBack(transaction);
	}
	return B_OK;
}


/** Frees all block_runs in the array which come after the specified size.
 *	It also trims the last block_run that contain the size.
 *	"offset" and "max" are maintained until the last block_run that doesn't
 *	have to be freed - after this, the values won't be correct anymore, but
 *	will still assure correct function for all subsequent calls.
 *	"max" is considered to be in file system byte order.
 */

status_t
Inode::FreeStreamArray(Transaction *transaction, block_run *array, uint32 arrayLength,
	off_t size, off_t &offset, off_t &max)
{
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
			run.start = array[i].start + ((size - offset) >> fVolume->BlockShift()) + 1;
			array[i].length = HOST_ENDIAN_TO_BFS_INT16(run.Start() - array[i].Start());
			run.length = HOST_ENDIAN_TO_BFS_INT16(run.Length() - array[i].Length());

			if (run.length == 0)
				continue;

			// update maximum range
			max = HOST_ENDIAN_TO_BFS_INT64(offset + ((off_t)array[i].Length() << fVolume->BlockShift()));
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
Inode::ShrinkStream(Transaction *transaction, off_t size)
{
	data_stream *data = &Node()->data;

	if (data->MaxDoubleIndirectRange() > size) {
		off_t *maxDoubleIndirect = &data->max_double_indirect_range;
			// gcc 4 work-around: "error: cannot bind packed field
			// 'data->data_stream::max_double_indirect_range' to 'off_t&'"
		FreeStaticStreamArray(transaction, 0, data->double_indirect, size,
			data->MaxIndirectRange(), *maxDoubleIndirect);

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
			block_run *array = (block_run *)cached.SetTo(block + i);
			if (array == NULL)
				break;

			off_t *maxIndirect = &data->max_indirect_range;
				// gcc 4 work-around: "error: cannot bind packed field
				// 'data->data_stream::max_indirect_range' to 'off_t&'"
			if (FreeStreamArray(transaction, array, fVolume->BlockSize() / sizeof(block_run),
					size, offset, *maxIndirect) == B_OK)
				cached.WriteBack(transaction);
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
			// 'data->data_stream::max_indirect_range' to 'off_t&'"
		FreeStreamArray(transaction, data->direct, NUM_DIRECT_BLOCKS, size, offset,
			*maxDirect);
	}

	data->size = HOST_ENDIAN_TO_BFS_INT64(size);
	return B_OK;
}


status_t 
Inode::SetFileSize(Transaction *transaction, off_t size)
{
	if (size < 0
		// uncached files can't be resized (Stream<Cache>::WriteAt() specifically
		// denies growing uncached files because of efficiency, so it had to be
		// adapted if this ever changes [which will probably happen in OpenBeOS]).
		|| Flags() & INODE_NO_CACHE)
		return B_BAD_VALUE;

	off_t oldSize = Size();

	if (size == oldSize)
		return B_OK;

	// should the data stream grow or shrink?
	status_t status;
	if (size > oldSize) {
		status = GrowStream(transaction, size);
		if (status < B_OK) {
			// if the growing of the stream fails, the whole operation
			// fails, so we should shrink the stream to its former size
			ShrinkStream(transaction, oldSize);
		}
	}
	else
		status = ShrinkStream(transaction, size);

	if (status < B_OK)
		return status;

	return WriteBack(transaction);
}


status_t 
Inode::Append(Transaction *transaction, off_t bytes)
{
	return SetFileSize(transaction, Size() + bytes);
}


status_t 
Inode::Trim(Transaction *transaction)
{
	status_t status = ShrinkStream(transaction, Size());
	if (status < B_OK)
		return status;

	return WriteBack(transaction);
}


status_t
Inode::Free(Transaction *transaction)
{
	FUNCTION();

	// Perhaps there should be an implementation of Inode::ShrinkStream() that
	// just frees the data_stream, but doesn't change the inode (since it is
	// freed anyway) - that would make an undelete command possible
	status_t status = SetFileSize(transaction, 0);
	if (status < B_OK)
		return status;

	// Free all attributes, and remove their indices
	{
		// We have to limit the scope of AttributeIterator, so that its
		// destructor is not called after the inode is deleted
		AttributeIterator iterator(this);

		char name[B_FILE_NAME_LENGTH];
		uint32 type;
		size_t length;
		vnode_id id;
		while ((status = iterator.GetNext(name, &length, &type, &id)) == B_OK)
			RemoveAttribute(transaction, name);
	}

	if (WriteBack(transaction) < B_OK)
		return B_ERROR;

	return fVolume->Free(transaction, BlockRun());
}


status_t 
Inode::Sync()
{
	// We may also want to flush the attribute's data stream to
	// disk here... (do we?)

	data_stream *data = &Node()->data;
	status_t status;

	// flush direct range

	for (int32 i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (data->direct[i].IsZero())
			return B_OK;

		status = flush_blocks(fVolume->Device(), fVolume->ToBlock(data->direct[i]),
			data->direct[i].Length());
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
		block_run *runs = (block_run *)cached.SetTo(block + j);
		if (runs == NULL)
			break;

		for (int32 i = 0; i < count; i++) {
			if (runs[i].IsZero())
				return B_OK;

			status = flush_blocks(fVolume->Device(), fVolume->ToBlock(runs[i]), runs[i].Length());
			if (status != B_OK)
				return status;
		}
	}

	// flush double indirect range

	if (data->max_double_indirect_range == 0)
		return B_OK;

	off_t indirectBlock = fVolume->ToBlock(data->double_indirect);
	
	for (int32 l = 0; l < data->double_indirect.Length(); l++) {
		block_run *indirectRuns = (block_run *)cached.SetTo(indirectBlock + l);
		if (indirectRuns == NULL)
			return B_FILE_ERROR;

		CachedBlock directCached(fVolume);

		for (int32 k = 0; k < count; k++) {
			if (indirectRuns[k].IsZero())
				return B_OK;

			block = fVolume->ToBlock(indirectRuns[k]);			
			for (int32 j = 0; j < indirectRuns[k].Length(); j++) {
				block_run *runs = (block_run *)directCached.SetTo(block + j);
				if (runs == NULL)
					return B_FILE_ERROR;

				for (int32 i = 0; i < count; i++) {
					if (runs[i].IsZero())
						return B_OK;

					// ToDo: combine single block_runs to bigger ones when
					// they are adjacent
					status = flush_blocks(fVolume->Device(), fVolume->ToBlock(runs[i]),
						runs[i].Length());
					if (status != B_OK)
						return status;
				}
			}
		}
	}
	return B_OK;
}


status_t
Inode::Remove(Transaction *transaction, const char *name, off_t *_id, bool isDirectory)
{
	BPlusTree *tree;
	if (GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	RecursiveLocker locker(fVolume->Lock());

	// does the file even exist?
	off_t id;
	if (tree->Find((uint8 *)name, (uint16)strlen(name), &id) < B_OK)
		return B_ENTRY_NOT_FOUND;

	if (_id)
		*_id = id;

	Vnode vnode(fVolume, id);
	Inode *inode;
	status_t status = vnode.Get(&inode);
	if (status < B_OK) {
		REPORT_ERROR(status);
		return B_ENTRY_NOT_FOUND;
	}

	// You can't unlink a mounted image or the VM file while it is being used - while
	// this is not really necessary, it copies the behaviour of the original BFS
	// and let you and me feel a little bit safer
	if (inode->Flags() & INODE_NO_CACHE)
		return B_NOT_ALLOWED;

	// Inode::IsContainer() is true also for indices (furthermore, the S_IFDIR
	// bit is set for indices in BFS, not for attribute directories) - but you
	// should really be able to do whatever you want with your indices
	// without having to remove all files first :)
	if (!inode->IsIndex()) {
		// if it's not of the correct type, don't delete it!
		if (inode->IsContainer() != isDirectory)
			return isDirectory ? B_NOT_A_DIRECTORY : B_IS_A_DIRECTORY;

		// only delete empty directories
		if (isDirectory && !inode->IsEmpty())
			return B_DIRECTORY_NOT_EMPTY;
	}

	// remove_vnode() allows the inode to be accessed until the last put_vnode()
	if (remove_vnode(fVolume->ID(), id) != B_OK)
		return B_ERROR;

	if (tree->Remove(transaction, name, id) < B_OK) {
		unremove_vnode(fVolume->ID(), id);
		RETURN_ERROR(B_ERROR);
	}

#ifdef DEBUG
	if (tree->Find((uint8 *)name, (uint16)strlen(name), &id) == B_OK) {
		DIE(("deleted entry still there"));
	}
#endif

	// update the inode, so that no one will ever doubt it's deleted :-)
	inode->Node()->flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED);
	inode->Node()->flags &= ~HOST_ENDIAN_TO_BFS_INT32(INODE_IN_USE);

	// In balance to the Inode::Create() method, the main indices
	// are updated here (name, size, & last_modified)

	Index index(fVolume);
	if (inode->IsRegularNode()) {
		index.RemoveName(transaction, name, inode);
			// If removing from the index fails, it is not regarded as a
			// fatal error and will not be reported back!
			// Deleted inodes won't be visible in queries anyway.
	}

	if ((inode->Mode() & (S_FILE | S_SYMLINK)) != 0) {
		if (inode->IsFile())
			index.RemoveSize(transaction, inode);
		index.RemoveLastModified(transaction, inode);
	}

	if (inode->WriteBack(transaction) < B_OK)
		return B_ERROR;

	return B_OK;
}


/**	Creates the inode with the specified parent directory, and automatically
 *	adds the created inode to that parent directory. If an attribute directory
 *	is created, it will also automatically added to the parent inode as such.
 *	However, the indices root node, and the regular root node won't be added
 *	to the super block.
 *	It will also create the initial B+tree for the inode if it's a directory
 *	of any kind.
 *	If the "_id" or "_inode" variable is given and non-NULL to store the inode's
 *	ID, the inode stays locked - you have to call put_vnode() if you don't use it
 *	anymore.
 */

status_t 
Inode::Create(Transaction *transaction, Inode *parent, const char *name, int32 mode,
	int omode, uint32 type, off_t *_id, Inode **_inode)
{
	FUNCTION();

	block_run parentRun = parent ? parent->BlockRun() : block_run::Run(0, 0, 0);
	Volume *volume = transaction->GetVolume();
	BPlusTree *tree = NULL;

	RecursiveLocker locker(volume->Lock());
		// ToDo: it would be nicer to only lock the parent directory, if possible
		//	(but that lock will already be held during any B+tree action)

	if (parent && (mode & S_ATTR_DIR) == 0 && parent->IsContainer()) {
		// check if the file already exists in the directory
		if (parent->GetTree(&tree) != B_OK)
			RETURN_ERROR(B_BAD_VALUE);

		// does the file already exist?
		off_t offset;
		if (tree->Find((uint8 *)name, (uint16)strlen(name), &offset) == B_OK) {
			// return if the file should be a directory or opened in exclusive mode
			if (mode & S_DIRECTORY || omode & O_EXCL)
				return B_FILE_EXISTS;

			Vnode vnode(volume, offset);
			Inode *inode;
			status_t status = vnode.Get(&inode);
			if (status < B_OK) {
				REPORT_ERROR(status);
				return B_ENTRY_NOT_FOUND;
			}

			// if it's a directory, bail out!
			if (inode->IsDirectory())
				return B_IS_A_DIRECTORY;

			// if it is a mounted device or the VM file, we don't allow to delete it
			// while it is open and in use
			if (inode->Flags() & INODE_NO_CACHE)
				return B_NOT_ALLOWED;

			// if omode & O_TRUNC, truncate the existing file
			if (omode & O_TRUNC) {
				WriteLocked locked(inode->Lock());

				status_t status = inode->SetFileSize(transaction, 0);
				if (status < B_OK)
					return status;
			}

			if (_id)
				*_id = offset;
			if (_inode)
				*_inode = inode;

			// only keep the vnode in memory if the _id or _inode pointer is provided
			if (_id != NULL || _inode != NULL)
				vnode.Keep();

			return B_OK;
		}
	} else if (parent && (mode & S_ATTR_DIR) == 0)
		return B_BAD_VALUE;

	// allocate space for the new inode
	InodeAllocator allocator(transaction);
	block_run run;
	Inode *inode;
	status_t status = allocator.New(&parentRun, mode, run, &inode);
	if (status < B_OK)
		return status;

	// Initialize the parts of the bfs_inode structure that
	// InodeAllocator::New() hasn't touched yet

	bfs_inode *node = inode->Node();

	if (parent == NULL) {
		// we set the parent to itself in this case
		// (only happens for the root and indices node)
		node->parent = run;
	} else
		node->parent = parentRun;

	node->uid = HOST_ENDIAN_TO_BFS_INT32(geteuid());
	node->gid = HOST_ENDIAN_TO_BFS_INT32(parent ? parent->Node()->gid : getegid());
		// the group ID is inherited from the parent, if available

	node->type = HOST_ENDIAN_TO_BFS_INT32(type);

	// only add the name to regular files, directories, or symlinks
	// don't add it to attributes, or indices
	if (tree && inode->IsRegularNode() && inode->SetName(transaction, name) < B_OK)
		return B_ERROR;

	// Initialize b+tree if it's a directory (and add "." & ".." if it's
	// a standard directory for files - not for attributes or indices)
	if (inode->IsContainer()) {
		status = allocator.CreateTree();
		if (status < B_OK)
			return status;
	}

	// Add a link to the inode from the parent, depending on its type
	// (the INODE_NOT_READY flag is set, so it is safe to make the inode
	// accessable to the file system here)
	if (tree) {
		status = tree->Insert(transaction, name, inode->ID());
	} else if (parent && (mode & S_ATTR_DIR) != 0) {
		parent->Attributes() = run;
		status = parent->WriteBack(transaction);
	}

	// Note, we only care if the inode could be made accessable for the
	// two cases above; the root node or the indices root node must
	// handle this case on their own (or other cases where "parent" is
	// NULL)
	if (status < B_OK)
		RETURN_ERROR(status);		

	// Update the main indices (name, size & last_modified)
	// (live queries might want to access us after this)

	Index index(volume);
	if (inode->IsRegularNode() && name != NULL) {
		// the name index only contains regular files
		// (but not the root node where name == NULL)
		status = index.InsertName(transaction, name, inode);
		if (status < B_OK && status != B_BAD_INDEX) {
			// We have to remove the node from the parent at this point,
			// because the InodeAllocator destructor can't handle this
			// case (and if it fails, we can't do anything about it...)
			if (tree)
				tree->Remove(transaction, name, inode->ID());
			else if (parent != NULL && (mode & S_ATTR_DIR) != 0)
				parent->Node()->attributes.SetTo(0, 0, 0);

			return status;
		}
	}

	inode->UpdateOldLastModified();

	// The "size" & "last_modified" indices don't contain directories
	if (inode->IsFile() || inode->IsSymLink()) {
		// if adding to these indices fails, the inode creation will not be harmed;
		// they are considered less important than the "name" index
		if (inode->IsFile())
			index.InsertSize(transaction, inode);
		index.InsertLastModified(transaction, inode);
	}

	// Everything worked well until this point, we have a fully
	// initialized inode, and we want to keep it
	allocator.Keep();

	// We hold the volume lock to make sure that bfs_read_vnode()
	// won't succeed in the meantime (between the call right
	// above and below)!

	if ((status = new_vnode(volume->ID(), inode->ID(), inode)) != B_OK) {
		// this is a really fatal error, and we can't recover from that
		// The only exception is that the volume is being initialized.
		if (volume->ID() >= 0) {
			FATAL(("new_vnode() failed with: %s\n", strerror(status)));
			DIE(("new_vnode() failed for inode!"));
		}
	}

	if (_id != NULL)
		*_id = inode->ID();
	if (_inode != NULL)
		*_inode = inode;

	// if either _id or _inode is passed, we will keep the inode locked
	if (_id == NULL && _inode == NULL)
		put_vnode(volume->ID(), inode->ID());

	return B_OK;
}


//	#pragma mark -


AttributeIterator::AttributeIterator(Inode *inode)
	:
	fCurrentSmallData(0),
	fInode(inode),
	fAttributes(NULL),
	fIterator(NULL),
	fBuffer(NULL)
{
	inode->AddIterator(this);
}


AttributeIterator::~AttributeIterator()
{
	if (fAttributes)
		put_vnode(fAttributes->GetVolume()->ID(), fAttributes->ID());

	delete fIterator;
	fInode->RemoveIterator(this);
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
AttributeIterator::GetNext(char *name, size_t *_length, uint32 *_type, vnode_id *_id)
{
	// read attributes out of the small data section

	if (fCurrentSmallData >= 0) {
		small_data *item = fInode->Node()->SmallDataStart();

		fInode->SmallDataLock().Lock();

		int32 i = 0;
		for (;;item = item->Next()) {
			if (item->IsLast(fInode->Node()))
				break;

			if (item->NameSize() == FILE_NAME_NAME_LENGTH
				&& *item->Name() == FILE_NAME_NAME)
				continue;

			if (i++ == fCurrentSmallData)
				break;
		}

		if (!item->IsLast(fInode->Node())) {
			strncpy(name, item->Name(), B_FILE_NAME_LENGTH);
			*_type = item->Type();
			*_length = item->NameSize();
			*_id = (vnode_id)fCurrentSmallData;

			fCurrentSmallData = i;
		}
		else {
			// stop traversing the small_data section
			fCurrentSmallData = -1;
		}

		fInode->SmallDataLock().Unlock();

		if (fCurrentSmallData != -1)
			return B_OK;
	}

	// read attributes out of the attribute directory

	if (fInode->Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	Volume *volume = fInode->GetVolume();

	// if you haven't yet access to the attributes directory, get it
	if (fAttributes == NULL) {
#ifdef UNSAFE_GET_VNODE
		RecursiveLocker locker(volume->Lock());
#endif
		if (get_vnode(volume->ID(), volume->ToVnode(fInode->Attributes()),
				(void **)&fAttributes) != 0
			|| fAttributes == NULL) {
			FATAL(("get_vnode() failed in AttributeIterator::GetNext(vnode_id = %Ld,name = \"%s\")\n",fInode->ID(),name));
			return B_ENTRY_NOT_FOUND;
		}

		BPlusTree *tree;
		if (fAttributes->GetTree(&tree) < B_OK
			|| (fIterator = new TreeIterator(tree)) == NULL) {
			FATAL(("could not get tree in AttributeIterator::GetNext(vnode_id = %Ld,name = \"%s\")\n",fInode->ID(),name));
			return B_ENTRY_NOT_FOUND;
		}
	}

	uint16 length;
	vnode_id id;
	status_t status = fIterator->GetNextEntry(name, &length, B_FILE_NAME_LENGTH, &id);
	if (status < B_OK)
		return status;

	Vnode vnode(volume,id);
	Inode *attribute;
	if ((status = vnode.Get(&attribute)) == B_OK) {
		*_type = attribute->Type();
		*_length = attribute->Size();
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

