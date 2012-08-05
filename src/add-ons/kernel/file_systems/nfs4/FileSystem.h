/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H


#include "CacheRevalidator.h"
#include "InodeIdMap.h"
#include "NFS4Defs.h"
#include "NFS4Server.h"


class Inode;
class RootInode;

class FileSystem {
public:
	static	status_t			Mount(FileSystem** pfs, RPC::Server* serv,
									const char* path, dev_t id);
								~FileSystem();

			status_t			GetInode(ino_t id, Inode** inode);
	inline	RootInode*			Root();

			status_t			Migrate(const RPC::Server* serv);

			OpenFileCookie*		OpenFilesLock();
			void				OpenFilesUnlock();
	inline	uint32				OpenFilesCount();
			void				AddOpenFile(OpenFileCookie* cookie);
			void				RemoveOpenFile(OpenFileCookie* cookie);

	inline	CacheRevalidator&	Revalidator();

	inline	bool				IsAttrSupported(Attribute attr) const;
	inline	uint32				ExpireType() const;

	inline	RPC::Server*		Server();
	inline	NFS4Server*			NFSServer();

	inline	const char*			Path() const;
	inline	const FileSystemId&	FsId() const;

	inline	uint64				AllocFileId();

	inline	dev_t				DevId() const;
	inline	InodeIdMap*			InoIdMap();

	inline	uint64				OpenOwner() const;
	inline	uint32				OpenOwnerSequenceLock();
	inline	void				OpenOwnerSequenceUnlock(bool increment = true);

			FileSystem*			fNext;
			FileSystem*			fPrev;
private:
								FileSystem();

			CacheRevalidator	fCacheRevalidator;

			OpenFileCookie*		fOpenFiles;
			uint32				fOpenCount;
			mutex				fOpenLock;

			uint64				fOpenOwner;
			uint32				fOpenOwnerSequence;
			mutex				fOpenOwnerLock;

			uint32				fExpireType;
			uint32				fSupAttrs[2];

			FileSystemId		fFsId;
			const char*			fPath;

			RootInode*			fRoot;

			RPC::Server*		fServer;

			vint64				fId;
			dev_t				fDevId;

			InodeIdMap			fInoIdMap;
};


inline CacheRevalidator&
FileSystem::Revalidator()
{
	return fCacheRevalidator;
}


inline RootInode*
FileSystem::Root()
{
	return fRoot;
}


inline uint32
FileSystem::OpenFilesCount()
{
	return fOpenCount;
}


inline bool
FileSystem::IsAttrSupported(Attribute attr) const
{
	return sIsAttrSet(attr, fSupAttrs, 2);
}


inline uint32
FileSystem::ExpireType() const
{
	return fExpireType;
}


inline RPC::Server*
FileSystem::Server()
{
	return fServer;
}


inline NFS4Server*
FileSystem::NFSServer()
{
	return reinterpret_cast<NFS4Server*>(fServer->PrivateData());
}


inline const char*
FileSystem::Path() const
{
	return fPath;
}


inline const FileSystemId&
FileSystem::FsId() const
{
	return fFsId;
}


inline uint64
FileSystem::AllocFileId()
{
	return atomic_add64(&fId, 1);
}


inline dev_t
FileSystem::DevId() const
{
	return fDevId;
}


inline InodeIdMap*
FileSystem::InoIdMap()
{
	return &fInoIdMap;
}


inline uint64
FileSystem::OpenOwner() const
{
	return fOpenOwner;
}


inline uint32
FileSystem::OpenOwnerSequenceLock()
{
	mutex_lock(&fOpenOwnerLock);
	return fOpenOwnerSequence;
}


inline void
FileSystem::OpenOwnerSequenceUnlock(bool increment = true)
{
	if (increment)
		fOpenOwnerSequence++;
	mutex_unlock(&fOpenOwnerLock);
}


#endif	// FILESYSTEM_H

