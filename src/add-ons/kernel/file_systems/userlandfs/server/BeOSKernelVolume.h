// BeOSKernelVolume.h

#ifndef USERLAND_FS_BEOS_KERNEL_VOLUME_H
#define USERLAND_FS_BEOS_KERNEL_VOLUME_H

#include "Volume.h"

struct beos_vnode_ops;

namespace UserlandFS {

class BeOSKernelVolume : public Volume {
public:
								BeOSKernelVolume(FileSystem* fileSystem,
									mount_id id, beos_vnode_ops* fsOps);
	virtual						~BeOSKernelVolume();

	// FS
	virtual	status_t			Mount(const char* device, uint32 flags,
									const char* parameters, vnode_id* rootID);
	virtual	status_t			Unmount();
	virtual	status_t			Sync();
	virtual	status_t			ReadFSInfo(fs_info* info);
	virtual	status_t			WriteFSInfo(const struct fs_info* info,
									uint32 mask);

	// vnodes
	virtual	status_t			Lookup(fs_vnode dir, const char* entryName,
									vnode_id* vnid, int* type);
	virtual	status_t			LookupNoType(fs_vnode dir,
									const char* entryName, vnode_id* vnid);
									// not required
	virtual	status_t			ReadVNode(vnode_id vnid, bool reenter,
									fs_vnode* node);
	virtual	status_t			WriteVNode(fs_vnode node, bool reenter);
	virtual	status_t			RemoveVNode(fs_vnode node, bool reenter);

	// nodes
	virtual	status_t			IOCtl(fs_vnode node, fs_cookie cookie,
									uint32 command, void* buffer, size_t size);
	virtual	status_t			SetFlags(fs_vnode node, fs_cookie cookie,
									int flags);
	virtual	status_t			Select(fs_vnode node, fs_cookie cookie,
									uint8 event, uint32 ref, selectsync* sync);
	virtual	status_t			Deselect(fs_vnode node, fs_cookie cookie,
									uint8 event, selectsync* sync);

	virtual	status_t			FSync(fs_vnode node);

	virtual	status_t			ReadSymlink(fs_vnode node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
	virtual	status_t			CreateSymlink(fs_vnode dir, const char* name,
									const char* target, int mode);

	virtual	status_t			Link(fs_vnode dir, const char* name,
									fs_vnode node);
	virtual	status_t			Unlink(fs_vnode dir, const char* name);
	virtual	status_t			Rename(fs_vnode oldDir, const char* oldName,
									fs_vnode newDir, const char* newName);

	virtual	status_t			Access(fs_vnode node, int mode);
	virtual	status_t			ReadStat(fs_vnode node, struct stat* st);
	virtual	status_t			WriteStat(fs_vnode node, const struct stat *st,
									uint32 mask);

	// files
	virtual	status_t			Create(fs_vnode dir, const char* name,
									int openMode, int mode, fs_cookie* cookie,
									vnode_id* vnid);
	virtual	status_t			Open(fs_vnode node, int openMode,
									fs_cookie* cookie);
	virtual	status_t			Close(fs_vnode node, fs_cookie cookie);
	virtual	status_t			FreeCookie(fs_vnode node, fs_cookie cookie);
	virtual	status_t			Read(fs_vnode node, fs_cookie cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			Write(fs_vnode node, fs_cookie cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);

	// directories
	virtual	status_t			CreateDir(fs_vnode dir, const char* name,
									int mode, vnode_id *newDir);
	virtual	status_t			RemoveDir(fs_vnode dir, const char* name);
	virtual	status_t			OpenDir(fs_vnode node, fs_cookie* cookie);
	virtual	status_t			CloseDir(fs_vnode node, fs_vnode cookie);
	virtual	status_t			FreeDirCookie(fs_vnode node, fs_vnode cookie);
	virtual	status_t			ReadDir(fs_vnode node, fs_vnode cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
	virtual	status_t			RewindDir(fs_vnode node, fs_vnode cookie);

	// attribute directories
	virtual	status_t			OpenAttrDir(fs_vnode node, fs_cookie *cookie);
	virtual	status_t			CloseAttrDir(fs_vnode node, fs_cookie cookie);
	virtual	status_t			FreeAttrDirCookie(fs_vnode node,
									fs_cookie cookie);
	virtual	status_t			ReadAttrDir(fs_vnode node, fs_cookie cookie,
									void* buffer, size_t bufferSize,
									uint32 count, uint32* countRead);
	virtual	status_t			RewindAttrDir(fs_vnode node, fs_cookie cookie);

	// attributes
	virtual	status_t			CreateAttr(fs_vnode node, const char *name,
									uint32 type, int openMode,
									fs_cookie *cookie);
	virtual	status_t			OpenAttr(fs_vnode node, const char *name,
									int openMode, fs_cookie *cookie);
	virtual	status_t			CloseAttr(fs_vnode node, fs_cookie cookie);
	virtual	status_t			FreeAttrCookie(fs_vnode node, fs_cookie cookie);
	virtual	status_t			ReadAttr(fs_vnode node, fs_cookie cookie,
									off_t pos, void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			WriteAttr(fs_vnode node, fs_cookie cookie,
									off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
	virtual	status_t			ReadAttrStat(fs_vnode node, fs_cookie cookie,
									struct stat *st);
	virtual	status_t			RenameAttr(fs_vnode oldNode,
									const char* oldName, fs_vnode newNode,
									const char* newName);
	virtual	status_t			RemoveAttr(fs_vnode node, const char* name);

	// indices
	virtual	status_t			OpenIndexDir(fs_cookie *cookie);
	virtual	status_t			CloseIndexDir(fs_cookie cookie);
	virtual	status_t			FreeIndexDirCookie(fs_cookie cookie);
	virtual	status_t			ReadIndexDir(fs_cookie cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);
	virtual	status_t			RewindIndexDir(fs_cookie cookie);
	virtual	status_t			CreateIndex(const char* name, uint32 type,
									uint32 flags);
	virtual	status_t			RemoveIndex(const char* name);
	virtual	status_t			ReadIndexStat(const char *name,
									struct stat *st);

	// queries
	virtual	status_t			OpenQuery(const char* queryString,
									uint32 flags, port_id port, uint32 token,
									fs_cookie *cookie);
	virtual	status_t			CloseQuery(fs_cookie cookie);
	virtual	status_t			FreeQueryCookie(fs_cookie cookie);
	virtual	status_t			ReadQuery(fs_cookie cookie, void* buffer,
									size_t bufferSize, uint32 count,
									uint32* countRead);

private:
	class AttributeCookie;

private:
			status_t			_OpenAttr(fs_vnode node, const char* name,
									uint32 type, int openMode, bool create,
									fs_cookie* cookie);

private:
			beos_vnode_ops*		fFSOps;
			void*				fVolumeCookie;
};

}	// namespace UserlandFS

using UserlandFS::BeOSKernelVolume;

#endif	// USERLAND_FS_BEOS_KERNEL_VOLUME_H
