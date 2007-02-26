// BeOSKernelVolume.cpp

#include "BeOSKernelVolume.h"

// constructor
BeOSKernelVolume::BeOSKernelVolume(FileSystem* fileSystem, nspace_id id,
	vnode_ops* fsOps)
	: Volume(fileSystem, id),
	  fFSOps(fsOps),
	  fVolumeCookie(NULL)
{
}

// destructor
BeOSKernelVolume::~BeOSKernelVolume()
{
}

// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
BeOSKernelVolume::Mount(const char* device, ulong flags, const char* parameters,
	int32 len, vnode_id* rootID)
{
	if (!fFSOps->mount)
		return B_BAD_VALUE;
	return fFSOps->mount(GetID(), device, flags, (void*)parameters, len,
		&fVolumeCookie, rootID);
}

// Unmount
status_t
BeOSKernelVolume::Unmount()
{
	if (!fFSOps->unmount)
		return B_BAD_VALUE;
	return fFSOps->unmount(fVolumeCookie);
}

// Sync
status_t
BeOSKernelVolume::Sync()
{
	if (!fFSOps->sync)
		return B_BAD_VALUE;
	return fFSOps->sync(fVolumeCookie);
}

// ReadFSStat
status_t
BeOSKernelVolume::ReadFSStat(fs_info* info)
{
	if (!fFSOps->rfsstat)
		return B_BAD_VALUE;
	return fFSOps->rfsstat(fVolumeCookie, info);
}

// WriteFSStat
status_t
BeOSKernelVolume::WriteFSStat(struct fs_info *info, long mask)
{
	if (!fFSOps->wfsstat)
		return B_BAD_VALUE;
	return fFSOps->wfsstat(fVolumeCookie, info, mask);
}

// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
BeOSKernelVolume::ReadVNode(vnode_id vnid, char reenter, void** node)
{
	if (!fFSOps->read_vnode)
		return B_BAD_VALUE;
	return fFSOps->read_vnode(fVolumeCookie, vnid, reenter, node);
}

// WriteVNode
status_t
BeOSKernelVolume::WriteVNode(void* node, char reenter)
{
	if (!fFSOps->write_vnode)
		return B_BAD_VALUE;
	return fFSOps->write_vnode(fVolumeCookie, node, reenter);
}

// RemoveVNode
status_t
BeOSKernelVolume::RemoveVNode(void* node, char reenter)
{
	if (!fFSOps->remove_vnode)
		return B_BAD_VALUE;
	return fFSOps->remove_vnode(fVolumeCookie, node, reenter);
}

// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
BeOSKernelVolume::FSync(void* node)
{
	if (!fFSOps->fsync)
		return B_BAD_VALUE;
	return fFSOps->fsync(fVolumeCookie, node);
}

// ReadStat
status_t
BeOSKernelVolume::ReadStat(void* node, struct stat* st)
{
	if (!fFSOps->rstat)
		return B_BAD_VALUE;
	return fFSOps->rstat(fVolumeCookie, node, st);
}

// WriteStat
status_t
BeOSKernelVolume::WriteStat(void* node, struct stat* st, long mask)
{
	if (!fFSOps->wstat)
		return B_BAD_VALUE;
	return fFSOps->wstat(fVolumeCookie, node, st, mask);
}

// Access
status_t
BeOSKernelVolume::Access(void* node, int mode)
{
	if (!fFSOps->access)
		return B_BAD_VALUE;
	return fFSOps->access(fVolumeCookie, node, mode);
}

// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
BeOSKernelVolume::Create(void* dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
{
	if (!fFSOps->create)
		return B_BAD_VALUE;
	return fFSOps->create(fVolumeCookie, dir, name, openMode, mode, vnid,
		cookie);
}

// Open
status_t
BeOSKernelVolume::Open(void* node, int openMode, void** cookie)
{
	if (!fFSOps->open)
		return B_BAD_VALUE;
	return fFSOps->open(fVolumeCookie, node, openMode, cookie);
}

// Close
status_t
BeOSKernelVolume::Close(void* node, void* cookie)
{
	if (!fFSOps->close)
		return B_OK;
	return fFSOps->close(fVolumeCookie, node, cookie);
}

// FreeCookie
status_t
BeOSKernelVolume::FreeCookie(void* node, void* cookie)
{
	if (!fFSOps->free_cookie)
		return B_OK;
	return fFSOps->free_cookie(fVolumeCookie, node, cookie);
}

// Read
status_t
BeOSKernelVolume::Read(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	if (!fFSOps->read)
		return B_BAD_VALUE;
	*bytesRead = bufferSize;
	return fFSOps->read(fVolumeCookie, node, cookie, pos, buffer, bytesRead);
}

// Write
status_t
BeOSKernelVolume::Write(void* node, void* cookie, off_t pos, const void* buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	if (!fFSOps->write)
		return B_BAD_VALUE;
	*bytesWritten = bufferSize;
	return fFSOps->write(fVolumeCookie, node, cookie, pos, buffer,
		bytesWritten);
}

// IOCtl
status_t
BeOSKernelVolume::IOCtl(void* node, void* cookie, int command, void *buffer,
	size_t size)
{
	if (!fFSOps->ioctl)
		return B_BAD_VALUE;
	return fFSOps->ioctl(fVolumeCookie, node, cookie, command, buffer, size);
}

// SetFlags
status_t
BeOSKernelVolume::SetFlags(void* node, void* cookie, int flags)
{
	if (!fFSOps->setflags)
		return B_BAD_VALUE;
	return fFSOps->setflags(fVolumeCookie, node, cookie, flags);
}

// Select
status_t
BeOSKernelVolume::Select(void* node, void* cookie, uint8 event, uint32 ref,
	selectsync* sync)
{
	if (!fFSOps->select) {
		notify_select_event(sync, ref);
		return B_OK;
	}
	return fFSOps->select(fVolumeCookie, node, cookie, event, ref, sync);
}

// Deselect
status_t
BeOSKernelVolume::Deselect(void* node, void* cookie, uint8 event,
	selectsync* sync)
{
	if (!fFSOps->select || !fFSOps->deselect)
		return B_OK;
	return fFSOps->deselect(fVolumeCookie, node, cookie, event, sync);
}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// Link
status_t
BeOSKernelVolume::Link(void* dir, const char* name, void* node)
{
	if (!fFSOps->link)
		return B_BAD_VALUE;
	return fFSOps->link(fVolumeCookie, dir, name, node);
}

// Unlink
status_t
BeOSKernelVolume::Unlink(void* dir, const char* name)
{
	if (!fFSOps->unlink)
		return B_BAD_VALUE;
	return fFSOps->unlink(fVolumeCookie, dir, name);
}

// Symlink
status_t
BeOSKernelVolume::Symlink(void* dir, const char* name, const char* target)
{
	if (!fFSOps->symlink)
		return B_BAD_VALUE;
	return fFSOps->symlink(fVolumeCookie, dir, name, target);
}

// ReadLink
status_t
BeOSKernelVolume::ReadLink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	if (!fFSOps->readlink)
		return B_BAD_VALUE;
	*bytesRead = bufferSize;
	return fFSOps->readlink(fVolumeCookie, node, buffer, bytesRead);
}

// Rename
status_t
BeOSKernelVolume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	if (!fFSOps->rename)
		return B_BAD_VALUE;
	return fFSOps->rename(fVolumeCookie, oldDir, oldName, newDir, newName);
}

// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
BeOSKernelVolume::MkDir(void* dir, const char* name, int mode)
{
	if (!fFSOps->mkdir)
		return B_BAD_VALUE;
	return fFSOps->mkdir(fVolumeCookie, dir, name, mode);
}

// RmDir
status_t
BeOSKernelVolume::RmDir(void* dir, const char* name)
{
	if (!fFSOps->rmdir)
		return B_BAD_VALUE;
	return fFSOps->rmdir(fVolumeCookie, dir, name);
}

// OpenDir
status_t
BeOSKernelVolume::OpenDir(void* node, void** cookie)
{
	if (!fFSOps->opendir)
		return B_BAD_VALUE;
	return fFSOps->opendir(fVolumeCookie, node, cookie);
}

