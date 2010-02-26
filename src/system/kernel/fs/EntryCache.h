/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENTRY_CACHE_H
#define ENTRY_CACHE_H


#include <stdlib.h>

#include <util/AutoLock.h>
#include <util/khash.h>
#include <util/OpenHashTable.h>


struct EntryCacheKey {
	EntryCacheKey(ino_t dirID, const char* name)
		:
		dir_id(dirID),
		name(name)
	{
		hash = (uint32)dir_id ^ (uint32)(dir_id >> 32) ^ hash_hash_string(name);
			// We cache the hash value, so we can easily compute it before
			// holding any locks.
	}

	ino_t		dir_id;
	const char*	name;
	size_t		hash;
};


struct EntryCacheEntry {
			EntryCacheEntry*	hash_link;
			ino_t				node_id;
			ino_t				dir_id;
			vint32				generation;
			vint32				index;
			char				name[1];
};


struct EntryCacheGeneration {
			vint32				next_index;
			EntryCacheEntry**	entries;

								EntryCacheGeneration();
								~EntryCacheGeneration();

			status_t			Init();
};


struct EntryCacheHashDefinition {
	typedef EntryCacheKey	KeyType;
	typedef EntryCacheEntry	ValueType;

	uint32 HashKey(const EntryCacheKey& key) const
	{
		return key.hash;
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
								EntryCache();
								~EntryCache();

			status_t			Init();

			status_t			Add(ino_t dirID, const char* name,
									ino_t nodeID);

			status_t			Remove(ino_t dirID, const char* name);

			bool				Lookup(ino_t dirID, const char* name,
									ino_t& nodeID);

			const char*			DebugReverseLookup(ino_t nodeID, ino_t& _dirID);

private:
	static	const int32			kGenerationCount = 8;

			typedef BOpenHashTable<EntryCacheHashDefinition> EntryTable;
			typedef DoublyLinkedList<EntryCacheEntry> EntryList;

private:
			void				_AddEntryToCurrentGeneration(
									EntryCacheEntry* entry);

private:
			rw_lock				fLock;
			EntryTable			fEntries;
			EntryCacheGeneration fGenerations[kGenerationCount];
			int32				fCurrentGeneration;
};


#endif	// ENTRY_CACHE_H
