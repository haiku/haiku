// kernel_interface.cpp
//
// Copyright (c) 2003, Axel Dörfler (axeld@pinc-software.de)
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include <new>

#include <fs_index.h>
#include <fs_query.h>
#include <KernelExport.h>
#include <TypeConstants.h>

//#include "lock.h"
//#include "cache.h"
#include "fsproto.h"

#include <AutoDeleter.h>

#include "AllocationInfo.h"
#include "AttributeIndex.h"
#include "AttributeIterator.h"
#include "Debug.h"
#include "Directory.h"
#include "Entry.h"
#include "EntryIterator.h"
#include "File.h"
#include "Index.h"
#include "IndexDirectory.h"
#include "Locking.h"
#include "Misc.h"
#include "Node.h"
#include "Query.h"
#include "ramfs_ioctl.h"
#include "SymLink.h"
#include "Volume.h"

// BFS returns the length of the entry name in dirent::d_reclen. This is
// not correct, since this field should be set to the length of the complete
// dirent. If set to != 0, KEEP_WRONG_DIRENT_RECLEN emulates the buggy
// bahavior.
#ifndef KEEP_WRONG_DIRENT_RECLEN
#define KEEP_WRONG_DIRENT_RECLEN	0
#endif

extern "C" {

static int ramfs_mount(nspace_id nsid, const char *device, ulong flags,
					   void *parameters, size_t len, void **data,
					   vnode_id *rootID);
static int ramfs_unmount(void *ns);
static int ramfs_initialize(const char *deviceName, void *parameters,
							size_t len);
static int ramfs_sync(void *_ns);

static int ramfs_read_vnode(void *ns, vnode_id vnid, char reenter,
							void **node);
static int ramfs_write_vnode(void *ns, void *_node, char reenter);
static int ramfs_remove_vnode(void *ns, void *_node, char reenter);
static int ramfs_walk(void *ns, void *_dir, const char *entryName,
					  char **resolvedPath, vnode_id *vnid);
static int ramfs_access(void *ns, void *_node, int mode);

static int ramfs_ioctl(void *ns, void *_node, void *_cookie, int cmd,
					   void *buffer, size_t bufferSize);
static int ramfs_setflags(void *ns, void *_node, void *_cookie, int flags);
static int ramfs_fsync(void *ns, void *_node);
static int ramfs_read_stat(void *ns, void *_node, struct stat *st);
static int ramfs_write_stat(void *ns, void *_node, struct stat *st, long mask);
static int ramfs_create(void *ns, void *dir, const char *name, int openMode,
						int mode, vnode_id *vnid, void **cookie);
static int ramfs_open(void *ns, void *_node, int openMode, void **cookie);
static int ramfs_close(void *ns, void *node, void *cookie);
static int ramfs_free_cookie(void *ns, void *node, void *cookie);
static int ramfs_read(void *ns, void *_node, void *cookie, off_t pos,
					  void *buffer, size_t *bufferSize);
static int ramfs_write(void *ns, void *_node, void *cookie, off_t pos,
					   const void *buffer, size_t *bufferSize);

static int ramfs_rename(void *ns, void *_oldDir, const char *oldName,
						void *_newDir, const char *newName);
static int ramfs_link(void *ns, void *_dir, const char *name, void *node);
static int ramfs_unlink(void *ns, void *_dir, const char *name);
static int ramfs_rmdir(void *ns, void *_dir, const char *name);
static int ramfs_mkdir(void *ns, void *_dir, const char *name, int mode);
static int ramfs_open_dir(void *ns, void *_node, void **cookie);
static int ramfs_read_dir(void *ns, void *_node, void *cookie, long *count,
						  struct dirent *buffer, size_t bufferSize);
static int ramfs_rewind_dir(void *ns, void *_node, void *cookie);
static int ramfs_close_dir(void *ns, void *_node, void *cookie);
static int ramfs_free_dir_cookie(void *ns, void *_node, void *cookie);

static int ramfs_read_fs_stat(void *ns, struct fs_info *info);
static int ramfs_write_fs_stat(void *ns, struct fs_info *info, long mask);

static int ramfs_symlink(void *ns, void *_dir, const char *name,
						 const char *path);
static int ramfs_read_link(void *ns, void *_node, char *buffer,
						   size_t *bufferSize);
// attributes
static int ramfs_open_attrdir(void *ns, void *_node, void **_cookie);
static int ramfs_close_attrdir(void *ns, void *_node, void *_cookie);
static int ramfs_free_attrdir_cookie(void *ns, void *_node, void *_cookie);
static int ramfs_rewind_attrdir(void *ns, void *_node, void *_cookie);
static int ramfs_read_attrdir(void *ns, void *_node, void *_cookie,
							  long *count, struct dirent *buffer,
							  size_t bufferSize);
static int ramfs_read_attr(void *ns, void *_node, const char *name, int type,
						   void *buffer, size_t *bufferSize, off_t pos);
static int ramfs_write_attr(void *ns, void *_node, const char *name, int type,
							const void *buffer, size_t *bufferSize, off_t pos);
static int ramfs_remove_attr(void *ns, void *_node, const char *name);
static int ramfs_rename_attr(void *ns, void *_node, const char *oldName,
							 const char *newName);
static int ramfs_stat_attr(void *ns, void *_node, const char *name,
						   struct attr_info *attrInfo);
// indices
static int ramfs_open_indexdir(void *ns, void **_cookie);
static int ramfs_close_indexdir(void *ns, void *_cookie);
static int ramfs_free_indexdir_cookie(void *ns, void *_node, void *_cookie);
static int ramfs_rewind_indexdir(void *_ns, void *_cookie);
static int ramfs_read_indexdir(void *ns, void *_cookie, long *count,
							   struct dirent *buffer, size_t bufferSize);
static int ramfs_create_index(void *ns, const char *name, int type, int flags);
static int ramfs_remove_index(void *ns, const char *name);
static int ramfs_rename_index(void *ns, const char *oldname,
							  const char *newname);
static int ramfs_stat_index(void *ns, const char *name,
							struct index_info *indexInfo);
// queries
int ramfs_open_query(void *ns, const char *queryString, ulong flags,
					 port_id port, long token, void **cookie);
int ramfs_close_query(void *ns, void *cookie);
int ramfs_free_query_cookie(void *ns, void *node, void *cookie);
int ramfs_read_query(void *ns, void *cookie, long *count,
					 struct dirent *buffer, size_t bufferSize);

} // extern "C"

/* vnode_ops struct. Fill this in to tell the kernel how to call
	functions in your driver.
*/

