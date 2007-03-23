// HaikuKernelVolume.cpp

#include "HaikuKernelVolume.h"

#include <new>

#include <fcntl.h>
#include <unistd.h>

#include "Debug.h"
#include "kernel_emu.h"
#include "haiku_fs_cache.h"


// constructor
HaikuKernelVolume::HaikuKernelVolume(FileSystem* fileSystem, mount_id id,
	file_system_module_info* fsModule)
	: Volume(fileSystem, id),
	  fFSModule(fsModule),
	  fVolumeCookie(NULL)
{
}

// destructor
HaikuKernelVolume::~HaikuKernelVolume()
{
}

// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
HaikuKernelVolume::Mount(const char* device, uint32 flags,
	const char* parameters, vnode_id* rootID)
{
	if (!fFSModule->mount)
		return B_BAD_VALUE;

	// make the volume know to the file cache emulation
	status_t error
		= UserlandFS::HaikuKernelEmu::file_cache_register_volume(this);
	if (error != B_OK)
		return error;

	// mount
	error = fFSModule->mount(GetID(), device, flags, parameters,
		&fVolumeCookie, rootID);
	if (error != B_OK) {
		UserlandFS::HaikuKernelEmu::file_cache_unregister_volume(this);
		return error;
	}

	return B_OK;
}

// Unmount
status_t
HaikuKernelVolume::Unmount()
{
	if (!fFSModule->unmount)
		return B_BAD_VALUE;

	// unmount
	status_t error = fFSModule->unmount(fVolumeCookie);

	// unregister with the file cache emulation
	UserlandFS::HaikuKernelEmu::file_cache_unregister_volume(this);

	return error;
}

// Sync
status_t
HaikuKernelVolume::Sync()
{
	if (!fFSModule->sync)
		return B_BAD_VALUE;
	return fFSModule->sync(fVolumeCookie);
}

// ReadFSInfo
status_t
HaikuKernelVolume::ReadFSInfo(fs_info* info)
{
	if (!fFSModule->read_fs_info)
		return B_BAD_VALUE;
	return fFSModule->read_fs_info(fVolumeCookie, info);
}

// WriteFSInfo
status_t
HaikuKernelVolume::WriteFSInfo(const struct fs_info* info, uint32 mask)
{
	if (!fFSModule->write_fs_info)
		return B_BAD_VALUE;
	return fFSModule->write_fs_info(fVolumeCookie, info, mask);
}


// #pragma mark - file cache


// GetFileMap
status_t
HaikuKernelVolume::GetFileMap(fs_vnode node, off_t offset, size_t size,
	struct file_io_vec* vecs, size_t* count)
{
	if (!fFSModule->get_file_map)
		return B_BAD_VALUE;
	return fFSModule->get_file_map(fVolumeCookie, node, offset, size, vecs,
		count);
}


// #pragma mark - vnodes


// Lookup
status_t
HaikuKernelVolume::Lookup(fs_vnode dir, const char* entryName, vnode_id* vnid,
	int* type)
{
	if (!fFSModule->lookup)
		return B_BAD_VALUE;
	return fFSModule->lookup(fVolumeCookie, dir, entryName, vnid, type);
}

// ReadVNode
status_t
HaikuKernelVolume::ReadVNode(vnode_id vnid, bool reenter, fs_vnode* node)
{
	if (!fFSModule->get_vnode)
		return B_BAD_VALUE;
	return fFSModule->get_vnode(fVolumeCookie, vnid, node, reenter);
}

// WriteVNode
status_t
HaikuKernelVolume::WriteVNode(fs_vnode node, bool reenter)
{
	if (!fFSModule->put_vnode)
		return B_BAD_VALUE;
	return fFSModule->put_vnode(fVolumeCookie, node, reenter);
}

// RemoveVNode
status_t
HaikuKernelVolume::RemoveVNode(fs_vnode node, bool reenter)
{
	if (!fFSModule->remove_vnode)
		return B_BAD_VALUE;
	return fFSModule->remove_vnode(fVolumeCookie, node, reenter);
}


// #pragma mark - nodes


// IOCtl
status_t
HaikuKernelVolume::IOCtl(fs_vnode node, fs_cookie cookie, uint32 command,
	void* buffer, size_t size)
{
	if (!fFSModule->ioctl)
		return B_BAD_VALUE;
	return fFSModule->ioctl(fVolumeCookie, node, cookie, command, buffer,
		size);
}

// SetFlags
status_t
HaikuKernelVolume::SetFlags(fs_vnode node, fs_cookie cookie, int flags)
{
	if (!fFSModule->set_flags)
		return B_BAD_VALUE;
	return fFSModule->set_flags(fVolumeCookie, node, cookie, flags);
}

