/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef NAME_INDEX_H
#define NAME_INDEX_H

#include "EntryListener.h"
#include "Index.h"
#include "TwoKeyAVLTree.h"

class NameIndexEntryIterator;

// NameIndex
class NameIndex : public Index, private EntryListener {
public:
	NameIndex(Volume *volume);
	virtual ~NameIndex();

	virtual int32 CountEntries() const;

	virtual status_t Changed(Entry *entry, const char *oldName);

private:
	virtual void EntryAdded(Entry *entry);
	virtual void EntryRemoved(Entry *entry);

protected:
	virtual AbstractIndexEntryIterator *InternalGetIterator();
	virtual AbstractIndexEntryIterator *InternalFind(const uint8 *key,
													 size_t length);

private:
	class EntryTree;
	friend class NameIndexEntryIterator;

	void _UpdateLiveQueries(Entry* entry, const char* oldName,
		const char* newName);

private:
	EntryTree	*fEntries;
};

#endif	// NAME_INDEX_H
