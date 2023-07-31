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
#include <vfs.h>
#include <NodeMonitor.h>
#include <file_systems/DeviceOpener.h>
#include <file_systems/fs_ops_support.h>
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

extern "C" int mkntfs_main(const char* devpath, const char* label);

typedef CObjectDeleter<ntfs_inode, int, ntfs_inode_close> NtfsInodeCloser;
typedef CObjectDeleter<ntfs_attr, void, ntfs_attr_close> NtfsAttrCloser;
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
		if (ntVolume == NULL)
			return errno ? errno : B_ERROR;

		if (ntVolume->vol_name != NULL && ntVolume->vol_name[0] != '\0')
			partition->content_name = strdup(ntVolume->vol_name);
		else
			partition->content_name = strdup("");
		ntfs_umount(ntVolume, true);
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
fs_initialize(int fd, partition_id partitionID, const char* name,
	const char* parameterString, off_t partitionSize, disk_job_id job)
{
	TRACE("fs_initialize: '%s', %s\n", name, parameterString);

	update_disk_device_job_progress(job, 0);

	char path[B_PATH_NAME_LENGTH];
	if (ioctl(fd, B_GET_PATH_FOR_DEVICE, path) == 0)
		return B_BAD_VALUE;

	status_t result = mkntfs_main(path, name);
	if (result != 0)
		return result;

	result = scan_partition(partitionID);
	if (result != B_OK)
		return result;

	update_disk_device_job_progress(job, 1);
	return B_OK;
}


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

	if (NVolReadOnly(volume->ntfs)) {
		if ((ntfsFlags & NTFS_MNT_RDONLY) == 0)
			ERROR("volume is hibernated, mounted as read-only\n");
		volume->fs_info_flags |= B_FS_IS_READONLY;
	}

	if (ntfs_volume_get_free_space(volume->ntfs) != 0) {
		ntfs_umount(volume->ntfs, true);
		return B_ERROR;
	}

	const bool showSystem = false, showHidden = true, hideDot = false;
	if (ntfs_set_shown_files(volume->ntfs, showSystem, showHidden, hideDot) != 0) {
		ntfs_umount(volume->ntfs, true);
		return B_ERROR;
	}

	// Fetch mount path, used when reading NTFS symlinks.
	dev_t deviceID;
	ino_t nodeID;
	status_t status = vfs_get_mount_point(_volume->id, &deviceID, &nodeID);
	char* mountpoint;
	if (status == B_OK) {
		mountpoint = (char*)malloc(PATH_MAX);
		status = vfs_entry_ref_to_path(deviceID, nodeID, NULL, true,
			mountpoint, PATH_MAX);
		if (status == B_OK) {
			char* reallocated = (char*)realloc(mountpoint, strlen(mountpoint) + 1);
			if (reallocated != NULL)
				mountpoint = reallocated;
		} else {
			free(mountpoint);
			mountpoint = NULL;
		}
	}
	if (status != B_OK)
		mountpoint = strdup("");

	// TODO: uid/gid mapping and real permissions

	// construct lowntfs_context
	volume->lowntfs.haiku_fs_volume = _volume;
	volume->lowntfs.current_close_state_vnode = NULL;

	volume->lowntfs.vol = volume->ntfs;
	volume->ntfs->abs_mnt_point = volume->lowntfs.abs_mnt_point = mountpoint;
	volume->lowntfs.dmask = 0;
	volume->lowntfs.fmask = S_IXUSR | S_IXGRP | S_IXOTH;
	volume->lowntfs.dmtime = 0;
	volume->lowntfs.special_files = NTFS_FILES_INTERIX;
	volume->lowntfs.posix_nlink = 0;
	volume->lowntfs.inherit = 0;
	volume->lowntfs.windows_names = 1;
	volume->lowntfs.latest_ghost = 0;

	*_rootID = root->inode = FILE_root;
	root->parent_inode = (u64)-1;
	root->mode = S_IFDIR | ACCESSPERMS;
	root->uid = root->gid = 0;
	root->size = 0;

	status = publish_vnode(_volume, root->inode, root, &gNtfsVnodeOps, S_IFDIR, 0);
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