vnode_ops fs_entry =  {
	&ramfs_read_vnode,				// read_vnode
	&ramfs_write_vnode,				// write_vnode
	&ramfs_remove_vnode,			// remove_vnode
	NULL,							// secure_vnode (not needed)
	&ramfs_walk,					// walk
	&ramfs_access,					// access
	&ramfs_create,					// create
	&ramfs_mkdir,					// mkdir
	&ramfs_symlink,					// symlink
	&ramfs_link,					// link
	&ramfs_rename,					// rename
	&ramfs_unlink,					// unlink
	&ramfs_rmdir,					// rmdir
	&ramfs_read_link,				// readlink
	&ramfs_open_dir,				// opendir
	&ramfs_close_dir,				// closedir
	&ramfs_free_dir_cookie,			// free_dircookie
	&ramfs_rewind_dir,				// rewinddir
	&ramfs_read_dir,				// readdir
	&ramfs_open,					// open file
	&ramfs_close,					// close file
	&ramfs_free_cookie,				// free cookie
	&ramfs_read,					// read file
	&ramfs_write,					// write file
	NULL,							// readv
	NULL,							// writev
	&ramfs_ioctl,					// ioctl
	&ramfs_setflags,				// setflags file
	&ramfs_read_stat,				// read stat
	&ramfs_write_stat,				// write stat
	&ramfs_fsync,					// fsync
	&ramfs_initialize,				// initialize
	&ramfs_mount,					// mount
	&ramfs_unmount,					// unmount
	&ramfs_sync,					// sync
	&ramfs_read_fs_stat,			// read fs stat
	&ramfs_write_fs_stat,			// write fs stat
	NULL,							// select
	NULL,							// deselect

	&ramfs_open_indexdir,			// open index dir
	&ramfs_close_indexdir,			// close index dir
	&ramfs_free_indexdir_cookie,	// free index dir cookie
	&ramfs_rewind_indexdir,			// rewind index dir
	&ramfs_read_indexdir,			// read index dir
	&ramfs_create_index,			// create index
	&ramfs_remove_index,			// remove index
	&ramfs_rename_index,			// rename index
	&ramfs_stat_index,				// stat index

	&ramfs_open_attrdir,			// open attr dir
	&ramfs_close_attrdir,			// close attr dir
	&ramfs_free_attrdir_cookie,		// free attr dir cookie
	&ramfs_rewind_attrdir,			// rewind attr dir
	&ramfs_read_attrdir,			// read attr dir
	&ramfs_write_attr,				// write attr
	&ramfs_read_attr,				// read attr
	&ramfs_remove_attr,				// remove attr
	&ramfs_rename_attr,				// rename attr
	&ramfs_stat_attr,				// stat attr

	&ramfs_open_query,				// open query
	&ramfs_close_query,				// close query
	&ramfs_free_query_cookie,		// free query cookie
	&ramfs_read_query,				// read query
};

int32 api_version = B_CUR_FS_API_VERSION;

static char *kFSName = "ramfs";
static const size_t kOptimalIOSize = 65536;
static const bigtime_t kNotificationInterval = 1000000LL;

// notify_if_stat_changed
void
notify_if_stat_changed(Volume *volume, Node *node)
{
	if (volume && node && node->IsModified()) {
		node->MarkUnmodified();
		notify_listener(B_STAT_CHANGED, volume->GetID(), 0, 0, node->GetID(),
						NULL);
	}
}


// #pragma mark - FS


// ramfs_mount
static
int
ramfs_mount(nspace_id nsid, const char */*device*/, ulong flags,
			void */*parameters*/, size_t /*len*/, void **data,
			vnode_id *rootID)
{
	init_debugging();
	FUNCTION_START();
	// parameters are ignored for now
	status_t error = B_OK;
	// fail, if read-only mounting is requested
	if (flags & B_MOUNT_READ_ONLY)
		error = B_BAD_VALUE;
	// allocate and init the volume
	Volume *volume = NULL;
	if (error == B_OK) {
		volume = new(std::nothrow) Volume;
		if (!volume)
			SET_ERROR(error, B_NO_MEMORY);
		if (error == B_OK)
			error = volume->Mount(nsid);
	}
	// set the results
	if (error == B_OK) {
		*rootID = volume->GetRootDirectory()->GetID();
		*data = volume;
	}
	// cleanup on failure
	if (error != B_OK && volume)
		delete volume;

if (error == B_OK) {
ramfs_create_index(volume, "myIndex", B_STRING_TYPE, 0);
}

	RETURN_ERROR(error);
}

// ramfs_unmount
static
int
ramfs_unmount(void *ns)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = volume->Unmount();
	if (error == B_OK)
		delete volume;
	if (error != B_OK)
		REPORT_ERROR(error);
	exit_debugging();
	return error;
}

// ramfs_initialize
static
int
ramfs_initialize(const char */*deviceName*/, void */*parameters*/,
				 size_t /*len*/)
{
	FUNCTION_START();
	return B_ERROR;
}

// ramfs_sync
static
int
ramfs_sync(void */*_ns*/)
{
	FUNCTION_START();
	return B_OK;
}


// #pragma mark - VNodes


