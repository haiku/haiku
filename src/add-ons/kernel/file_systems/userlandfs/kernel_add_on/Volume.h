/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_VOLUME_H
#define USERLAND_FS_VOLUME_H

#include <fs_interface.h>

#include <Referenceable.h>

#include <kernel/lock.h>

#include "FSCapabilities.h"

namespace UserlandFSUtil {

class Request;
class RequestAllocator;
class RequestHandler;
class RequestPort;
struct userlandfs_ioctl;

}

using UserlandFSUtil::FSVolumeCapabilities;
using UserlandFSUtil::Request;
using UserlandFSUtil::RequestAllocator;
using UserlandFSUtil::RequestHandler;
using UserlandFSUtil::RequestPort;
using UserlandFSUtil::userlandfs_ioctl;

class FileSystem;

class Volume : public Referenceable {
public:
								Volume(FileSystem* fileSystem,
									fs_volume* fsVolume);
								~Volume();

			FileSystem*			GetFileSystem() const;
	inline	dev_t				GetID() const;

	inline	bool				HasCapability(int capability) const;

			void*				GetUserlandVolume() const;
			ino_t				GetRootID() const;

			// client methods
			status_t			GetVNode(ino_t vnid, void** node);
			status_t			PutVNode(ino_t vnid);
			status_t			AcquireVNode(ino_t vnid);
			status_t			NewVNode(ino_t vnid, void* node);
			status_t			PublishVNode(ino_t vnid, void* node,
									int type, uint32 flags);
			status_t			RemoveVNode(ino_t vnid);
			status_t			UnremoveVNode(ino_t vnid);
			status_t			GetVNodeRemoved(ino_t vnid, bool* removed);

			status_t			CreateFileCache(ino_t vnodeID, off_t size);
			status_t			DeleteFileCache(ino_t vnodeID);
			status_t			SetFileCacheEnabled(ino_t vnodeID,
									bool enabled);
			status_t			SetFileCacheSize(ino_t vnodeID, off_t size);
			status_t			SyncFileCache(ino_t vnodeID);
			status_t			ReadFileCache(ino_t vnodeID, void* cookie,
									off_t offset, void* buffer, size_t* _size);
			status_t			WriteFileCache(ino_t vnodeID, void* cookie,
									off_t offset, const void *buffer,
									size_t *_size);

			// FS
			status_t			Mount(const char* device, uint32 flags,
									const char* parameters);
			status_t			Unmount();
			status_t			Sync();
			status_t			ReadFSInfo(fs_info* info);
			status_t			WriteFSInfo(const struct fs_info* info,
									uint32 mask);

			// vnodes
			status_t			Lookup(void* dir, const char* entryName,
									ino_t* vnid);
			status_t			GetVNodeName(void* node, char* buffer,
									size_t bufferSize);
			status_t			ReadVNode(ino_t vnid, bool reenter,
									void** node, int* type, uint32* flags);
			status_t			WriteVNode(void* node, bool reenter);
			status_t			RemoveVNode(void* node, bool reenter);

			// nodes
			status_t			IOCtl(void* node, void* cookie,
									uint32 command, void *buffer, size_t size);
			status_t			SetFlags(void* node, void* cookie,
									int flags);
			status_t			Select(void* node, void* cookie, uint8 event,
									selectsync* sync);
			status_t			Deselect(void* node, void* cookie, uint8 event,
									selectsync* sync);

			status_t			FSync(void* node);