static status_t
fs_write_fs_info(fs_volume* _volume, const struct fs_info* info, uint32 mask)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);

	if ((volume->fs_info_flags & B_FS_IS_READONLY) != 0)
		return B_READ_ONLY_DEVICE;

	status_t status = B_OK;

	if ((mask & FS_WRITE_FSINFO_NAME) != 0) {
		ntfschar* label = NULL;
		int label_len = ntfs_mbstoucs(info->volume_name, &label);
		if (label_len <= 0 || label == NULL)
			return -1;
		MemoryDeleter nameDeleter(label);

		if (ntfs_volume_rename(volume->ntfs, label, label_len) != 0)
			status = errno;
	}

	return status;
}


//	#pragma mark -


static status_t
fs_init_vnode(fs_volume* _volume, ino_t parent, ino_t nid, vnode** _vnode, bool publish = false)
{
	volume* volume = (struct volume*)_volume->private_volume;
	ASSERT_LOCKED_MUTEX(&volume->lock);

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

	node->inode = nid;
	node->parent_inode = parent;
	node->uid = statbuf.st_uid;
	node->gid = statbuf.st_gid;
	node->mode = statbuf.st_mode;
	node->size = statbuf.st_size;

	// cache the node's name
	char path[B_FILE_NAME_LENGTH];
	if (utils_inode_get_name(ni, path, sizeof(path)) == 0)
		return B_NO_MEMORY;
	node->name = strdup(strrchr(path, '/') + 1);

	if (publish) {
		status_t status = publish_vnode(_volume, node->inode, node, &gNtfsVnodeOps, node->mode, 0);
		if (status != B_OK)
			return status;
	}

	if ((node->mode & S_IFDIR) == 0) {
		node->file_cache = file_cache_create(_volume->id, nid, node->size);
		if (node->file_cache == NULL)
			return B_NO_INIT;
	}

	vnodeDeleter.Detach();
	*_vnode = node;
	return B_OK;
}


static status_t
fs_get_vnode(fs_volume* _volume, ino_t nid, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	TRACE("get_vnode %" B_PRIdINO "\n", nid);
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(reenter ? NULL : &volume->lock);

	vnode* vnode;
	status_t status = fs_init_vnode(_volume, -1 /* set by fs_lookup */, nid, &vnode);
	if (status != B_OK)
		return status;

	_node->private_node = vnode;
	_node->ops = &gNtfsVnodeOps;
	*_type = vnode->mode;
	*_flags = 0;

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


static status_t
fs_remove_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(reenter ? NULL : &volume->lock);
	vnode* node = (vnode*)_node->private_node;

	if (ntfs_fuse_release(&volume->lowntfs, node->parent_inode, node->inode,
			node->lowntfs_close_state, node->lowntfs_ghost) != 0)
		return errno;

	file_cache_delete(node->file_cache);
	delete node;
	return B_OK;
}


int*
ntfs_haiku_get_close_state(struct lowntfs_context *ctx, u64 ino)
{
	if (ctx->current_close_state_vnode != NULL)
		panic("NTFS current_close_state_vnode should be NULL!");

	vnode* node = NULL;
	if (get_vnode((fs_volume*)ctx->haiku_fs_volume, ino, (void**)&node) != B_OK)
		return NULL;
	ctx->current_close_state_vnode = node;
	return &node->lowntfs_close_state;
}