// Select
status_t
HaikuKernelVolume::Select(fs_vnode node, fs_cookie cookie, uint8 event,
	uint32 ref, selectsync* sync)
{
	if (!fFSModule->select) {
		UserlandFS::KernelEmu::notify_select_event(sync, ref, event, false);
		return B_OK;
	}
	return fFSModule->select(fVolumeCookie, node, cookie, event, ref, sync);
}

// Deselect
status_t
HaikuKernelVolume::Deselect(fs_vnode node, fs_cookie cookie, uint8 event,
	selectsync* sync)
{
	if (!fFSModule->select || !fFSModule->deselect)
		return B_OK;
	return fFSModule->deselect(fVolumeCookie, node, cookie, event, sync);
}

// FSync
status_t
HaikuKernelVolume::FSync(fs_vnode node)
{
	if (!fFSModule->fsync)
		return B_BAD_VALUE;
	return fFSModule->fsync(fVolumeCookie, node);
}

// ReadSymlink
status_t
HaikuKernelVolume::ReadSymlink(fs_vnode node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	if (!fFSModule->read_symlink)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;

	return fFSModule->read_symlink(fVolumeCookie, node, buffer, bytesRead);
}

// CreateSymlink
status_t
HaikuKernelVolume::CreateSymlink(fs_vnode dir, const char* name,
	const char* target, int mode)
{
	if (!fFSModule->create_symlink)
		return B_BAD_VALUE;
	return fFSModule->create_symlink(fVolumeCookie, dir, name, target, mode);
}

// Link
status_t
HaikuKernelVolume::Link(fs_vnode dir, const char* name, fs_vnode node)
{
	if (!fFSModule->link)
		return B_BAD_VALUE;
	return fFSModule->link(fVolumeCookie, dir, name, node);
}

// Unlink
status_t
HaikuKernelVolume::Unlink(fs_vnode dir, const char* name)
{
	if (!fFSModule->unlink)
		return B_BAD_VALUE;
	return fFSModule->unlink(fVolumeCookie, dir, name);
}

// Rename
status_t
HaikuKernelVolume::Rename(fs_vnode oldDir, const char* oldName, fs_vnode newDir,
	const char* newName)
{
	if (!fFSModule->rename)
		return B_BAD_VALUE;
	return fFSModule->rename(fVolumeCookie, oldDir, oldName, newDir, newName);
}

// Access
status_t
HaikuKernelVolume::Access(fs_vnode node, int mode)
{
	if (!fFSModule->access)
		return B_OK;
	return fFSModule->access(fVolumeCookie, node, mode);
}

// ReadStat
status_t
HaikuKernelVolume::ReadStat(fs_vnode node, struct stat* st)
{
	if (!fFSModule->read_stat)
		return B_BAD_VALUE;
	return fFSModule->read_stat(fVolumeCookie, node, st);
}

// WriteStat
status_t
HaikuKernelVolume::WriteStat(fs_vnode node, const struct stat *st, uint32 mask)
{
	if (!fFSModule->write_stat)
		return B_BAD_VALUE;
	return fFSModule->write_stat(fVolumeCookie, node, st, mask);
}


// #pragma mark - files


// Create
status_t
HaikuKernelVolume::Create(fs_vnode dir, const char* name, int openMode, int mode,
	fs_cookie* cookie, vnode_id* vnid)
{
	if (!fFSModule->create)
		return B_BAD_VALUE;
	return fFSModule->create(fVolumeCookie, dir, name, openMode, mode, cookie,
		vnid);
}

// Open
status_t
HaikuKernelVolume::Open(fs_vnode node, int openMode, fs_cookie* cookie)
{
	if (!fFSModule->open)
		return B_BAD_VALUE;
	return fFSModule->open(fVolumeCookie, node, openMode, cookie);
}

// Close
status_t
HaikuKernelVolume::Close(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->close)
		return B_OK;
	return fFSModule->close(fVolumeCookie, node, cookie);
}

// FreeCookie
status_t
HaikuKernelVolume::FreeCookie(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->free_cookie)
		return B_OK;
	return fFSModule->free_cookie(fVolumeCookie, node, cookie);
}

// Read
status_t
HaikuKernelVolume::Read(fs_vnode node, fs_cookie cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	if (!fFSModule->read)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;

	return fFSModule->read(fVolumeCookie, node, cookie, pos, buffer, bytesRead);
}

// Write
status_t
HaikuKernelVolume::Write(fs_vnode node, fs_cookie cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	if (!fFSModule->write)
		return B_BAD_VALUE;

	*bytesWritten = bufferSize;

	return fFSModule->write(fVolumeCookie, node, cookie, pos, buffer,
		bytesWritten);
}


// #pragma mark -  directories


// CreateDir
status_t
HaikuKernelVolume::CreateDir(fs_vnode dir, const char* name, int mode,
	vnode_id *newDir)
{
	if (!fFSModule->create_dir)
		return B_BAD_VALUE;
	return fFSModule->create_dir(fVolumeCookie, dir, name, mode, newDir);
}

