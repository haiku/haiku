// Volume.h

#ifndef USERLAND_FS_VOLUME_H
#define USERLAND_FS_VOLUME_H

#include <fs_interface.h>

#include "Referencable.h"

namespace UserlandFSUtil {

class Request;
class RequestAllocator;
class RequestHandler;

}

using UserlandFSUtil::Request;
using UserlandFSUtil::RequestAllocator;
using UserlandFSUtil::RequestHandler;

class FileSystem;
struct userlandfs_ioctl;

class Volume : public Referencable {
public:
								Volume(FileSystem* fileSystem, mount_id id);
								~Volume();

			FileSystem*			GetFileSystem() const;
			mount_id			GetID() const;

			void*				GetUserlandVolume() const;
			vnode_id			GetRootID() const;
			bool				IsMounting() const;

			// client methods
			status_t			GetVNode(vnode_id vnid, fs_vnode* node);
			status_t			PutVNode(vnode_id vnid);
			status_t			NewVNode(vnode_id vnid, fs_vnode node);
			status_t			PublishVNode(vnode_id vnid, fs_vnode node);
			status_t			RemoveVNode(vnode_id vnid);
			status_t			UnremoveVNode(vnode_id vnid);
			status_t			IsVNodeRemoved(vnode_id vnid);

			// FS
			status_t			Mount(const char* device, uint32 flags,
									const char* parameters);
			status_t			Unmount();
			status_t			Sync();
			status_t			ReadFSInfo(fs_info* info);
			status_t			WriteFSInfo(const struct fs_info* info,
									uint32 mask);

			// vnodes
			status_t			Lookup(fs_vnode dir, const char* entryName,
									vnode_id* vnid, int* type);
			status_t			ReadVNode(vnode_id vnid, bool reenter,
									fs_vnode* node);
			status_t			WriteVNode(fs_vnode node, bool reenter);
			status_t			RemoveVNode(fs_vnode node, bool reenter);

			// nodes
			status_t			IOCtl(fs_vnode node, fs_cookie cookie,
									uint32 command, void *buffer, size_t size);
			status_t			SetFlags(fs_vnode node, fs_cookie cookie,
									int flags);
			status_t			Select(fs_vnode node, fs_cookie cookie,
									uint8 event, uint32 ref, selectsync* sync);
			status_t			Deselect(fs_vnode node, fs_cookie cookie,
									uint8 event, selectsync* sync);

			status_t			FSync(fs_vnode node);

