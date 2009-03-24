/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FUSE_ENTRY_H
#define USERLAND_FS_FUSE_ENTRY_H

#include <new>

#include <RWLockManager.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "String.h"

#include "fuse_api.h"


namespace UserlandFS {

struct FUSENode;


struct FUSEEntryRef {
	ino_t		parentID;
	const char*	name;

	FUSEEntryRef(ino_t parentID, const char* name)
		:
		parentID(parentID),
		name(name)
	{
	}
};


struct FUSEEntry : public HashTableLink<FUSEEntry>,
		DoublyLinkedListLinkImpl<FUSEEntry> {
	FUSENode*	parent;
	char*		name;
	FUSENode*	node;

	FUSEEntry()
		:
		name(NULL)
	{
	}

	~FUSEEntry()
	{
		free(name);
	}


	static FUSEEntry* Create(FUSENode* parent, const char* name, FUSENode* node)
	{
		FUSEEntry* entry = new(std::nothrow) FUSEEntry;
		if (entry == NULL)
			return NULL;

		char* clonedName = strdup(name);
		if (clonedName == NULL) {
			delete entry;
			return NULL;
		}

		entry->parent = parent;
		entry->name = clonedName;
		entry->node = node;

		return entry;
	}
};


typedef DoublyLinkedList<FUSEEntry> FUSEEntryList;


struct FUSENode : RWLockable, HashTableLink<FUSENode> {
	ino_t			id;
	FUSEEntryList	entries;
	int				type;
	int32			refCount;

	FUSENode(ino_t id, int type)
		:
		id(id),
		type(type),
		refCount(1)
	{
	}

	FUSENode* Parent() const
	{
		FUSEEntry* entry = entries.Head();
		return entry != NULL ? entry->parent : NULL;
	}
};


struct FUSEEntryHashDefinition {
	typedef FUSEEntryRef	KeyType;
	typedef	FUSEEntry		ValueType;

	size_t HashKey(const FUSEEntryRef& key) const
		{ return ((uint32)key.parentID ^ (uint32)(key.parentID >> 32)) * 37
			+ string_hash(key.name); }
	size_t Hash(const FUSEEntry* value) const
		{ return HashKey(FUSEEntryRef(value->parent->id, value->name)); }
	bool Compare(const FUSEEntryRef& key, const FUSEEntry* value) const
		{ return value->parent->id == key.parentID
			&& strcmp(value->name, key.name) == 0; }
	HashTableLink<FUSEEntry>* GetLink(FUSEEntry* value) const
		{ return value; }
};


struct FUSENodeHashDefinition {
	typedef ino_t		KeyType;
	typedef	FUSENode	ValueType;

	size_t HashKey(ino_t key) const
		{ return (uint32)key ^ (uint32)(key >> 32); }
	size_t Hash(const FUSENode* value) const
		{ return HashKey(value->id); }
	bool Compare(ino_t key, const FUSENode* value) const
		{ return value->id == key; }
	HashTableLink<FUSENode>* GetLink(FUSENode* value) const
		{ return value; }
};


typedef OpenHashTable<FUSEEntryHashDefinition> FUSEEntryTable;
typedef OpenHashTable<FUSENodeHashDefinition> FUSENodeTable;


}	// namespace UserlandFS

using UserlandFS::FUSENode;


#endif	// USERLAND_FS_FUSE_ENTRY_H
