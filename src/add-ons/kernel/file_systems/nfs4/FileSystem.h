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
#include "Delegation.h"
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

			DoublyLinkedList<OpenState>&	OpenFilesLock();
			void				OpenFilesUnlock();
	inline	uint32				OpenFilesCount();
			void				AddOpenFile(OpenState* state);
			void				RemoveOpenFile(OpenState* state);

			DoublyLinkedList<Delegation>&	DelegationsLock();
			void				DelegationsUnlock();
			void				AddDelegation(Delegation* delegation);
			void				RemoveDelegation(Delegation* delegation);
			Delegation*			GetDelegation(const FileHandle& handle);

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
	inline	void				OpenOwnerSequenceUnlock(uint32 sequence);

	inline	bool				NamedAttrs();
	inline	void				SetNamedAttrs(bool attrs);

			FileSystem*			fNext;
			FileSystem*			fPrev;
private:
								FileSystem();

			CacheRevalidator	fCacheRevalidator;

			mutex				fDelegationLock;
			DoublyLinkedList<Delegation>	fDelegationList;
			AVLTreeMap<FileHandle, Delegation*> fHandleToDelegation;

			DoublyLinkedList<OpenState>		fOpenFiles;
			uint32				fOpenCount;
			mutex				fOpenLock;

			uint64				fOpenOwner;
			uint32				fOpenOwnerSequence;
			mutex				fOpenOwnerLock;

			uint32				fExpireType;
			uint32				fSupAttrs[2];
			bool				fNamedAttrs;

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
FileSystem::OpenOwnerSequenceUnlock(uint32 sequence)
{
	fOpenOwnerSequence = sequence;
	mutex_unlock(&fOpenOwnerLock);
}


inline bool
FileSystem::NamedAttrs()
{
	return fNamedAttrs;
}


inline void
FileSystem::SetNamedAttrs(bool attrs)
{
	fNamedAttrs = attrs;
}


#endif	// FILESYSTEM_H