void
ntfs_haiku_put_close_state(struct lowntfs_context *ctx, u64 ino, u64 ghost)
{
	vnode* node = (vnode*)ctx->current_close_state_vnode;
	if (node == NULL)
		return;

	node->lowntfs_ghost = ghost;
	if ((node->lowntfs_close_state & CLOSE_GHOST) != 0) {
		fs_volume* _volume = (fs_volume*)ctx->haiku_fs_volume;
		entry_cache_remove(_volume->id, node->parent_inode, node->name);
		notify_entry_removed(_volume->id, node->parent_inode, node->name, node->inode);
		remove_vnode(_volume, node->inode);
	}

	ctx->current_close_state_vnode = NULL;
	put_vnode((fs_volume*)ctx->haiku_fs_volume, node->inode);
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


static status_t
fs_write_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;

	TRACE("write_pages inode: %" B_PRIdINO", pos: %" B_PRIdOFF "; vecs: %p; "
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
		const int written = ntfs_fuse_write(&volume->lowntfs, ni, (char*)vecs[i].iov_base, ioSize, pos);
		if (written < 0)
			return errno;

		pos += written;
		*_numBytes += written;
		bytesLeft -= written;

		if (size_t(written) != ioSize)
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


static status_t
fs_write_stat(fs_volume* _volume, fs_vnode* _node, const struct stat* stat, uint32 mask)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;

	if ((volume->fs_info_flags & B_FS_IS_READONLY) != 0)
		return B_READ_ONLY_DEVICE;

	ntfs_inode* ni = ntfs_inode_open(volume->ntfs, node->inode);
	if (ni == NULL)
		return B_FILE_ERROR;
	NtfsInodeCloser niCloser(ni);

	bool updateTime = false;
	const uid_t euid = geteuid();

	const bool isOwnerOrRoot = (euid == 0 || euid == (uid_t)node->uid);
	const bool hasWriteAccess = fs_access(_volume, _node, W_OK);

	if ((mask & B_STAT_SIZE) != 0 && node->size != stat->st_size) {
		if ((node->mode & S_IFDIR) != 0)
			return B_IS_A_DIRECTORY;
		if (!hasWriteAccess)
			return B_NOT_ALLOWED;

		ntfs_attr* na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
		if (na == NULL)
			return errno;
		NtfsAttrCloser naCloser(na);

		if (ntfs_attr_truncate(na, stat->st_size) != 0)
			return errno;
		node->size = na->data_size;
		file_cache_set_size(node->file_cache, node->size);

		updateTime = true;
	}

	if ((mask & B_STAT_UID) != 0) {
		// only root should be allowed
		if (euid != 0)
			return B_NOT_ALLOWED;

		// We don't support this (yet.)
		if (node->uid != stat->st_uid)
			return B_UNSUPPORTED;
	}

	if ((mask & B_STAT_GID) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			return B_NOT_ALLOWED;

		// We don't support this (yet.)
		if (node->gid != stat->st_gid)
			return B_UNSUPPORTED;
	}

	if ((mask & B_STAT_MODE) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			return B_NOT_ALLOWED;

		// We don't support this (yet.)
		if (node->mode != stat->st_mode)
			return B_UNSUPPORTED;
	}

	if ((mask & B_STAT_CREATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			return B_NOT_ALLOWED;

		ni->creation_time = timespec2ntfs(stat->st_crtim);
	}

	if ((mask & B_STAT_MODIFICATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			return B_NOT_ALLOWED;

		ni->last_data_change_time = timespec2ntfs(stat->st_mtim);
	}

	if ((mask & B_STAT_CHANGE_TIME) != 0 || updateTime) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			return B_NOT_ALLOWED;

		ni->last_mft_change_time = timespec2ntfs(stat->st_ctim);
	}

	if ((mask & B_STAT_ACCESS_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			return B_NOT_ALLOWED;

		ni->last_access_time = timespec2ntfs(stat->st_atim);
	}

	ntfs_inode_mark_dirty(ni);

	notify_stat_changed(_volume->id, node->parent_inode, node->inode, mask);
	return B_OK;
}


static status_t
fs_generic_create(fs_volume* _volume, vnode* directory, const char* name, int mode,
	ino_t* _inode)
{
	volume* volume = (struct volume*)_volume->private_volume;
	ASSERT_LOCKED_MUTEX(&volume->lock);

	if ((directory->mode & S_IFDIR) == 0)
		return B_BAD_TYPE;

	ino_t inode = -1;
	if (ntfs_fuse_create(&volume->lowntfs, directory->inode, name, mode & (S_IFMT | 07777),
			0, (char*)NULL, &inode) != 0)
		return errno;

	vnode* node;
	status_t status = fs_init_vnode(_volume, directory->inode, inode, &node, true);
	if (status != B_OK)
		return status;

	entry_cache_add(_volume->id, directory->inode, name, inode);
	notify_entry_created(_volume->id, directory->inode, name, inode);
	*_inode = inode;
	return B_OK;
}


static status_t
fs_create(fs_volume* _volume, fs_vnode* _directory, const char* name,
	int openMode, int mode, void** _cookie, ino_t* _vnodeID)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* directory = (vnode*)_directory->private_node;

	if ((directory->mode & S_IFDIR) == 0)
		return B_NOT_A_DIRECTORY;

	status_t status = fs_access(_volume, _directory, W_OK);
	if (status != B_OK)
		return status;

#if 1
	if ((openMode & O_NOCACHE) != 0)
		return B_UNSUPPORTED;
#endif

	status = fs_generic_create(_volume, directory, name, S_IFREG | (mode & 07777), _vnodeID);
	if (status != B_OK)
		return status;

	file_cookie* cookie = new file_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	cookie->open_mode = openMode;

	cookieDeleter.Detach();
	*_cookie = cookie;
	return B_OK;
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

	if ((openMode & O_TRUNC) != 0) {
		if ((openMode & O_RWMASK) == O_RDONLY)
			return B_NOT_ALLOWED;

		ntfs_attr* na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
		if (na == NULL)
			return errno;
		NtfsAttrCloser naCloser(na);

		if (ntfs_attr_truncate(na, 0) != 0)
			return errno;
		node->size = na->data_size;
		file_cache_set_size(node->file_cache, node->size);
	}

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
fs_write(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	const void* buffer, size_t* _length)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* node = (vnode*)_node->private_node;
	file_cookie* cookie = (file_cookie*)_cookie;

	if ((node->mode & S_IFDIR) != 0)
		return B_IS_A_DIRECTORY;

	ASSERT((cookie->open_mode & O_WRONLY) != 0 || (cookie->open_mode & O_RDWR) != 0);

	if (cookie->open_mode & O_APPEND)
		pos = node->size;
	if (pos < 0)
		return B_BAD_VALUE;

	if (pos + s64(*_length) > node->size) {
		ntfs_inode* ni = ntfs_inode_open(volume->ntfs, node->inode);
		if (ni == NULL)
			return errno;
		NtfsInodeCloser niCloser(ni);

		ntfs_attr* na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
		if (na == NULL)
			return errno;
		NtfsAttrCloser naCloser(na);

		if (ntfs_attr_truncate(na, pos + *_length) != 0)
			return errno;
		node->size = na->data_size;
		file_cache_set_size(node->file_cache, node->size);
	}

	lock.Unlock();

	status_t status = file_cache_write(node->file_cache, cookie, pos, buffer, _length);
	if (status != B_OK)
		return status;

	lock.Lock();

	// periodically notify if the file size has changed
	if ((node->lowntfs_close_state & CLOSE_GHOST) == 0
			&& cookie->last_size != node->size
			&& system_time() > cookie->last_notification + INODE_NOTIFICATION_INTERVAL) {
		notify_stat_changed(_volume->id, node->parent_inode, node->inode,
			B_STAT_MODIFICATION_TIME | B_STAT_SIZE | B_STAT_INTERIM_UPDATE);
		cookie->last_size = node->size;
		cookie->last_notification = system_time();
	}
	return status;
}


static status_t
fs_fsync(fs_volume* _volume, fs_vnode* _node)
{
	CALLED();
	vnode* node = (vnode*)_node->private_node;

	return file_cache_sync(node->file_cache);
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
fs_generic_unlink(fs_volume* _volume, fs_vnode* _directory, const char* name, RM_TYPES type)
{
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* directory = (vnode*)_directory->private_node;

	status_t status = fs_access(_volume, _directory, W_OK);
	if (status != B_OK)
		return status;

	if (ntfs_fuse_rm(&volume->lowntfs, directory->inode, name, type) != 0)
		return errno;

	// remove_vnode() et al. will be called by put_close_state.
	return B_OK;
}


static status_t
fs_unlink(fs_volume* _volume, fs_vnode* _directory, const char* name)
{
	CALLED();
	return fs_generic_unlink(_volume, _directory, name, RM_LINK);
}


static status_t
fs_rename(fs_volume* _volume, fs_vnode* _oldDir, const char* oldName,
	fs_vnode* _newDir, const char* newName)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);

	vnode* old_directory = (vnode*)_oldDir->private_node;
	vnode* new_directory = (vnode*)_newDir->private_node;

	if (old_directory == new_directory && strcmp(oldName, newName) == 0)
		return B_OK;

	status_t status = fs_access(_volume, _oldDir, W_OK);
	if (status == B_OK)
		status = fs_access(_volume, _newDir, W_OK);
	if (status != B_OK)
		return status;

	// Prevent moving a directory into one of its own children.
	if (old_directory != new_directory) {
		u64 oldIno = ntfs_fuse_inode_lookup(&volume->lowntfs, old_directory->inode, oldName);
		if (oldIno == (u64)-1)
			return B_ENTRY_NOT_FOUND;

		ino_t parent = new_directory->inode;
		const ino_t root = FILE_root;

		while (true) {
			if (parent == oldIno)
				return B_BAD_VALUE;
			else if (parent == root || parent == old_directory->inode)
				break;

			vnode* parentNode;
			if (get_vnode(_volume, parent, (void**)&parentNode) != B_OK)
				return B_ERROR;

			parent = parentNode->parent_inode;
			put_vnode(_volume, parentNode->inode);
		}
	}

	if (ntfs_fuse_rename(&volume->lowntfs, old_directory->inode, oldName,
			new_directory->inode, newName) != 0)
		return errno;

	u64 ino = ntfs_fuse_inode_lookup(&volume->lowntfs, new_directory->inode, newName);
	if (ino == (u64)-1)
		return B_ENTRY_NOT_FOUND;

	vnode* node;
	status = get_vnode(_volume, ino, (void**)&node);
	if (status != B_OK)
		return status;

	free(node->name);
	node->name = strdup(newName);
	node->parent_inode = new_directory->inode;

	if ((node->mode & S_IFDIR) != 0)
		entry_cache_add(_volume->id, ino, "..", new_directory->inode);

	put_vnode(_volume, ino);

	entry_cache_remove(_volume->id, old_directory->inode, oldName);
	entry_cache_add(_volume->id, new_directory->inode, newName, ino);
	notify_entry_moved(_volume->id, old_directory->inode, oldName,
		new_directory->inode, newName, ino);

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
fs_create_dir(fs_volume* _volume, fs_vnode* _directory, const char* name, int mode)
{
	CALLED();
	volume* volume = (struct volume*)_volume->private_volume;
	MutexLocker lock(volume->lock);
	vnode* directory = (vnode*)_directory->private_node;

	status_t status = fs_access(_volume, _directory, W_OK);
	if (status != B_OK)
		return status;

	ino_t inode = -1;
	status = fs_generic_create(_volume, directory, name, S_IFDIR | (mode & 07777), &inode);
	if (status != B_OK)
		return status;

	return B_OK;
}


static status_t
fs_remove_dir(fs_volume* _volume, fs_vnode* _directory, const char* name)
{
	CALLED();
	ino_t directory_inode = ((vnode*)_directory->private_node)->inode;
	status_t status = fs_generic_unlink(_volume, _directory, name, RM_DIR);
	if (status != B_OK)
		return status;

	entry_cache_remove(_volume->id, directory_inode, "..");
	return B_OK;
}


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
		size_t length = bufferSize - offsetof(struct dirent, d_name);
		if (length < (cookie->current->name_length + 1)) {
			// the remaining name buffer length is too small
			if (count == 0)
				return B_BUFFER_OVERFLOW;
			break;
		}
		length = cookie->current->name_length;

		dirent->d_dev = _volume->id;
		dirent->d_ino = cookie->current->inode;
		strlcpy(dirent->d_name, cookie->current->name, length + 1);

		dirent = next_dirent(dirent, length, bufferSize);
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
	&fs_write_fs_info,
	NULL,	// fs_sync,
	&fs_get_vnode,
};


fs_vnode_ops gNtfsVnodeOps = {
	/* vnode operations */
	&fs_lookup,
	&fs_get_vnode_name,
	&fs_put_vnode,
	&fs_remove_vnode,

	/* VM file access */
	&fs_can_page,
	&fs_read_pages,
	&fs_write_pages,

	NULL,	// io
	NULL,	// cancel_io

	NULL,	// get_file_map

	&fs_ioctl,
	NULL,
	NULL,	// fs_select
	NULL,	// fs_deselect
	&fs_fsync,

	&fs_read_link,
	NULL,	// fs_create_symlink

	NULL,	// fs_link,
	&fs_unlink,
	&fs_rename,

	&fs_access,
	&fs_read_stat,
	&fs_write_stat,
	NULL,	// fs_preallocate

	/* file operations */
	&fs_create,
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	&fs_write,

	/* directory operations */
	&fs_create_dir,
	&fs_remove_dir,
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

	// DDM flags
	0
	| B_DISK_SYSTEM_IS_FILE_SYSTEM
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_WRITING
	,

	// scanning
	fs_identify_partition,
	fs_scan_partition,
	fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,

	// capability querying operations
	NULL,	// get_supported_operations

	NULL,	// validate_resize
	NULL,	// validate_move
	NULL,	// validate_set_content_name
	NULL,	// validate_set_content_parameters
	NULL,	// validate_initialize,

	/* shadow partition modification */
	NULL,	// shadow_changed

	/* writing */
	NULL,	// defragment
	NULL,	// repair
	NULL,	// resize
	NULL,	// move
	NULL,	// set_content_name
	NULL,	// set_content_parameters
	fs_initialize,
	NULL	// uninitialize
};


module_info *modules[] = {
	(module_info *)&sNtfsFileSystem,
	NULL,
};
