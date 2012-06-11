/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef INODEIDMAP_H
#define INODEIDMAP_H


#include <SupportDefs.h>
#include <util/AVLTreeMap.h>

#include "Filehandle.h"


class InodeIdMap {
public:
	inline	status_t						AddEntry(const Filehandle& fh,
												const Filehandle& parent,
												const char* name, uint64 fileId,
												ino_t id);
	inline	status_t						GetFileInfo(FileInfo* fi, ino_t id);

private:
			AVLTreeMap<ino_t, FileInfo>	fMap;

};


inline status_t
InodeIdMap::AddEntry(const Filehandle& fh, const Filehandle& parent,
	const char* name, uint64 fileId, ino_t id)
{
	FileInfo fi;
	fi.fFileId = fileId;
	fi.fFH = fh;
	fi.fParent = parent;
	fi.fName = strdup(name);
	return fMap.Insert(id, fi);
}


inline status_t
InodeIdMap::GetFileInfo(FileInfo* fi, ino_t id)
{
	AVLTreeMap<ino_t, FileInfo>::Iterator it = fMap.Find(id);
	if (!it.HasCurrent())
		return B_ENTRY_NOT_FOUND;

	*fi = it.Current();
	return B_OK;
}


#endif	// INODEIDMAP_H

