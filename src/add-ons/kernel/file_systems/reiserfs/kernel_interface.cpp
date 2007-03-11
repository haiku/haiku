// kernel_interface.cpp
//
// Copyright (c) 2003-2004, Ingo Weinhold (bonefish@cs.tu-berlin.de)
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

#include <new>

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

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>

#include "DirItem.h"
#include "Iterators.h"
#include "StatItem.h"
#include "Tree.h"
#include "VNode.h"
#include "Volume.h"


static const size_t kOptimalIOSize = 65536;


inline static bool is_user_in_group(gid_t gid);


// #pragma mark - FS


// reiserfs_mount
static status_t
reiserfs_mount(mount_id nsid, const char *device, uint32 flags,
	const char *parameters, fs_volume *data, vnode_id *rootID)
{
	TOUCH(flags); TOUCH(parameters);
	FUNCTION_START();
	// parameters are ignored for now
	status_t error = B_OK;

	// allocate and init the volume
	Volume *volume = new(nothrow) Volume;
	if (!volume)
		error = B_NO_MEMORY;
	if (error == B_OK)
		error = volume->Mount(nsid, device);

	// set the results
	if (error == B_OK) {
		*rootID = volume->GetRootVNode()->GetID();
		*data = volume;
	}

	// cleanup on failure
	if (error != B_OK && volume)
		delete volume;
	RETURN_ERROR(error);
}

// reiserfs_unmount
static status_t
reiserfs_unmount(fs_volume fs)
{
	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	status_t error = volume->Unmount();
	if (error == B_OK)
		delete volume;
	RETURN_ERROR(error);
}

// reiserfs_read_fs_info
static status_t
reiserfs_read_fs_info(fs_volume fs, struct fs_info *info)
{
	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	info->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY;
	info->block_size = volume->GetBlockSize();
	info->io_size = kOptimalIOSize;
	info->total_blocks = volume->CountBlocks();
	info->free_blocks = volume->CountFreeBlocks();
	strncpy(info->device_name, volume->GetDeviceName(),
			sizeof(info->device_name));
	strncpy(info->volume_name, volume->GetName(), sizeof(info->volume_name));
	return B_OK;
}


// #pragma mark - VNodes


// reiserfs_lookup
static status_t
reiserfs_lookup(fs_volume fs, fs_vnode _dir, const char *entryName,
	vnode_id *vnid, int *type)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *dir = (VNode*)_dir;
FUNCTION(("dir: (%Ld: %lu, %lu), entry: `%s'\n", dir->GetID(), dir->GetDirID(),
		  dir->GetObjectID(), entryName));
	status_t error = B_OK;
	VNode *entryNode = NULL;

	// check for non-directories
	if (!dir->IsDir()) {
		error = B_ENTRY_NOT_FOUND;

	// special entries: "." and ".."
	} else if (!strcmp(entryName, ".")) {
		*vnid = dir->GetID();
		if (volume->GetVNode(*vnid, &entryNode) != B_OK)
			error = B_BAD_VALUE;
	} else if (!strcmp(entryName, "..")) {
		*vnid = dir->GetParentID();
		if (volume->GetVNode(*vnid, &entryNode) != B_OK)
			error = B_BAD_VALUE;

	// ordinary entries
	} else {
		// find the entry
		VNode foundNode;
		error = volume->FindDirEntry(dir, entryName, &foundNode, true);

		// hide non-file/dir/symlink entries, if the user desires that, and
		// those entries explicitly set to hidden
		if (error == B_OK
			&& (foundNode.IsEsoteric() && volume->GetHideEsoteric()
				|| volume->IsNegativeEntry(foundNode.GetID()))) {
			error = B_ENTRY_NOT_FOUND;
		}
		if (error == B_OK) {
			*vnid = foundNode.GetID();
			error = volume->GetVNode(*vnid, &entryNode);
		}
	}

	// get type
	if (error == B_OK)
		*type = entryNode->GetStatData()->GetMode() & S_IFMT;

	RETURN_ERROR(error);
}