// ramfs_read_vnode
static
int
ramfs_read_vnode(void *ns, vnode_id vnid, char /*reenter*/, void **node)
{
//	FUNCTION_START();
FUNCTION(("node: %Ld\n", vnid));
	Volume *volume = (Volume*)ns;
	Node *foundNode = NULL;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		error = volume->FindNode(vnid, &foundNode);
		if (error == B_OK)
			*node = foundNode;
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_write_vnode
static
int
ramfs_write_vnode(void */*ns*/, void *DARG(_node), char /*reenter*/)
{
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	FUNCTION_START();
//FUNCTION(("node: %Ld\n", ((Node*)_node)->GetID()));
	status_t error = B_OK;
	RETURN_ERROR(error);
}

// ramfs_remove_vnode
static
int
ramfs_remove_vnode(void *ns, void *_node, char /*reenter*/)
{
FUNCTION(("node: %Ld\n", ((Node*)_node)->GetID()));
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeWriteLocker locker = volume) {
		volume->NodeRemoved(node);
		delete node;
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


// #pragma mark - Nodes


// ramfs_walk
static
int
ramfs_walk(void *ns, void *_dir, const char *entryName, char **resolvedPath,
		   vnode_id *vnid)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
FUNCTION(("dir: (%Lu), entry: `%s'\n", (dir ? dir->GetID() : -1), entryName));
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		Node *node = NULL;
		// check for non-directories
		if (!dir) {
			error = B_NOT_A_DIRECTORY;
		// special entries: "." and ".."
		} else if (!strcmp(entryName, ".")) {
			*vnid = dir->GetID();
			if (volume->GetVNode(*vnid, &node) != B_OK)
				error = B_BAD_VALUE;
		} else if (!strcmp(entryName, "..")) {
			Directory *parent = dir->GetParent();
			if (parent && volume->GetVNode(parent->GetID(), &node) == B_OK)
				*vnid = node->GetID();
			else
				error = B_BAD_VALUE;
		// ordinary entries
		} else {
			// find the entry
			error = dir->FindAndGetNode(entryName, &node);
SET_ERROR(error, error);
			if (error == B_OK)
				*vnid = node->GetID();
			// if it is a symlink, resolve it, if desired
			if (error == B_OK && resolvedPath && node->IsSymLink()) {
				SymLink *symLink = dynamic_cast<SymLink*>(node);
				*resolvedPath = strdup(symLink->GetLinkedPath());
				if (!*resolvedPath)
					SET_ERROR(error, B_NO_MEMORY);
				volume->PutVNode(*vnid);
			}
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_ioctl
static
int
ramfs_ioctl(void *ns, void */*_node*/, void */*_cookie*/, int cmd,
			void *buffer, size_t /*bufferSize*/)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	switch (cmd) {
		case RAMFS_IOCTL_GET_ALLOCATION_INFO:
		{
			if (buffer) {
				if (VolumeReadLocker locker = volume) {
					AllocationInfo *info = (AllocationInfo*)buffer;
					volume->GetAllocationInfo(*info);
				} else
					SET_ERROR(error, B_ERROR);
			} else
				SET_ERROR(error, B_BAD_VALUE);
			break;
		}
		case RAMFS_IOCTL_DUMP_INDEX:
		{
			if (buffer) {
				if (VolumeReadLocker locker = volume) {
					const char *name = (const char*)buffer;
PRINT(("  RAMFS_IOCTL_DUMP_INDEX, `%s'\n", name));
					IndexDirectory *indexDir = volume->GetIndexDirectory();
					if (indexDir) {
						if (Index *index = indexDir->FindIndex(name))
							index->Dump();
						else
							SET_ERROR(error, B_ENTRY_NOT_FOUND);
					} else
						SET_ERROR(error, B_ENTRY_NOT_FOUND);
				} else
					SET_ERROR(error, B_ERROR);
			} else
				SET_ERROR(error, B_BAD_VALUE);
			break;
		}
		default:
			error = B_BAD_VALUE;
			break;
	}
	RETURN_ERROR(error);
}

// ramfs_setflags
static
int
ramfs_setflags(void */*ns*/, void */*_node*/, void */*_cookie*/, int /*flags*/)
{
	FUNCTION_START();
// TODO:...
	return B_OK;
}

// ramfs_fsync
static
int
ramfs_fsync(void */*ns*/, void */*_node*/)
{
	FUNCTION_START();
	return B_OK;
}

// ramfs_read_stat
static
int
ramfs_read_stat(void *ns, void *_node, struct stat *st)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
FUNCTION(("node: %Ld\n", node->GetID()));
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		st->st_dev = volume->GetID();
		st->st_ino = node->GetID();
		st->st_mode = node->GetMode();
		st->st_nlink = node->GetRefCount();
		st->st_uid = node->GetUID();
		st->st_gid = node->GetGID();
		st->st_size = node->GetSize();
		st->st_blksize = kOptimalIOSize;
		st->st_atime = node->GetATime();
		st->st_mtime = node->GetMTime();
		st->st_ctime = node->GetCTime();
		st->st_crtime = node->GetCrTime();
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_write_stat
static
int
ramfs_write_stat(void *ns, void *_node, struct stat *st, long mask)
{
	FUNCTION(("mask: %lx\n", mask));
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(node);
		// check permissions
		error = node->CheckPermissions(ACCESS_W);
		// size
		if (error == B_OK && (mask & WSTAT_SIZE))
			error = node->SetSize(st->st_size);
		if (error == B_OK) {
			// permissions
			if (mask & WSTAT_MODE) {
				node->SetMode(node->GetMode() & ~S_IUMSK
							  | st->st_mode & S_IUMSK);
			}
			// UID
			if (mask & WSTAT_UID)
				node->SetUID(st->st_uid);
			// GID
			if (mask & WSTAT_GID)
				node->SetGID(st->st_gid);
			// mtime
			if (mask & WSTAT_MTIME)
				node->SetMTime(st->st_mtime);
			// crtime
			if (mask & WSTAT_CRTIME)
				node->SetCrTime(st->st_crtime);
		}
		// notify listeners
		if (error == B_OK)
			notify_if_stat_changed(volume, node);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


// #pragma mark - Files


// FileCookie
class FileCookie {
public:
	FileCookie(int openMode) : fOpenMode(openMode), fLastNotificationTime(0) {}

	inline int GetOpenMode()				{ return fOpenMode; }

	inline bigtime_t GetLastNotificationTime()
		{ return fLastNotificationTime; }

	inline bool NotificationIntervalElapsed(bool set = false)
	{
		bigtime_t currentTime = system_time();
		bool result = (currentTime - fLastNotificationTime
					   > kNotificationInterval);
		if (set && result)
			fLastNotificationTime = currentTime;
		return result;
	}

private:
	int			fOpenMode;
	bigtime_t	fLastNotificationTime;
};

// ramfs_create
static
int
ramfs_create(void *ns, void *_dir, const char *name, int openMode,
			 int mode, vnode_id *vnid, void **_cookie)
{
//	FUNCTION_START();
	FUNCTION(("name: `%s', open mode: %x, mode: %x\n", name, openMode, mode));
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
	status_t error = B_OK;
	// check name
	if (!name || *name == '\0') {
		SET_ERROR(error, B_BAD_VALUE);
	// check directory
	} else if (!dir) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(dir);
		// directory deleted?
		if (is_vnode_removed(volume->GetID(), dir->GetID()) > 0)
			SET_ERROR(error, B_NOT_ALLOWED);
		// create the file cookie
		FileCookie *cookie = NULL;
		if (error == B_OK) {
			cookie = new(std::nothrow) FileCookie(openMode);
			if (!cookie)
				SET_ERROR(error, B_NO_MEMORY);
		}
		Node *node = NULL;
		if (error == B_OK) {
			// check if entry does already exist
			if (dir->FindNode(name, &node) == B_OK) {
				// entry does already exist
				// fail, if we shall fail, when the file exists
				if (openMode & O_EXCL) {
					SET_ERROR(error, B_FILE_EXISTS);
				// don't create a file over an existing directory or symlink
				} else if (!node->IsFile()) {
					SET_ERROR(error, B_NOT_ALLOWED);
				// the user must have write permission for an existing entry
				} else if ((error = node->CheckPermissions(ACCESS_W))
						   == B_OK) {
					// truncate, if requested
					if (openMode & O_TRUNC)
						error = node->SetSize(0);
					// we ignore the supplied permissions in this case
					// get vnode
					if (error == B_OK) {
						*vnid = node->GetID();
						error = volume->GetVNode(node->GetID(), &node);
					}
				}
			// the user must have dir write permission to create a new entry
			} else if ((error = dir->CheckPermissions(ACCESS_W)) == B_OK) {
				// entry doesn't exist: create a file
				File *file = NULL;
				error = dir->CreateFile(name, &file);
				if (error == B_OK) {
					node = file;
					*vnid = node->GetID();
					// set permissions, owner and group
					node->SetMode(mode);
					node->SetUID(geteuid());
					node->SetGID(getegid());
				}
			}
			// set result / cleanup on failure
			if (error == B_OK)
				*_cookie = cookie;
			else if (cookie)
				delete cookie;
		}
		NodeMTimeUpdater mTimeUpdater2(node);
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_CREATED, volume->GetID(), dir->GetID(), 0,
							*vnid, name);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_open
static
int
ramfs_open(void *ns, void *_node, int openMode, void **_cookie)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
FUNCTION(("node: %Ld\n", node->GetID()));
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// directory can be opened read-only
		if (node->IsDirectory() && (openMode & O_RWMASK))
			openMode &= ~O_RWMASK;
		int accessMode = open_mode_to_access(openMode);
		// truncating requires write permission
		if (error == B_OK && (openMode & O_TRUNC))
			accessMode |= ACCESS_W;
		// check open mode against permissions
		if (error == B_OK)
			error = node->CheckPermissions(accessMode);
		// create the cookie
		FileCookie *cookie = NULL;
		if (error == B_OK) {
			cookie = new(std::nothrow) FileCookie(openMode);
			if (!cookie)
				SET_ERROR(error, B_NO_MEMORY);
		}
		// truncate if requested
		if (error == B_OK && (openMode & O_TRUNC))
			error = node->SetSize(0);
		NodeMTimeUpdater mTimeUpdater(node);
		// set result / cleanup on failure
		if (error == B_OK)
			*_cookie = cookie;
		else if (cookie)
			delete cookie;
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_close
static
int
ramfs_close(void *ns, void *_node, void */*_cookie*/)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
FUNCTION(("node: %Ld\n", node->GetID()));
	status_t error = B_OK;
	// notify listeners
	if (VolumeReadLocker locker = volume) {
		notify_if_stat_changed(volume, node);
	} else
		SET_ERROR(error, B_ERROR);
	return B_OK;

}

// ramfs_free_cookie
static
int
ramfs_free_cookie(void */*ns*/, void */*node*/, void *_cookie)
{
	FUNCTION_START();
	FileCookie *cookie = (FileCookie*)_cookie;
	delete cookie;
	return B_OK;
}

// ramfs_read
static
int
ramfs_read(void *ns, void *_node, void *_cookie, off_t pos, void *buffer,
		   size_t *bufferSize)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	FileCookie *cookie = (FileCookie*)_cookie;
//	FUNCTION(("((%lu, %lu), %Ld, %p, %lu)\n", node->GetDirID(),
//			  node->GetObjectID(), pos, buffer, *bufferSize));
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// don't read anything but files
		if (!node->IsFile())
			SET_ERROR(error, B_BAD_VALUE);
		// check, if reading is allowed
		int rwMode = cookie->GetOpenMode() & O_RWMASK;
		if (error == B_OK && rwMode != O_RDONLY && rwMode != O_RDWR)
			SET_ERROR(error, B_FILE_ERROR);
		// read
		if (error == B_OK) {
			if (File *file = dynamic_cast<File*>(node))
				error = file->ReadAt(pos, buffer, *bufferSize, bufferSize);
			else {
				FATAL(("Node %Ld pretends to be a File, but isn't!\n",
					   node->GetID()));
				error = B_BAD_VALUE;
			}
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_write
static
int
ramfs_write(void *ns, void *_node, void *_cookie, off_t pos,
			const void *buffer, size_t *bufferSize)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	FileCookie *cookie = (FileCookie*)_cookie;
//	FUNCTION(("((%lu, %lu), %Ld, %p, %lu)\n", node->GetDirID(),
//			  node->GetObjectID(), pos, buffer, *bufferSize));
	status_t error = B_OK;
	if (VolumeWriteLocker locker = volume) {
		// don't write anything but files
		if (!node->IsFile())
			SET_ERROR(error, B_BAD_VALUE);
		if (error == B_OK) {
			// check, if reading is allowed
			int rwMode = cookie->GetOpenMode() & O_RWMASK;
			if (error == B_OK && rwMode != O_WRONLY && rwMode != O_RDWR)
				SET_ERROR(error, B_FILE_ERROR);
			if (error == B_OK) {
				// reset the position, if opened in append mode
				if (cookie->GetOpenMode() & O_APPEND)
					pos = node->GetSize();
				// write
				if (File *file = dynamic_cast<File*>(node)) {
					error = file->WriteAt(pos, buffer, *bufferSize,
										  bufferSize);
				} else {
					FATAL(("Node %Ld pretends to be a File, but isn't!\n",
						   node->GetID()));
					error = B_BAD_VALUE;
				}
			}
		}
		// notify listeners
		if (error == B_OK && cookie->NotificationIntervalElapsed(true))
			notify_if_stat_changed(volume, node);
		NodeMTimeUpdater mTimeUpdater(node);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_access
static
int
ramfs_access(void *ns, void *_node, int mode)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		error = node->CheckPermissions(mode);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}



// #pragma mark - Directories


// ramfs_rename
static
int
ramfs_rename(void *ns, void *_oldDir, const char *oldName,
			 void *_newDir, const char *newName)
{
	Volume *volume = (Volume*)ns;
	Directory *oldDir = dynamic_cast<Directory*>((Node*)_oldDir);
	Directory *newDir = dynamic_cast<Directory*>((Node*)_newDir);
	status_t error = B_OK;

	// check name
	if (!oldName || *oldName == '\0'
		|| !strcmp(oldName, ".")  || !strcmp(oldName, "..")
		|| !newName || *newName == '\0'
		|| !strcmp(newName, ".")  || !strcmp(newName, "..")) {
		SET_ERROR(error, B_BAD_VALUE);

	// check nodes
	} else if (!oldDir || !newDir) {
		SET_ERROR(error, B_BAD_VALUE);

	// check if the entry isn't actually moved or renamed
	} else if (oldDir == newDir && !strcmp(oldName, newName)) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
FUNCTION(("old dir: %Ld, old name: `%s', new dir: %Ld, new name: `%s'\n",
oldDir->GetID(), oldName, newDir->GetID(), newName));
		NodeMTimeUpdater mTimeUpdater1(oldDir);
		NodeMTimeUpdater mTimeUpdater2(newDir);

		// target directory deleted?
		if (is_vnode_removed(volume->GetID(), newDir->GetID()) > 0)
			SET_ERROR(error, B_NOT_ALLOWED);

		// check directory write permissions
		if (error == B_OK)
			error = oldDir->CheckPermissions(ACCESS_W);
		if (error == B_OK)
			error = newDir->CheckPermissions(ACCESS_W);

		Node *node = NULL;
		Entry *entry = NULL;
		if (error == B_OK) {
			// check if entry exists
			if (oldDir->FindAndGetNode(oldName, &node, &entry) != B_OK) {
				SET_ERROR(error, B_ENTRY_NOT_FOUND);
			} else {
				if (oldDir != newDir) {
					// check whether the entry is a descendent of the target
					// directory
					for (Directory *parent = newDir;
						 parent;
						 parent = parent->GetParent()) {
						if (parent == node) {
							error = B_BAD_VALUE;
							break;
						} else if (parent == oldDir)
							break;
					}
				}
			}

			// check the target directory situation
			Node *clobberNode = NULL;
			Entry *clobberEntry = NULL;
			if (error == B_OK) {
				if (newDir->FindAndGetNode(newName, &clobberNode,
										   &clobberEntry) == B_OK) {
					if (clobberNode->IsDirectory()
						&& !dynamic_cast<Directory*>(clobberNode)->IsEmpty()) {
						SET_ERROR(error, B_NAME_IN_USE);
					}
				}
			}

			// do the job
			if (error == B_OK) {
				// temporarily acquire an additional reference to make
				// sure the node isn't deleted when we remove the entry
				error = node->AddReference();
				if (error == B_OK) {
					// delete the original entry
					error = oldDir->DeleteEntry(entry);
					if (error == B_OK) {
						// create the new one/relink the target entry
						if (clobberEntry)
							error = clobberEntry->Link(node);
						else
							error = newDir->CreateEntry(node, newName);

						if (error == B_OK) {
							// send a "removed" notification for the clobbered
							// entry
							if (clobberEntry) {
								notify_listener(B_ENTRY_REMOVED,
									volume->GetID(), newDir->GetID(), 0,
									clobberNode->GetID(), newName);
							}
						} else {
							// try to recreate the original entry, in case of
							// failure
							newDir->CreateEntry(node, oldName);
						}
					}
					node->RemoveReference();
				}
			}

			// release the entries
			if (clobberEntry)
				volume->PutVNode(clobberNode);
			if (entry)
				volume->PutVNode(node);
		}

		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_MOVED, volume->GetID(), oldDir->GetID(),
							newDir->GetID(), node->GetID(), newName);
		}
	} else
		SET_ERROR(error, B_ERROR);

	RETURN_ERROR(error);
}