// CloseDir
status_t
BeOSKernelVolume::CloseDir(void* node, void* cookie)
{
	if (!fFSOps->closedir)
		return B_OK;
	return fFSOps->closedir(fVolumeCookie, node, cookie);
}

// FreeDirCookie
status_t
BeOSKernelVolume::FreeDirCookie(void* node, void* cookie)
{
	if (!fFSOps->free_dircookie)
		return B_OK;
	return fFSOps->free_dircookie(fVolumeCookie, node, cookie);
}

// ReadDir
status_t
BeOSKernelVolume::ReadDir(void* node, void* cookie, void* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	if (!fFSOps->readdir)
		return B_BAD_VALUE;
	*countRead = count;
	return fFSOps->readdir(fVolumeCookie, node, cookie, countRead,
		(dirent*)buffer, bufferSize);
}

// RewindDir
status_t
BeOSKernelVolume::RewindDir(void* node, void* cookie)
{
	if (!fFSOps->rewinddir)
		return B_BAD_VALUE;
	return fFSOps->rewinddir(fVolumeCookie, node, cookie);
}

// Walk
status_t
BeOSKernelVolume::Walk(void* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	if (!fFSOps->walk)
		return B_BAD_VALUE;
	return fFSOps->walk(fVolumeCookie, dir, entryName, resolvedPath, vnid);
}

// #pragma mark -
// #pragma mark ----- attributes -----

// OpenAttrDir
status_t
BeOSKernelVolume::OpenAttrDir(void* node, void** cookie)
{
	if (!fFSOps->open_attrdir)
		return B_BAD_VALUE;
	return fFSOps->open_attrdir(fVolumeCookie, node, cookie);
}

// CloseAttrDir
status_t
BeOSKernelVolume::CloseAttrDir(void* node, void* cookie)
{
	if (!fFSOps->close_attrdir)
		return B_OK;
	return fFSOps->close_attrdir(fVolumeCookie, node, cookie);
}

// FreeAttrDirCookie
status_t
BeOSKernelVolume::FreeAttrDirCookie(void* node, void* cookie)
{
	if (!fFSOps->free_attrdircookie)
		return B_OK;
	return fFSOps->free_attrdircookie(fVolumeCookie, node, cookie);
}

// ReadAttrDir
status_t
BeOSKernelVolume::ReadAttrDir(void* node, void* cookie, void* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	if (!fFSOps->read_attrdir)
		return B_BAD_VALUE;
	*countRead = count;
	return fFSOps->read_attrdir(fVolumeCookie, node, cookie, countRead,
		(struct dirent*)buffer, bufferSize);
}

// RewindAttrDir
status_t
BeOSKernelVolume::RewindAttrDir(void* node, void* cookie)
{
	if (!fFSOps->rewind_attrdir)
		return B_BAD_VALUE;
	return fFSOps->rewind_attrdir(fVolumeCookie, node, cookie);
}

// ReadAttr
status_t
BeOSKernelVolume::ReadAttr(void* node, const char* name, int type, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	if (!fFSOps->read_attr)
		return B_BAD_VALUE;
	*bytesRead = bufferSize;
	return fFSOps->read_attr(fVolumeCookie, node, name, type, buffer, bytesRead,
		pos);
}

// WriteAttr
status_t
BeOSKernelVolume::WriteAttr(void* node, const char* name, int type, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	if (!fFSOps->write_attr)
		return B_BAD_VALUE;
	*bytesWritten = bufferSize;
	return fFSOps->write_attr(fVolumeCookie, node, name, type, buffer,
		bytesWritten, pos);
}

// RemoveAttr
status_t
BeOSKernelVolume::RemoveAttr(void* node, const char* name)
{
	if (!fFSOps->remove_attr)
		return B_BAD_VALUE;
	return fFSOps->remove_attr(fVolumeCookie, node, name);
}

// RenameAttr
status_t
BeOSKernelVolume::RenameAttr(void* node, const char* oldName, const char* newName)
{
	if (!fFSOps->rename_attr)
		return B_BAD_VALUE;
	return fFSOps->rename_attr(fVolumeCookie, node, oldName, newName);
}

