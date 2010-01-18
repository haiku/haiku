/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BeOSKernelVolume.h"

#include <new>

#include <fcntl.h>
#include <unistd.h>

#include "Debug.h"

#include "../kernel_emu.h"

#include "BeOSKernelFileSystem.h"
#include "fs_interface.h"


using std::nothrow;

static int open_mode_to_access(int openMode);


// AttributeCookie
class BeOSKernelVolume::AttributeCookie {
public:
	AttributeCookie(const char* name, uint32 type, int openMode, bool exists,
		bool create)
		: fType(type),
		  fOpenMode(openMode),
		  fExists(exists),
		  fCreate(create)
	{
		strcpy(fName, name);
	}

	char	fName[B_ATTR_NAME_LENGTH];
	uint32	fType;
	int		fOpenMode;
	bool	fExists;
	bool	fCreate;
};


// _FileSystem
inline BeOSKernelFileSystem*
BeOSKernelVolume::_FileSystem() const
{
	return static_cast<BeOSKernelFileSystem*>(fFileSystem);
}


// constructor
BeOSKernelVolume::BeOSKernelVolume(FileSystem* fileSystem, dev_t id,
	beos_vnode_ops* fsOps, const FSVolumeCapabilities& capabilities)
	:
	Volume(fileSystem, id),
	fFSOps(fsOps),
	fVolumeCookie(NULL),
	fMounted(false)
{
	fCapabilities = capabilities;
}

// destructor
BeOSKernelVolume::~BeOSKernelVolume()
{
}

// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
BeOSKernelVolume::Mount(const char* device, uint32 flags,
	const char* parameters, ino_t* rootID)
{
	if (!fFSOps->mount)
		return B_BAD_VALUE;

	size_t len = (parameters ? strlen(parameters) : 0);
	status_t error = fFSOps->mount(GetID(), device, flags, (void*)parameters,
		len, &fVolumeCookie, rootID);
	if (error != B_OK)
		return error;

	fMounted = true;
	return B_OK;
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

// ReadFSInfo
status_t
BeOSKernelVolume::ReadFSInfo(fs_info* info)
{
	if (!fFSOps->rfsstat)
		return B_BAD_VALUE;

	// Haiku's fs_info equals BeOS's version
	return fFSOps->rfsstat(fVolumeCookie, (beos_fs_info*)info);
}

// WriteFSInfo
status_t
BeOSKernelVolume::WriteFSInfo(const struct fs_info* info, uint32 mask)
{
	if (!fFSOps->wfsstat)
		return B_BAD_VALUE;

	// Haiku's fs_info equals BeOS's version
	return fFSOps->wfsstat(fVolumeCookie, (beos_fs_info*)info, (long)mask);
}


// #pragma mark - vnodes


// Lookup
status_t
BeOSKernelVolume::Lookup(void* dir, const char* entryName, ino_t* vnid)
{
	if (!fFSOps->walk)
		return B_BAD_VALUE;
	return fFSOps->walk(fVolumeCookie, dir, entryName, NULL, vnid);
}


// GetVNodeType
status_t
BeOSKernelVolume::GetVNodeType(void* node, int* type)
{
	if (fMounted) {
		// The volume is mounted. We can stat() the node to get its type.
		struct stat st;
		status_t error = ReadStat(node, &st);
		if (error != B_OK)
			return error;

		*type = st.st_mode & S_IFMT;
	} else {
		// Not mounted yet. That particularly means we don't have a volume
		// cookie yet and cannot use calls into the FS to get the node type.
		// Just assume the node is a directory. That definitely is the case for
		// the root node and shouldn't do harm for the index directory or
		// indices, which could get published while mounting as well.
		*type = S_IFDIR;
			// TODO: Store the concerned nodes and check their type as soon as
			// possible (at the end of Mount()). The incorrect ones could be
			// corrected in the kernel: remove_vnode(), x*put_vnode() (catching
			// the "remove_vnode()" callback), publish_vnode(),
			// (x-1)*get_vnode().
	}

	return B_OK;
}


// ReadVNode
status_t
BeOSKernelVolume::ReadVNode(ino_t vnid, bool reenter, void** node, int* type,
	uint32* flags, FSVNodeCapabilities* _capabilities)
{
	if (!fFSOps->read_vnode)
		return B_BAD_VALUE;

	// get the node
	status_t error = fFSOps->read_vnode(fVolumeCookie, vnid, (char)reenter,
		node);
	if (error != B_OK)
		return error;

	// stat it -- we need to get the node type
	struct stat st;
	error = ReadStat(*node, &st);
	if (error != B_OK) {
		WriteVNode(*node, reenter);
		return error;
	}

	*type = (st.st_mode & S_IFMT);
	*flags = 0;
	_FileSystem()->GetNodeCapabilities(*_capabilities);

	return B_OK;
}

// WriteVNode
status_t
BeOSKernelVolume::WriteVNode(void* node, bool reenter)
{
	if (!fFSOps->write_vnode)
		return B_BAD_VALUE;
	return fFSOps->write_vnode(fVolumeCookie, node, (char)reenter);
}

// RemoveVNode
status_t
BeOSKernelVolume::RemoveVNode(void* node, bool reenter)
{
	if (!fFSOps->remove_vnode)
		return B_BAD_VALUE;
	return fFSOps->remove_vnode(fVolumeCookie, node, (char)reenter);
}


// #pragma mark - nodes


// IOCtl
status_t
BeOSKernelVolume::IOCtl(void* node, void* cookie, uint32 command,
	void* buffer, size_t size)
{
	if (!fFSOps->ioctl)
		return B_BAD_VALUE;
	return fFSOps->ioctl(fVolumeCookie, node, cookie, (int)command, buffer,
		size);
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
BeOSKernelVolume::Select(void* node, void* cookie, uint8 event,
	selectsync* sync)
{
	if (!fFSOps->select) {
		UserlandFS::KernelEmu::notify_select_event(sync, event, false);
		return B_OK;
	}
	return fFSOps->select(fVolumeCookie, node, cookie, event, 0, sync);
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

// FSync
status_t
BeOSKernelVolume::FSync(void* node)
{
	if (!fFSOps->fsync)
		return B_BAD_VALUE;
	return fFSOps->fsync(fVolumeCookie, node);
}

// ReadSymlink
status_t
BeOSKernelVolume::ReadSymlink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	if (!fFSOps->readlink)
		return B_BAD_VALUE;
	*bytesRead = bufferSize;
	return fFSOps->readlink(fVolumeCookie, node, buffer, bytesRead);
}

// CreateSymlink
status_t
BeOSKernelVolume::CreateSymlink(void* dir, const char* name,
	const char* target, int mode)
{
	if (!fFSOps->symlink)
		return B_BAD_VALUE;
// TODO: Don't ignore mode?
	return fFSOps->symlink(fVolumeCookie, dir, name, target);
}

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

// Rename
status_t
BeOSKernelVolume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	if (!fFSOps->rename)
		return B_BAD_VALUE;
	return fFSOps->rename(fVolumeCookie, oldDir, oldName, newDir, newName);
}

// Access
status_t
BeOSKernelVolume::Access(void* node, int mode)
{
	if (!fFSOps->access)
		return B_OK;
	return fFSOps->access(fVolumeCookie, node, mode);
}

// ReadStat
status_t
BeOSKernelVolume::ReadStat(void* node, struct stat* st)
{
	if (!fFSOps->rstat)
		return B_BAD_VALUE;

	// Haiku's struct stat has an additional st_type field (for an attribute
	// type), but that doesn't matter here
	return fFSOps->rstat(fVolumeCookie, node, (struct beos_stat*)st);
}

// WriteStat
status_t
BeOSKernelVolume::WriteStat(void* node, const struct stat *st, uint32 mask)
{
	if (!fFSOps->wstat)
		return B_BAD_VALUE;

	// Haiku's struct stat has an additional st_type field (for an attribute
	// type), but that doesn't matter here
	return fFSOps->wstat(fVolumeCookie, node, (struct beos_stat*)st,
		(long)mask);
}


// #pragma mark - files


// Create
status_t
BeOSKernelVolume::Create(void* dir, const char* name, int openMode, int mode,
	void** cookie, ino_t* vnid)
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
BeOSKernelVolume::Write(void* node, void* cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	if (!fFSOps->write)
		return B_BAD_VALUE;
	*bytesWritten = bufferSize;
	return fFSOps->write(fVolumeCookie, node, cookie, pos, buffer,
		bytesWritten);
}


// #pragma mark -  directories


// CreateDir
status_t
BeOSKernelVolume::CreateDir(void* dir, const char* name, int mode)
{
	if (!fFSOps->mkdir)
		return B_BAD_VALUE;

	return fFSOps->mkdir(fVolumeCookie, dir, name, mode);
}

// RemoveDir
status_t
BeOSKernelVolume::RemoveDir(void* dir, const char* name)
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
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fFSOps->readdir)
		return B_BAD_VALUE;

	*countRead = count;

	// Haiku's struct dirent equals BeOS's version
	return fFSOps->readdir(fVolumeCookie, node, cookie, (long*)countRead,
		(beos_dirent*)buffer, bufferSize);
}