			status_t			ReadSymlink(fs_vnode node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
			status_t			CreateSymlink(fs_vnode dir, const char* name,
									const char* target, int mode);

			status_t			Link(fs_vnode dir, const char* name,
									fs_vnode node);
			status_t			Unlink(fs_vnode dir, const char* name);
			status_t			Rename(fs_vnode oldDir, const char* oldName,
									fs_vnode newDir, const char* newName);

			status_t			Access(fs_vnode node, int mode);
			status_t			ReadStat(fs_vnode node, struct stat* st);
			status_t			WriteStat(fs_vnode node, const struct stat *st,
									uint32 mask);

			// files
			status_t			Create(fs_vnode dir, const char* name,
									int openMode, int mode, fs_cookie* cookie,
									vnode_id* vnid);
			status_t			Open(fs_vnode node, int openMode,
									fs_cookie* cookie);
			status_t			Close(fs_vnode node, fs_cookie cookie);
			status_t			FreeCookie(fs_vnode node, fs_cookie cookie);
			status_t			Read(fs_vnode node, fs_cookie cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
			status_t			Write(fs_vnode node, fs_cookie cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);

			// directories
			status_t			CreateDir(fs_vnode dir, const char* name,
									int mode, vnode_id *newDir);
			status_t			RemoveDir(fs_vnode dir, const char* name);
			status_t			OpenDir(fs_vnode node, fs_cookie* cookie);
			status_t			CloseDir(fs_vnode node, fs_vnode cookie);
			status_t			FreeDirCookie(fs_vnode node, fs_vnode cookie);
			status_t			ReadDir(fs_vnode node, fs_vnode cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
			status_t			RewindDir(fs_vnode node, fs_vnode cookie);

			// attribute directories
			status_t			OpenAttrDir(fs_vnode node, fs_cookie *cookie);
			status_t			CloseAttrDir(fs_vnode node, fs_cookie cookie);
			status_t			FreeAttrDirCookie(fs_vnode node,
									fs_cookie cookie);
			status_t			ReadAttrDir(fs_vnode node, fs_cookie cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
			status_t			RewindAttrDir(fs_vnode node, fs_cookie cookie);

			// attributes
			status_t			CreateAttr(fs_vnode node, const char* name,
									uint32 type, int openMode,
									fs_cookie* cookie);
			status_t			OpenAttr(fs_vnode node, const char* name,
									int openMode, fs_cookie* cookie);
			status_t			CloseAttr(fs_vnode node, fs_cookie cookie);
			status_t			FreeAttrCookie(fs_vnode node, fs_cookie cookie);
			status_t			ReadAttr(fs_vnode node, fs_cookie cookie,
									off_t pos, void* buffer, size_t bufferSize,
									size_t* bytesRead);
			status_t			WriteAttr(fs_vnode node, fs_cookie cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
			status_t			ReadAttrStat(fs_vnode node, fs_cookie cookie,
									struct stat *st);
			status_t			RenameAttr(fs_vnode oldNode,
									const char* oldName, fs_vnode newNode,
									const char* newName);
			status_t			RemoveAttr(fs_vnode node, const char* name);

			// indices
			status_t			OpenIndexDir(fs_cookie *cookie);
			status_t			CloseIndexDir(fs_cookie cookie);
			status_t			FreeIndexDirCookie(fs_cookie cookie);
			status_t			ReadIndexDir(fs_cookie cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);
			status_t			RewindIndexDir(fs_cookie cookie);
			status_t			CreateIndex(const char* name, uint32 type,
									uint32 flags);
			status_t			RemoveIndex(const char* name);
			status_t			ReadIndexStat(const char *name,
									struct stat *st);

			// queries
			status_t			OpenQuery(const char* queryString,
									uint32 flags, port_id port, uint32 token,
									fs_cookie *cookie);
			status_t			CloseQuery(fs_cookie cookie);
			status_t			FreeQueryCookie(fs_cookie cookie);
			status_t			ReadQuery(fs_cookie cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);

private:
			status_t			_Mount(const char* device, uint32 flags,
									const char* parameters);
			status_t			_Unmount();
			status_t			_Lookup(fs_vnode dir, const char* entryName,
									vnode_id* vnid, int* type);
			status_t			_WriteVNode(fs_vnode node, bool reenter);
			status_t			_Close(fs_vnode node, fs_cookie cookie);
			status_t			_FreeCookie(fs_vnode node, fs_cookie cookie);
			status_t			_CloseDir(fs_vnode node, fs_vnode cookie);
			status_t			_FreeDirCookie(fs_vnode node, fs_vnode cookie);
			status_t			_CloseAttrDir(fs_vnode node, fs_cookie cookie);
			status_t			_FreeAttrDirCookie(fs_vnode node,
									fs_cookie cookie);
			status_t			_CloseAttr(fs_vnode node, fs_cookie cookie);
			status_t			_FreeAttrCookie(fs_vnode node,
									fs_cookie cookie);
			status_t			_CloseIndexDir(fs_cookie cookie);
			status_t			_FreeIndexDirCookie(fs_cookie cookie);
			status_t			_CloseQuery(fs_cookie cookie);
			status_t			_FreeQueryCookie(fs_cookie cookie);

			status_t			_SendRequest(RequestPort* port,
									RequestAllocator* allocator,
									RequestHandler* handler, Request** reply);
			status_t			_SendReceiptAck(RequestPort* port);

			void				_IncrementVNodeCount(vnode_id vnid);
			void				_DecrementVNodeCount(vnode_id vnid);

			status_t			_InternalIOCtl(userlandfs_ioctl* buffer,
									int32 bufferSize);

			status_t			_PutAllPendingVNodes();

private:
			struct MountVNodeMap;
			struct VNodeCountMap;
			class AutoIncrementer;

			FileSystem*			fFileSystem;
			mount_id			fID;
			void*				fUserlandVolume;
			vnode_id			fRootID;
			fs_vnode			fRootNode;
			MountVNodeMap*		fMountVNodes;
			vint32				fOpenFiles;
			vint32				fOpenDirectories;
			vint32				fOpenAttributeDirectories;
			vint32				fOpenAttributes;
			vint32				fOpenIndexDirectories;
			vint32				fOpenQueries;
			VNodeCountMap*		fVNodeCountMap;
									// Tracks the number of new/get_vnode()
									// calls to be balanced by the FS by
									// corresponding put_vnode()s.
	volatile bool				fVNodeCountingEnabled;
};

#endif	// USERLAND_FS_VOLUME_H
