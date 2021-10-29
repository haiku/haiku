/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

#include <dirent.h>
#include <unistd.h>
#include <util/kernel_cpp.h>
#include <string.h>

#include <AutoDeleter.h>
#include <fs_cache.h>
#include <fs_info.h>
#include <file_systems/DeviceOpener.h>
#include <util/AutoLock.h>

#include "ntfs.h"

extern "C" {
#include "libntfs/bootsect.h"
#include "libntfs/dir.h"
#include "utils/utils.h"
}

//#define TRACE_NTFS
#ifdef TRACE_NTFS
#	define TRACE(X...)	dprintf("ntfs: " X)
#else
#	define TRACE(X...) ;
#endif
#define CALLED()		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)
#define ERROR(X...)		dprintf("ntfs: error: " X)


struct identify_cookie {
	NTFS_BOOT_SECTOR boot;
};

typedef CObjectDeleter<ntfs_inode, int, ntfs_inode_close> NtfsInodeCloser;
static status_t fs_access(fs_volume* _volume, fs_vnode* _node, int accessMode);


//	#pragma mark - Scanning


static float
fs_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	CALLED();

	NTFS_BOOT_SECTOR boot;
	if (read_pos(fd, 0, (void*)&boot, 512) != 512) {
		ERROR("identify_partition: failed to read boot sector\n");
		return -1;
	}

	if (!ntfs_boot_sector_is_ntfs(&boot)) {
		ERROR("identify_partition: boot signature doesn't match\n");
		return -1;
	}

	identify_cookie* cookie = new identify_cookie;
	if (cookie == NULL) {
		ERROR("identify_partition: cookie allocation failed\n");
		return -1;
	}

	memcpy(&cookie->boot, &boot, sizeof(boot));
	*_cookie = cookie;

	// This value overrides the Intel partition identifier.
	return 0.82f;
}


static status_t
fs_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	CALLED();

	identify_cookie *cookie = (identify_cookie*)_cookie;
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = sle64_to_cpu(cookie->boot.number_of_sectors)
		* le16_to_cpu(cookie->boot.bpb.bytes_per_sector);
	partition->block_size = le16_to_cpu(cookie->boot.bpb.bytes_per_sector);

	// get volume name
	ntfs_volume* ntVolume;
	char path[B_PATH_NAME_LENGTH];
	if (ioctl(fd, B_GET_PATH_FOR_DEVICE, path) != 0) {
		ntVolume = utils_mount_volume(path, NTFS_MNT_RDONLY | NTFS_MNT_RECOVER);
		if (ntVolume != NULL) {
			if (ntVolume->vol_name != NULL && ntVolume->vol_name[0] != '\0')
				partition->content_name = strdup(ntVolume->vol_name);
			ntfs_umount(ntVolume, true);
		}
	}

	return partition->content_name != NULL ? B_OK : B_NO_MEMORY;
}


static void
fs_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	CALLED();

	delete (identify_cookie*)_cookie;
}


//	#pragma mark -


