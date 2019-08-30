/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <TypeConstants.h>

#include "DebugSupport.h"
#include "Entry.h"
#include "IndexImpl.h"
#include "NameIndex.h"
#include "ramfs.h"
#include "Volume.h"

// NameIndexPrimaryKey
class NameIndexPrimaryKey {
public:
	NameIndexPrimaryKey(const Entry *entry,
						const char *name = NULL)
		: entry(entry), name(name ? name : entry->GetName()) {}
	NameIndexPrimaryKey(const char *name)
		: entry(NULL), name(name) {}

	const Entry	*entry;
	const char	*name;
};

// NameIndexGetPrimaryKey
class NameIndexGetPrimaryKey {
public:
	inline NameIndexPrimaryKey operator()(const Entry *a)
	{
		return NameIndexPrimaryKey(a);
	}

	inline NameIndexPrimaryKey operator()(const Entry *a) const
	{
		return NameIndexPrimaryKey(a);
	}
};


// NameIndexPrimaryKeyCompare
class NameIndexPrimaryKeyCompare
{
public:
	inline int operator()(const NameIndexPrimaryKey &a,
						  const NameIndexPrimaryKey &b) const
	{
		if (a.entry != NULL && a.entry == b.entry)
			return 0;
		return strcmp(a.name, b.name);
	}
};


// EntryTree

typedef TwoKeyAVLTree<Entry*, NameIndexPrimaryKey, NameIndexPrimaryKeyCompare,
					  NameIndexGetPrimaryKey>
		_EntryTree;

class NameIndex::EntryTree : public _EntryTree {};


// NameIndexEntryIterator
class NameIndexEntryIterator : public AbstractIndexEntryIterator,
	public EntryListener {
public:
	NameIndexEntryIterator();
	virtual ~NameIndexEntryIterator();

	virtual Entry *GetCurrent();
	virtual Entry *GetCurrent(uint8 *buffer, size_t *keyLength);
	virtual Entry *GetPrevious();
	virtual Entry *GetNext();

	virtual status_t Suspend();
	virtual status_t Resume();

	bool SetTo(NameIndex *index, const char *name, bool ignoreValue = false);

	virtual void EntryRemoved(Entry *entry);

private:
	friend class NameIndex;

	typedef AbstractIndexEntryIterator BaseClass;

private:
	NameIndex						*fIndex;
	NameIndex::EntryTree::Iterator	fIterator;
	bool							fSuspended;
	bool							fIsNext;
};


// NameIndex

// constructor
NameIndex::NameIndex(Volume *volume)
	: Index(volume, "name", B_STRING_TYPE, false),
	  fEntries(new(nothrow) EntryTree)
{
	if (fInitStatus == B_OK && !fEntries)
		fInitStatus = B_NO_MEMORY;
	if (fInitStatus == B_OK) {
		fInitStatus = fVolume->AddEntryListener(this,
			NULL, ENTRY_LISTEN_ANY_ENTRY | ENTRY_LISTEN_ALL);
	}
}

// destructor
NameIndex::~NameIndex()
{
	if (fVolume)
		fVolume->RemoveEntryListener(this, NULL);
	if (fEntries)
		delete fEntries;
	// Actually we would need to maintain a list of iterators and unset the
	// still existing iterators here. But since the name index is deleted
	// when the volume is unmounted, there shouldn't be any iterators left
	// anymore.
}

// CountEntries
int32
NameIndex::CountEntries() const
{
	return fEntries->CountItems();
}

// Changed
status_t
NameIndex::Changed(Entry *entry, const char *oldName)
{
	status_t error = B_BAD_VALUE;
	if (entry && oldName) {
		EntryTree::Iterator it;
		Entry **foundEntry
			= fEntries->Find(NameIndexPrimaryKey(entry, oldName), entry, &it);
		if (foundEntry && *foundEntry == entry) {
			it.Remove();
			error = fEntries->Insert(entry);

			// udpate live queries
			_UpdateLiveQueries(entry, oldName, entry->GetName());
		}
	}
	return error;
}

// EntryAdded
void
NameIndex::EntryAdded(Entry *entry)
{
	if (entry) {
		fEntries->Insert(entry);

		// udpate live queries
		_UpdateLiveQueries(entry, NULL, entry->GetName());
	}
}

// EntryRemoved
void
NameIndex::EntryRemoved(Entry *entry)
{
	if (entry) {
		fEntries->Remove(entry, entry);

		// udpate live queries
		_UpdateLiveQueries(entry, entry->GetName(), NULL);
	}
}

