/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FUSE_VOLUME_H
#define USERLAND_FS_FUSE_VOLUME_H

#include <AutoLocker.h>
#include <RWLockManager.h>

#include "Locker.h"

#include "fuse_fs.h"
#include "FUSEEntry.h"

#include "../Volume.h"


namespace UserlandFS {

class FUSEFileSystem;


class FUSEVolume : public Volume {
public:
								FUSEVolume(FUSEFileSystem* fileSystem,
									dev_t id);
	virtual						~FUSEVolume();

			status_t			Init();

			void				SetFS(fuse_fs* fs)	{ fFS = fs; }

	// FS
	virtual	status_t			Mount(const char* device, uint32 flags,
									const char* parameters, ino_t* rootID);
	virtual	status_t			Unmount();
	virtual	status_t			Sync();
	virtual	status_t			ReadFSInfo(fs_info* info);

	// vnodes
	virtual	status_t			Lookup(void* dir, const char* entryName,
									ino_t* vnid);
	virtual	status_t			GetVNodeName(void* node, char* buffer,
									size_t bufferSize);
	virtual	status_t			ReadVNode(ino_t vnid, bool reenter,
									void** node, int* type, uint32* flags,
									FSVNodeCapabilities* _capabilities);
	virtual	status_t			WriteVNode(void* node, bool reenter);
	virtual	status_t			RemoveVNode(void* node, bool reenter);

	// nodes
	virtual	status_t			SetFlags(void* node, void* cookie,
									int flags);

	virtual	status_t			FSync(void* node);

	virtual	status_t			ReadSymlink(void* node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
	virtual	status_t			CreateSymlink(void* dir, const char* name,
									const char* target, int mode);

	virtual	status_t			Link(void* dir, const char* name,
									void* node);
	virtual	status_t			Unlink(void* dir, const char* name);
	virtual	status_t			Rename(void* oldDir, const char* oldName,
									void* newDir, const char* newName);

	virtual	status_t			Access(void* node, int mode);
	virtual	status_t			ReadStat(void* node, struct stat* st);
	virtual	status_t			WriteStat(void* node, const struct stat* st,
									uint32 mask);

	// files
	virtual	status_t			Create(void* dir, const char* name,
									int openMode, int mode, void** cookie,
									ino_t* vnid);
	virtual	status_t			Open(void* node, int openMode,
									void** cookie);
	virtual	status_t			Close(void* node, void* cookie);
	virtual	status_t			FreeCookie(void* node, void* cookie);
	virtual	status_t			Read(void* node, void* cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			Write(void* node, void* cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);

	// directories
	virtual	status_t			CreateDir(void* dir, const char* name,
									int mode);
	virtual	status_t			RemoveDir(void* dir, const char* name);
	virtual	status_t			OpenDir(void* node, void** cookie);
	virtual	status_t			CloseDir(void* node, void* cookie);
	virtual	status_t			FreeDirCookie(void* node, void* cookie);
	virtual	status_t			ReadDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
	virtual	status_t			RewindDir(void* node, void* cookie);

	// attribute directories
	virtual	status_t			OpenAttrDir(void* node, void** cookie);
	virtual	status_t			CloseAttrDir(void* node, void* cookie);
	virtual	status_t			FreeAttrDirCookie(void* node,
									void* cookie);
	virtual	status_t			ReadAttrDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
	virtual	status_t			RewindAttrDir(void* node, void* cookie);

	// attributes
	virtual	status_t			OpenAttr(void* node, const char* name,
									int openMode, void** cookie);
	virtual	status_t			CloseAttr(void* node, void* cookie);
	virtual	status_t			FreeAttrCookie(void* node, void* cookie);
	virtual	status_t			ReadAttr(void* node, void* cookie,
									off_t pos, void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			ReadAttrStat(void* node, void* cookie,
									struct stat* st);

private:
	struct DirEntryCache;
	struct DirCookie;
	struct FileCookie;
	struct AttrDirCookie;
	struct AttrCookie;
	struct ReadDirBuffer;
	struct LockIterator;
	struct RWLockableReadLocking;
	struct RWLockableWriteLocking;
	struct RWLockableReadLocker;
	struct RWLockableWriteLocker;
	struct NodeLocker;
	struct NodeReadLocker;
	struct NodeWriteLocker;
	struct MultiNodeLocker;

	friend struct LockIterator;
	friend struct RWLockableReadLocking;
	friend struct RWLockableWriteLocking;
	friend struct NodeLocker;
	friend struct MultiNodeLocker;

private:
	inline	FUSEFileSystem*		_FileSystem() const;

			ino_t				_GenerateNodeID();

			bool				_GetNodeID(FUSENode* dir, const char* entryName,
									ino_t* _nodeID);
			status_t			_GetNode(FUSENode* dir, const char* entryName,
									FUSENode** _node);
			status_t			_InternalGetNode(FUSENode* dir,
									const char* entryName, FUSENode** _node,
									AutoLocker<Locker>& locker);
			void				_PutNode(FUSENode* node);
			void				_PutNodes(FUSENode* const* nodes, int32 count);

			void				_RemoveEntry(FUSEEntry* entry);
			status_t			_RemoveEntry(FUSENode* dir, const char* name);
			status_t			_RenameEntry(FUSENode* oldDir,
									const char* oldName, FUSENode* newDir,
									const char* newName);

			status_t			_LockNodeChain(FUSENode* node, bool lockParent,
									bool writeLock);
			void				_UnlockNodeChain(FUSENode* node, bool parent,
									bool writeLock);
			void				_UnlockNodeChainInternal(FUSENode* node,
									bool writeLock, FUSENode* stopNode,
									FUSENode* stopBeforeNode);

			status_t			_LockNodeChains(FUSENode* node1,
									bool lockParent1, bool writeLock1,
									FUSENode* node2, bool lockParent2,
									bool writeLock2);
			status_t			_LockNodeChainsInternal(FUSENode* node1,
									bool lockParent1, bool writeLock1,
									FUSENode* node2, bool lockParent2,
									bool writeLock2, bool* _retry);
			void				_UnlockNodeChains(FUSENode* node1, bool parent1,
									bool writeLock1, FUSENode* node2,
									bool parent2, bool writeLock2);

			bool				_FindCommonAncestor(FUSENode* node1,
									FUSENode* node2, FUSENode** _commonAncestor,
									bool* _inverseLockingOrder);
			bool				_GetNodeAncestors(FUSENode* node,
									FUSENode** ancestors, uint32* _count);

			status_t			_BuildPath(FUSENode* dir, const char* entryName,
									char* path, size_t& pathLen);
			status_t			_BuildPath(FUSENode* node, char* path,
									size_t& pathLen);

	static	int					_AddReadDirEntry(void* buffer, const char* name,
									const struct stat* st, off_t offset);
	static	int					_AddReadDirEntryGetDir(fuse_dirh_t handle,
									const char* name, int type, ino_t nodeID);
			int					_AddReadDirEntry(ReadDirBuffer* buffer,
									const char* name, int type, ino_t nodeID,
									off_t offset);

private:
			RWLockManager		fLockManager;
			Locker				fLock;
			fuse_fs*			fFS;
			FUSEEntryTable		fEntries;
			FUSENodeTable		fNodes;
			FUSENode*			fRootNode;
			ino_t				fNextNodeID;
			bool				fUseNodeIDs;	// TODO: Actually read the
												// option!
			char				fName[B_OS_NAME_LENGTH];
};

}	// namespace UserlandFS

using UserlandFS::FUSEVolume;

#endif	// USERLAND_FS_FUSE_VOLUME_H
