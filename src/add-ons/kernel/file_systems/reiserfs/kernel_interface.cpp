// kernel_interface.cpp
//
// Copyright (c) 2003-2010, Ingo Weinhold (bonefish@cs.tu-berlin.de)
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

#include <fs_cache.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>

#include "DirItem.h"
#include "Iterators.h"
#include "StatItem.h"
#include "Tree.h"
#include "VNode.h"
#include "Volume.h"


using std::nothrow;

static const size_t kOptimalIOSize = 65536;

extern fs_volume_ops gReiserFSVolumeOps;
extern fs_vnode_ops gReiserFSVnodeOps;


// #pragma mark - FS


// reiserfs_identify_partition
static float
reiserfs_identify_partition(int fd, partition_data *partition, void **cookie)
{
	Volume* volume = new(nothrow) Volume();
	if (volume == NULL)
		return -1.0;

	status_t status = volume->Identify(fd, partition);
	if (status != B_OK) {
		delete volume;
		return -1.0;
	}

	*cookie = (void*)volume;
	return 0.8;
}


// reiserfs_scan_partition
static status_t
reiserfs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	Volume* volume = (Volume*)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = volume->CountBlocks()
		* volume->GetBlockSize();
	partition->block_size = volume->GetBlockSize();

	volume->UpdateName(partition->id);
		// must be done after setting the content size
	partition->content_name = strdup(volume->GetName());
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


// reiserfs_free_identify_partition_cookie
static void
reiserfs_free_identify_partition_cookie(partition_data* partition,
	void* _cookie)
{
	delete (Volume*)_cookie;
}


//	#pragma mark -


// reiserfs_mount
static status_t
reiserfs_mount(fs_volume *_volume, const char *device, uint32 flags,
	const char *parameters, ino_t *rootID)
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
		error = volume->Mount(_volume, device);

	// set the results
	if (error == B_OK) {
		*rootID = volume->GetRootVNode()->GetID();
		_volume->private_volume = volume;
		_volume->ops = &gReiserFSVolumeOps;
	}

	// cleanup on failure
	if (error != B_OK && volume)
		delete volume;
	RETURN_ERROR(error);
}


// reiserfs_unmount
static status_t
reiserfs_unmount(fs_volume* fs)
{
	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	status_t error = volume->Unmount();
	if (error == B_OK)
		delete volume;
	RETURN_ERROR(error);
}

// reiserfs_read_fs_info
static status_t
reiserfs_read_fs_info(fs_volume* fs, struct fs_info *info)
{
	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	info->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY;
	info->block_size = volume->GetBlockSize();
	info->io_size = kOptimalIOSize;
	info->total_blocks = volume->CountBlocks();
	info->free_blocks = volume->CountFreeBlocks();
	strlcpy(info->device_name, volume->GetDeviceName(),
			sizeof(info->device_name));
	strlcpy(info->volume_name, volume->GetName(), sizeof(info->volume_name));
	return B_OK;
}


// #pragma mark - VNodes


// reiserfs_lookup
static status_t
reiserfs_lookup(fs_volume* fs, fs_vnode* _dir, const char *entryName,
	ino_t *vnid)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *dir = (VNode*)_dir->private_node;
	FUNCTION(("dir: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 "), "
			"entry: `%s'\n",
		dir->GetID(), dir->GetDirID(), dir->GetObjectID(),
		entryName));
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
			&& ((foundNode.IsEsoteric() && volume->GetHideEsoteric())
				|| volume->IsNegativeEntry(foundNode.GetID()))) {
			error = B_ENTRY_NOT_FOUND;
		}
		if (error == B_OK) {
			*vnid = foundNode.GetID();
			error = volume->GetVNode(*vnid, &entryNode);
		}
	}

	// add to the entry cache
	if (error == B_OK) {
		entry_cache_add(volume->GetID(), dir->GetID(), entryName,
			*vnid);
	}

	RETURN_ERROR(error);
}