// reiserfs_read_vnode
static status_t
reiserfs_read_vnode(fs_volume fs, vnode_id vnid, fs_vnode *node, bool reenter)
{
	TOUCH(reenter);
//	FUNCTION_START();
	FUNCTION(("(%Ld: %lu, %ld)\n", vnid, VNode::GetDirIDFor(vnid),
			  VNode::GetObjectIDFor(vnid)));
	Volume *volume = (Volume*)fs;
	status_t error = B_OK;
	VNode *foundNode = new(nothrow) VNode;
	if (foundNode) {
		error = volume->FindVNode(vnid, foundNode);
		if (error == B_OK)
			*node = foundNode;
		else
			delete foundNode;;
	} else
		error = B_NO_MEMORY;
	RETURN_ERROR(error);
}

// reiserfs_write_vnode
static status_t
reiserfs_write_vnode(fs_volume fs, fs_vnode _node, bool reenter)
{
	TOUCH(reenter);
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
	status_t error = B_OK;
	if (node != volume->GetRootVNode())
		delete node;
//	RETURN_ERROR(error);
	return error;
}


// #pragma mark - Nodes


// reiserfs_read_symlink
static status_t
reiserfs_read_symlink(fs_volume fs, fs_vnode _node, char *buffer,
	size_t *bufferSize)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	status_t error = B_OK;
	// read symlinks only
	if (!node->IsSymlink())
		error = B_BAD_VALUE;
	// read
	if (error == B_OK)
		error = volume->ReadLink(node, buffer, *bufferSize, bufferSize);
	RETURN_ERROR(error);
}

// reiserfs_access
static status_t
reiserfs_access(fs_volume fs, fs_vnode _node, int mode)
{
	TOUCH(fs);
//	FUNCTION_START();
//	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	// write access requested?
	if (mode & W_OK)
		return B_READ_ONLY_DEVICE;
	// get node permissions
	StatData *statData = node->GetStatData();
	int userPermissions = (statData->GetMode() & S_IRWXU) >> 6;
	int groupPermissions = (statData->GetMode() & S_IRWXG) >> 3;
	int otherPermissions = statData->GetMode() & S_IRWXO;
	// get the permissions for this uid/gid
	int permissions = 0;
	uid_t uid = geteuid();
	// user is root
	if (uid == 0) {
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		permissions = userPermissions | groupPermissions | otherPermissions
			| S_IROTH | S_IWOTH;
	// user is node owner
	} else if (uid == statData->GetUID())
		permissions = userPermissions;
	// user is in owning group
	else if (is_user_in_group(statData->GetGID()))
		permissions = groupPermissions;
	// user is one of the others
	else
		permissions = otherPermissions;
	// do the check
	if (mode & ~permissions)
		return B_NOT_ALLOWED;
	return B_OK;
}

// reiserfs_read_stat
static status_t
reiserfs_read_stat(fs_volume fs, fs_vnode _node, struct stat *st)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	status_t error = B_OK;
	StatData *statData = node->GetStatData();
	st->st_dev = volume->GetID();
	st->st_ino = node->GetID();
	st->st_mode = statData->GetMode();
	st->st_nlink = statData->GetNLink();
	st->st_uid = statData->GetUID();
	st->st_gid = statData->GetGID();
	st->st_size = statData->GetSize();
	st->st_blksize = kOptimalIOSize;
	st->st_atime = statData->GetATime();
	st->st_mtime = st->st_ctime = statData->GetMTime();
	st->st_crtime = statData->GetCTime();
	RETURN_ERROR(error);
}


// #pragma mark - Files


// reiserfs_open
static status_t
reiserfs_open(fs_volume fs, fs_vnode _node, int openMode, fs_cookie *cookie)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	status_t error = B_OK;
	// check the open mode
	if ((openMode & O_RWMASK) == O_WRONLY || (openMode & O_RWMASK) == O_RDWR
		|| (openMode & (O_TRUNC | O_CREAT))) {
		error = B_READ_ONLY_DEVICE;
	}
	// create a StreamReader
	if (error == B_OK) {
		StreamReader *reader = new(nothrow) StreamReader(volume->GetTree(),
			node->GetDirID(), node->GetObjectID());
		if (reader) {
			error = reader->Suspend();
			if (error == B_OK)
				*cookie = reader;
			else
				delete reader;
		} else
			error = B_NO_MEMORY;
	}
	RETURN_ERROR(error);
}

