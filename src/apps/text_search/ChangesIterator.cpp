/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ChangesIterator.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Directory.h>

#include "Model.h"

using std::nothrow;

ChangesIterator::ChangesIterator(const Model* model) 
	: FileIterator(),
	  fPathMap(),
	  fIteratorIndex(0),

	  fRecurseDirs(model->fRecurseDirs),
	  fRecurseLinks(model->fRecurseLinks),
	  fSkipDotDirs(model->fSkipDotDirs),
	  fTextOnly(model->fTextOnly)
{
}


ChangesIterator::~ChangesIterator()
{
}


bool
ChangesIterator::IsValid() const
{
	return fPathMap.InitCheck() == B_OK;
}


bool
ChangesIterator::GetNextName(char* buffer)
{
	// TODO: inefficient
	PathMap::Iterator iterator = fPathMap.GetIterator();
	int32 index = 0;
	while (index < fIteratorIndex && iterator.HasNext())
		iterator.Next();

	if (iterator.HasNext()) {
		const PathMap::Entry& entry = iterator.Next();
		sprintf(buffer, "%s", entry.key.GetString());

		fIteratorIndex++;
		return true;
	}

	return false;
}


bool
ChangesIterator::NotifyNegatives() const
{
	return true;
}


// #pragma mark -


void
ChangesIterator::EntryAdded(const char* path)
{
	HashString key(path);
	if (fPathMap.ContainsKey(key))
		return;

	fPathMap.Put(key, ENTRY_ADDED);
}


void
ChangesIterator::EntryRemoved(const char* path)
{
	HashString key(path);
	if (fPathMap.ContainsKey(key)) {
		uint32 mode = fPathMap.Get(key);
		if (mode == ENTRY_ADDED)
			fPathMap.Remove(key);
		else if (mode != ENTRY_REMOVED)
			fPathMap.Put(key, ENTRY_REMOVED);
	} else
		fPathMap.Put(key, ENTRY_REMOVED);
}


void
ChangesIterator::EntryChanged(const char* path)
{
	HashString key(path);
	if (fPathMap.ContainsKey(key) && fPathMap.Get(key) == ENTRY_ADDED)
		return;

	fPathMap.Put(key, ENTRY_CHANGED);
}
