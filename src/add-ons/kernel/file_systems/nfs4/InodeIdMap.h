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
	inline	status_t						AddEntry(const Filehandle& inode,
												ino_t id);
	inline	status_t						GetFilehandle(Filehandle* fh,
												ino_t id);

private:
			AVLTreeMap<ino_t, Filehandle>	fMap;

};


inline status_t
InodeIdMap::AddEntry(const Filehandle& inode, ino_t id)
{
	return fMap.Insert(id, inode);
}


inline status_t
InodeIdMap::GetFilehandle(Filehandle* fh, ino_t id)
{
	AVLTreeMap<ino_t, Filehandle>::Iterator it = fMap.Find(id);
	if (!it.HasCurrent())
		return B_BAD_VALUE;

	*fh = it.Current();
	return B_OK;
}


#endif	// INODEIDMAP_H