// reiserfs_read_vnode
static status_t
reiserfs_read_vnode(fs_volume *fs, ino_t vnid, fs_vnode *node, int *_type,
	uint32 *_flags, bool reenter)
{
	TOUCH(reenter);
//	FUNCTION_START();
	FUNCTION(("(%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		vnid, VNode::GetDirIDFor(vnid), VNode::GetObjectIDFor(vnid)));
	Volume *volume = (Volume*)fs->private_volume;
	status_t error = B_OK;
	VNode *foundNode = new(nothrow) VNode;
	if (foundNode) {
		error = volume->FindVNode(vnid, foundNode);
		if (error == B_OK) {
			node->private_node = foundNode;
			node->ops = &gReiserFSVnodeOps;
			*_type = foundNode->GetStatData()->GetMode() & S_IFMT;
			*_flags = 0;
		} else
			delete foundNode;
	} else
		error = B_NO_MEMORY;
	RETURN_ERROR(error);
}

// reiserfs_write_vnode
static status_t
reiserfs_write_vnode(fs_volume *fs, fs_vnode *_node, bool reenter)
{
	TOUCH(reenter);
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	status_t error = B_OK;
	if (node != volume->GetRootVNode())
		delete node;
//	RETURN_ERROR(error);
	return error;
}


// #pragma mark - Nodes


// reiserfs_read_symlink
static status_t
reiserfs_read_symlink(fs_volume *fs, fs_vnode *_node, char *buffer,
	size_t *bufferSize)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
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
reiserfs_access(fs_volume *fs, fs_vnode *_node, int mode)
{
	TOUCH(fs);
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));

	// write access requested?
	if (mode & W_OK)
		return B_READ_ONLY_DEVICE;

	// get node permissions
	StatData *statData = node->GetStatData();

	return check_access_permissions(mode, statData->GetMode(),
		statData->GetGID(), statData->GetUID());
}

// reiserfs_read_stat
static status_t
reiserfs_read_stat(fs_volume *fs, fs_vnode *_node, struct stat *st)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
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
reiserfs_open(fs_volume *fs, fs_vnode *_node, int openMode, void **cookie)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
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
reiserfs_close(fs_volume *fs, fs_vnode *_node, void *cookie)
{
	TOUCH(fs); TOUCH(cookie);
//	FUNCTION_START();
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
	TOUCH(node);
	return B_OK;
}

// reiserfs_free_cookie
static status_t
reiserfs_free_cookie(fs_volume *fs, fs_vnode *_node, void *cookie)
{
	TOUCH(fs);
//	FUNCTION_START();
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
	TOUCH(node);
	StreamReader *reader = (StreamReader*)cookie;
	delete reader;
	return B_OK;
}

// reiserfs_read
static status_t
reiserfs_read(fs_volume *fs, fs_vnode *_node, void *cookie, off_t pos,
	void *buffer, size_t *bufferSize)
{
	TOUCH(fs);
//	FUNCTION_START();
//	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("((%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 "), "
			"%" B_PRIdOFF ", %p, %lu)\n",
		node->GetID(), node->GetDirID(), node->GetObjectID(),
		pos, buffer, *bufferSize));
	status_t error = B_OK;
	// don't read anything but files
	if (!node->IsFile()) {
		if (node->IsDir())
			error = B_IS_A_DIRECTORY;
		else
			error = B_BAD_VALUE;
	}

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
reiserfs_open_dir(fs_volume *fs, fs_vnode *_node, void **cookie)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
	status_t error = (node->IsDir() ? B_OK : B_NOT_A_DIRECTORY);
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
reiserfs_close_dir(fs_volume *fs, fs_vnode *_node, void *cookie)
{
	TOUCH(fs); TOUCH(cookie);
//	FUNCTION_START();
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
	TOUCH(node);
	return B_OK;
}

// reiserfs_free_dir_cookie
static status_t
reiserfs_free_dir_cookie(fs_volume *fs, fs_vnode *_node, void *cookie)
{
	TOUCH(fs);
//	FUNCTION_START();
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
	TOUCH(node);
	DirectoryCookie *iterator = (DirectoryCookie*)cookie;
	delete iterator;
	return B_OK;
}