static status_t
fs_mount(fs_volume* _volume, const char* device, uint32 flags,
	const char* args, ino_t* _rootID)
{
	CALLED();

	volume* volume = new struct volume;
	vnode* root = new vnode;
	if (volume == NULL || root == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<struct volume> volumeDeleter(volume);

	mutex_init(&volume->lock, "NTFS volume lock");
	volume->fs_info_flags = B_FS_IS_PERSISTENT;

	unsigned long ntfsFlags = NTFS_MNT_RECOVER | NTFS_MNT_MAY_RDONLY;
	if ((flags & B_MOUNT_READ_ONLY) != 0 || DeviceOpener(device, O_RDWR).IsReadOnly())
		ntfsFlags |= NTFS_MNT_RDONLY;

	// mount
	volume->ntfs = utils_mount_volume(device, ntfsFlags);
	if (volume->ntfs == NULL)
		return errno;

#if 0 // TODO!
	if (NVolReadOnly(volume->ntfs))
#endif
		volume->fs_info_flags |= B_FS_IS_READONLY;

	if (ntfs_volume_get_free_space(volume->ntfs) != 0) {
		ntfs_umount(volume->ntfs, true);
		return B_ERROR;
	}

	const bool showSystem = false, showHidden = true, hideDot = false;
	if (ntfs_set_shown_files(volume->ntfs, showSystem, showHidden, hideDot) != 0) {
		ntfs_umount(volume->ntfs, true);
		return B_ERROR;
	}

	// TODO: uid/gid mapping and real permissions

	// construct lowntfs_context
	volume->lowntfs.vol = volume->ntfs;
	volume->lowntfs.abs_mnt_point = NULL;
	volume->lowntfs.dmask = 0;
	volume->lowntfs.fmask = S_IXUSR | S_IXGRP | S_IXOTH;
	volume->lowntfs.posix_nlink = 0;

	*_rootID = root->inode = FILE_root;
	root->parent_inode = (u64)-1;
	root->mode = S_IFDIR | ACCESSPERMS;
	root->uid = root->gid = 0;

	status_t status = publish_vnode(_volume, root->inode, root, &gNtfsVnodeOps, S_IFDIR, 0);
	if (status != B_OK) {
		ntfs_umount(volume->ntfs, true);
		return status;
	}

	volumeDeleter.Detach();

	_volume->private_volume = volume;
	_volume->ops = &gNtfsVolumeOps;
	return B_OK;
}


static status_t
fs_unmount(fs_volume* _volume)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;

	if (ntfs_umount(volume->ntfs, false) < 0)
		return errno;

	delete volume;
	_volume->private_volume = NULL;

	return B_OK;
}


static status_t
fs_read_fs_info(fs_volume* _volume, struct fs_info* info)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);

	info->flags = volume->fs_info_flags;
	info->block_size = volume->ntfs->cluster_size;
	info->total_blocks = volume->ntfs->nr_clusters;
	info->free_blocks = volume->ntfs->free_clusters;

	info->io_size = 65536;

	strlcpy(info->volume_name, volume->ntfs->vol_name, sizeof(info->volume_name));
	strlcpy(info->fsh_name, "NTFS", sizeof(info->fsh_name));

	return B_OK;
}


//	#pragma mark -


static status_t
fs_get_vnode(fs_volume* _volume, ino_t nid, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	TRACE("get_vnode %" B_PRIdINO "\n", nid);
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(reenter ? NULL : &volume->lock);

	ntfs_inode* ni = ntfs_inode_open(volume->ntfs, nid);
	if (ni == NULL)
		return ENOENT;
	NtfsInodeCloser niCloser(ni);

	vnode* node = new vnode;
	if (node == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<vnode> vnodeDeleter(node);

	struct stat statbuf;
	if (ntfs_fuse_getstat(&volume->lowntfs, NULL, ni, &statbuf) != 0)
		return errno;

	node->uid = statbuf.st_uid;
	node->gid = statbuf.st_gid;
	node->mode = statbuf.st_mode;

	node->inode = nid;
	_node->private_node = node;
	_node->ops = &gNtfsVnodeOps;
	*_type = node->mode;
	*_flags = 0;

	// cache the node's name
	char path[B_FILE_NAME_LENGTH];
	if (utils_inode_get_name(ni, path, sizeof(path)) == 0)
		return errno;
	node->name = strdup(strrchr(path, '/') + 1);

	// fs_lookup will set this, if needed.
	node->parent_inode = (u64)-1;

	if ((node->mode & S_IFDIR) == 0)
		node->file_cache = file_cache_create(_volume->id, nid, statbuf.st_size);

	vnodeDeleter.Detach();
	return B_OK;
}


static status_t
fs_put_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(reenter ? NULL : &volume->lock);
	vnode* node = (vnode*)_node->private_node;

	file_cache_delete(node->file_cache);
	delete node;
	return B_OK;
}


static bool
fs_can_page(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	return true;
}


static status_t
fs_read_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;

	TRACE("read_pages inode: %" B_PRIdINO", pos: %" B_PRIdOFF "; vecs: %p; "
		"count: %" B_PRIuSIZE "; numBytes: %" B_PRIuSIZE "\n", node->inode, pos,
		vecs, count, *_numBytes);

	ntfs_inode* ni = ntfs_inode_open(volume->ntfs, node->inode);
	if (ni == NULL)
		return B_FILE_ERROR;
	NtfsInodeCloser niCloser(ni);

	if (pos < 0 || pos >= ni->data_size)
		return B_BAD_VALUE;

	size_t bytesLeft = min_c(*_numBytes, size_t(ni->data_size - pos));
	*_numBytes = 0;
	for (size_t i = 0; i < count && bytesLeft > 0; i++) {
		const size_t ioSize = min_c(bytesLeft, vecs[i].iov_len);
		const int read = ntfs_fuse_read(ni, pos, (char*)vecs[i].iov_base, ioSize);
		if (read < 0)
			return errno;

		pos += read;
		*_numBytes += read;
		bytesLeft -= read;

		if (size_t(read) != ioSize)
			return errno;
	}

	return B_OK;
}


