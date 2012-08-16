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


class InodeIdMap {
public:
	inline									InodeIdMap();
	inline									~InodeIdMap();

	inline	status_t						AddEntry(const FileInfo& fi,
												ino_t id, bool weak = false);
	inline	status_t						RemoveEntry(ino_t id);
	inline	status_t						GetFileInfo(FileInfo* fi, ino_t id);

private:
			AVLTreeMap<ino_t, FileInfo>		fMap;
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
	MutexLocker _(fLock);
	//if (weak)
		fMap.Remove(id);
	return fMap.Insert(id, fi);
}


inline status_t
InodeIdMap::RemoveEntry(ino_t id)
{
	MutexLocker _(fLock);
	return fMap.Remove(id);
}


inline status_t
InodeIdMap::GetFileInfo(FileInfo* fi, ino_t id)
{
	MutexLocker _(fLock);
	AVLTreeMap<ino_t, FileInfo>::Iterator it = fMap.Find(id);
	if (!it.HasCurrent())
		return B_ENTRY_NOT_FOUND;

	*fi = it.Current();
	return B_OK;
}


#endif	// INODEIDMAP_H