// RemoveDir
status_t
HaikuKernelVolume::RemoveDir(fs_vnode dir, const char* name)
{
	if (!fFSModule->remove_dir)
		return B_BAD_VALUE;
	return fFSModule->remove_dir(fVolumeCookie, dir, name);
}

// OpenDir
status_t
HaikuKernelVolume::OpenDir(fs_vnode node, fs_cookie* cookie)
{
	if (!fFSModule->open_dir)
		return B_BAD_VALUE;
	return fFSModule->open_dir(fVolumeCookie, node, cookie);
}

// CloseDir
status_t
HaikuKernelVolume::CloseDir(fs_vnode node, fs_vnode cookie)
{
	if (!fFSModule->close_dir)
		return B_OK;
	return fFSModule->close_dir(fVolumeCookie, node, cookie);
}

// FreeDirCookie
status_t
HaikuKernelVolume::FreeDirCookie(fs_vnode node, fs_vnode cookie)
{
	if (!fFSModule->free_dir_cookie)
		return B_OK;
	return fFSModule->free_dir_cookie(fVolumeCookie, node, cookie);
}

// ReadDir
status_t
HaikuKernelVolume::ReadDir(fs_vnode node, fs_vnode cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fFSModule->read_dir)
		return B_BAD_VALUE;

	*countRead = count;

	return fFSModule->read_dir(fVolumeCookie, node, cookie,
		(struct dirent*)buffer, bufferSize, countRead);
}

// RewindDir
status_t
HaikuKernelVolume::RewindDir(fs_vnode node, fs_vnode cookie)
{
	if (!fFSModule->rewind_dir)
		return B_BAD_VALUE;
	return fFSModule->rewind_dir(fVolumeCookie, node, cookie);
}


// #pragma mark - attribute directories


// OpenAttrDir
status_t
HaikuKernelVolume::OpenAttrDir(fs_vnode node, fs_cookie *cookie)
{
	if (!fFSModule->open_attr_dir)
		return B_BAD_VALUE;
	return fFSModule->open_attr_dir(fVolumeCookie, node, cookie);
}

// CloseAttrDir
status_t
HaikuKernelVolume::CloseAttrDir(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->close_attr_dir)
		return B_OK;
	return fFSModule->close_attr_dir(fVolumeCookie, node, cookie);
}

// FreeAttrDirCookie
status_t
HaikuKernelVolume::FreeAttrDirCookie(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->free_attr_dir_cookie)
		return B_OK;
	return fFSModule->free_attr_dir_cookie(fVolumeCookie, node, cookie);
}

// ReadAttrDir
status_t
HaikuKernelVolume::ReadAttrDir(fs_vnode node, fs_cookie cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fFSModule->read_attr_dir)
		return B_BAD_VALUE;

	*countRead = count;

	return fFSModule->read_attr_dir(fVolumeCookie, node, cookie,
		(struct dirent*)buffer, bufferSize, countRead);
}

// RewindAttrDir
status_t
HaikuKernelVolume::RewindAttrDir(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->rewind_attr_dir)
		return B_BAD_VALUE;
	return fFSModule->rewind_attr_dir(fVolumeCookie, node, cookie);
}


// #pragma mark - attributes


// CreateAttr
status_t
HaikuKernelVolume::CreateAttr(fs_vnode node, const char* name, uint32 type,
	int openMode, fs_cookie* cookie)
{
	if (!fFSModule->create_attr)
		return B_BAD_VALUE;
	return fFSModule->create_attr(fVolumeCookie, node, name, type, openMode,
		cookie);
}

// OpenAttr
status_t
HaikuKernelVolume::OpenAttr(fs_vnode node, const char* name, int openMode,
	fs_cookie* cookie)
{
	if (!fFSModule->open_attr)
		return B_BAD_VALUE;
	return fFSModule->open_attr(fVolumeCookie, node, name, openMode, cookie);
}

// CloseAttr
status_t
HaikuKernelVolume::CloseAttr(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->close_attr)
		return B_OK;
	return fFSModule->close_attr(fVolumeCookie, node, cookie);
}

// FreeAttrCookie
status_t
HaikuKernelVolume::FreeAttrCookie(fs_vnode node, fs_cookie cookie)
{
	if (!fFSModule->free_attr_cookie)
		return B_OK;
	return fFSModule->free_attr_cookie(fVolumeCookie, node, cookie);
}

// ReadAttr
status_t
HaikuKernelVolume::ReadAttr(fs_vnode node, fs_cookie cookie, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	if (!fFSModule->read_attr)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;

	return fFSModule->read_attr(fVolumeCookie, node, cookie, pos, buffer,
		bytesRead);
}

