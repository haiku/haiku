/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENTRY_CACHE_H
#define ENTRY_CACHE_H


#include <stdlib.h>

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <util/StringHash.h>


struct EntryCacheKey {
	EntryCacheKey(ino_t dirID, const char* name)
		:
		dir_id(dirID),
		name(name)
	{
		hash = Hash(dirID, name);
			// We cache the hash value, so we can easily compute it before
			// holding any locks.
	}

	static uint32 Hash(ino_t dirID, const char* name)
	{
		return (uint32)dirID ^ (uint32)(dirID >> 32) ^ hash_hash_string(name);
	}

	ino_t		dir_id;
	const char*	name;
	uint32		hash;
};


struct EntryCacheEntry {
	EntryCacheEntry*	hash_link;
	ino_t				node_id;
	ino_t				dir_id;
	uint32				hash;
	int32				generation;
	int32				index;
	bool				missing;
	char				name[1];
};


struct EntryCacheGeneration {
			int32				next_index;
			int32				entries_size;
			EntryCacheEntry**	entries;

								EntryCacheGeneration();
								~EntryCacheGeneration();

			status_t			Init(int32 entriesSize);
};


struct EntryCacheHashDefinition {
	typedef EntryCacheKey	KeyType;
	typedef EntryCacheEntry	ValueType;

	uint32 HashKey(const EntryCacheKey& key) const
	{
		return key.hash;
	}

	uint32 Hash(const EntryCacheEntry* value) const
	{
		return value->hash;
	}

	bool Compare(const EntryCacheKey& key, const EntryCacheEntry* value) const
	{
		if (key.hash != value->hash)
			return false;
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
								EntryCache();
								~EntryCache();

			status_t			Init();

			status_t			Add(ino_t dirID, const char* name,
									ino_t nodeID, bool missing);

			status_t			Remove(ino_t dirID, const char* name);

			bool				Lookup(ino_t dirID, const char* name,
									ino_t& nodeID, bool& missing);

			const char*			DebugReverseLookup(ino_t nodeID, ino_t& _dirID);

private:
			typedef BOpenHashTable<EntryCacheHashDefinition> EntryTable;
			typedef DoublyLinkedList<EntryCacheEntry> EntryList;

private:
			void				_AddEntryToCurrentGeneration(
									EntryCacheEntry* entry);

private:
			rw_lock				fLock;
			EntryTable			fEntries;
			int32				fGenerationCount;
			EntryCacheGeneration* fGenerations;
			int32				fCurrentGeneration;
};


#endif	// ENTRY_CACHE_H
