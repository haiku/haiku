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
#include "ReplyInterpreter.h"


class Inode {
public:
								Inode(Filesystem* fs, const Filehandle &fh,
									bool root = false);

	inline			ino_t		ID() const;
	inline			mode_t		Type() const;

					status_t	LookUp(const char* name, ino_t* id);
					status_t	Stat(struct stat* st);

					status_t	OpenDir(uint64* cookie);
					status_t	ReadDir(void* buffer, uint32 size,
									uint32* count, uint64* cookie);

private:
					status_t	_ReadDirOnce(DirEntry** dirents, uint32* count,
									uint64* cookie, bool* eof);
					status_t	_FillDirEntry(struct dirent* de, ino_t id,
									const char* name, uint32 pos, uint32 size);
					status_t	_ReadDirUp(struct dirent* de, uint32 pos,
									uint32 size);

	static inline	ino_t		_FileIdToInoT(uint64 fileid);

					uint64		fFileId;
					uint32		fType;

					Filehandle	fHandle;
					Filesystem*	fFilesystem;

					bool		fRoot;
};


inline ino_t
Inode::_FileIdToInoT(uint64 fileid)
{
	if (sizeof(ino_t) >= sizeof(uint64))
		return fileid;
	else
		return (ino_t)fileid ^ (fileid >>
					(sizeof(uint64) - sizeof(ino_t)) * 8);
}


inline ino_t
Inode::ID() const
{
	return _FileIdToInoT(fFileId);
}


inline mode_t
Inode::Type() const
{
	return sNFSFileTypeToHaiku[fType];
}


#endif	// INODE_H