// ramfs_link
static
int
ramfs_link(void *ns, void *_dir, const char *name, void *_node)
{
	FUNCTION(("name: `%s'\n", name));
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
	Node *node = (Node*)_node;
	status_t error = B_OK;
	// check directory
	if (!dir) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(dir);
		// directory deleted?
		if (is_vnode_removed(volume->GetID(), dir->GetID()) > 0)
			SET_ERROR(error, B_NOT_ALLOWED);
		// check directory write permissions
		error = dir->CheckPermissions(ACCESS_W);
		Entry *entry = NULL;
		if (error == B_OK) {
			// check if entry does already exist
			if (dir->FindEntry(name, &entry) == B_OK) {
				SET_ERROR(error, B_FILE_EXISTS);
			} else {
				// entry doesn't exist: create a link
				error = dir->CreateEntry(node, name);
			}
		}
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_CREATED, volume->GetID(), dir->GetID(), 0,
							node->GetID(), name);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_unlink
static
int
ramfs_unlink(void *ns, void *_dir, const char *name)
{
	FUNCTION(("name: `%s'\n", name));
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
	status_t error = B_OK;
	// check name
	if (!name || *name == '\0' || !strcmp(name, ".") || !strcmp(name, "..")) {
		SET_ERROR(error, B_BAD_VALUE);
	// check node
	} else if (!dir) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(dir);
		// check directory write permissions
		error = dir->CheckPermissions(ACCESS_W);
		vnode_id nodeID = -1;
		if (error == B_OK) {
			// check if entry exists
			Node *node = NULL;
			Entry *entry = NULL;
			if (dir->FindAndGetNode(name, &node, &entry) == B_OK) {
				nodeID = node->GetID();
				// unlink the entry, if it isn't a non-empty directory
				if (node->IsDirectory()
					&& !dynamic_cast<Directory*>(node)->IsEmpty()) {
					SET_ERROR(error, B_DIRECTORY_NOT_EMPTY);
				} else
					error = dir->DeleteEntry(entry);
				volume->PutVNode(node);
			} else
				SET_ERROR(error, B_ENTRY_NOT_FOUND);
		}
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_REMOVED, volume->GetID(), dir->GetID(), 0,
							nodeID, NULL);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_rmdir