// reiserfs_close
static status_t
reiserfs_close(fs_volume fs, fs_vnode _node, fs_cookie cookie)
{
	TOUCH(fs); TOUCH(cookie);
//	FUNCTION_START();
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	TOUCH(node);
	return B_OK;
}

// reiserfs_free_cookie
static status_t
reiserfs_free_cookie(fs_volume fs, fs_vnode _node, fs_cookie cookie)
{
	TOUCH(fs);
//	FUNCTION_START();
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	TOUCH(node);
	StreamReader *reader = (StreamReader*)cookie;
	delete reader;
	return B_OK;
}

// reiserfs_read
static status_t
reiserfs_read(fs_volume fs, fs_vnode _node, fs_cookie cookie, off_t pos,
	void *buffer, size_t *bufferSize)
{
	TOUCH(fs);
//	FUNCTION_START();
//	Volume *volume = (Volume*)ns;
	VNode *node = (VNode*)_node;
	FUNCTION(("((%Ld: %lu, %lu), %Ld, %p, %lu)\n", node->GetID(),
			  node->GetDirID(), node->GetObjectID(), pos, buffer,
			  *bufferSize));
	status_t error = B_OK;
	// don't read anything but files
	if (!node->IsFile())
		error = B_BAD_VALUE;
	// read
	StreamReader *reader = (StreamReader*)cookie;
	if (error == B_OK) {
		error = reader->Resume();
		if (error == B_OK) {
			error = reader->ReadAt(pos, buffer, *bufferSize, bufferSize);
			reader->Suspend();
		}
	}
	RETURN_ERROR(error);
}

// is_user_in_group
inline static bool
is_user_in_group(gid_t gid)
{
// Either I miss something, or we don't have getgroups() in the kernel. :-(
/*
	gid_t groups[NGROUPS_MAX];
	int groupCount = getgroups(NGROUPS_MAX, groups);
	for (int i = 0; i < groupCount; i++) {
		if (gid == groups[i])
			return true;
	}
*/
	return (gid == getegid());
}

// DirectoryCookie
class DirectoryCookie : public DirEntryIterator {
public:
	DirectoryCookie(Tree *tree, uint32 dirID, uint32 objectID,
					uint64 startOffset = 0, bool fixedHash = false)
		: DirEntryIterator(tree, dirID, objectID, startOffset,
						   fixedHash),
		  fEncounteredDotDot(false)
	{
	}

	bool EncounteredDotDot() const
	{
		return fEncounteredDotDot;
	}

	void SetEncounteredDotDot(bool flag)
	{
		fEncounteredDotDot = flag;
	}

	bool	fEncounteredDotDot;
};


// #pragma mark - Directories


// reiserfs_open_dir
static status_t
reiserfs_open_dir(fs_volume fs, fs_vnode _node, fs_cookie *cookie)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	status_t error = (node->IsDir() ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		DirectoryCookie *iterator = new(nothrow) DirectoryCookie(
			volume->GetTree(), node->GetDirID(), node->GetObjectID());
		if (iterator) {
			error = iterator->Suspend();
			if (error == B_OK)
				*cookie = iterator;
			else
				delete iterator;
		} else
			error = B_NO_MEMORY;
	}
	FUNCTION_END();
	RETURN_ERROR(error);
}

// set_dirent_name
static status_t
set_dirent_name(struct dirent *buffer, size_t bufferSize,
						 const char *name, int32 nameLen)
{
	size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
	if (length <= bufferSize) {
		memcpy(buffer->d_name, name, nameLen);
		buffer->d_name[nameLen] = '\0';
		buffer->d_reclen = length;
		return B_OK;
	} else
		RETURN_ERROR(B_BUFFER_OVERFLOW);
}

// reiserfs_close_dir
static status_t
reiserfs_close_dir(void *ns, void *_node, void *cookie)
{
	TOUCH(ns); TOUCH(cookie);
//	FUNCTION_START();
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	TOUCH(node);
	return B_OK;
}

// reiserfs_free_dir_cookie
static status_t
reiserfs_free_dir_cookie(void *ns, void *_node, void *cookie)
{
	TOUCH(ns);
//	FUNCTION_START();
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	TOUCH(node);
	DirectoryCookie *iterator = (DirectoryCookie*)cookie;
	delete iterator;
	return B_OK;
}
			
