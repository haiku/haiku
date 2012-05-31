/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef INODE_H
#define INODE_H


#include <sys/stat.h>

#include <SupportDefs.h>

#include "Filesystem.h"
#include "NFS4Defs.h"


class Inode {
public:
						Inode(Filesystem* fs, const Filehandle &fh);

	inline	ino_t		ID() const;
	inline	mode_t		Type() const;

			status_t	Stat(struct stat* st);

private:
			uint64		fFileId;
			uint32		fType;

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


inline mode_t
Inode::Type() const
{
	return sNFSFileTypeToHaiku[fType];
}


#endif	// INODE_H