static
int
ramfs_rmdir(void *ns, void *_dir, const char *name)
{
	FUNCTION(("name: `%s'\n", name));
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
	status_t error = B_OK;
	// check name
	if (!name || *name == '\0' || !strcmp(name, ".") || !strcmp(name, "..")) {
		SET_ERROR(error, B_BAD_VALUE);
	// check node
	} else if (!dir) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(dir);
		// check directory write permissions
		error = dir->CheckPermissions(ACCESS_W);
		vnode_id nodeID = -1;
		if (error == B_OK) {
			// check if entry exists
			Node *node = NULL;
			Entry *entry = NULL;
			if (dir->FindAndGetNode(name, &node, &entry) == B_OK) {
				nodeID = node->GetID();
				if (!node->IsDirectory()) {
					SET_ERROR(error, B_NOT_A_DIRECTORY);
				} else if (!dynamic_cast<Directory*>(node)->IsEmpty()) {
					SET_ERROR(error, B_DIRECTORY_NOT_EMPTY);
				} else
					error = dir->DeleteEntry(entry);
				volume->PutVNode(node);
			} else
				SET_ERROR(error, B_ENTRY_NOT_FOUND);
		}
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_REMOVED, volume->GetID(), dir->GetID(), 0,
							nodeID, NULL);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// DirectoryCookie
class DirectoryCookie {
public:
	DirectoryCookie(Directory *directory = NULL)
		: fIterator(directory),
		  fDotIndex(DOT_INDEX),
		  // debugging
		  fIteratorID(atomic_add(&fNextIteratorID, 1)),
		  fGetNextCounter(0)
	{
	}

	void Unset() { fIterator.Unset(); }

//	EntryIterator *GetIterator() const { return &fIterator; }

	status_t GetNext(ino_t *nodeID, const char **entryName)
	{
fGetNextCounter++;
		status_t error = B_OK;
		if (fDotIndex == DOT_INDEX) {
			// "."
			Node *entry = fIterator.GetDirectory();
			*nodeID = entry->GetID();
			*entryName = ".";
			fDotIndex++;
		} else if (fDotIndex == DOT_DOT_INDEX) {
			// ".."
			Directory *dir = fIterator.GetDirectory();
			if (dir->GetParent())
				*nodeID = dir->GetParent()->GetID();
			else
				*nodeID = dir->GetID();
			*entryName = "..";
			fDotIndex++;
		} else {
			// ordinary entries
			Entry *entry = NULL;
			error = fIterator.GetNext(&entry);
			if (error == B_OK) {
				*nodeID = entry->GetNode()->GetID();
				*entryName = entry->GetName();
			}
		}
PRINT(("EntryIterator %ld, GetNext() counter: %ld, entry: %p (%Ld)\n",
fIteratorID, fGetNextCounter, fIterator.GetCurrent(),
(fIterator.GetCurrent() ? fIterator.GetCurrent()->GetNode()->GetID() : -1)));
		return error;
	}

	status_t Rewind()
	{
		fDotIndex = DOT_INDEX;
		return fIterator.Rewind();
	}

	status_t Suspend() { return fIterator.Suspend(); }
	status_t Resume() { return fIterator.Resume(); }

private:
	enum {
		DOT_INDEX		= 0,
		DOT_DOT_INDEX	= 1,
		ENTRY_INDEX		= 2,
	};

private:
	EntryIterator	fIterator;
	uint32			fDotIndex;

	// debugging
	int32			fIteratorID;
	int32			fGetNextCounter;
	static vint32	fNextIteratorID;
};
vint32 DirectoryCookie::fNextIteratorID = 0;