// RewindDir
status_t
BeOSKernelVolume::RewindDir(void* node, void* cookie)
{
	if (!fFSOps->rewinddir)
		return B_BAD_VALUE;
	return fFSOps->rewinddir(fVolumeCookie, node, cookie);
}


// #pragma mark - attribute directories


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
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fFSOps->read_attrdir)
		return B_BAD_VALUE;

	*countRead = count;

	// Haiku's struct dirent equals BeOS's version
	return fFSOps->read_attrdir(fVolumeCookie, node, cookie, (long*)countRead,
		(struct beos_dirent*)buffer, bufferSize);
}

// RewindAttrDir
status_t
BeOSKernelVolume::RewindAttrDir(void* node, void* cookie)
{
	if (!fFSOps->rewind_attrdir)
		return B_BAD_VALUE;
	return fFSOps->rewind_attrdir(fVolumeCookie, node, cookie);
}


// #pragma mark - attributes


// CreateAttr
status_t
BeOSKernelVolume::CreateAttr(void* node, const char* name, uint32 type,
	int openMode, void** cookie)
{
	return _OpenAttr(node, name, type, openMode, true, cookie);
}

// OpenAttr
status_t
BeOSKernelVolume::OpenAttr(void* node, const char* name, int openMode,
	void** cookie)
{
	return _OpenAttr(node, name, 0, openMode, false, cookie);
}

// CloseAttr
status_t
BeOSKernelVolume::CloseAttr(void* node, void* cookie)
{
	return B_OK;
}

// FreeAttrCookie
status_t
BeOSKernelVolume::FreeAttrCookie(void* node, void* _cookie)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	// If the attribute doesn't exist yet and it was opened with
	// CreateAttr(), we could create it now. We have a race condition here
	// though, since someone else could have created it in the meantime.

	delete cookie;

	return B_OK;
}

// ReadAttr
status_t
BeOSKernelVolume::ReadAttr(void* node, void* _cookie, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	// check, if open mode allows reading
	if ((open_mode_to_access(cookie->fOpenMode) | R_OK) == 0)
		return B_FILE_ERROR;

	// read
	if (!fFSOps->read_attr)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;
	return fFSOps->read_attr(fVolumeCookie, node, cookie->fName, cookie->fType,
		buffer, bytesRead, pos);
}

// WriteAttr
status_t
BeOSKernelVolume::WriteAttr(void* node, void* _cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	// check, if open mode allows writing
	if ((open_mode_to_access(cookie->fOpenMode) | W_OK) == 0)
		return B_FILE_ERROR;

	// write
	if (!fFSOps->write_attr)
		return B_BAD_VALUE;

	*bytesWritten = bufferSize;
	return fFSOps->write_attr(fVolumeCookie, node, cookie->fName, cookie->fType,
		buffer, bytesWritten, pos);
}

// ReadAttrStat
status_t
BeOSKernelVolume::ReadAttrStat(void* node, void* _cookie,
	struct stat *st)
{
	AttributeCookie* cookie = (AttributeCookie*)_cookie;

	// get the stats
	beos_attr_info attrInfo;
	if (!fFSOps->stat_attr)
		return B_BAD_VALUE;

	status_t error = fFSOps->stat_attr(fVolumeCookie, node, cookie->fName,
		&attrInfo);
	if (error != B_OK)
		return error;

	// translate to struct stat
	st->st_size = attrInfo.size;
	st->st_type = attrInfo.type;

	return B_OK;
}

// RenameAttr
status_t
BeOSKernelVolume::RenameAttr(void* oldNode, const char* oldName,
	void* newNode, const char* newName)
{
	if (!fFSOps->rename_attr)
		return B_BAD_VALUE;
	if (oldNode != newNode)
		return B_BAD_VALUE;

	return fFSOps->rename_attr(fVolumeCookie, oldNode, oldName, newName);
}

