// Volume.h
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

#ifndef VOLUME_H
#define VOLUME_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include "Entry.h"
#include "List.h"
#include "Locker.h"
#include "Query.h"
#include "String.h"

class AllocationInfo;
class Block;
class BlockAllocator;
class BlockReference;
class Directory;
class DirectoryEntryTable;
class Entry;
class EntryListener;
class EntryListenerTree;
class Index;
class IndexDirectory;
class LastModifiedIndex;
class NameIndex;
class Node;
class NodeAttributeTable;
class NodeListener;
class NodeListenerTree;
class NodeTable;
class SizeIndex;

const ino_t kRootParentID = 0;

// NodeListenerValue
class NodeListenerValue {
public:
	inline NodeListenerValue() {}
	inline NodeListenerValue(int) {}
	inline NodeListenerValue(NodeListener *listener, Node *node, uint32 flags)
		: listener(listener), node(node), flags(flags) {}

	inline bool operator==(const NodeListenerValue &other)
		{ return listener == other.listener; }

	NodeListener	*listener;
	Node			*node;
	uint32			flags;
};
typedef List<NodeListenerValue> NodeListenerList;

// EntryListenerValue
class EntryListenerValue {
public:
	inline EntryListenerValue() {}
	inline EntryListenerValue(int) {}
	inline EntryListenerValue(EntryListener *listener, Entry *entry,
							  uint32 flags)
		: listener(listener), entry(entry), flags(flags) {}

	inline bool operator==(const EntryListenerValue &other)
		{ return listener == other.listener; }

	EntryListener	*listener;
	Entry			*entry;
	uint32			flags;
};
typedef List<EntryListenerValue> EntryListenerList;

// Volume
class Volume {
public:
							Volume(fs_volume* volume);
							~Volume();

	status_t Mount(uint32 flags);
	status_t Unmount();

	dev_t GetID() const { return fID; }
	fs_volume* FSVolume() const { return fVolume; }

	off_t GetBlockSize() const;
	off_t CountBlocks() const;
	off_t CountFreeBlocks() const;

	status_t SetName(const char *name);
	const char *GetName() const;

	Directory *GetRootDirectory() const		{ return fRootDirectory; }

	status_t NewVNode(Node *node);
	status_t PublishVNode(Node *node);
	status_t GetVNode(ino_t id, Node **node);
	status_t GetVNode(Node *node);
	status_t PutVNode(ino_t id);
	status_t PutVNode(Node *node);
	status_t RemoveVNode(Node *node);
	status_t UnremoveVNode(Node *node);

	// node table and listeners
	status_t NodeAdded(Node *node);
	status_t NodeRemoved(Node *node);
	status_t FindNode(ino_t id, Node **node);
	status_t AddNodeListener(NodeListener *listener, Node *node,
							 uint32 flags);
	status_t RemoveNodeListener(NodeListener *listener, Node *node);

	// entry table and listeners
	status_t EntryAdded(ino_t id, Entry *entry);
	status_t EntryRemoved(ino_t id, Entry *entry);
	status_t FindEntry(ino_t id, const char *name, Entry **entry);
	status_t AddEntryListener(EntryListener *listener, Entry *entry,
							  uint32 flags);
	status_t RemoveEntryListener(EntryListener *listener, Entry *entry);

	// node attribute table
	status_t NodeAttributeAdded(ino_t id, Attribute *attribute);
	status_t NodeAttributeRemoved(ino_t id, Attribute *attribute);
	status_t FindNodeAttribute(ino_t id, const char *name,
							   Attribute **attribute);

	// indices
	IndexDirectory *GetIndexDirectory() const	{ return fIndexDirectory; }
	NameIndex *GetNameIndex() const;
	LastModifiedIndex *GetLastModifiedIndex() const;
	SizeIndex *GetSizeIndex() const;
	Index *FindIndex(const char *name);
	AttributeIndex *FindAttributeIndex(const char *name, uint32 type);

	// queries
	void AddQuery(Query *query);
	void RemoveQuery(Query *query);
	void UpdateLiveQueries(Entry *entry, Node* node, const char *attribute,
			int32 type, const uint8 *oldKey, size_t oldLength,
			const uint8 *newKey, size_t newLength);

	ino_t NextNodeID() { return fNextNodeID++; }

	status_t AllocateBlock(size_t size, BlockReference **block);
	void FreeBlock(BlockReference *block);
	BlockReference *ResizeBlock(BlockReference *block, size_t size);
	// debugging only
	bool CheckBlock(BlockReference *block, size_t size = 0);
	void GetAllocationInfo(AllocationInfo &info);

	bigtime_t GetAccessTime() const	{ return fAccessTime; }

	// locking
	bool ReadLock();
	void ReadUnlock();
	bool WriteLock();
	void WriteUnlock();

	bool IteratorLock();
	void IteratorUnlock();

protected:
	fs_volume*				fVolume;

private:
	typedef DoublyLinkedList<Query>	QueryList;

	dev_t					fID;
	ino_t					fNextNodeID;
	NodeTable				*fNodeTable;
	DirectoryEntryTable		*fDirectoryEntryTable;
	NodeAttributeTable		*fNodeAttributeTable;
	IndexDirectory			*fIndexDirectory;
	Directory				*fRootDirectory;
	String					fName;
	Locker					fLocker;
	Locker					fIteratorLocker;
	Locker					fQueryLocker;
	NodeListenerTree		*fNodeListeners;
	NodeListenerList		fAnyNodeListeners;
	EntryListenerTree		*fEntryListeners;
	EntryListenerList		fAnyEntryListeners;
	QueryList				fQueries;
	BlockAllocator			*fBlockAllocator;
	off_t					fBlockSize;
	off_t					fAllocatedBlocks;
	bigtime_t				fAccessTime;
	bool					fMounted;
};

#endif	// VOLUME_H