// ramfs_mkdir
static
int
ramfs_mkdir(void *ns, void *_dir, const char *name, int mode)
{
	FUNCTION(("name: `%s', mode: %x\n", name, mode));
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
	status_t error = B_OK;
	// check name
	if (!name || *name == '\0') {
		SET_ERROR(error, B_BAD_VALUE);
	// check directory
	} else if (!dir) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(dir);
		// directory deleted?
		if (is_vnode_removed(volume->GetID(), dir->GetID()) > 0)
			SET_ERROR(error, B_NOT_ALLOWED);
		// check directory write permissions
		error = dir->CheckPermissions(ACCESS_W);
		Node *node = NULL;
		if (error == B_OK) {
			// check if entry does already exist
			if (dir->FindNode(name, &node) == B_OK) {
				SET_ERROR(error, B_FILE_EXISTS);
			} else {
				// entry doesn't exist: create a directory
				Directory *newDir = NULL;
				error = dir->CreateDirectory(name, &newDir);
				if (error == B_OK) {
					node = newDir;
					// set permissions, owner and group
					node->SetMode(mode);
					node->SetUID(geteuid());
					node->SetGID(getegid());
					// put the node
					volume->PutVNode(node->GetID());
				}
			}
		}
		NodeMTimeUpdater mTimeUpdater2(node);
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_CREATED, volume->GetID(), dir->GetID(), 0,
							node->GetID(), name);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_open_dir