// WriteAttr
status_t
HaikuKernelVolume::WriteAttr(fs_vnode node, fs_cookie cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	if (!fFSModule->write_attr)
		return B_BAD_VALUE;

	*bytesWritten = bufferSize;

	return fFSModule->write_attr(fVolumeCookie, node, cookie, pos, buffer,
		bytesWritten);
}

// ReadAttrStat
status_t
HaikuKernelVolume::ReadAttrStat(fs_vnode node, fs_cookie cookie,
	struct stat *st)
{
	if (!fFSModule->read_attr_stat)
		return B_BAD_VALUE;
	return fFSModule->read_attr_stat(fVolumeCookie, node, cookie, st);
}

// RenameAttr
status_t
HaikuKernelVolume::RenameAttr(fs_vnode oldNode, const char* oldName,
	fs_vnode newNode, const char* newName)
{
	if (!fFSModule->rename_attr)
		return B_BAD_VALUE;
	return fFSModule->rename_attr(fVolumeCookie, oldNode, oldName, newNode,
		newName);
}

// RemoveAttr
status_t
HaikuKernelVolume::RemoveAttr(fs_vnode node, const char* name)
{
	if (!fFSModule->remove_attr)
		return B_BAD_VALUE;
	return fFSModule->remove_attr(fVolumeCookie, node, name);
}


// #pragma mark - indices


// OpenIndexDir
status_t
HaikuKernelVolume::OpenIndexDir(fs_cookie *cookie)
{
	if (!fFSModule->open_index_dir)
		return B_BAD_VALUE;
	return fFSModule->open_index_dir(fVolumeCookie, cookie);
}

// CloseIndexDir
status_t
HaikuKernelVolume::CloseIndexDir(fs_cookie cookie)
{
	if (!fFSModule->close_index_dir)
		return B_OK;
	return fFSModule->close_index_dir(fVolumeCookie, cookie);
}

// FreeIndexDirCookie
status_t
HaikuKernelVolume::FreeIndexDirCookie(fs_cookie cookie)
{
	if (!fFSModule->free_index_dir_cookie)
		return B_OK;
	return fFSModule->free_index_dir_cookie(fVolumeCookie, cookie);
}

// ReadIndexDir
status_t
HaikuKernelVolume::ReadIndexDir(fs_cookie cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fFSModule->read_index_dir)
		return B_BAD_VALUE;

	*countRead = count;

	return fFSModule->read_index_dir(fVolumeCookie, cookie,
		(struct dirent*)buffer, bufferSize, countRead);
}

// RewindIndexDir
status_t
HaikuKernelVolume::RewindIndexDir(fs_cookie cookie)
{
	if (!fFSModule->rewind_index_dir)
		return B_BAD_VALUE;
	return fFSModule->rewind_index_dir(fVolumeCookie, cookie);
}

// CreateIndex
status_t
HaikuKernelVolume::CreateIndex(const char* name, uint32 type, uint32 flags)
{
	if (!fFSModule->create_index)
		return B_BAD_VALUE;
	return fFSModule->create_index(fVolumeCookie, name, type, flags);
}

// RemoveIndex
status_t
HaikuKernelVolume::RemoveIndex(const char* name)
{
	if (!fFSModule->remove_index)
		return B_BAD_VALUE;
	return fFSModule->remove_index(fVolumeCookie, name);
}

// StatIndex
status_t
HaikuKernelVolume::ReadIndexStat(const char *name, struct stat *st)
{
	if (!fFSModule->read_index_stat)
		return B_BAD_VALUE;
	return fFSModule->read_index_stat(fVolumeCookie, name, st);
}


// #pragma mark - queries


// OpenQuery
status_t
HaikuKernelVolume::OpenQuery(const char* queryString, uint32 flags,
	port_id port, uint32 token, fs_cookie *cookie)
{
	if (!fFSModule->open_query)
		return B_BAD_VALUE;
	return fFSModule->open_query(fVolumeCookie, queryString, flags, port,
		token, cookie);
}

// CloseQuery
status_t
HaikuKernelVolume::CloseQuery(fs_cookie cookie)
{
	if (!fFSModule->close_query)
		return B_OK;
	return fFSModule->close_query(fVolumeCookie, cookie);
}

// FreeQueryCookie
status_t
HaikuKernelVolume::FreeQueryCookie(fs_cookie cookie)
{
	if (!fFSModule->free_query_cookie)
		return B_OK;
	return fFSModule->free_query_cookie(fVolumeCookie, cookie);
}

// ReadQuery
status_t
HaikuKernelVolume::ReadQuery(fs_cookie cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	if (!fFSModule->read_query)
		return B_BAD_VALUE;

	*countRead = count;

	return fFSModule->read_query(fVolumeCookie, cookie, (struct dirent*)buffer,
		bufferSize, countRead);
}
