/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef INODE_H
#define INODE_H


#include <SupportDefs.h>

#include "Filesystem.h"
#include "NFS4Defs.h"


class Inode {
public:
						Inode(Filesystem* fs, const Filehandle &fh);

	inline	ino_t		ID() const;

private:
			uint64		fFileId;

			Filehandle	fHandle;
			Filesystem*	fFilesystem;
};


inline ino_t
Inode::ID() const
{
	if (sizeof(ino_t) >= sizeof(uint64))
		return fFileId;
	else
		return (ino_t)fFileId ^ (fFileId >>
					(sizeof(uint64) - sizeof(ino_t)) * 8);
}


#endif	// INODE_H