//	#pragma mark -


static status_t
fs_lookup(fs_volume* _volume, fs_vnode* _directory, const char* name,
	ino_t* _vnodeID)
{
	TRACE("fs_lookup: name address: %p (%s)\n", name, name);
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* directory = (vnode*)_directory->private_node;

	status_t result;
	if (strcmp(name, ".") == 0) {
		*_vnodeID = directory->inode;
	} else if (strcmp(name, "..") == 0) {
		if (directory->inode == FILE_root)
			return ENOENT;
		*_vnodeID = directory->parent_inode;
	} else {
		u64 inode = ntfs_fuse_inode_lookup(&volume->lowntfs, directory->inode, name);
		if (inode == (u64)-1)
			return errno;
		*_vnodeID = inode;
	}

	result = entry_cache_add(_volume->id, directory->inode, name, *_vnodeID);
	if (result != B_OK)
		return result;

	vnode* node = NULL;
	result = get_vnode(_volume, *_vnodeID, (void**)&node);
	if (result != B_OK)
		return result;

	if (node->parent_inode == (u64)-1)
		node->parent_inode = directory->inode;

	TRACE("fs_lookup: ID %" B_PRIdINO "\n", *_vnodeID);
	return B_OK;
}


static status_t
fs_get_vnode_name(fs_volume* _volume, fs_vnode* _node, char* buffer, size_t bufferSize)
{
	// CALLED();
	vnode* node = (vnode*)_node->private_node;

	if (strlcpy(buffer, node->name, bufferSize) >= bufferSize)
		return B_BUFFER_OVERFLOW;
	return B_OK;
}


static status_t
fs_ioctl(fs_volume* _volume, fs_vnode* _node, void* _cookie, uint32 cmd,
	void* buffer, size_t bufferLength)
{
	// TODO?
	return B_DEV_INVALID_IOCTL;
}


static status_t
fs_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat* stat)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;

	ntfs_inode* ni = ntfs_inode_open(volume->ntfs, node->inode);
	if (ni == NULL)
		return errno;
	NtfsInodeCloser niCloser(ni);

	if (ntfs_fuse_getstat(&volume->lowntfs, NULL, ni, stat) != 0)
		return errno;
	return B_OK;
}


static inline int
open_mode_to_access(int openMode)
{
	if ((openMode & O_RWMASK) == O_RDONLY)
		return R_OK;
	if ((openMode & O_RWMASK) == O_WRONLY)
		return W_OK;
	if ((openMode & O_RWMASK) == O_RDWR)
		return R_OK | W_OK;
	return 0;
}


static status_t
fs_open(fs_volume* _volume, fs_vnode* _node, int openMode, void** _cookie)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;

	// opening a directory read-only is allowed (but no data can be read)
	if ((node->mode & S_IFDIR) != 0 && (openMode & O_RWMASK) != 0)
		return B_IS_A_DIRECTORY;
	if ((openMode & O_DIRECTORY) != 0 && (node->mode & S_IFDIR) == 0)
		return B_NOT_A_DIRECTORY;

	status_t status = fs_access(_volume, _node, open_mode_to_access(openMode));
	if (status != B_OK)
		return status;

	ntfs_inode* ni = ntfs_inode_open(volume->ntfs, node->inode);
	if (ni == NULL)
		return errno;
	NtfsInodeCloser niCloser(ni);

	file_cookie* cookie = new file_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	cookie->open_mode = openMode;

	// We don't actually support uncached mode; it would require us to handle
	// passing user buffers to libntfs, among other things.
#if 0
	if ((openMode & O_NOCACHE) != 0 && node->file_cache != NULL) {
		status_t status = file_cache_disable(node->file_cache);
		if (status != B_OK)
			return status;
	}
#else
	if ((openMode & O_NOCACHE) != 0)
		return B_UNSUPPORTED;
