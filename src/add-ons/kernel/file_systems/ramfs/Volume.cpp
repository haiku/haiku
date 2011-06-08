// Volume.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Block.h"
#include "BlockAllocator.h"
#include "Debug.h"
#include "Directory.h"
#include "Entry.h"
#include "EntryListener.h"
#include "IndexDirectory.h"
#include "Locking.h"
#include "Misc.h"
#include "NameIndex.h"
#include "Node.h"
#include "NodeChildTable.h"
#include "NodeListener.h"
#include "NodeTable.h"
#include "TwoKeyAVLTree.h"
#include "Volume.h"

// default block size
static const off_t kDefaultBlockSize = 4096;

static const size_t kDefaultAreaSize = kDefaultBlockSize * 128;

// default volume name
static const char *kDefaultVolumeName = "RAM FS";

// NodeListenerGetPrimaryKey
class NodeListenerGetPrimaryKey {
public:
	inline Node *operator()(const NodeListenerValue &a)
	{
		return a.node;
	}

	inline Node *operator()(const NodeListenerValue &a) const
	{
		return a.node;
	}
};

// NodeListenerGetSecondaryKey
class NodeListenerGetSecondaryKey {
public:
	inline NodeListener *operator()(const NodeListenerValue &a)
	{
		return a.listener;
	}

	inline NodeListener *operator()(const NodeListenerValue &a) const
	{
		return a.listener;
	}
};

// NodeListenerTree
typedef TwoKeyAVLTree<NodeListenerValue, Node*,
					  TwoKeyAVLTreeStandardCompare<Node*>,
					  NodeListenerGetPrimaryKey, NodeListener*,
					  TwoKeyAVLTreeStandardCompare<NodeListener*>,
					  NodeListenerGetSecondaryKey > _NodeListenerTree;
class NodeListenerTree : public _NodeListenerTree {};

// EntryListenerGetPrimaryKey
class EntryListenerGetPrimaryKey {
public:
	inline Entry *operator()(const EntryListenerValue &a)
	{
		return a.entry;
	}

	inline Entry *operator()(const EntryListenerValue &a) const
	{
		return a.entry;
	}
};

// EntryListenerGetSecondaryKey
class EntryListenerGetSecondaryKey {
public:
	inline EntryListener *operator()(const EntryListenerValue &a)
	{
		return a.listener;
	}

	inline EntryListener *operator()(const EntryListenerValue &a) const
	{
		return a.listener;
	}
};

// EntryListenerTree
typedef TwoKeyAVLTree<EntryListenerValue, Entry*,
					  TwoKeyAVLTreeStandardCompare<Entry*>,
					  EntryListenerGetPrimaryKey, EntryListener*,
					  TwoKeyAVLTreeStandardCompare<EntryListener*>,
					  EntryListenerGetSecondaryKey > _EntryListenerTree;
class EntryListenerTree : public _EntryListenerTree {};


/*!
	\class Volume
	\brief Represents a volume.
*/

// constructor
Volume::Volume(fs_volume* volume)
	:
	fVolume(volume),
	fID(0),
	fNextNodeID(kRootParentID + 1),
	fNodeTable(NULL),
	fDirectoryEntryTable(NULL),
	fNodeAttributeTable(NULL),
	fIndexDirectory(NULL),
	fRootDirectory(NULL),
	fName(kDefaultVolumeName),
	fLocker("volume"),
	fIteratorLocker("iterators"),
	fQueryLocker("queries"),
	fNodeListeners(NULL),
	fAnyNodeListeners(),
	fEntryListeners(NULL),
	fAnyEntryListeners(),
	fBlockAllocator(NULL),
	fBlockSize(kDefaultBlockSize),
	fAllocatedBlocks(0),
	fAccessTime(0),
	fMounted(false)
{
}


// destructor
Volume::~Volume()
{
	Unmount();
}