static
int
ramfs_open_dir(void */*ns*/, void *_node, void **_cookie)
{
//	FUNCTION_START();
//	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
FUNCTION(("dir: (%Lu)\n", node->GetID()));
	// get the Directory
	status_t error = (node->IsDirectory() ? B_OK : B_BAD_VALUE);
	Directory *dir = NULL;
	if (error == B_OK) {
		dir = dynamic_cast<Directory*>(node);
		if (!dir) {
			FATAL(("Node %Ld pretends to be a Directory, but isn't!\n",
				   node->GetID()));
			error = B_BAD_VALUE;
		}
	}
	// create a DirectoryCookie
	if (error == B_OK) {
		DirectoryCookie *cookie = new(std::nothrow) DirectoryCookie(dir);
		if (cookie) {
			error = cookie->Suspend();
			if (error == B_OK)
				*_cookie = cookie;
			else
				delete cookie;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	FUNCTION_END();
	RETURN_ERROR(error);
}

// ramfs_read_dir
static
int
ramfs_read_dir(void *ns, void *DARG(_node), void *_cookie, long *count,
			   struct dirent *buffer, size_t bufferSize)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
DARG(Node *node = (Node*)_node; )
FUNCTION(("dir: (%Lu)\n", node->GetID()));
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		error = cookie->Resume();
		if (error == B_OK) {
			// read one entry
			ino_t nodeID = -1;
			const char *name = NULL;
			if (cookie->GetNext(&nodeID, &name) == B_OK) {
PRINT(("  entry: `%s'\n", name));
				size_t nameLen = strlen(name);
				// check, whether the entry fits into the buffer,
				// and fill it in
				size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
				if (length <= bufferSize) {
					buffer->d_dev = volume->GetID();
					buffer->d_ino = nodeID;
					memcpy(buffer->d_name, name, nameLen);
					buffer->d_name[nameLen] = '\0';
#if KEEP_WRONG_DIRENT_RECLEN
					buffer->d_reclen = nameLen;
#else
					buffer->d_reclen = length;
#endif
					*count = 1;
				} else {
					SET_ERROR(error, B_BUFFER_OVERFLOW);
				}
	 		} else
	 			*count = 0;
	 		cookie->Suspend();
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_rewind_dir
static
int
ramfs_rewind_dir(void */*ns*/, void */*_node*/, void *_cookie)
{
	FUNCTION_START();
	// No locking needed, since the Directory is guaranteed to live at this
	// time and for iterators there is a separate locking.
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	// no need to Resume(), iterator remains suspended
	status_t error = cookie->Rewind();
	RETURN_ERROR(error);
}

// ramfs_close_dir
static
int
ramfs_close_dir(void */*ns*/, void *DARG(_node), void *_cookie)
{
	FUNCTION_START();
FUNCTION(("dir: (%Lu)\n", ((Node*)_node)->GetID()));
	// No locking needed, since the Directory is guaranteed to live at this
	// time and for iterators there is a separate locking.
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	cookie->Unset();
	return B_OK;
}

// ramfs_free_dir_cookie
static
int
ramfs_free_dir_cookie(void */*ns*/, void */*_node*/, void *_cookie)
{
	FUNCTION_START();
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	delete cookie;
	return B_OK;
}


// #pragma mark - FS Stats


// ramfs_read_fs_stat
static
int
ramfs_read_fs_stat(void *ns, struct fs_info *info)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR | B_FS_HAS_MIME
					  | B_FS_HAS_QUERY;
		info->block_size = volume->GetBlockSize();
		info->io_size = kOptimalIOSize;
		info->total_blocks = volume->CountBlocks();
		info->free_blocks = volume->CountFreeBlocks();
		info->device_name[0] = '\0';
		strncpy(info->volume_name, volume->GetName(), sizeof(info->volume_name));
		strcpy(info->fsh_name, kFSName);
	} else
		SET_ERROR(error, B_ERROR);
	return B_OK;
}


// ramfs_write_fs_stat
static
int
ramfs_write_fs_stat(void *ns, struct fs_info *info, long mask)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	if (VolumeWriteLocker locker = volume) {
		if (mask & WFSSTAT_NAME)
			error = volume->SetName(info->volume_name);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


// #pragma mark - Symlinks


// ramfs_symlink
static
int
ramfs_symlink(void *ns, void *_dir, const char *name, const char *path)
{
	FUNCTION(("name: `%s', path: `%s'\n", name, path));
	Volume *volume = (Volume*)ns;
	Directory *dir = dynamic_cast<Directory*>((Node*)_dir);
	status_t error = B_OK;
	// check name
	if (!name || *name == '\0') {
		SET_ERROR(error, B_BAD_VALUE);
	// check directory
	} else if (!dir) {
		SET_ERROR(error, B_BAD_VALUE);
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(dir);
		// directory deleted?
		if (is_vnode_removed(volume->GetID(), dir->GetID()) > 0)
			SET_ERROR(error, B_NOT_ALLOWED);
		// check directory write permissions
		error = dir->CheckPermissions(ACCESS_W);
		Node *node = NULL;
		if (error == B_OK) {
			// check if entry does already exist
			if (dir->FindNode(name, &node) == B_OK) {
				SET_ERROR(error, B_FILE_EXISTS);
			} else {
				// entry doesn't exist: create a symlink
				SymLink *symLink = NULL;
				error = dir->CreateSymLink(name, path, &symLink);
				if (error == B_OK) {
					node = symLink;
					// set permissions, owner and group
					node->SetMode(S_IRWXU | S_IRWXG | S_IRWXO);
					node->SetUID(geteuid());
					node->SetGID(getegid());
					// put the node
					volume->PutVNode(node->GetID());
				}
			}
		}
		NodeMTimeUpdater mTimeUpdater2(node);
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ENTRY_CREATED, volume->GetID(), dir->GetID(), 0,
							node->GetID(), name);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_read_link
static
int
ramfs_read_link(void *ns, void *_node, char *buffer, size_t *bufferSize)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// read symlinks only
		if (!node->IsSymLink())
			error = B_BAD_VALUE;
		if (error == B_OK) {
			if (SymLink *symLink = dynamic_cast<SymLink*>(node)) {
				// copy the link contents
				size_t toRead = min(*bufferSize,
									symLink->GetLinkedPathLength());
				if (toRead > 0)
					memcpy(buffer, symLink->GetLinkedPath(), toRead);
				*bufferSize = toRead;
			} else {
				FATAL(("Node %Ld pretends to be a SymLink, but isn't!\n",
					   node->GetID()));
				error = B_BAD_VALUE;
			}
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


// #pragma mark - Attributes


// ramfs_open_attrdir
static
int
ramfs_open_attrdir(void *ns, void *_node, void **cookie)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// check permissions
		error = node->CheckPermissions(ACCESS_R);
		// create iterator
		AttributeIterator *iterator = NULL;
		if (error == B_OK) {
			iterator = new(std::nothrow) AttributeIterator(node);
			if (iterator)
				error = iterator->Suspend();
			else
				SET_ERROR(error, B_NO_MEMORY);
		}
		// set result / cleanup on failure
		if (error == B_OK)
			*cookie = iterator;
		else
			delete iterator;
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_close_attrdir
static
int
ramfs_close_attrdir(void */*ns*/, void */*_node*/, void *cookie)
{
	FUNCTION_START();
	// No locking needed, since the Node is guaranteed to live at this time
	// and for iterators there is a separate locking.
	AttributeIterator *iterator = (AttributeIterator*)cookie;
	iterator->Unset();
	return B_OK;
}

// ramfs_free_attrdir_cookie
static
int
ramfs_free_attrdir_cookie(void */*ns*/, void */*_node*/, void *cookie)
{
	FUNCTION_START();
	// No locking needed, since the Node is guaranteed to live at this time
	// and for iterators there is a separate locking.
	AttributeIterator *iterator = (AttributeIterator*)cookie;
	delete iterator;
	return B_OK;
}

// ramfs_rewind_attrdir
static
int
ramfs_rewind_attrdir(void */*ns*/, void */*_node*/, void *cookie)
{
	FUNCTION_START();
	// No locking needed, since the Node is guaranteed to live at this time
	// and for iterators there is a separate locking.
	AttributeIterator *iterator = (AttributeIterator*)cookie;
	// no need to Resume(), iterator remains suspended
	status_t error = iterator->Rewind();
	RETURN_ERROR(error);
}

// ramfs_read_attrdir
static
int
ramfs_read_attrdir(void *ns, void */*_node*/, void *cookie, long *count,
				   struct dirent *buffer, size_t bufferSize)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	AttributeIterator *iterator = (AttributeIterator*)cookie;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		error = iterator->Resume();
		if (error == B_OK) {
			// get next attribute
			Attribute *attribute = NULL;
			if (iterator->GetNext(&attribute) == B_OK) {
				const char *name = attribute->GetName();
				size_t nameLen = strlen(name);
				// check, whether the entry fits into the buffer,
				// and fill it in
				size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
				if (length <= bufferSize) {
					buffer->d_dev = volume->GetID();
					buffer->d_ino = -1;	// attributes don't have a node ID
					memcpy(buffer->d_name, name, nameLen);
					buffer->d_name[nameLen] = '\0';
#if KEEP_WRONG_DIRENT_RECLEN
					buffer->d_reclen = nameLen;
#else
					buffer->d_reclen = length;
#endif
					*count = 1;
				} else {
					SET_ERROR(error, B_BUFFER_OVERFLOW);
				}
			} else
				*count = 0;
	 		iterator->Suspend();
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_write_attr
static
int
ramfs_write_attr(void *ns, void *_node, const char *name, int type,
				 const void *buffer, size_t *bufferSize, off_t pos)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	// Don't allow writing the reserved attributes.
	if (name[0] == '\0' || !strcmp(name, "name")
		|| !strcmp(name, "last_modified") || !strcmp(name, "size")) {
//FUNCTION(("failed: node: %s, attribute: %s\n", node->GetName(), name));
		error = B_NOT_ALLOWED;
	} else if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(node);
		// check permissions
		error = node->CheckPermissions(ACCESS_W);
		// find the attribute or create it, if it doesn't exist yet
		Attribute *attribute = NULL;
		if (error == B_OK && node->FindAttribute(name, &attribute) != B_OK)
			error = node->CreateAttribute(name, &attribute);
REPORT_ERROR(error);
		// set the new type and write the data
		if (error == B_OK) {
			attribute->SetType(type);
			error = attribute->WriteAt(pos, buffer, *bufferSize, bufferSize);
REPORT_ERROR(error);
		}
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ATTR_CHANGED, volume->GetID(), 0, 0,
							node->GetID(), name);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_read_attr
static
int
ramfs_read_attr(void *ns, void *_node, const char *name, int /*type*/,
				void *buffer, size_t *bufferSize, off_t pos)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// check permissions
		error = node->CheckPermissions(ACCESS_R);
		// find the attribute
		Attribute *attribute = NULL;
		if (error == B_OK)
			error = node->FindAttribute(name, &attribute);
		// read
		if (error == B_OK)
			error = attribute->ReadAt(pos, buffer, *bufferSize, bufferSize);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_remove_attr
static
int
ramfs_remove_attr(void *ns, void *_node, const char *name)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeWriteLocker locker = volume) {
		NodeMTimeUpdater mTimeUpdater(node);
		// check permissions
		error = node->CheckPermissions(ACCESS_W);
		// find the attribute
		Attribute *attribute = NULL;
		if (error == B_OK)
			error = node->FindAttribute(name, &attribute);
		// delete it
		if (error == B_OK)
			error = node->DeleteAttribute(attribute);
		// notify listeners
		if (error == B_OK) {
			notify_listener(B_ATTR_CHANGED, volume->GetID(), 0, 0,
							node->GetID(), name);
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_rename_attr
static
int
ramfs_rename_attr(void */*ns*/, void */*_node*/, const char */*oldName*/,
				  const char */*newName*/)
{
	// TODO:...
	return B_ENTRY_NOT_FOUND;
}

// ramfs_stat_attr
static
int
ramfs_stat_attr(void *ns, void *_node, const char *name,
				struct attr_info *attrInfo)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	Node *node = (Node*)_node;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// check permissions
		error = node->CheckPermissions(ACCESS_R);
		// find the attribute
		Attribute *attribute = NULL;
		if (error == B_OK)
			error = node->FindAttribute(name, &attribute);
		// read
		if (error == B_OK) {
			attrInfo->type = attribute->GetType();
			attrInfo->size = attribute->GetSize();
		}
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


// #pragma mark - Indices


// IndexDirCookie
class IndexDirCookie {
public:
	IndexDirCookie() : index_index(0) {}

	int32	index_index;
};

// ramfs_open_indexdir
static
int
ramfs_open_indexdir(void *ns, void **_cookie)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// check whether an index directory exists
		if (volume->GetIndexDirectory()) {
			IndexDirCookie *cookie = new(std::nothrow) IndexDirCookie;
			if (cookie)
				*_cookie = cookie;
			else
				SET_ERROR(error, B_NO_MEMORY);
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_close_indexdir
static
int
ramfs_close_indexdir(void */*ns*/, void */*_cookie*/)
{
	FUNCTION_START();
	return B_OK;
}

// ramfs_free_indexdir_cookie
static
int
ramfs_free_indexdir_cookie(void */*ns*/, void */*_node*/, void *_cookie)
{
	FUNCTION_START();
	IndexDirCookie *cookie = (IndexDirCookie*)_cookie;
	delete cookie;
	return B_OK;
}

// ramfs_rewind_indexdir
static
int
ramfs_rewind_indexdir(void */*_ns*/, void *_cookie)
{
	FUNCTION_START();
	IndexDirCookie *cookie = (IndexDirCookie*)_cookie;
	cookie->index_index = 0;
	return B_OK;
}

// ramfs_read_indexdir
static
int
ramfs_read_indexdir(void *ns, void *_cookie, long *count,
					struct dirent *buffer, size_t bufferSize)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	IndexDirCookie *cookie = (IndexDirCookie*)_cookie;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// get the next index
		Index *index = volume->GetIndexDirectory()->IndexAt(
			cookie->index_index++);
		if (index) {
			const char *name = index->GetName();
			size_t nameLen = strlen(name);
			// check, whether the entry fits into the buffer,
			// and fill it in
			size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
			if (length <= bufferSize) {
				buffer->d_dev = volume->GetID();
				buffer->d_ino = -1;	// indices don't have a node ID
				memcpy(buffer->d_name, name, nameLen);
				buffer->d_name[nameLen] = '\0';
#if KEEP_WRONG_DIRENT_RECLEN
				buffer->d_reclen = nameLen;
#else
				buffer->d_reclen = length;
#endif
				*count = 1;
			} else {
				SET_ERROR(error, B_BUFFER_OVERFLOW);
			}
		} else
			*count = 0;
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_create_index
static
int
ramfs_create_index(void *ns, const char *name, int type, int /*flags*/)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	// only root is allowed to manipulate the indices
	if (geteuid() != 0) {
		SET_ERROR(error, B_NOT_ALLOWED);
	} else if (VolumeWriteLocker locker = volume) {
		// get the index directory
		if (IndexDirectory *indexDir = volume->GetIndexDirectory()) {
			// check whether an index with that name does already exist
			if (indexDir->FindIndex(name)) {
				SET_ERROR(error, B_FILE_EXISTS);
			} else {
				// create the index
				AttributeIndex *index;
				error = indexDir->CreateIndex(name, type, &index);
			}
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_remove_index
static
int
ramfs_remove_index(void *ns, const char *name)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	// only root is allowed to manipulate the indices
	if (geteuid() != 0) {
		SET_ERROR(error, B_NOT_ALLOWED);
	} else if (VolumeWriteLocker locker = volume) {
		// get the index directory
		if (IndexDirectory *indexDir = volume->GetIndexDirectory()) {
			// check whether an index with that name does exist
			if (Index *index = indexDir->FindIndex(name)) {
				// don't delete a special index
				if (indexDir->IsSpecialIndex(index)) {
					SET_ERROR(error, B_BAD_VALUE);
				} else
					indexDir->DeleteIndex(index);
			} else
				SET_ERROR(error, B_ENTRY_NOT_FOUND);
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}

// ramfs_rename_index
static
int
ramfs_rename_index(void */*ns*/, const char */*oldname*/,
				   const char */*newname*/)
{
	FUNCTION_START();
	return B_ERROR;
}

// ramfs_stat_index
static
int
ramfs_stat_index(void *ns, const char *name, struct index_info *indexInfo)
{
	FUNCTION_START();
	Volume *volume = (Volume*)ns;
	status_t error = B_OK;
	if (VolumeReadLocker locker = volume) {
		// get the index directory
		if (IndexDirectory *indexDir = volume->GetIndexDirectory()) {
			// find the index
			if (Index *index = indexDir->FindIndex(name)) {
				indexInfo->type = index->GetType();
				if (index->HasFixedKeyLength())
					indexInfo->size = index->GetKeyLength();
				else
					indexInfo->size = kMaxIndexKeyLength;
				indexInfo->modification_time = 0;	// TODO: index times
				indexInfo->creation_time = 0;		// ...
				indexInfo->uid = 0;		// root owns the indices
				indexInfo->gid = 0;		//
			} else
				SET_ERROR(error, B_ENTRY_NOT_FOUND);
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


// #pragma mark - Queries

// Query implementation by Axel Dörfler. Slightly adjusted.

// ramfs_open_query
int
ramfs_open_query(void *ns, const char *queryString, ulong flags, port_id port,
				 long token, void **cookie)
{
	FUNCTION_START();
	if (ns == NULL || queryString == NULL || cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	PRINT(("query = \"%s\", flags = %lu, port_id = %ld, token = %ld\n", queryString, flags, port, token));

	Volume *volume = (Volume *)ns;

	// lock the volume
	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// parse the query expression
	Expression *expression = new Expression((char *)queryString);
	if (expression == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Expression> expressionDeleter(expression);

	if (expression->InitCheck() < B_OK) {
		WARN(("Could not parse query, stopped at: \"%s\"\n",
			expression->Position()));
		RETURN_ERROR(B_BAD_VALUE);
	}

	// create the query
	Query *query = new Query(volume, expression, flags);
	if (query == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	expressionDeleter.Detach();
	// TODO: The Query references an Index, but nothing prevents the Index
	// from being deleted, while the Query is in existence.

	if (flags & B_LIVE_QUERY)
		query->SetLiveMode(port, token);

	*cookie = (void *)query;

	return B_OK;
}

// ramfs_close_query
int
ramfs_close_query(void */*ns*/, void */*cookie*/)
{
	FUNCTION_START();
	return B_OK;
}

// ramfs_free_query_cookie
int
ramfs_free_query_cookie(void *ns, void */*node*/, void *cookie)
{
	FUNCTION_START();
	if (ns == NULL || cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)ns;

	// lock the volume
	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	Query *query = (Query *)cookie;
	Expression *expression = query->GetExpression();
	delete query;
	delete expression;

	return B_OK;
}

// ramfs_read_query
int
ramfs_read_query(void *ns, void *cookie, long *count,
				 struct dirent *buffer, size_t bufferSize)
{
	FUNCTION_START();
	Query *query = (Query *)cookie;
	if (ns == NULL || query == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)ns;

	// lock the volume
	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t status = query->GetNextEntry(buffer, bufferSize);
	if (status == B_OK)
		*count = 1;
	else if (status == B_ENTRY_NOT_FOUND)
		*count = 0;
	else
		return status;

	return B_OK;
}