#endif

	cookieDeleter.Detach();
	*_cookie = cookie;
	return B_OK;
}


static status_t
fs_read(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	void* buffer, size_t* length)
{
	vnode* node = (vnode*)_node->private_node;
	file_cookie* cookie = (file_cookie*)_cookie;

	if ((node->mode & S_IFDIR) != 0)
		return B_IS_A_DIRECTORY;

	ASSERT((cookie->open_mode & O_RWMASK) == O_RDONLY || (cookie->open_mode & O_RDWR) != 0);

	return file_cache_read(node->file_cache, cookie, pos, buffer, length);
}


static status_t
fs_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	CALLED();

	// Nothing to do.
	return B_OK;
}


static status_t
fs_free_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
fs_access(fs_volume* _volume, fs_vnode* _node, int accessMode)
{
	// CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	vnode* node = (vnode*)_node->private_node;

	if ((accessMode & W_OK) != 0 && (volume->fs_info_flags & B_FS_IS_READONLY) != 0)
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, node->mode, node->gid, node->uid);
}


static status_t
fs_read_link(fs_volume* _volume, fs_vnode* _node, char* buffer, size_t* bufferSize)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;

	if (ntfs_fuse_readlink(&volume->lowntfs, node->inode, buffer, bufferSize) != 0)
		return errno;
	return B_OK;
}


//	#pragma mark - Directory functions


static status_t
fs_open_dir(fs_volume* _volume, fs_vnode* _node, void** _cookie)
{
	CALLED();
	vnode* node = (vnode*)_node->private_node;

	status_t status = fs_access(_volume, _node, R_OK);
	if (status != B_OK)
		return status;

	if ((node->mode & S_IFDIR) == 0)
		return B_NOT_A_DIRECTORY;

	directory_cookie* cookie = new directory_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->first = cookie->current = NULL;
	*_cookie = (void*)cookie;
	return B_OK;
}


static int
_ntfs_readdir_callback(void* _cookie, const ntfschar* ntfs_name, const int ntfs_name_len,
	const int name_type, const s64 pos, const MFT_REF mref, const unsigned dt_type)
{
	if (name_type == FILE_NAME_DOS)
		return 0;

	directory_cookie* cookie = (directory_cookie*)_cookie;

	char* name = NULL;
	int name_len = ntfs_ucstombs(ntfs_name, ntfs_name_len, &name, 0);
	if (name_len <= 0 || name == NULL)
		return -1;
	MemoryDeleter nameDeleter(name);

	directory_cookie::entry* entry =
		(directory_cookie::entry*)malloc(sizeof(directory_cookie::entry) + name_len);
	if (entry == NULL)
		return -1;
	entry->next = NULL;

	entry->inode = MREF(mref);
	entry->name_length = name_len;
	memcpy(entry->name, name, name_len + 1);

	if (cookie->first == NULL) {
		cookie->first = cookie->current = entry;
	} else {
		cookie->current->next = entry;
		cookie->current = entry;
	}

	return 0;
}


static status_t
fs_read_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	struct dirent* dirent, size_t bufferSize, uint32* _num)
{
	CALLED();
	directory_cookie* cookie = (directory_cookie*)_cookie;

	// TODO: While lowntfs-3g seems to also read the entire directory at once into memory,
	// we could optimize things here by storing the data in the vnode, not the inode, and
	// only freeing it after some period of inactivity.

	// See if we need to read the directory ourselves first.
	if (cookie->first == NULL) {
		volume* volume = (struct volume*)_volume->private_volume;
		MutexLocker lock(volume->lock);
		vnode* node = (vnode*)_node->private_node;

		ntfs_inode* ni = ntfs_inode_open(volume->ntfs, node->inode);
		if (ni == NULL)
			return errno;
		NtfsInodeCloser niCloser(ni);

		s64 pos = 0;
		if (ntfs_readdir(ni, &pos, cookie, _ntfs_readdir_callback) != 0)
			return errno;
		cookie->current = cookie->first;
	}
	if (cookie->first == NULL)
		return ENOENT;
	if (cookie->current == NULL) {
		*_num = 0;
		return B_OK;
	}

	uint32 maxCount = *_num;
	uint32 count = 0;
	while (count < maxCount && bufferSize > sizeof(struct dirent)) {
		size_t length = bufferSize - sizeof(struct dirent);
		if (length < cookie->current->name_length) {
			// the remaining name buffer length is too small
			if (count == 0)
				return B_BUFFER_OVERFLOW;
			break;
		}
		length = cookie->current->name_length;

		dirent->d_dev = _volume->id;
		dirent->d_ino = cookie->current->inode;
		strlcpy(dirent->d_name, cookie->current->name, length + 1);
		dirent->d_reclen = sizeof(struct dirent) + length;

		bufferSize -= dirent->d_reclen;
		dirent = (struct dirent*)((uint8*)dirent + dirent->d_reclen);
		count++;

		cookie->current = cookie->current->next;
		if (cookie->current == NULL)
			break;
	}

	*_num = count;
	return B_OK;
}


