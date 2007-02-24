// UserVolume.h

#ifndef USERLAND_FS_USER_VOLUME_H
#define USERLAND_FS_USER_VOLUME_H

#include <fsproto.h>
#include <SupportDefs.h>

namespace UserlandFS {

class UserFileSystem;

class UserVolume {
public:
								UserVolume(UserFileSystem* fileSystem,
									nspace_id id);
	virtual						~UserVolume();

			UserFileSystem*		GetFileSystem() const;
			nspace_id			GetID() const;

	// FS
	virtual	status_t			Mount(const char* device, ulong flags,
									const char* parameters, int32 len,
									vnode_id* rootID);
	virtual	status_t			Unmount();
	virtual	status_t			Sync();
	virtual	status_t			ReadFSStat(fs_info* info);
	virtual	status_t			WriteFSStat(struct fs_info *info, long mask);

	// vnodes
	virtual	status_t			ReadVNode(vnode_id vnid, char reenter,
									void** node);
	virtual	status_t			WriteVNode(void* node, char reenter);
	virtual	status_t			RemoveVNode(void* node, char reenter);

	// nodes
	virtual	status_t			FSync(void* node);
	virtual	status_t			ReadStat(void* node, struct stat* st);
	virtual	status_t			WriteStat(void* node, struct stat* st,
									long mask);
	virtual	status_t			Access(void* node, int mode);

	// files
	virtual	status_t			Create(void* dir, const char* name,
									int openMode, int mode, vnode_id* vnid,
									void** cookie);
	virtual	status_t			Open(void* node, int openMode, void** cookie);
	virtual	status_t			Close(void* node, void* cookie);
	virtual	status_t			FreeCookie(void* node, void* cookie);
	virtual	status_t			Read(void* node, void* cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			Write(void* node, void* cookie, off_t pos,
									const void* buffer, size_t bufferSize,
									size_t* bytesWritten);
	virtual	status_t			IOCtl(void* node, void* cookie, int command,
									void *buffer, size_t size);
	virtual	status_t			SetFlags(void* node, void* cookie, int flags);
	virtual	status_t			Select(void* node, void* cookie, uint8 event,
									uint32 ref, selectsync* sync);
	virtual	status_t			Deselect(void* node, void* cookie, uint8 event,
									selectsync* sync);

	// hard links / symlinks
	virtual	status_t			Link(void* dir, const char* name, void* node);
	virtual	status_t			Unlink(void* dir, const char* name);
	virtual	status_t			Symlink(void* dir, const char* name,
									const char* target);
	virtual	status_t			ReadLink(void* node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
	virtual	status_t			Rename(void* oldDir, const char* oldName,
									void* newDir, const char* newName);

	// directories
	virtual	status_t			MkDir(void* dir, const char* name, int mode);
	virtual	status_t			RmDir(void* dir, const char* name);
	virtual	status_t			OpenDir(void* node, void** cookie);
	virtual	status_t			CloseDir(void* node, void* cookie);
	virtual	status_t			FreeDirCookie(void* node, void* cookie);
	virtual	status_t			ReadDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									int32 count, int32* countRead);
	virtual	status_t			RewindDir(void* node, void* cookie);
	virtual	status_t			Walk(void* dir, const char* entryName,
									char** resolvedPath, vnode_id* vnid);

	// attributes
	virtual	status_t			OpenAttrDir(void* node, void** cookie);
	virtual	status_t			CloseAttrDir(void* node, void* cookie);
	virtual	status_t			FreeAttrDirCookie(void* node, void* cookie);
	virtual	status_t			ReadAttrDir(void* node, void* cookie,
									void* buffer, size_t bufferSize,
									int32 count, int32* countRead);
	virtual	status_t			RewindAttrDir(void* node, void* cookie);
	virtual	status_t			ReadAttr(void* node, const char* name,
									int type, off_t pos, void* buffer,
									size_t bufferSize, size_t* bytesRead);
	virtual	status_t			WriteAttr(void* node, const char* name,
									int type, off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
	virtual	status_t			RemoveAttr(void* node, const char* name);
	virtual	status_t			RenameAttr(void* node, const char* oldName,
									const char* newName);
	virtual	status_t			StatAttr(void* node, const char* name,
									struct attr_info* attrInfo);

	// indices
	virtual	status_t			OpenIndexDir(void** cookie);
	virtual	status_t			CloseIndexDir(void* cookie);
	virtual	status_t			FreeIndexDirCookie(void* cookie);
	virtual	status_t			ReadIndexDir(void* cookie, void* buffer,
									size_t bufferSize, int32 count,
									int32* countRead);
	virtual	status_t			RewindIndexDir(void* cookie);
	virtual	status_t			CreateIndex(const char* name, int type,
									int flags);
	virtual	status_t			RemoveIndex(const char* name);
	virtual	status_t			RenameIndex(const char* oldName,
									const char* newName);
	virtual	status_t			StatIndex(const char *name,
									struct index_info* indexInfo);

	// queries
	virtual	status_t			OpenQuery(const char* queryString,
									ulong flags, port_id port, long token,
									void** cookie);
	virtual	status_t			CloseQuery(void* cookie);
	virtual	status_t			FreeQueryCookie(void* cookie);
	virtual	status_t			ReadQuery(void* cookie, void* buffer,
									size_t bufferSize, int32 count,
									int32* countRead);

protected:
			UserFileSystem*		fFileSystem;
			nspace_id			fID;
};

}	// namespace UserlandFS

using UserlandFS::UserVolume;

#endif	// USERLAND_FS_USER_VOLUME_H