// InternalGetIterator
AbstractIndexEntryIterator *
NameIndex::InternalGetIterator()
{
	NameIndexEntryIterator *iterator = new(nothrow) NameIndexEntryIterator;
	if (iterator) {
		if (!iterator->SetTo(this, NULL, true)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// InternalFind
AbstractIndexEntryIterator *
NameIndex::InternalFind(const uint8 *key, size_t length)
{
	if (!key || length == 0)
		return NULL;

	// if the key is not null-terminated, copy it
	uint8 clonedKey[kMaxIndexKeyLength];
	if (key[length - 1] != '\0') {
		if (length >= kMaxIndexKeyLength)
			length = kMaxIndexKeyLength - 1;

		memcpy(clonedKey, key, length);
		clonedKey[length] = '\0';
		length++;
		key = clonedKey;
	}

	NameIndexEntryIterator *iterator = new(nothrow) NameIndexEntryIterator;
	if (iterator) {
		if (!iterator->SetTo(this, (const char *)key)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// _UpdateLiveQueries
void
NameIndex::_UpdateLiveQueries(Entry* entry, const char* oldName,
	const char* newName)
{
	fVolume->UpdateLiveQueries(entry, entry->GetNode(), GetName(),
		GetType(), (const uint8*)oldName, (oldName ? strlen(oldName) : 0),
		(const uint8*)newName, (newName ? strlen(newName) : 0));
}


// NameIndexEntryIterator

// constructor
NameIndexEntryIterator::NameIndexEntryIterator()
	: AbstractIndexEntryIterator(),
	  fIndex(NULL),
	  fIterator(),
	  fSuspended(false),
	  fIsNext(false)
{
}

// destructor
NameIndexEntryIterator::~NameIndexEntryIterator()
{
	SetTo(NULL, NULL);
}

// GetCurrent
Entry *
NameIndexEntryIterator::GetCurrent()
{
	return (fIndex && fIterator.GetCurrent() ? *fIterator.GetCurrent() : NULL);
}

// GetCurrent
Entry *
NameIndexEntryIterator::GetCurrent(uint8 *buffer, size_t *keyLength)
{
	Entry *entry = GetCurrent();
	if (entry) {
		strncpy((char*)buffer, entry->GetName(), kMaxIndexKeyLength);
		*keyLength = strlen(entry->GetName());
	}
	return entry;
}

// GetPrevious
Entry *
NameIndexEntryIterator::GetPrevious()
{
	if (fSuspended)
		return NULL;
	if (!(fIterator.GetCurrent() && fIsNext))
		fIterator.GetPrevious();
	fIsNext = false;
	return (fIndex && fIterator.GetCurrent() ? *fIterator.GetCurrent() : NULL);
}

// GetNext
Entry *
NameIndexEntryIterator::GetNext()
{
	if (fSuspended)
		return NULL;
	if (!(fIterator.GetCurrent() && fIsNext))
		fIterator.GetNext();
	fIsNext = false;
	return (fIndex && fIterator.GetCurrent() ? *fIterator.GetCurrent() : NULL);
}

// Suspend
status_t
NameIndexEntryIterator::Suspend()
{
	status_t error = (!fSuspended ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (fIterator.GetCurrent()) {
			error = fIndex->GetVolume()->AddEntryListener(this,
				*fIterator.GetCurrent(), ENTRY_LISTEN_REMOVED);
		}
		if (error == B_OK)
			fSuspended = true;
	}
	return error;
}

// Resume
status_t
NameIndexEntryIterator::Resume()
{
	status_t error = (fSuspended ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (fIterator.GetCurrent()) {
			error = fIndex->GetVolume()->RemoveEntryListener(this,
				*fIterator.GetCurrent());
		}
		if (error == B_OK)
			fSuspended = false;
	}
	return error;
}

// SetTo
bool
NameIndexEntryIterator::SetTo(NameIndex *index, const char *name,
							  bool ignoreValue)
{
	Resume();
	fIndex = index;
	fSuspended = false;
	fIsNext = false;
	if (fIndex) {
		if (ignoreValue) {
			fIndex->fEntries->GetIterator(&fIterator);
			return fIterator.GetCurrent();
		}
		return fIndex->fEntries->FindFirst(name, &fIterator);
	}
	return false;
}

// EntryRemoved
void
NameIndexEntryIterator::EntryRemoved(Entry */*entry*/)
{
	Resume();
	fIsNext = GetNext();
	Suspend();
}