static status_t
fs_rewind_dir(fs_volume* /*_volume*/, fs_vnode* /*node*/, void *_cookie)
{
	CALLED();
	directory_cookie* cookie = (directory_cookie*)_cookie;
	cookie->current = cookie->first;
	return B_OK;
}


static status_t
fs_close_dir(fs_volume* /*_volume*/, fs_vnode* /*node*/, void* /*_cookie*/)
{
	return B_OK;
}


static status_t
fs_free_dir_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	CALLED();
	directory_cookie* cookie = (directory_cookie*)_cookie;
	if (cookie == NULL)
		return B_OK;

	// Free entries.
	cookie->current = cookie->first;
	while (cookie->current != NULL) {
		directory_cookie::entry* next = cookie->current->next;
		free(cookie->current);
		cookie->current = next;
	}

	delete cookie;
	return B_OK;
}


fs_volume_ops gNtfsVolumeOps = {
	&fs_unmount,
	&fs_read_fs_info,
	NULL,	// write_fs_info()
	NULL,	// fs_sync,
	&fs_get_vnode,
};


fs_vnode_ops gNtfsVnodeOps = {
	/* vnode operations */
	&fs_lookup,
	&fs_get_vnode_name,
	&fs_put_vnode,
	NULL,	// fs_remove_vnode,

	/* VM file access */
	&fs_can_page,
	&fs_read_pages,
	NULL,	// fs_write_pages,

	NULL,	// io
	NULL,	// cancel_io

	NULL,	// get_file_map

	&fs_ioctl,
	NULL,
	NULL,	// fs_select
	NULL,	// fs_deselect
	NULL,	// fs_fsync

	&fs_read_link,
	NULL,	// fs_create_symlink

	NULL,	// fs_link,
	NULL,	// fs_unlink,
	NULL,	// fs_rename,

	&fs_access,
	&fs_read_stat,
	NULL,	// fs_write_stat,
	NULL,	// fs_preallocate

	/* file operations */
	NULL,	// fs_create,
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	NULL,	//	fs_write,

	/* directory operations */
	NULL, 	// fs_create_dir,
	NULL, 	// fs_remove_dir,
	&fs_open_dir,
	&fs_close_dir,
	&fs_free_dir_cookie,
	&fs_read_dir,
	&fs_rewind_dir,

	/* attribute directory operations */
	NULL, 	// fs_open_attr_dir,
	NULL,	// fs_close_attr_dir,
	NULL,	// fs_free_attr_dir_cookie,
	NULL,	// fs_read_attr_dir,
	NULL,	// fs_rewind_attr_dir,

	/* attribute operations */
	NULL,	// fs_create_attr,
	NULL,	// fs_open_attr,
	NULL,	// fs_close_attr,
	NULL,	// fs_free_attr_cookie,
	NULL,	// fs_read_attr,
	NULL,	// fs_write_attr,
	NULL,	// fs_read_attr_stat,
	NULL,	// fs_write_attr_stat,
	NULL,	// fs_rename_attr,
	NULL,	// fs_remove_attr,
};


static file_system_module_info sNtfsFileSystem = {
	{
		"file_systems/ntfs" B_CURRENT_FS_API_VERSION,
		0,
		NULL,
	},

	"ntfs",							// short_name
	"NT File System",				// pretty_name
	0,								// DDM flags

	// scanning
	fs_identify_partition,
	fs_scan_partition,
	fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,

	NULL,
};


module_info *modules[] = {
	(module_info *)&sNtfsFileSystem,
	NULL,
};