			status_t			ReadSymlink(void* node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
			status_t			CreateSymlink(void* dir, const char* name,
									const char* target, int mode);

			status_t			Link(void* dir, const char* name,
									void* node);
			status_t			Unlink(void* dir, const char* name);
			status_t			Rename(void* oldDir, const char* oldName,
									void* newDir, const char* newName);

			status_t			Access(void* node, int mode);
			status_t			ReadStat(void* node, struct stat* st);
			status_t			WriteStat(void* node, const struct stat *st,
									uint32 mask);

			// files
			status_t			Create(void* dir, const char* name,
									int openMode, int mode, void** cookie,
									ino_t* vnid);
			status_t			Open(void* node, int openMode,
									void** cookie);
			status_t			Close(void* node, void* cookie);
			status_t			FreeCookie(void* node, void* cookie);
			status_t			Read(void* node, void* cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
			status_t			Write(void* node, void* cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);

			// directories
			status_t			CreateDir(void* dir, const char* name,
									int mode, ino_t *newDir);
			status_t			RemoveDir(void* dir, const char* name);
			status_t			OpenDir(void* node, void** cookie);
			status_t			CloseDir(void* node, void* cookie);
			status_t			FreeDirCookie(void* node, void* cookie);
			status_t			ReadDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
			status_t			RewindDir(void* node, void* cookie);

			// attribute directories
			status_t			OpenAttrDir(void* node, void** cookie);
			status_t			CloseAttrDir(void* node, void* cookie);
			status_t			FreeAttrDirCookie(void* node,
									void* cookie);
			status_t			ReadAttrDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
			status_t			RewindAttrDir(void* node, void* cookie);

			// attributes
			status_t			CreateAttr(void* node, const char* name,
									uint32 type, int openMode,
									void** cookie);
			status_t			OpenAttr(void* node, const char* name,
									int openMode, void** cookie);
			status_t			CloseAttr(void* node, void* cookie);
			status_t			FreeAttrCookie(void* node, void* cookie);
			status_t			ReadAttr(void* node, void* cookie,
									off_t pos, void* buffer, size_t bufferSize,
									size_t* bytesRead);
			status_t			WriteAttr(void* node, void* cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
			status_t			ReadAttrStat(void* node, void* cookie,
									struct stat *st);
			status_t			WriteAttrStat(void* node, void* cookie,
									const struct stat *st, int statMask);
			status_t			RenameAttr(void* oldNode,
									const char* oldName, void* newNode,
									const char* newName);
			status_t			RemoveAttr(void* node, const char* name);

			// indices
			status_t			OpenIndexDir(void** cookie);
			status_t			CloseIndexDir(void* cookie);
			status_t			FreeIndexDirCookie(void* cookie);
			status_t			ReadIndexDir(void* cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);
			status_t			RewindIndexDir(void* cookie);
			status_t			CreateIndex(const char* name, uint32 type,
									uint32 flags);
			status_t			RemoveIndex(const char* name);
			status_t			ReadIndexStat(const char *name,
									struct stat *st);

			// queries
			status_t			OpenQuery(const char* queryString,
									uint32 flags, port_id port, uint32 token,
									void** cookie);
			status_t			CloseQuery(void* cookie);
			status_t			FreeQueryCookie(void* cookie);
			status_t			ReadQuery(void* cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);
			status_t			RewindQuery(void* cookie);

private:
			struct VNode;
			struct VNodeHashDefinition;
			struct VNodeMap;

			class AutoIncrementer;

private:
			status_t			_Mount(const char* device, uint32 flags,
									const char* parameters);
			status_t			_Unmount();
			status_t			_ReadFSInfo(fs_info* info);
			status_t			_Lookup(void* dir, const char* entryName,
									ino_t* vnid);
			status_t			_WriteVNode(void* node, bool reenter);
			status_t			_ReadStat(void* node, struct stat* st);
			status_t			_Close(void* node, void* cookie);
			status_t			_FreeCookie(void* node, void* cookie);
			status_t			_CloseDir(void* node, void* cookie);
			status_t			_FreeDirCookie(void* node, void* cookie);
			status_t			_CloseAttrDir(void* node, void* cookie);
			status_t			_FreeAttrDirCookie(void* node,
									void* cookie);
			status_t			_CloseAttr(void* node, void* cookie);
			status_t			_FreeAttrCookie(void* node,
									void* cookie);
			status_t			_CloseIndexDir(void* cookie);
			status_t			_FreeIndexDirCookie(void* cookie);
			status_t			_CloseQuery(void* cookie);
			status_t			_FreeQueryCookie(void* cookie);

			status_t			_SendRequest(RequestPort* port,
									RequestAllocator* allocator,
									RequestHandler* handler, Request** reply);
			status_t			_SendReceiptAck(RequestPort* port);

			void				_IncrementVNodeCount(ino_t vnid);
			void				_DecrementVNodeCount(ino_t vnid);

			status_t			_InternalIOCtl(userlandfs_ioctl* buffer,
									int32 bufferSize);

			status_t			_PutAllPendingVNodes();

	inline	bool				HasVNodeCapability(VNode* vnode,
									int capability) const;

private:
			mutex				fLock;
			FileSystem*			fFileSystem;
			fs_volume*			fFSVolume;
			FSVolumeCapabilities fCapabilities;
			void*				fUserlandVolume;
			ino_t				fRootID;
			VNode*				fRootNode;
			vint32				fOpenFiles;
			vint32				fOpenDirectories;
			vint32				fOpenAttributeDirectories;
			vint32				fOpenAttributes;
			vint32				fOpenIndexDirectories;
			vint32				fOpenQueries;
			VNodeMap*			fVNodes;
	volatile bool				fVNodeCountingEnabled;
};


// GetID
inline dev_t
Volume::GetID() const
{
	return fFSVolume->id;
}


inline bool
Volume::HasCapability(int capability) const
{
	return fCapabilities.Get(capability);
}


inline bool
Volume::HasVNodeCapability(VNode* vnode, int capability) const
{
	// TODO: Implement for real!
	return true;
}


#endif	// USERLAND_FS_VOLUME_H
