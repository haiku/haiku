/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef INODEIDMAP_H
#define INODEIDMAP_H


#include <lock.h>
#include <SupportDefs.h>
#include <util/AutoLock.h>
#include <util/AVLTreeMap.h>

#include "FileInfo.h"

struct InodeIdMapEntry {
	FileInfo	fFileInfo;
	bool		fRemoved;
};

class InodeIdMap {
public:
	inline									InodeIdMap();
	inline									~InodeIdMap();

	inline	status_t						AddEntry(const FileInfo& fi,
												ino_t id, bool weak = false);
	inline	status_t						MarkRemoved(ino_t id);
	inline	status_t						RemoveEntry(ino_t id);
	inline	status_t						GetFileInfo(FileInfo* fi, ino_t id);

protected:
	inline	bool							_IsEntryRemoved(ino_t id);

private:
			AVLTreeMap<ino_t, InodeIdMapEntry>	fMap;
			mutex							fLock;

};


inline
InodeIdMap::InodeIdMap()
{
	mutex_init(&fLock, NULL);
}


inline
InodeIdMap::~InodeIdMap()
{
	mutex_destroy(&fLock);
}


inline status_t
InodeIdMap::AddEntry(const FileInfo& fi, ino_t id, bool weak)
{
	InodeIdMapEntry entry;

	MutexLocker _(fLock);
	if (!weak || _IsEntryRemoved(id))
		fMap.Remove(id);

	entry.fFileInfo = fi;
	entry.fRemoved = false;

	return fMap.Insert(id, entry);
}


inline status_t
InodeIdMap::MarkRemoved(ino_t id)
{
	MutexLocker _(fLock);
	AVLTreeMap<ino_t, InodeIdMapEntry>::Iterator it = fMap.Find(id);
	if (!it.HasCurrent())
		return B_ENTRY_NOT_FOUND;

	it.CurrentValuePointer()->fRemoved = true;
	return B_OK;
}


inline status_t
InodeIdMap::RemoveEntry(ino_t id)
{
	MutexLocker _(fLock);
	if (_IsEntryRemoved(id))
		return fMap.Remove(id);
	return B_OK;
}


inline status_t
InodeIdMap::GetFileInfo(FileInfo* fi, ino_t id)
{
	ASSERT(fi != NULL);

	MutexLocker _(fLock);
	AVLTreeMap<ino_t, InodeIdMapEntry>::Iterator it = fMap.Find(id);
	if (!it.HasCurrent())
		return B_ENTRY_NOT_FOUND;

	*fi = it.Current().fFileInfo;
	return B_OK;
}


// Caller must hold fLock
inline bool
InodeIdMap::_IsEntryRemoved(ino_t id)
{
	AVLTreeMap<ino_t, InodeIdMapEntry>::Iterator it = fMap.Find(id);
	if (!it.HasCurrent())
		return true;

	return it.Current().fRemoved;
}


#endif	// INODEIDMAP_H

