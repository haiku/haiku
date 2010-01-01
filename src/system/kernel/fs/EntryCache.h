/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENTRY_CACHE_H
#define ENTRY_CACHE_H


#include <stdlib.h>

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/khash.h>
#include <util/OpenHashTable.h>


const static uint32 kMaxEntryCacheEntryCount = 8192;
	// Maximum number of entries per entry cache. It's a hard limit ATM.

struct EntryCacheKey {
	EntryCacheKey(ino_t dirID, const char* name)
		:
		dir_id(dirID),
		name(name)
	{
	}

	ino_t		dir_id;
	const char*	name;
};


struct EntryCacheEntry : DoublyLinkedListLinkImpl<EntryCacheEntry> {
	EntryCacheEntry*	hash_link;
	ino_t				node_id;
	ino_t				dir_id;
	char				name[1];
};


struct EntryCacheHashDefinition {
	typedef EntryCacheKey	KeyType;
	typedef EntryCacheEntry	ValueType;

	uint32 HashKey(const EntryCacheKey& key) const
	{
		return (uint32)key.dir_id ^ (uint32)(key.dir_id >> 32)
			^ hash_hash_string(key.name);
	}

	size_t Hash(const EntryCacheEntry* value) const
	{
		return (uint32)value->dir_id ^ (uint32)(value->dir_id >> 32)
			^ hash_hash_string(value->name);
	}

	bool Compare(const EntryCacheKey& key, const EntryCacheEntry* value) const
	{
		return value->dir_id == key.dir_id
			&& strcmp(value->name, key.name) == 0;
	}

	EntryCacheEntry*& GetLink(EntryCacheEntry* value) const
	{
		return value->hash_link;
	}
};


class EntryCache {
public:
	EntryCache()
	{
		mutex_init(&fLock, "entry cache");

		new(&fEntries) EntryTable;
		new(&fUsedEntries) EntryList;
		fEntryCount = 0;
	}

	~EntryCache()
	{
		while (EntryCacheEntry* entry = fUsedEntries.Head())
			_Remove(entry);

		mutex_destroy(&fLock);
	}

	status_t Init()
	{
		return fEntries.Init();
	}

	status_t Add(ino_t dirID, const char* name, ino_t nodeID)
	{
		MutexLocker _(fLock);

		EntryCacheEntry* entry = fEntries.Lookup(EntryCacheKey(dirID, name));
		if (entry != NULL) {
			entry->node_id = nodeID;
			return B_OK;
		}

		if (fEntryCount >= kMaxEntryCacheEntryCount)
			_Remove(fUsedEntries.Head());

		entry = (EntryCacheEntry*)malloc(sizeof(EntryCacheEntry)
			+ strlen(name));
		if (entry == NULL)
			return B_NO_MEMORY;

		entry->node_id = nodeID;
		entry->dir_id = dirID;
		strcpy(entry->name, name);

		fEntries.Insert(entry);
		fUsedEntries.Add(entry);
		fEntryCount++;

		return B_OK;
	}

	status_t Remove(ino_t dirID, const char* name)
	{
		MutexLocker _(fLock);

		EntryCacheEntry* entry = fEntries.Lookup(EntryCacheKey(dirID, name));
		if (entry == NULL)
			return B_ENTRY_NOT_FOUND;

		_Remove(entry);

		return B_OK;
	}

	bool Lookup(ino_t dirID, const char* name, ino_t& nodeID)
	{
		MutexLocker _(fLock);

		EntryCacheEntry* entry = fEntries.Lookup(EntryCacheKey(dirID, name));
		if (entry == NULL)
			return false;

		// requeue at the end
		fUsedEntries.Remove(entry);
		fUsedEntries.Add(entry);

		nodeID = entry->node_id;
		return true;
	}

	void _Remove(EntryCacheEntry* entry)
	{
		fEntries.Remove(entry);
		fUsedEntries.Remove(entry);
		free(entry);
		fEntryCount--;
	}

private:
	typedef BOpenHashTable<EntryCacheHashDefinition> EntryTable;
	typedef DoublyLinkedList<EntryCacheEntry> EntryList;

	mutex		fLock;
	EntryTable	fEntries;
	EntryList	fUsedEntries;	// LRU queue (LRU entry at the head)
	uint32		fEntryCount;
};


#endif	// ENTRY_CACHE_H
