/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EntryCache.h"

#include <new>


static const int32 kEntriesPerGeneration = 1024;

static const int32 kEntryNotInArray = -1;
static const int32 kEntryRemoved = -2;


// #pragma mark - EntryCacheGeneration


EntryCacheGeneration::EntryCacheGeneration()
	:
	next_index(0),
	entries(NULL)
{
}


EntryCacheGeneration::~EntryCacheGeneration()
{
	delete[] entries;
}


status_t
EntryCacheGeneration::Init()
{
	entries = new(std::nothrow) EntryCacheEntry*[kEntriesPerGeneration];
	if (entries == NULL)
		return B_NO_MEMORY;

	memset(entries, 0, sizeof(EntryCacheEntry*) * kEntriesPerGeneration);

	return B_OK;
}


// #pragma mark - EntryCache


EntryCache::EntryCache()
	:
	fCurrentGeneration(0)
{
	rw_lock_init(&fLock, "entry cache");

	new(&fEntries) EntryTable;
}


EntryCache::~EntryCache()
{
	// delete entries
	EntryCacheEntry* entry = fEntries.Clear(true);
	while (entry != NULL) {
		EntryCacheEntry* next = entry->hash_link;
		free(entry);
		entry = next;
	}

	rw_lock_destroy(&fLock);
}


status_t
EntryCache::Init()
{
	status_t error = fEntries.Init();
	if (error != B_OK)
		return error;

	for (int32 i = 0; i < kGenerationCount; i++) {
		error = fGenerations[i].Init();
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
EntryCache::Add(ino_t dirID, const char* name, ino_t nodeID)
{
	EntryCacheKey key(dirID, name);

	WriteLocker _(fLock);

	EntryCacheEntry* entry = fEntries.Lookup(key);
	if (entry != NULL) {
		entry->node_id = nodeID;
		if (entry->generation != fCurrentGeneration) {
			if (entry->index >= 0) {
				fGenerations[entry->generation].entries[entry->index] = NULL;
				_AddEntryToCurrentGeneration(entry);
			}
		}
		return B_OK;
	}

	entry = (EntryCacheEntry*)malloc(sizeof(EntryCacheEntry) + strlen(name));
	if (entry == NULL)
		return B_NO_MEMORY;

	entry->node_id = nodeID;
	entry->dir_id = dirID;
	entry->generation = fCurrentGeneration;
	entry->index = kEntryNotInArray;
	strcpy(entry->name, name);

	fEntries.Insert(entry);

	_AddEntryToCurrentGeneration(entry);

	return B_OK;
}


status_t
EntryCache::Remove(ino_t dirID, const char* name)
{
	EntryCacheKey key(dirID, name);

	WriteLocker writeLocker(fLock);

	EntryCacheEntry* entry = fEntries.Lookup(key);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	fEntries.Remove(entry);

	if (entry->index >= 0) {
		// remove the entry from its generation and delete it
		fGenerations[entry->generation].entries[entry->index] = NULL;
		free(entry);
	} else {
		// We can't free it, since another thread is about to try to move it
		// to another generation. We mark it removed and the other thread will
		// take care of deleting it.
		entry->index = kEntryRemoved;
	}

	return B_OK;
}


bool
EntryCache::Lookup(ino_t dirID, const char* name, ino_t& _nodeID)
{
	EntryCacheKey key(dirID, name);

	ReadLocker readLocker(fLock);

	EntryCacheEntry* entry = fEntries.Lookup(key);
	if (entry == NULL)
		return false;

	int32 oldGeneration = atomic_set(&entry->generation, fCurrentGeneration);
	if (oldGeneration == fCurrentGeneration || entry->index < 0) {
		// The entry is already in the current generation or is being moved to
		// it by another thread.
		_nodeID = entry->node_id;
		return true;
	}

	// remove from old generation array
	fGenerations[oldGeneration].entries[entry->index] = NULL;
	entry->index = kEntryNotInArray;

	// add to the current generation
	int32 index = atomic_add(&fGenerations[oldGeneration].next_index, 1);
	if (index < kEntriesPerGeneration) {
		fGenerations[fCurrentGeneration].entries[index] = entry;
		entry->index = index;
		_nodeID = entry->node_id;
		return true;
	}

	// The current generation is full, so we probably need to clear the oldest
	// one to make room. We need the write lock for that.
	readLocker.Unlock();
	WriteLocker writeLocker(fLock);

	if (entry->index == kEntryRemoved) {
		// the entry has been removed in the meantime
		free(entry);
		return false;
	}

	_AddEntryToCurrentGeneration(entry);

	_nodeID = entry->node_id;
	return true;
}


const char*
EntryCache::DebugReverseLookup(ino_t nodeID, ino_t& _dirID)
{
	for (EntryTable::Iterator it = fEntries.GetIterator();
			EntryCacheEntry* entry = it.Next();) {
		if (nodeID == entry->node_id && strcmp(entry->name, ".") != 0
				&& strcmp(entry->name, "..") != 0) {
			_dirID = entry->dir_id;
			return entry->name;
		}
	}

	return NULL;
}


void
EntryCache::_AddEntryToCurrentGeneration(EntryCacheEntry* entry)
{
	// the generation might not be full yet
	int32 index = fGenerations[fCurrentGeneration].next_index++;
	if (index < kEntriesPerGeneration) {
		fGenerations[fCurrentGeneration].entries[index] = entry;
		entry->generation = fCurrentGeneration;
		entry->index = index;
		return;
	}

	// we have to clear the oldest generation
	int32 newGeneration = (fCurrentGeneration + 1) % kGenerationCount;
	for (int32 i = 0; i < kEntriesPerGeneration; i++) {
		EntryCacheEntry* otherEntry = fGenerations[newGeneration].entries[i];
		if (otherEntry == NULL)
			continue;

		fGenerations[newGeneration].entries[i] = NULL;
		fEntries.Remove(otherEntry);
		free(otherEntry);
	}

	// set the new generation and add the entry
	fCurrentGeneration = newGeneration;
	fGenerations[newGeneration].next_index = 1;
	fGenerations[newGeneration].entries[0] = entry;
	entry->generation = newGeneration;
	entry->index = 0;
}