// reiserfs_read_dir
static status_t
reiserfs_read_dir(fs_volume *fs, fs_vnode *_node, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
//	FUNCTION_START();
	Volume *volume = (Volume*)fs->private_volume;
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
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
				|| (statData.IsEsoteric() && volume->GetHideEsoteric())) {
				continue;
			}
			// get the name
			size_t nameLen = 0;
			const char *name = item.EntryNameAt(index, &nameLen);
			if (!name || nameLen == 0)	// bad data: skip it gracefully
				continue;
			// fill in the entry name -- checks whether the
			// entry fits into the buffer
			error = set_dirent_name(buffer, bufferSize, name, nameLen);
			if (error == B_OK) {
				// fill in the other data
				buffer->d_dev = volume->GetID();
				buffer->d_ino = VNode::GetIDFor(dirID, objectID);
				*count = 1;
				PRINT(("Successfully read entry: dir: (%" B_PRIdINO ": "
						"%" B_PRIu32 ", %" B_PRIu32 "), name: `%s', "
						"id: (%" B_PRIdINO ", %" B_PRIu32 ", %" B_PRIu32 "), "
						"reclen: %hu\n",
					node->GetID(),
					node->GetDirID(), node->GetObjectID(), buffer->d_name,
					buffer->d_ino, dirID, objectID,
					buffer->d_reclen));
				if (!strcmp("..", buffer->d_name))
					iterator->SetEncounteredDotDot(true);
				done = true;
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
					PRINT(("faking `..' entry: dir: (%" B_PRIdINO ": "
							"%" B_PRIu32 ", %" B_PRIu32 "), name: `%s', "
							"id: (%" B_PRIdINO ", %" B_PRIu32 ", %" B_PRIu32
							"), reclen: %hu\n",
						node->GetID(),
						node->GetDirID(), node->GetObjectID(), buffer->d_name,
						buffer->d_ino, node->GetDirID(), node->GetObjectID(),
						buffer->d_reclen));
					iterator->SetEncounteredDotDot(true);
				}
			}
		}
		iterator->Suspend();
	}
	PRINT(("returning %" B_PRIu32 " entries\n", *count));
	RETURN_ERROR(error);
}

// reiserfs_rewind_dir
static status_t
reiserfs_rewind_dir(fs_volume *fs, fs_vnode *_node, void *cookie)
{
	TOUCH(fs);
//	FUNCTION_START();
	VNode *node = (VNode*)_node->private_node;
	FUNCTION(("node: (%" B_PRIdINO ": %" B_PRIu32 ", %" B_PRIu32 ")\n",
		node->GetID(), node->GetDirID(), node->GetObjectID()));
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

	"reiserfs",					// short_name
	"Reiser File System",		// pretty_name
	0,							// DDM flags


	// scanning
	&reiserfs_identify_partition,
	&reiserfs_scan_partition,
	&reiserfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&reiserfs_mount
};


fs_volume_ops gReiserFSVolumeOps = {
	&reiserfs_unmount,
	&reiserfs_read_fs_info,
	NULL,	// &reiserfs_write_fs_info,
	NULL,	// &reiserfs_sync,

	&reiserfs_read_vnode
};


fs_vnode_ops gReiserFSVnodeOps = {
	/* vnode operations */
	&reiserfs_lookup,
	NULL,	// &reiserfs_get_vnode_name,
	&reiserfs_write_vnode,
	NULL,	// &reiserfs_remove_vnode,

	/* VM file access */
	NULL,	// &reiserfs_can_page,
	NULL,	// &reiserfs_read_pages,
	NULL,	// &reiserfs_write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

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
	NULL,	// &reiserfs_preallocate,

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
	&reiserfs_rewind_dir
};


module_info *modules[] = {
	(module_info *)&sReiserFSModuleInfo,
	NULL,
};
