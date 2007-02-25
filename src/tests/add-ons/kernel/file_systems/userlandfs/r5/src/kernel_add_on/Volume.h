// Volume.h

#ifndef USERLAND_FS_VOLUME_H
#define USERLAND_FS_VOLUME_H

#include <fsproto.h>

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
								Volume(FileSystem* fileSystem, nspace_id id);
								~Volume();

			FileSystem*			GetFileSystem() const;
			nspace_id			GetID() const;

			void*				GetUserlandVolume() const;
			vnode_id			GetRootID() const;
			bool				IsMounting() const;

			// client methods
			status_t			GetVNode(vnode_id vnid, void** node);
			status_t			PutVNode(vnode_id vnid);
			status_t			NewVNode(vnode_id vnid, void* node);
			status_t			RemoveVNode(vnode_id vnid);
			status_t			UnremoveVNode(vnode_id vnid);
			status_t			IsVNodeRemoved(vnode_id vnid);

			// FS
			status_t			Mount(const char* device, ulong flags,
									const char* parameters, int32 len);
			status_t			Unmount();
			status_t			Sync();
			status_t			ReadFSStat(fs_info* info);
			status_t			WriteFSStat(struct fs_info* info, long mask);

			// vnodes
			status_t			ReadVNode(vnode_id vnid, char reenter,
									void** node);
			status_t			WriteVNode(void* node, char reenter);
			status_t			RemoveVNode(void* node, char reenter);

			// nodes
			status_t			FSync(void* node);
			status_t			ReadStat(void* node, struct stat* st);
			status_t			WriteStat(void* node, struct stat *st,
									long mask);
			status_t			Access(void* node, int mode);

			// files
			status_t			Create(void* dir, const char* name,
									int openMode, int mode, vnode_id* vnid,
									void** cookie);
			status_t			Open(void* node, int openMode, void** cookie);
			status_t			Close(void* node, void* cookie);
			status_t			FreeCookie(void* node, void* cookie);
			status_t			Read(void* node, void* cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
			status_t			Write(void* node, void* cookie, off_t pos,
									const void* buffer, size_t bufferSize,
									size_t* bytesWritten);
			status_t			IOCtl(void* node, void* cookie, int command,
									void *buffer, size_t size);
			status_t			SetFlags(void* node, void* cookie, int flags);
			status_t			Select(void* node, void* cookie, uint8 event,
									uint32 ref, selectsync* sync);
			status_t			Deselect(void* node, void* cookie, uint8 event,
									selectsync* sync);

			// hard links / symlinks
			status_t			Link(void* dir, const char* name, void* node);
			status_t			Unlink(void* dir, const char* name);
			status_t			Symlink(void* dir, const char* name,
									const char* target);
			status_t			ReadLink(void* node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
			status_t			Rename(void* oldDir, const char* oldName,
									void* newDir, const char* newName);

			// directories
			status_t			MkDir(void* dir, const char* name, int mode);
			status_t			RmDir(void* dir, const char* name);
			status_t			OpenDir(void* node, void** cookie);
			status_t			CloseDir(void* node, void* cookie);
			status_t			FreeDirCookie(void* node, void* cookie);
			status_t			ReadDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									int32 count, int32* countRead);
			status_t			RewindDir(void* node, void* cookie);
			status_t			Walk(void* dir, const char* entryName,
									char** resolvedPath, vnode_id* vnid);

			// attributes
			status_t			OpenAttrDir(void* node, void** cookie);
			status_t			CloseAttrDir(void* node, void* cookie);
			status_t			FreeAttrDirCookie(void* node, void* cookie);
			status_t			ReadAttrDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									int32 count, int32* countRead);
			status_t			RewindAttrDir(void* node, void* cookie);
			status_t			ReadAttr(void* node, const char* name,
									int type, off_t pos, void* buffer,
									size_t bufferSize, size_t* bytesRead);
			status_t			WriteAttr(void* node, const char* name,
									int type, off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
			status_t			RemoveAttr(void* node, const char* name);
			status_t			RenameAttr(void* node, const char* oldName,
									const char* newName);
			status_t			StatAttr(void* node, const char* name,
									struct attr_info* attrInfo);

			// indices
			status_t			OpenIndexDir(void** cookie);
			status_t			CloseIndexDir(void* cookie);
			status_t			FreeIndexDirCookie(void* cookie);
			status_t			ReadIndexDir(void* cookie, void* buffer,
									size_t bufferSize, int32 count,
									int32* countRead);
			status_t			RewindIndexDir(void* cookie);
			status_t			CreateIndex(const char* name, int type,
									int flags);
			status_t			RemoveIndex(const char* name);
			status_t			RenameIndex(const char* oldName,
									const char* newName);
			status_t			StatIndex(const char *name,
									struct index_info* indexInfo);

			// queries
			status_t			OpenQuery(const char* queryString,
									ulong flags, port_id port, long token,
									void** cookie);
			status_t			CloseQuery(void* cookie);
			status_t			FreeQueryCookie(void* cookie);
			status_t			ReadQuery(void* cookie, void* buffer,
									size_t bufferSize, int32 count,
									int32* countRead);

private:
			status_t			_Mount(const char* device, ulong flags,
									const char* parameters, int32 len);
			status_t			_Unmount();
			status_t			_WriteVNode(void* node, char reenter);
			status_t			_Close(void* node, void* cookie);
			status_t			_FreeCookie(void* node, void* cookie);
			status_t			_CloseDir(void* node, void* cookie);
			status_t			_FreeDirCookie(void* node, void* cookie);
			status_t			_Walk(void* dir, const char* entryName,
									char** resolvedPath, vnode_id* vnid);
			status_t			_CloseAttrDir(void* node, void* cookie);
			status_t			_FreeAttrDirCookie(void* node, void* cookie);
			status_t			_CloseIndexDir(void* cookie);
			status_t			_FreeIndexDirCookie(void* cookie);
			status_t			_CloseQuery(void* cookie);
			status_t			_FreeQueryCookie(void* cookie);

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
			nspace_id			fID;
			void*				fUserlandVolume;
			vnode_id			fRootID;
			void*				fRootNode;
			MountVNodeMap*		fMountVNodes;
			vint32				fOpenFiles;
			vint32				fOpenDirectories;
			vint32				fOpenAttributeDirectories;
			vint32				fOpenIndexDirectories;
			vint32				fOpenQueries;
			VNodeCountMap*		fVNodeCountMap;
									// Tracks the number of new/get_vnode()
									// calls to be balanced by the FS by
									// corresponding put_vnode()s.
	volatile bool				fVNodeCountingEnabled;
};

#endif	// USERLAND_FS_VOLUME_H
