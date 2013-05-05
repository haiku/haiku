/*
 * Copyright 2012,2013 Haiku, Inc. All rights reserved.
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

			status_t						AddName(FileInfo& fileInfo,
												InodeNames* parent,
												const char* name, ino_t id);
			bool							RemoveName(ino_t id,
												InodeNames* parent,
												const char* name);
			status_t						RemoveEntry(ino_t id);
			status_t						GetFileInfo(FileInfo* fileInfo,
												ino_t id);

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


#endif	// INODEIDMAP_H

