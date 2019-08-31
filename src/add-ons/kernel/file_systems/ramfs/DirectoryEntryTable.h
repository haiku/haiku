/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef DIRECTORY_ENTRY_TABLE_H
#define DIRECTORY_ENTRY_TABLE_H

#include <util/OpenHashTable.h>

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "Misc.h"
#include "Node.h"

// DirectoryEntryHash
struct DirectoryEntryHash {
	struct Key {
		ino_t id;
		const char* name;

		Key(ino_t i, const char* n) : id(i), name(n) {}
	};
	typedef Key			KeyType;
	typedef	Entry		ValueType;

	size_t HashKey(KeyType key) const
	{
		return node_child_hash(key.id, key.name);
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(Key(value->GetParent()->GetID(), value->GetName()));
	}

	bool Compare(KeyType key, ValueType* value) const
	{
		return (value->GetParent()->GetID() == key.id
			&& !strcmp(value->GetName(), key.name));
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->HashLink();
	}
};

// DirectoryEntryTable
class DirectoryEntryTable {
public:
	DirectoryEntryTable();
	~DirectoryEntryTable();

	status_t InitCheck() const;

	status_t AddEntry(Directory *node, Entry *child);
	status_t AddEntry(ino_t, Entry *child);
	status_t RemoveEntry(Directory *node, Entry *child);
	status_t RemoveEntry(ino_t id, Entry *child);
	status_t RemoveEntry(ino_t id, const char *name);
	Entry *GetEntry(ino_t id, const char *name);

	void GetAllocationInfo(AllocationInfo &info)
	{
		info.AddDirectoryEntryTableAllocation(0, fTable.TableSize(),
			sizeof(void*), fTable.CountElements());
	}

protected:
	BOpenHashTable<DirectoryEntryHash> fTable;
	status_t fInitStatus;
};

// constructor
DirectoryEntryTable::DirectoryEntryTable()
{
	fInitStatus = fTable.Init(1000);
}

// destructor
DirectoryEntryTable::~DirectoryEntryTable()
{
}

// InitCheck
status_t
DirectoryEntryTable::InitCheck() const
{
	RETURN_ERROR(fInitStatus);
}

// AddEntry
status_t
DirectoryEntryTable::AddEntry(Directory *node, Entry *child)
{
	status_t error = (node && child ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = AddEntry(node->GetID(), child);
	return error;
}

// AddEntry
status_t
DirectoryEntryTable::AddEntry(ino_t id, Entry *child)
{
	status_t error = (child ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		RemoveEntry(id, child);
		SET_ERROR(error, fTable.Insert(child));
	}
	return error;
}

// RemoveEntry
status_t
DirectoryEntryTable::RemoveEntry(Directory *node, Entry *child)
{
	status_t error = (node && child ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = RemoveEntry(node->GetID(), child->GetName());
	return error;
}

// RemoveEntry
status_t
DirectoryEntryTable::RemoveEntry(ino_t id, Entry *child)
{
	status_t error = (child ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = RemoveEntry(id, child->GetName());
	return error;
}

// RemoveEntry
status_t
DirectoryEntryTable::RemoveEntry(ino_t id, const char *name)
{
	Entry* child = fTable.Lookup(DirectoryEntryHash::Key(id, name));
	if (!child)
		return B_NAME_NOT_FOUND;
	return fTable.Remove(child) ? B_OK : B_ERROR;
}

// GetEntry
Entry *
DirectoryEntryTable::GetEntry(ino_t id, const char *name)
{
	Entry *child = fTable.Lookup(DirectoryEntryHash::Key(id, name));
	return child;
}

#endif	// DIRECTORY_ENTRY_TABLE_H
