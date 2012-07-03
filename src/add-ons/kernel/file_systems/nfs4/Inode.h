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

#include "Cookie.h"
#include "Filesystem.h"
#include "NFS4Defs.h"
#include "ReplyInterpreter.h"


class Inode {
public:
	static			status_t	CreateInode(Filesystem* fs, const FileInfo& fi,
									Inode** inode);
								~Inode();

	inline			ino_t		ID() const;
	inline			mode_t		Type() const;
	inline			const char*	Name() const;
	inline			Filesystem*	FileSystem() const;

					status_t	LookUp(const char* name, ino_t* id);

					status_t	CreateLink(const char* name, const char* path,
									int mode);
					status_t	ReadLink(void* buffer, size_t* length);

					status_t	Link(Inode* dir, const char* name);
					status_t	Remove(const char* name, FileType type);
	static			status_t	Rename(Inode* from, Inode* to,
									const char* fromName, const char* toName);

					status_t	Access(int mode);
					status_t	Stat(struct stat* st);
					status_t	WriteStat(const struct stat* st, uint32 mask);

					status_t	Create(const char* name, int mode, int perms,
									OpenFileCookie* cookie, ino_t* id);
					status_t	Open(int mode, OpenFileCookie* cookie);
					status_t	Close(OpenFileCookie* cookie);
					status_t	Read(OpenFileCookie* cookie, off_t pos,
									void* buffer, size_t* length);
					status_t	Write(OpenFileCookie* cookie, off_t pos,
									const void* buffer, size_t *_length);

					status_t	CreateDir(const char* name, int mode);
					status_t	OpenDir(OpenDirCookie* cookie);
					status_t	ReadDir(void* buffer, uint32 size,
									uint32* count, OpenDirCookie* cookie);

					status_t	TestLock(OpenFileCookie* cookie,
									struct flock* lock);
					status_t	AcquireLock(OpenFileCookie* cookie,
									const struct flock* lock, bool wait);
					status_t	ReleaseLock(OpenFileCookie* cookie,
									const struct flock* lock);
					status_t	ReleaseAllLocks(OpenFileCookie* cookie);


protected:
								Inode();

					bool		_HandleErrors(uint32 nfs4Error,
									RPC::Server* serv,
									OpenFileCookie* cookie = NULL);

					status_t	_ConfirmOpen(const Filehandle& fh,
									OpenFileCookie* cookie);

					status_t	_ReadDirOnce(DirEntry** dirents, uint32* count,
									OpenDirCookie* cookie, bool* eof);
					status_t	_FillDirEntry(struct dirent* de, ino_t id,
									const char* name, uint32 pos, uint32 size);
					status_t	_ReadDirUp(struct dirent* de, uint32 pos,
									uint32 size);

	static inline	status_t	_CheckLockType(short ltype, uint32 mode);

	static inline	ino_t		_FileIdToInoT(uint64 fileid);

					uint32		fType;

					FileInfo	fInfo;
					Filesystem*	fFilesystem;
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
	return _FileIdToInoT(fInfo.fFileId);
}


inline mode_t
Inode::Type() const
{
	return sNFSFileTypeToHaiku[fType];
}


inline const char*
Inode::Name() const
{
	return fInfo.fName;
}


inline Filesystem*
Inode::FileSystem() const
{
	return fFilesystem;
}


#endif	// INODE_H

