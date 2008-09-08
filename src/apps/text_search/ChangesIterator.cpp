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


//#define TRACE_CHANGES_ITERATOR
#ifdef TRACE_CHANGES_ITERATOR
# define TRACE(x...) printf(x)
#else
# define TRACE(x...)
#endif


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
	while (index < fIteratorIndex && iterator.HasNext()) {
		iterator.Next();
		index++;
	}

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

	TRACE("added: %s\n", path);

	fPathMap.Put(key, ENTRY_ADDED);
}


void
ChangesIterator::EntryRemoved(const char* path)
{
	HashString key(path);
	if (fPathMap.ContainsKey(key)) {
		TRACE("ignoring: %s\n", path);
		fPathMap.Remove(key);
	}
}


void
ChangesIterator::EntryChanged(const char* path)
{
	HashString key(path);
	if (fPathMap.ContainsKey(key) && fPathMap.Get(key) == ENTRY_ADDED)
		return;

	TRACE("changed: %s\n", path);

	fPathMap.Put(key, ENTRY_CHANGED);
}


bool
ChangesIterator::IsEmpty() const
{
	PathMap::Iterator iterator = fPathMap.GetIterator();
	return !iterator.HasNext();
}

void
ChangesIterator::PrintToStream() const
{
	printf("ChangesIterator contents:\n");
	PathMap::Iterator iterator = fPathMap.GetIterator();
	while (iterator.HasNext()) {
		const PathMap::Entry& entry = iterator.Next();
		const char* value;
		switch (entry.value) {
			case ENTRY_ADDED:
				value = "ADDED";
				break;
			case ENTRY_REMOVED:
				value = "REMOVED";
				break;
			case ENTRY_CHANGED:
				value = "CHANGED";
				break;
			default:
				value = "???";
				break;
		}
		printf("entry: %s - %s\n", entry.key.GetString(), value);
	}
}