// StatAttr
status_t
BeOSKernelVolume::StatAttr(void* node, const char* name,
	struct attr_info* attrInfo)
{
	if (!fFSOps->stat_attr)
		return B_BAD_VALUE;
	return fFSOps->stat_attr(fVolumeCookie, node, name, attrInfo);
}

// #pragma mark -
// #pragma mark ----- indices -----

// OpenIndexDir
status_t
BeOSKernelVolume::OpenIndexDir(void** cookie)
{
	if (!fFSOps->open_indexdir)
		return B_BAD_VALUE;
	return fFSOps->open_indexdir(fVolumeCookie, cookie);
}

// CloseIndexDir
status_t
BeOSKernelVolume::CloseIndexDir(void* cookie)
{
	if (!fFSOps->close_indexdir)
		return B_OK;
	return fFSOps->close_indexdir(fVolumeCookie, cookie);
}

// FreeIndexDirCookie
status_t
BeOSKernelVolume::FreeIndexDirCookie(void* cookie)
{
	if (!fFSOps->free_indexdircookie)
		return B_OK;
	return fFSOps->free_indexdircookie(fVolumeCookie, NULL, cookie);
}

// ReadIndexDir
status_t
BeOSKernelVolume::ReadIndexDir(void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	if (!fFSOps->read_indexdir)
		return B_BAD_VALUE;
	*countRead = count;
	return fFSOps->read_indexdir(fVolumeCookie, cookie, countRead,
		(struct dirent*)buffer, bufferSize);
}

// RewindIndexDir
status_t
BeOSKernelVolume::RewindIndexDir(void* cookie)
{
	if (!fFSOps->rewind_indexdir)
		return B_BAD_VALUE;
	return fFSOps->rewind_indexdir(fVolumeCookie, cookie);
}

// CreateIndex
status_t
BeOSKernelVolume::CreateIndex(const char* name, int type, int flags)
{
	if (!fFSOps->create_index)
		return B_BAD_VALUE;
	return fFSOps->create_index(fVolumeCookie, name, type, flags);
}

// RemoveIndex
status_t
BeOSKernelVolume::RemoveIndex(const char* name)
{
	if (!fFSOps->remove_index)
		return B_BAD_VALUE;
	return fFSOps->remove_index(fVolumeCookie, name);
}

// RenameIndex
status_t
BeOSKernelVolume::RenameIndex(const char* oldName, const char* newName)
{
	if (!fFSOps->rename_index)
		return B_BAD_VALUE;
	return fFSOps->rename_index(fVolumeCookie, oldName, newName);
}

// StatIndex
status_t
BeOSKernelVolume::StatIndex(const char *name, struct index_info* indexInfo)
{
	if (!fFSOps->stat_index)
		return B_BAD_VALUE;
	return fFSOps->stat_index(fVolumeCookie, name, indexInfo);
}

// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
BeOSKernelVolume::OpenQuery(const char* queryString, ulong flags, port_id port,
	long token, void** cookie)
{
	if (!fFSOps->open_query)
		return B_BAD_VALUE;
	return fFSOps->open_query(fVolumeCookie, queryString, flags, port,
		token, cookie);
}

// CloseQuery
status_t
BeOSKernelVolume::CloseQuery(void* cookie)
{
	if (!fFSOps->close_query)
		return B_OK;
	return fFSOps->close_query(fVolumeCookie, cookie);
}

// FreeQueryCookie
status_t
BeOSKernelVolume::FreeQueryCookie(void* cookie)
{
	if (!fFSOps->free_querycookie)
		return B_OK;
	return fFSOps->free_querycookie(fVolumeCookie, NULL, cookie);
}

// ReadQuery
status_t
BeOSKernelVolume::ReadQuery(void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	if (!fFSOps->read_query)
		return B_BAD_VALUE;
	*countRead = count;
	return fFSOps->read_query(fVolumeCookie, cookie, countRead,
		(struct dirent*)buffer, bufferSize);
}