// reiserfs_read_dir
static status_t
reiserfs_read_dir(fs_volume fs, fs_vnode _node, fs_cookie cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs;
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	DirectoryCookie *iterator = (DirectoryCookie*)cookie;
	status_t error = iterator->Resume();
	if (error == B_OK) {
		// read one entry
		DirItem item;
		int32 index = 0;
		DirEntry *entry = NULL;
		bool done = false;
		while (error == B_OK && !done
			   && (error = iterator->GetNext(&item, &index, &entry)) == B_OK) {
			uint32 dirID = entry->GetDirID();
			uint32 objectID = entry->GetObjectID();
			// skip hidden entries and entries the user specified to be hidden
			if (entry->IsHidden() || volume->IsNegativeEntry(dirID, objectID))
				continue;
			// skip entry, if we can't get the stat data, or it is neither a
			// file, a dir nor a symlink and the user desired to hide those.
			StatData statData;
			StatItem statItem;
			if (volume->GetTree()->FindStatItem(dirID, objectID, &statItem)
					!= B_OK
				|| statItem.GetStatData(&statData) != B_OK
				|| statData.IsEsoteric() && volume->GetHideEsoteric()) {
				continue;
			}
			if (error == B_OK) {
				// get the name
				size_t nameLen = 0;
				const char *name = item.EntryNameAt(index, &nameLen);
				if (!name || nameLen == 0)	// bad data: skip it gracefully
					continue;
				// fill in the entry name -- checks whether the
				// entry fits into the buffer
				error = set_dirent_name(buffer, bufferSize, name,
										nameLen);
				if (error == B_OK) {
					// fill in the other data
					buffer->d_dev = volume->GetID();
					buffer->d_ino = VNode::GetIDFor(dirID, objectID);
					*count = 1;
PRINT(("Successfully read entry: dir: (%Ld: %ld, %ld), name: `%s', "
	   "id: (%Ld, %ld, %ld), reclen: %hu\n", node->GetID(), node->GetDirID(),
	   node->GetObjectID(), buffer->d_name, buffer->d_ino, dirID, objectID,
	   buffer->d_reclen));
					if (!strcmp("..", buffer->d_name))
						iterator->SetEncounteredDotDot(true);
					done = true;
				}
	 		}
 		}
 		if (error == B_ENTRY_NOT_FOUND) {
 			if (iterator->EncounteredDotDot()) {
	 			error = B_OK;
				*count = 0;
			} else {
				// this is necessary for the root directory
				// it usually has no ".." entry, so we simulate one
				// get the name
				const char *name = "..";
				size_t nameLen = strlen(name);
				// fill in the entry name -- checks whether the
				// entry fits into the buffer
				error = set_dirent_name(buffer, bufferSize, name,
										nameLen);
				if (error == B_OK) {
					// fill in the other data
					buffer->d_dev = volume->GetID();
					buffer->d_ino = node->GetID();
	// < That's not correct!
					*count = 1;
PRINT(("faking `..' entry: dir: (%Ld: %ld, %ld), name: `%s', "
	   "id: (%Ld, %ld, %ld), reclen: %hu\n", node->GetID(), node->GetDirID(),
	   node->GetObjectID(), buffer->d_name, buffer->d_ino, node->GetDirID(),
	   node->GetObjectID(), buffer->d_reclen));
					iterator->SetEncounteredDotDot(true);
				}
			}
 		}
 		iterator->Suspend();
	}
PRINT(("returning %ld entries\n", *count));
	RETURN_ERROR(error);
}

// reiserfs_rewind_dir
static status_t
reiserfs_rewind_dir(fs_volume fs, fs_vnode _node, fs_cookie cookie)
{
	TOUCH(fs);
//	FUNCTION_START();
	VNode *node = (VNode*)_node;
FUNCTION(("node: (%Ld: %lu, %lu)\n", node->GetID(), node->GetDirID(),
		  node->GetObjectID()));
	TOUCH(node);
	DirectoryCookie *iterator = (DirectoryCookie*)cookie;
	status_t error = iterator->Rewind();	// no need to Resume()
	if (error == B_OK)
		error = iterator->Suspend();
	RETURN_ERROR(error);
}