// RemoveAttr
status_t
BeOSKernelVolume::RemoveAttr(void* node, const char* name)
{
	if (!fFSOps->remove_attr)
		return B_BAD_VALUE;
	return fFSOps->remove_attr(fVolumeCookie, node, name);
}


// #pragma mark - indices


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
BeOSKernelVolume::ReadIndexDir(void* cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fFSOps->read_indexdir)
		return B_BAD_VALUE;

	*countRead = count;

	// Haiku's struct dirent equals BeOS's version
	return fFSOps->read_indexdir(fVolumeCookie, cookie, (long*)countRead,
		(struct beos_dirent*)buffer, bufferSize);
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
BeOSKernelVolume::CreateIndex(const char* name, uint32 type, uint32 flags)
{
	if (!fFSOps->create_index)
		return B_BAD_VALUE;
	return fFSOps->create_index(fVolumeCookie, name, (int)type, (int)flags);
}

// RemoveIndex
status_t
BeOSKernelVolume::RemoveIndex(const char* name)
{
	if (!fFSOps->remove_index)
		return B_BAD_VALUE;
	return fFSOps->remove_index(fVolumeCookie, name);
}

// StatIndex
status_t
BeOSKernelVolume::ReadIndexStat(const char *name, struct stat *st)
{
	if (!fFSOps->stat_index)
		return B_BAD_VALUE;

	beos_index_info indexInfo;
	status_t error = fFSOps->stat_index(fVolumeCookie, name, &indexInfo);
	if (error != B_OK)
		return error;

	// translate index_info into struct stat
	st->st_type = indexInfo.type;
	st->st_size = indexInfo.size;
	st->st_mtime = indexInfo.modification_time;
	st->st_crtime = indexInfo.creation_time;
	st->st_uid = indexInfo.uid;
	st->st_gid = indexInfo.gid;

	return B_OK;
}


// #pragma mark - queries


// OpenQuery
status_t
BeOSKernelVolume::OpenQuery(const char* queryString, uint32 flags, port_id port,
	uint32 token, void** cookie)
{
	if (!fFSOps->open_query)
		return B_BAD_VALUE;
	return fFSOps->open_query(fVolumeCookie, queryString, flags, port,
		(long)token, cookie);
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
	uint32 count, uint32* countRead)
{
	if (!fFSOps->read_query)
		return B_BAD_VALUE;

	*countRead = count;

	// Haiku's struct dirent equals BeOS's version
	return fFSOps->read_query(fVolumeCookie, cookie, (long*)countRead,
		(struct beos_dirent*)buffer, bufferSize);
}


// #pragma mark - Private


// _OpenAttr
status_t
BeOSKernelVolume::_OpenAttr(void* node, const char* name, uint32 type,
	int openMode, bool create, void** _cookie)
{
	// check permissions first
	int accessMode = open_mode_to_access(openMode) | (create ? W_OK : 0);
	status_t error = Access(node, accessMode);
	if (error != B_OK)
		return error;

	// check whether the attribute already exists
	beos_attr_info attrInfo;
	if (!fFSOps->stat_attr)
		return B_BAD_VALUE;
	bool exists
		= (fFSOps->stat_attr(fVolumeCookie, node, name, &attrInfo) == B_OK);

	if (create) {
		// create: fail, if attribute exists and non-existence was required
		if (exists && (openMode & O_EXCL))
			return B_FILE_EXISTS;
	} else {
		// open: fail, if attribute doesn't exist
		if (!exists)
			return B_ENTRY_NOT_FOUND;

		// keep the attribute type
		type = attrInfo.type;
	}

	// create an attribute cookie
	AttributeCookie* cookie = new(nothrow) AttributeCookie(name, type,
		openMode, exists, create);
	if (!cookie)
		return B_NO_MEMORY;

	// TODO: If we want to support O_TRUNC, we should do that here.

	*_cookie = cookie;
	return B_OK;
}

// open_mode_to_access
static int
open_mode_to_access(int openMode)
{
 	switch (openMode & O_RWMASK) {
		case O_RDONLY:
			return R_OK;
		case O_WRONLY:
			return W_OK;
		case O_RDWR:
		default:
			return W_OK | R_OK;
 	}
}

