/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4INODE_H
#define NFS4INODE_H


#include <sys/stat.h>

#include <SupportDefs.h>

#include "NFS4Object.h"
#include "ReplyInterpreter.h"


class NFS4Inode : protected NFS4Object {
public:
			status_t	GetChangeInfo(uint64* change);
			status_t	ReadLink(void* buffer, size_t* length);

protected:
			status_t	Access(uint32* allowed);

			status_t	CommitWrites();

			status_t	LookUp(const char* name, uint64* change, uint64* fileID,
							FileHandle* handle);

			status_t	Link(Inode* dir, const char* name,
							ChangeInfo* changeInfo);

	static	status_t	Rename(Inode* from, Inode* to, const char* fromName,
							const char* toName, ChangeInfo* fromChange,
							ChangeInfo* toChange, uint64* fileID);

			status_t	GetStat(AttrValue** values, uint32* count);
			status_t	WriteStat(OpenFileCookie* cookie, AttrValue* attrs,
							uint32 attrCount);

			status_t	CreateFile(const char* name, int mode, int perms,
							OpenFileCookie* cookie, ChangeInfo* changeInfo,
							uint64* fileID, FileHandle* handle);
			status_t	OpenFile(OpenFileCookie* cookie, int mode);
			status_t	CloseFile(OpenFileCookie* cookie);

			status_t	ReadFile(OpenFileCookie* cookie, uint64 position,
							uint32* length, void* buffer, bool* eof);
			status_t	WriteFile(OpenFileCookie* cookie, uint64 position,
							uint32* length, const void* buffer);

			status_t	CreateObject(const char* name, const char* path,
							int mode, FileType type, ChangeInfo* changeInfo,
							uint64* fileID, FileHandle* handle);
			status_t	RemoveObject(const char* name, FileType type,
							ChangeInfo* changeInfo);

			status_t	ReadDirOnce(DirEntry** dirents, uint32* count,
							OpenDirCookie* cookie, bool* eof, uint64* change,
							uint64* dirCookie, uint64* dirCookieVerf);

			status_t	TestLock(OpenFileCookie* cookie, LockType* type,
							uint64* position, uint64* length, bool& conflict);
			status_t	AcquireLock(OpenFileCookie* cookie, LockInfo* lockInfo,
							bool wait);
			status_t	ReleaseLock(OpenFileCookie* cookie, LockInfo* lockInfo);

			status_t	ConfirmOpen(const FileHandle& fileHandle,
							OpenFileCookie* cookie);

};


#endif	// NFS4INODE_H