// #pragma mark - Module Interface


// reiserfs_std_ops
static status_t
reiserfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			init_debugging();
			PRINT(("reiserfs_std_ops(): B_MODULE_INIT\n"));
			return B_OK;
		}

		case B_MODULE_UNINIT:
			PRINT(("reiserfs_std_ops(): B_MODULE_UNINIT\n"));
			exit_debugging();
			return B_OK;

		default:
			return B_ERROR;
	}
}

static file_system_module_info sReiserFSModuleInfo = {
	{
		"file_systems/reiserfs" B_CURRENT_FS_API_VERSION,
		0,
		reiserfs_std_ops,
	},

	"Reiser File System",

	// scanning
	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	&reiserfs_mount,
	&reiserfs_unmount,
	&reiserfs_read_fs_info,
	NULL,	// &reiserfs_write_fs_info,
	NULL,	// &reiserfs_sync,

	/* vnode operations */
	&reiserfs_lookup,
	NULL,	// &reiserfs_get_vnode_name,
	&reiserfs_read_vnode,
	&reiserfs_write_vnode,
	NULL,	// &reiserfs_remove_vnode,

	/* VM file access */
	NULL,	// &reiserfs_can_page,
	NULL,	// &reiserfs_read_pages,
	NULL,	// &reiserfs_write_pages,

	NULL,	// &reiserfs_get_file_map,

	NULL,	// &reiserfs_ioctl,
	NULL,	// &reiserfs_set_flags,
	NULL,	// &reiserfs_select,
	NULL,	// &reiserfs_deselect,
	NULL,	// &reiserfs_fsync,

	&reiserfs_read_symlink,
	NULL,	// &reiserfs_create_symlink,

	NULL,	// &reiserfs_link,
	NULL,	// &reiserfs_unlink,
	NULL,	// &reiserfs_rename,

	&reiserfs_access,
	&reiserfs_read_stat,
	NULL,	// &reiserfs_write_stat,

	/* file operations */
	NULL,	// &reiserfs_create,
	&reiserfs_open,
	&reiserfs_close,
	&reiserfs_free_cookie,
	&reiserfs_read,
	NULL,	// &reiserfs_write,

	/* directory operations */
	NULL,	// &reiserfs_create_dir,
	NULL,	// &reiserfs_remove_dir,
	&reiserfs_open_dir,
	&reiserfs_close_dir,
	&reiserfs_free_dir_cookie,
	&reiserfs_read_dir,
	&reiserfs_rewind_dir,
	
	/* attribute directory operations */
	NULL,	// &reiserfs_open_attr_dir,
	NULL,	// &reiserfs_close_attr_dir,
	NULL,	// &reiserfs_free_attr_dir_cookie,
	NULL,	// &reiserfs_read_attr_dir,
	NULL,	// &reiserfs_rewind_attr_dir,

	/* attribute operations */
	NULL,	// &reiserfs_create_attr,
	NULL,	// &reiserfs_open_attr,
	NULL,	// &reiserfs_close_attr,
	NULL,	// &reiserfs_free_attr_cookie,
	NULL,	// &reiserfs_read_attr,
	NULL,	// &reiserfs_write_attr,

	NULL,	// &reiserfs_read_attr_stat,
	NULL,	// &reiserfs_write_attr_stat,
	NULL,	// &reiserfs_rename_attr,
	NULL,	// &reiserfs_remove_attr,

	/* index directory & index operations */
	NULL,	// &reiserfs_open_index_dir,
	NULL,	// &reiserfs_close_index_dir,
	NULL,	// &reiserfs_free_index_dir_cookie,
	NULL,	// &reiserfs_read_index_dir,
	NULL,	// &reiserfs_rewind_index_dir,

	NULL,	// &reiserfs_create_index,
	NULL,	// &reiserfs_remove_index,
	NULL,	// &reiserfs_read_index_stat,

	/* query operations */
	NULL,	// &reiserfs_open_query,
	NULL,	// &reiserfs_close_query,
	NULL,	// &reiserfs_free_query_cookie,
	NULL,	// &reiserfs_read_query,
	NULL,	// &reiserfs_rewind_query,
};

module_info *modules[] = {
	(module_info *)&sReiserFSModuleInfo,
	NULL,
};