// Mount
status_t
Volume::Mount(uint32 flags)
{
	Unmount();

	// check the locker's semaphores
	if (fLocker.Sem() < 0)
		return fLocker.Sem();
	if (fIteratorLocker.Sem() < 0)
		return fIteratorLocker.Sem();
	if (fQueryLocker.Sem() < 0)
		return fQueryLocker.Sem();

	status_t error = B_OK;
	fID = id;
	// create a block allocator
	if (error == B_OK) {
		fBlockAllocator = new(nothrow) BlockAllocator(kDefaultAreaSize);
		if (fBlockAllocator)
			error = fBlockAllocator->InitCheck();
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// create the listener trees
	if (error == B_OK) {
		fNodeListeners = new(nothrow) NodeListenerTree;
		if (!fNodeListeners)
			error = B_NO_MEMORY;
	}
	if (error == B_OK) {
		fEntryListeners = new(nothrow) EntryListenerTree;
		if (!fEntryListeners)
			error = B_NO_MEMORY;
	}
	// create the node table
	if (error == B_OK) {
		fNodeTable = new(nothrow) NodeTable;
		if (fNodeTable)
			error = fNodeTable->InitCheck();
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// create the directory entry table
	if (error == B_OK) {
		fDirectoryEntryTable = new(nothrow) DirectoryEntryTable;
		if (fDirectoryEntryTable)
			error = fDirectoryEntryTable->InitCheck();
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// create the node attribute table
	if (error == B_OK) {
		fNodeAttributeTable = new(nothrow) NodeAttributeTable;
		if (fNodeAttributeTable)
			error = fNodeAttributeTable->InitCheck();
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// create the index directory
	if (error == B_OK) {
		fIndexDirectory = new(nothrow) IndexDirectory(this);
		if (!fIndexDirectory)
			SET_ERROR(error, B_NO_MEMORY);
	}
	// create the root dir
	if (error == B_OK) {
		fRootDirectory = new(nothrow) Directory(this);
		if (fRootDirectory) {
			// set permissions: -rwxr-xr-x
			fRootDirectory->SetMode(
				S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			error = fRootDirectory->Link(NULL);
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// set mounted flag / cleanup on error
	if (error == B_OK)
		fMounted = true;
	else
		Unmount();
	RETURN_ERROR(error);
}

// Unmount
status_t
Volume::Unmount()
{
	fMounted = false;
	// delete the root directory
	if (fRootDirectory) {
		// deleting the root directory destroys the complete hierarchy
		delete fRootDirectory;
		fRootDirectory = NULL;
	}
	// delete the index directory
	if (fIndexDirectory) {
		delete fIndexDirectory;
		fIndexDirectory = NULL;
	}
	// delete the listener trees
	if (fEntryListeners) {
		delete fEntryListeners;
		fEntryListeners = NULL;
	}
	if (fNodeListeners) {
		delete fNodeListeners;
		fNodeListeners = NULL;
	}
	// delete the tables
	if (fNodeAttributeTable) {
		delete fNodeAttributeTable;
		fNodeAttributeTable = NULL;
	}
	if (fDirectoryEntryTable) {
		delete fDirectoryEntryTable;
		fDirectoryEntryTable = NULL;
	}
	if (fNodeTable) {
		delete fNodeTable;
		fNodeTable = NULL;
	}
	// delete the block allocator
	if (fBlockAllocator) {
		delete fBlockAllocator;
		fBlockAllocator = NULL;
	}
	fID = 0;
	return B_OK;
}

// GetBlockSize
off_t
Volume::GetBlockSize() const
{
	return fBlockSize;
}

// CountBlocks
off_t
Volume::CountBlocks() const
{
	size_t bytes = 0;
	system_info sysInfo;
	if (get_system_info(&sysInfo) == B_OK) {
		int32 freePages = sysInfo.max_pages - sysInfo.used_pages;
		bytes = (uint32)freePages * B_PAGE_SIZE
				+ fBlockAllocator->GetAvailableBytes();
	}
	return bytes / kDefaultBlockSize;
}

// CountFreeBlocks
off_t
Volume::CountFreeBlocks() const
{
	// TODO:...
	return CountBlocks() - fBlockAllocator->GetUsedBytes() / kDefaultBlockSize;
}

// SetName
status_t
Volume::SetName(const char *name)
{
	status_t error = (name ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!fName.SetTo(name))
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// GetName
const char *
Volume::GetName() const
{
	return fName.GetString();
}

// NewVNode
status_t
Volume::NewVNode(Node *node)
{
	status_t error = NodeAdded(node);
	if (error == B_OK) {
		error = new_vnode(GetID(), node->GetID(), node);
		if (error != B_OK)
			NodeRemoved(node);
	}
	return error;
}

// PublishVNode
status_t
Volume::PublishVNode(Node *node)
{
	status_t error = NodeAdded(node);
	if (error == B_OK) {
		error = publish_vnode(GetID(), node->GetID(), node);
		if (error != B_OK)
			NodeRemoved(node);
	}
	return error;
}

// GetVNode
status_t
Volume::GetVNode(ino_t id, Node **node)
{
	return (fMounted ? get_vnode(GetID(), id, (void**)node) : B_BAD_VALUE);
}

// GetVNode
status_t
Volume::GetVNode(Node *node)
{
	Node *dummy = NULL;
	status_t error = (fMounted ? GetVNode(node->GetID(), &dummy)
							   : B_BAD_VALUE );
	if (error == B_OK && dummy != node) {
		FATAL(("Two Nodes have the same ID: %Ld!\n", node->GetID()));
		PutVNode(dummy);
		error = B_ERROR;
	}
	return error;
}

// PutVNode
status_t
Volume::PutVNode(ino_t id)
{
	return (fMounted ? put_vnode(GetID(), id) : B_BAD_VALUE);
}

// PutVNode
status_t
Volume::PutVNode(Node *node)
{
	return (fMounted ? put_vnode(GetID(), node->GetID()) : B_BAD_VALUE);
}

// RemoveVNode
status_t
Volume::RemoveVNode(Node *node)
{
	if (fMounted)
		return remove_vnode(GetID(), node->GetID());
	status_t error = NodeRemoved(node);
	if (error == B_OK)
		delete node;
	return error;
}

// UnremoveVNode
status_t
Volume::UnremoveVNode(Node *node)
{
	return (fMounted ? unremove_vnode(GetID(), node->GetID()) : B_BAD_VALUE);
}

// NodeAdded
status_t
Volume::NodeAdded(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = fNodeTable->AddNode(node);
		// notify listeners
		if (error == B_OK) {
			// listeners interested in that node
			NodeListenerTree::Iterator it;
			if (fNodeListeners->FindFirst(node, &it)) {
				for (NodeListenerValue *value = it.GetCurrent();
					 value && value->node == node;
					 value = it.GetNext()) {
					 if (value->flags & NODE_LISTEN_ADDED)
						 value->listener->NodeAdded(node);
				}
			}
			// listeners interested in any node
			int32 count = fAnyNodeListeners.CountItems();
			for (int32 i = 0; i < count; i++) {
				const NodeListenerValue &value = fAnyNodeListeners.ItemAt(i);
				 if (value.flags & NODE_LISTEN_ADDED)
					 value.listener->NodeAdded(node);
			}
		}
	}
	return error;
}

// NodeRemoved
status_t
Volume::NodeRemoved(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = fNodeTable->RemoveNode(node);
		// notify listeners
		if (error == B_OK) {
			// listeners interested in that node
			NodeListenerTree::Iterator it;
			if (fNodeListeners->FindFirst(node, &it)) {
				for (NodeListenerValue *value = it.GetCurrent();
					 value && value->node == node;
					 value = it.GetNext()) {
					 if (value->flags & NODE_LISTEN_REMOVED)
						 value->listener->NodeRemoved(node);
				}
			}
			// listeners interested in any node
			int32 count = fAnyNodeListeners.CountItems();
			for (int32 i = 0; i < count; i++) {
				const NodeListenerValue &value = fAnyNodeListeners.ItemAt(i);
				 if (value.flags & NODE_LISTEN_REMOVED)
					 value.listener->NodeRemoved(node);
			}
		}
	}
	return error;
}

// FindNode
/*!	\brief Finds the node identified by a ino_t.

	\note The method does not initialize the parent ID for non-directory nodes.

	\param id ID of the node to be found.
	\param node pointer to a pre-allocated Node* to be set to the found node.
	\return \c B_OK, if everything went fine.
*/
status_t
Volume::FindNode(ino_t id, Node **node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*node = fNodeTable->GetNode(id);
		if (!*node)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// AddNodeListener
status_t
Volume::AddNodeListener(NodeListener *listener, Node *node, uint32 flags)
{
	// check parameters
	if (!listener || !node && !(flags & NODE_LISTEN_ANY_NODE)
		|| !(flags & NODE_LISTEN_ALL)) {
		return B_BAD_VALUE;
	}
	// add the listener to the right container
	status_t error = B_OK;
	NodeListenerValue value(listener, node, flags);
	if (flags & NODE_LISTEN_ANY_NODE) {
		if (!fAnyNodeListeners.AddItem(value))
			error = B_NO_MEMORY;
	} else
		error = fNodeListeners->Insert(value);
	return error;
}

// RemoveNodeListener
status_t
Volume::RemoveNodeListener(NodeListener *listener, Node *node)
{
	if (!listener)
		return B_BAD_VALUE;
	status_t error = B_OK;
	if (node)
		error = fNodeListeners->Remove(node, listener);
	else {
		NodeListenerValue value(listener, node, 0);
		if (!fAnyNodeListeners.RemoveItem(value))
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// EntryAdded
status_t
Volume::EntryAdded(ino_t id, Entry *entry)
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = fDirectoryEntryTable->AddNodeChild(id, entry);
		if (error == B_OK) {
			// notify listeners
			// listeners interested in that entry
			EntryListenerTree::Iterator it;
			if (fEntryListeners->FindFirst(entry, &it)) {
				for (EntryListenerValue *value = it.GetCurrent();
					 value && value->entry == entry;
					 value = it.GetNext()) {
					 if (value->flags & ENTRY_LISTEN_ADDED)
						 value->listener->EntryAdded(entry);
				}
			}
			// listeners interested in any entry
			int32 count = fAnyEntryListeners.CountItems();
			for (int32 i = 0; i < count; i++) {
				const EntryListenerValue &value = fAnyEntryListeners.ItemAt(i);
				 if (value.flags & ENTRY_LISTEN_ADDED)
					 value.listener->EntryAdded(entry);
			}
		}
	}
	return error;
}

// EntryRemoved
status_t
Volume::EntryRemoved(ino_t id, Entry *entry)
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = fDirectoryEntryTable->RemoveNodeChild(id, entry);
		if (error == B_OK) {
			// notify listeners
			// listeners interested in that entry
			EntryListenerTree::Iterator it;
			if (fEntryListeners->FindFirst(entry, &it)) {
				for (EntryListenerValue *value = it.GetCurrent();
					 value && value->entry == entry;
					 value = it.GetNext()) {
					 if (value->flags & ENTRY_LISTEN_REMOVED)
						 value->listener->EntryRemoved(entry);
				}
			}
			// listeners interested in any entry
			int32 count = fAnyEntryListeners.CountItems();
			for (int32 i = 0; i < count; i++) {
				const EntryListenerValue &value = fAnyEntryListeners.ItemAt(i);
				 if (value.flags & ENTRY_LISTEN_REMOVED)
					 value.listener->EntryRemoved(entry);
			}
		}
	}
	return error;
}

// FindEntry
status_t
Volume::FindEntry(ino_t id, const char *name, Entry **entry)
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*entry = fDirectoryEntryTable->GetNodeChild(id, name);
		if (!*entry)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// AddEntryListener
status_t
Volume::AddEntryListener(EntryListener *listener, Entry *entry, uint32 flags)
{
	// check parameters
	if (!listener || !entry && !(flags & ENTRY_LISTEN_ANY_ENTRY)
		|| !(flags & ENTRY_LISTEN_ALL)) {
		return B_BAD_VALUE;
	}
	// add the listener to the right container
	status_t error = B_OK;
	EntryListenerValue value(listener, entry, flags);
	if (flags & ENTRY_LISTEN_ANY_ENTRY) {
		if (!fAnyEntryListeners.AddItem(value))
			error = B_NO_MEMORY;
	} else
		error = fEntryListeners->Insert(value);
	return error;
}

// RemoveEntryListener
status_t
Volume::RemoveEntryListener(EntryListener *listener, Entry *entry)
{
	if (!listener)
		return B_BAD_VALUE;
	status_t error = B_OK;
	if (entry)
		error = fEntryListeners->Remove(entry, listener);
	else {
		EntryListenerValue value(listener, entry, 0);
		if (!fAnyEntryListeners.RemoveItem(value))
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// NodeAttributeAdded
status_t
Volume::NodeAttributeAdded(ino_t id, Attribute *attribute)
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = fNodeAttributeTable->AddNodeChild(id, attribute);
		// notify the respective attribute index
		if (error == B_OK) {
			if (AttributeIndex *index = FindAttributeIndex(
					attribute->GetName(), attribute->GetType())) {
				index->Added(attribute);
			}
		}
	}
	return error;
}

// NodeAttributeRemoved
status_t
Volume::NodeAttributeRemoved(ino_t id, Attribute *attribute)
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = fNodeAttributeTable->RemoveNodeChild(id, attribute);
		// notify the respective attribute index
		if (error == B_OK) {
			if (AttributeIndex *index = FindAttributeIndex(
					attribute->GetName(), attribute->GetType())) {
				index->Removed(attribute);
			}
		}

		// update live queries
		if (error == B_OK && attribute->GetNode()) {
			const uint8* oldKey;
			size_t oldLength;
			attribute->GetKey(&oldKey, &oldLength);
			UpdateLiveQueries(NULL, attribute->GetNode(), attribute->GetName(),
				attribute->GetType(), oldKey, oldLength, NULL, 0);
		}
	}
	return error;
}

// FindNodeAttribute
status_t
Volume::FindNodeAttribute(ino_t id, const char *name, Attribute **attribute)
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*attribute = fNodeAttributeTable->GetNodeChild(id, name);
		if (!*attribute)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetNameIndex
NameIndex *
Volume::GetNameIndex() const
{
	return (fIndexDirectory ? fIndexDirectory->GetNameIndex() : NULL);
}

// GetLastModifiedIndex
LastModifiedIndex *
Volume::GetLastModifiedIndex() const
{
	return (fIndexDirectory ? fIndexDirectory->GetLastModifiedIndex() : NULL);
}

// GetSizeIndex
SizeIndex *
Volume::GetSizeIndex() const
{
	return (fIndexDirectory ? fIndexDirectory->GetSizeIndex() : NULL);
}

// FindIndex
Index *
Volume::FindIndex(const char *name)
{
	return (fIndexDirectory ? fIndexDirectory->FindIndex(name) : NULL);
}

// FindAttributeIndex
AttributeIndex *
Volume::FindAttributeIndex(const char *name, uint32 type)
{
	return (fIndexDirectory
			? fIndexDirectory->FindAttributeIndex(name, type) : NULL);
}

// AddQuery
void
Volume::AddQuery(Query *query)
{
	AutoLocker<Locker> _(fQueryLocker);

	if (query)
		fQueries.Insert(query);
}

// RemoveQuery
void
Volume::RemoveQuery(Query *query)
{
	AutoLocker<Locker> _(fQueryLocker);

	if (query)
		fQueries.Remove(query);
}

// UpdateLiveQueries
void
Volume::UpdateLiveQueries(Entry *entry, Node* node, const char *attribute,
	int32 type, const uint8 *oldKey, size_t oldLength, const uint8 *newKey,
	size_t newLength)
{
	AutoLocker<Locker> _(fQueryLocker);

	for (Query* query = fQueries.First();
		 query;
		 query = fQueries.GetNext(query)) {
		query->LiveUpdate(entry, node, attribute, type, oldKey, oldLength,
			newKey, newLength);
	}
}

// AllocateBlock
status_t
Volume::AllocateBlock(size_t size, BlockReference **block)
{
	status_t error = (size > 0 && size <= fBlockSize && block
					  ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*block = fBlockAllocator->AllocateBlock(size);
		if (*block)
			fAllocatedBlocks++;
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// FreeBlock
void
Volume::FreeBlock(BlockReference *block)
{
	if (block) {
		fBlockAllocator->FreeBlock(block);
		fAllocatedBlocks--;
	}
}

// ResizeBlock
BlockReference *
Volume::ResizeBlock(BlockReference *block, size_t size)
{
	BlockReference *newBlock = NULL;
	if (size <= fBlockSize && block) {
		if (size == 0) {
			fBlockAllocator->FreeBlock(block);
			fAllocatedBlocks--;
		} else
			newBlock = fBlockAllocator->ResizeBlock(block, size);
	}
	return newBlock;
}

// CheckBlock
bool
Volume::CheckBlock(BlockReference *block, size_t size)
{
	return fBlockAllocator->CheckBlock(block, size);
}

// GetAllocationInfo
void
Volume::GetAllocationInfo(AllocationInfo &info)
{
	// tables
	info.AddOtherAllocation(sizeof(NodeTable));
	fNodeTable->GetAllocationInfo(info);
	info.AddOtherAllocation(sizeof(DirectoryEntryTable));
	fDirectoryEntryTable->GetAllocationInfo(info);
	info.AddOtherAllocation(sizeof(NodeAttributeTable));
	fNodeAttributeTable->GetAllocationInfo(info);
	// node hierarchy
	fRootDirectory->GetAllocationInfo(info);
	// name
	info.AddStringAllocation(fName.GetLength());
	// block allocator
	info.AddOtherAllocation(sizeof(BlockAllocator));
	fBlockAllocator->GetAllocationInfo(info);
}

// ReadLock
bool
Volume::ReadLock()
{
	bool alreadyLocked = fLocker.IsLocked();
	if (fLocker.Lock()) {
		if (!alreadyLocked)
			fAccessTime = system_time();
		return true;
	}
	return false;
}

// ReadUnlock
void
Volume::ReadUnlock()
{
	fLocker.Unlock();
}

// WriteLock
bool
Volume::WriteLock()
{
	bool alreadyLocked = fLocker.IsLocked();
	if (fLocker.Lock()) {
		if (!alreadyLocked)
			fAccessTime = system_time();
		return true;
	}
	return false;
}

// WriteUnlock
void
Volume::WriteUnlock()
{
	fLocker.Unlock();
}

// IteratorLock
bool
Volume::IteratorLock()
{
	return fIteratorLocker.Lock();
}

// IteratorUnlock
void
Volume::IteratorUnlock()
{
	fIteratorLocker.Unlock();
}

