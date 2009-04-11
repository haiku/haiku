/*
 * Copyright 2008 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "PathList.h"

#include <new>
#include <stdlib.h>
#include <string.h>


struct PathList::path_entry {
	path_entry(const char* _path)
		:
		ref_count(1)
	{
		path = strdup(_path);
	}

	~path_entry()
	{
		free((char*)path);
	}

	const char* path;
	int32 ref_count;
};


PathList::PathList()
	:
	fPaths(10, true)
{
}


PathList::~PathList()
{
}


bool
PathList::HasPath(const char* path, int32* _index) const
{
	for (int32 i = fPaths.CountItems(); i-- > 0;) {
		if (!strcmp(fPaths.ItemAt(i)->path, path)) {
			if (_index != NULL)
				*_index = i;
			return true;
		}
	}

	return false;
}


status_t
PathList::AddPath(const char* path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	int32 index;
	if (HasPath(path, &index)) {
		fPaths.ItemAt(index)->ref_count++;
		return B_OK;
	}

	path_entry* entry = new(std::nothrow) path_entry(path);
	if (entry == NULL || entry->path == NULL || !fPaths.AddItem(entry)) {
		delete entry;
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
PathList::RemovePath(const char* path)
{
	int32 index;
	if (!HasPath(path, &index))
		return B_ENTRY_NOT_FOUND;

	if (--fPaths.ItemAt(index)->ref_count == 0)
		delete fPaths.RemoveItemAt(index);

	return B_OK;
}


int32
PathList::CountPaths() const
{
	return fPaths.CountItems();
}


const char*
PathList::PathAt(int32 index) const
{
	path_entry* entry = fPaths.ItemAt(index);
	if (entry == NULL)
		return NULL;

	return entry->path;
}
