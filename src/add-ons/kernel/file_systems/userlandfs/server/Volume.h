/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_VOLUME_H
#define USERLAND_FS_VOLUME_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include <kernel/util/DoublyLinkedList.h>

#include "FSCapabilities.h"


namespace UserlandFS {

class FileSystem;
class IORequestInfo;

using UserlandFSUtil::FSVolumeCapabilities;

class Volume : public DoublyLinkedListLinkImpl<Volume> {
public:
								Volume(FileSystem* fileSystem, dev_t id);
	virtual						~Volume();

			FileSystem*			GetFileSystem() const;
			dev_t				GetID() const;

			void				GetCapabilities(
									FSVolumeCapabilities& capabilities) const
									{ capabilities = fCapabilities; }

	// FS
	virtual	status_t			Mount(const char* device, uint32 flags,
									const char* parameters, ino_t* rootID);
	virtual	status_t			Unmount();
	virtual	status_t			Sync();
	virtual	status_t			ReadFSInfo(fs_info* info);
	virtual	status_t			WriteFSInfo(const struct fs_info* info,
									uint32 mask);

	// vnodes
	virtual	status_t			Lookup(void* dir, const char* entryName,
									ino_t* vnid);
	virtual	status_t			GetVNodeType(void* node, int* type);
									// Only needs to be implemented when
									// the three parameters publish_vnode() is
									// used.
	virtual	status_t			GetVNodeName(void* node, char* buffer,
									size_t bufferSize);
	virtual	status_t			ReadVNode(ino_t vnid, bool reenter,
									void** node, int* type, uint32* flags,
									FSVNodeCapabilities* _capabilities);
	virtual	status_t			WriteVNode(void* node, bool reenter);
	virtual	status_t			RemoveVNode(void* node, bool reenter);

	// asynchronous I/O
	virtual	status_t			DoIO(void* node, void* cookie,
									const IORequestInfo& requestInfo);
	virtual	status_t			CancelIO(void* node, void* cookie,
									int32 ioRequestID);
	virtual	status_t			IterativeIOGetVecs(void* cookie,
									int32 requestID, off_t offset, size_t size,
									struct file_io_vec* vecs, size_t* _count);
	virtual	status_t			IterativeIOFinished(void* cookie,
									int32 requestID, status_t status,
									bool partialTransfer,
									size_t bytesTransferred);

	// nodes
	virtual	status_t			IOCtl(void* node, void* cookie,
									uint32 command, void* buffer, size_t size);
	virtual	status_t			SetFlags(void* node, void* cookie,
									int flags);
	virtual	status_t			Select(void* node, void* cookie,
									uint8 event, selectsync* sync);
	virtual	status_t			Deselect(void* node, void* cookie,
									uint8 event, selectsync* sync);

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
	virtual	status_t			WriteStat(void* node, const struct stat *st,
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
	virtual	status_t			CreateAttr(void* node, const char* name,
									uint32 type, int openMode,
									void** cookie);
	virtual	status_t			OpenAttr(void* node, const char* name,
									int openMode, void** cookie);
	virtual	status_t			CloseAttr(void* node, void* cookie);
	virtual	status_t			FreeAttrCookie(void* node, void* cookie);
	virtual	status_t			ReadAttr(void* node, void* cookie,
									off_t pos, void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			WriteAttr(void* node, void* cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
	virtual	status_t			ReadAttrStat(void* node, void* cookie,
									struct stat* st);
	virtual	status_t			WriteAttrStat(void* node, void* cookie,
									const struct stat* st, int statMask);
	virtual	status_t			RenameAttr(void* oldNode,
									const char* oldName, void* newNode,
									const char* newName);
	virtual	status_t			RemoveAttr(void* node, const char* name);

	// indices
	virtual	status_t			OpenIndexDir(void** cookie);
	virtual	status_t			CloseIndexDir(void* cookie);
	virtual	status_t			FreeIndexDirCookie(void* cookie);
	virtual	status_t			ReadIndexDir(void* cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);
	virtual	status_t			RewindIndexDir(void* cookie);
	virtual	status_t			CreateIndex(const char* name, uint32 type,
									uint32 flags);
	virtual	status_t			RemoveIndex(const char* name);
	virtual	status_t			ReadIndexStat(const char *name,
									struct stat *st);

	// queries
	virtual	status_t			OpenQuery(const char* queryString,
									uint32 flags, port_id port, uint32 token,
									void** cookie);
	virtual	status_t			CloseQuery(void* cookie);
	virtual	status_t			FreeQueryCookie(void* cookie);
	virtual	status_t			ReadQuery(void* cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);
	virtual	status_t			RewindQuery(void* cookie);

protected:
			FileSystem*			fFileSystem;
			dev_t				fID;
			FSVolumeCapabilities fCapabilities;
};

}	// namespace UserlandFS

using UserlandFS::Volume;

#endif	// USERLAND_FS_VOLUME_H
