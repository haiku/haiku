/*
 * Copyright 2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "FDPath.h"

#include <new>
#include <util/AutoLock.h>

#include "vfs.h"	// for dump_fd_paths_hash_table
#include "Vnode.h"


void
PathInfo::SetTo(file_descriptor* fd, ino_t dir, const char* name)
{
	descriptor = fd;
	directory = dir;
	strlcpy(filename, name, B_FILE_NAME_LENGTH);
}


struct vnode_hash_key {
	dev_t	device;
	ino_t	vnode;
};


static int
fd_paths_compare(void* _fd_paths, const void* _key)
{
	vnode* node = ((fd_paths*)_fd_paths)->node;
	const vnode_hash_key* key = (const vnode_hash_key*)_key;

	if (node->device == key->device && node->id == key->vnode)
		return 0;

	return -1;
}


static uint32
fd_paths_hash(void* _fd_paths, const void* _key, uint32 range)
{
	fd_paths* fdPaths = (fd_paths*)_fd_paths;
	const vnode_hash_key* key = (const vnode_hash_key*)_key;

#define VHASH(mountid, vnodeid) \
	(((uint32)((vnodeid) >> 32) + (uint32)(vnodeid)) ^ (uint32)(mountid))

	if (fdPaths != NULL) {
		vnode* node = fdPaths->node;
		return VHASH(node->device, node->id) % range;
	}

	return VHASH(key->device, key->vnode) % range;

#undef VHASH
}


#define FD_PATH_HASH_TABLE_SIZE 1024
static hash_table* sFdPathsTable = NULL;


bool
init_fd_paths_hash_table()
{
	struct fd_paths dummyFdPaths;
	sFdPathsTable = hash_init(FD_PATH_HASH_TABLE_SIZE,
		offset_of_member(dummyFdPaths, next), &fd_paths_compare,
		&fd_paths_hash);
	if (sFdPathsTable == NULL)
		return false;
	return true;
}


#if 0
void
dump_fd_paths_hash_table()
{
	hash_iterator it;
	hash_rewind(sFdPathsTable, &it);
	dprintf("fd_paths hash table:\n");
	for (unsigned int i = 0; i < hash_count_elements(sFdPathsTable); i++) {
		fd_paths* fdPath = (fd_paths*)hash_next(sFdPathsTable, &it);
		dprintf("vnode: device %i, node %i\n", (int)fdPath->node->device,
			(int)fdPath->node->id);
		PathInfoList::Iterator pathIt(&fdPath->paths);
		for (PathInfo* info = pathIt.Next(); info != NULL;
			info = pathIt.Next()) {
			char path[B_PATH_NAME_LENGTH];
			vfs_entry_ref_to_path(fdPath->node->device, info->directory,
				info->filename, path, B_PATH_NAME_LENGTH);
			dprintf("\tentry: fd %p, dir %i, name %s, path %s\n",
				info->descriptor, (int)info->directory, info->filename, path);
		}
	}
}
#endif


static rw_lock sFdPathsLock = RW_LOCK_INITIALIZER("fd_path_lock");


void
read_lock_fd_paths()
{
	rw_lock_read_lock(&sFdPathsLock);
}


void
read_unlock_fd_paths()
{
	rw_lock_read_unlock(&sFdPathsLock);
}


static fd_paths*
lookup_fd_paths(vnode* node)
{
	vnode_hash_key key;
	key.device = node->device;
	key.vnode = node->id;
	return (fd_paths*)hash_lookup(sFdPathsTable, &key);
}


fd_paths*
lookup_fd_paths(dev_t mountID, ino_t vnodeID)
{
	vnode_hash_key key;
	key.device = mountID;
	key.vnode = vnodeID;
	return (fd_paths*)hash_lookup(sFdPathsTable, &key);
}


static status_t
insert_path_info(vnode* node, PathInfo* _info)
{
	WriteLocker locker(sFdPathsLock);

	fd_paths* paths = lookup_fd_paths(node);
	if (paths == NULL) {
		paths = new(std::nothrow) fd_paths;
		if (paths == NULL)
			return B_NO_MEMORY;

		paths->node = node;
		if (hash_insert(sFdPathsTable, paths) != B_OK)
			delete paths;
	}

	paths->paths.Add(_info);
	return B_OK;
}


status_t
insert_fd_path(vnode* node, int fd, bool kernel, ino_t directory,
	const char* name)
{
	if (S_ISREG(node->Type()) != true)
		return B_OK;

	file_descriptor* descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return B_ERROR;

	PathInfo* info = new(std::nothrow) PathInfo;
	if (info == NULL) {
		put_fd(descriptor);
		return B_NO_MEMORY;
	}
	info->SetTo(descriptor, directory, name);

	status_t status = insert_path_info(descriptor->u.vnode, info);
	if (status != B_OK)
		free(info);

	put_fd(descriptor);
	return status;
}


status_t
remove_fd_path(file_descriptor*	descriptor)
{
	if (S_ISREG(descriptor->u.vnode->Type()) != true)
		return B_OK;

	WriteLocker locker(sFdPathsLock);

	fd_paths* paths = lookup_fd_paths(descriptor->u.vnode);
	if (paths == NULL)
		return B_ERROR;

	PathInfoList::Iterator it(&paths->paths);
	for (PathInfo* info = it.Next(); info != NULL; info = it.Next()) {
		if (info->descriptor == descriptor) {
			paths->paths.Remove(info);
			delete info;
			break;
		}
	}

	if (paths->paths.IsEmpty()) {
		// it was the last path remove the complete entry
		hash_remove(sFdPathsTable, paths);
		delete paths;
	}

	return B_OK;
}


status_t
move_fd_path(dev_t device, ino_t node, const char *fromName, ino_t newDirectory,
	const char* newName)
{
	WriteLocker locker(sFdPathsLock);

	fd_paths* paths = lookup_fd_paths(device, node);
	if (paths == NULL)
		return B_ERROR;

	PathInfoList::Iterator it(&paths->paths);
	for (PathInfo* info = it.Next(); info != NULL; info = it.Next()) {
		if (strncmp(info->filename, fromName, B_FILE_NAME_LENGTH) == 0) {
			info->directory = newDirectory;
			strlcpy(info->filename, newName, B_FILE_NAME_LENGTH);
		}
	}
	return B_OK;
}
