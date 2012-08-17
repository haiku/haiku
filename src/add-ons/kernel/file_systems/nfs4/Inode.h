/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef INODE_H
#define INODE_H


#include "MetadataCache.h"
#include "NFS4Inode.h"
#include "OpenState.h"


class Delegation;

class Inode : public NFS4Inode {
public:
	static			status_t	CreateInode(FileSystem* fs, const FileInfo& fi,
									Inode** inode);
	virtual						~Inode();

	inline			ino_t		ID() const;
	inline			mode_t		Type() const;
	virtual			const char*	Name() const;
	inline			FileSystem*	GetFileSystem() const;

	inline			void		SetOpenState(OpenState* state);

	inline			void*		FileCache();
					status_t	RevalidateFileCache();

	inline			uint64		MaxFileSize();

	inline			uint64		Change();
	inline			bool		Dirty();

	inline			OpenState*	GetOpenState();

					void		SetDelegation(Delegation* delegation);
					void		RecallDelegation(bool truncate = false);
					void		RecallReadDelegation();

					status_t	LookUp(const char* name, ino_t* id);

					status_t	Access(int mode);

					status_t	Commit();
					status_t	SyncAndCommit(bool force = false);

					status_t	CreateObject(const char* name, const char* path,
									int mode, FileType type);

					status_t	CreateLink(const char* name, const char* path,
									int mode);

					status_t	Link(Inode* dir, const char* name);
					status_t	Remove(const char* name, FileType type,
									ino_t* id);
	static			status_t	Rename(Inode* from, Inode* to,
									const char* fromName, const char* toName,
									bool attribute = false);

					status_t	Stat(struct stat* st,
									OpenAttrCookie* attr = NULL);
					status_t	WriteStat(const struct stat* st, uint32 mask,
									OpenAttrCookie* attr = NULL);

					status_t	Create(const char* name, int mode, int perms,
									OpenFileCookie* cookie,
									OpenDelegationData* data, ino_t* id);
					status_t	Open(int mode, OpenFileCookie* cookie);
					status_t	Close(OpenFileCookie* cookie);

					status_t	OpenAttr(const char* name,  int mode,
									OpenAttrCookie* cookie, bool create,
									int32 type = 0);
					status_t	CloseAttr(OpenAttrCookie* cookie);

					status_t	Read(OpenFileCookie* cookie, off_t pos,
									void* buffer, size_t* length);
					status_t	Write(OpenFileCookie* cookie, off_t pos,
									const void* buffer, size_t *_length);

					status_t	ReadDirect(OpenStateCookie* cookie, off_t pos,
									void* buffer, size_t* length, bool* eof);
					status_t	WriteDirect(OpenStateCookie* cookie, off_t pos,
									const void* buffer, size_t *_length);

					status_t	CreateDir(const char* name, int mode);
					status_t	OpenDir(OpenDirCookie* cookie);
					status_t	ReadDir(void* buffer, uint32 size,
									uint32* count, OpenDirCookie* cookie);

					status_t	OpenAttrDir(OpenDirCookie* cookie);

					status_t	TestLock(OpenFileCookie* cookie,
									struct flock* lock);
					status_t	AcquireLock(OpenFileCookie* cookie,
									const struct flock* lock, bool wait);
					status_t	ReleaseLock(OpenFileCookie* cookie,
									const struct flock* lock);
					status_t	ReleaseAllLocks(OpenFileCookie* cookie);

					status_t	GetDirSnapshot(DirectoryCacheSnapshot**
									_snapshot, OpenDirCookie* cookie,
									uint64* _change, bool attribute);

					status_t	LoadAttrDirHandle();

	static inline	ino_t		FileIdToInoT(uint64 fileid);
protected:
								Inode();

					void		ReleaseOpenState();

					status_t	CreateState(const char* name, int mode,
									int perms, OpenState* state,
									OpenDelegationData* data);

					void		ReturnDelegation(bool truncate);

					status_t	ReadDirUp(struct dirent* de, uint32 pos,
									uint32 size);
					status_t	FillDirEntry(struct dirent* de, ino_t id,
									const char* name, uint32 pos, uint32 size);

					status_t	ChildAdded(const char* name, uint64 fileID,
									const FileHandle& fileHandle);

					status_t	GetStat(struct stat* st,
									OpenAttrCookie* attr = NULL);

					char*		AttrToFileName(const char* path);

	static inline	status_t	CheckLockType(short ltype, uint32 mode);

private:
					uint32		fType;

					MetadataCache	fMetaCache;
					DirectoryCache*	fCache;
					DirectoryCache*	fAttrCache;

					rw_lock		fDelegationLock;
					Delegation*	fDelegation;

					uint64		fChange;
					void*		fFileCache;
					mutex		fFileCacheLock;
					uint64		fMaxFileSize;

					OpenState*	fOpenState;
					mutex		fStateLock;

					bool		fWriteDirty;
};


inline ino_t
Inode::FileIdToInoT(uint64 fileid)
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
	return FileIdToInoT(fInfo.fFileId);
}


inline mode_t
Inode::Type() const
{
	return sNFSFileTypeToHaiku[fType];
}


inline FileSystem*
Inode::GetFileSystem() const
{
	return fFileSystem;
}


inline void*
Inode::FileCache()
{
	return fFileCache;
}


inline void
Inode::SetOpenState(OpenState* state)
{
	MutexLocker _(fStateLock);
	fOpenState = state;
}


inline uint64
Inode::MaxFileSize()
{
	return fMaxFileSize;
}


inline uint64
Inode::Change()
{
	return fChange;
}


inline bool
Inode::Dirty()
{
	return fWriteDirty;
}


inline OpenState*
Inode::GetOpenState()
{
	return fOpenState;
}


#endif	// INODE_H

