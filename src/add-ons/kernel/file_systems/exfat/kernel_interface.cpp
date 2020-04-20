/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, korli@users.berlios.de
 *		John Scipione, jscipione@gmail.com
 */


#include <dirent.h>
#include <unistd.h>
#include <util/kernel_cpp.h>
#include <string.h>

#include <new>

#include <AutoDeleter.h>
#include <fs_cache.h>
#include <fs_info.h>
#include <io_requests.h>
#include <NodeMonitor.h>
#include <StorageDefs.h>
#include <util/AutoLock.h>

#include "DirectoryIterator.h"
#include "exfat.h"
#include "Inode.h"
#include "Utility.h"


//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACE(x...) dprintf("\33[34mexfat:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)


#define EXFAT_IO_SIZE	65536


struct identify_cookie {
	exfat_super_block super_block;
	char name[B_FILE_NAME_LENGTH];
};


//!	exfat_io() callback hook
static status_t
iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	Inode* inode = (Inode*)cookie;

	return file_map_translate(inode->Map(), offset, size, vecs, _count,
		inode->GetVolume()->BlockSize());
}


//!	exfat_io() callback hook
static status_t
iterative_io_finished_hook(void* cookie, io_request* request, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	Inode* inode = (Inode*)cookie;
	rw_lock_read_unlock(inode->Lock());
	return B_OK;
}


//	#pragma mark - Scanning


static float
exfat_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	struct exfat_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return -1;

	identify_cookie* cookie = new (std::nothrow)identify_cookie;
	if (cookie == NULL)
		return -1;

	memcpy(&cookie->super_block, &superBlock, sizeof(exfat_super_block));
	memset(cookie->name, 0, sizeof(cookie->name));
		// zero out volume name

	uint32 rootDirCluster = superBlock.RootDirCluster();
	uint32 blockSize = 1 << superBlock.BlockShift();
	uint32 clusterSize = blockSize << superBlock.BlocksPerClusterShift();
	uint64 rootDirectoryOffset = EXFAT_SUPER_BLOCK_OFFSET
		+ (uint64)superBlock.FirstDataBlock() * blockSize
		+ (rootDirCluster - 2) * clusterSize;
	struct exfat_entry entry;
	size_t entrySize = sizeof(struct exfat_entry);
	for (uint32 i = 0; read_pos(fd, rootDirectoryOffset + i * entrySize,
			&entry, entrySize) == (ssize_t)entrySize; i++) {
		if (entry.type == EXFAT_ENTRY_TYPE_NOT_IN_USE
			|| entry.type == EXFAT_ENTRY_TYPE_LABEL) {
			if (get_volume_name(&entry, cookie->name, sizeof(cookie->name))
					!= B_OK) {
				delete cookie;
				return -1;
			}
			break;
		}
	}

	if (cookie->name[0] == '\0') {
		off_t fileSystemSize = (off_t)superBlock.NumBlocks()
			<< superBlock.BlockShift();
		get_default_volume_name(fileSystemSize, cookie->name,
			sizeof(cookie->name));
	}

	*_cookie = cookie;
	return 0.8f;
}


static status_t
exfat_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	identify_cookie* cookie = (identify_cookie*)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.NumBlocks()
		<< cookie->super_block.BlockShift();
	partition->block_size = 1 << cookie->super_block.BlockShift();
	partition->content_name = strdup(cookie->name);

	return partition->content_name != NULL ? B_OK : B_NO_MEMORY;
}


static void
exfat_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	delete (identify_cookie*)_cookie;
}


//	#pragma mark -


static status_t
exfat_mount(fs_volume* _volume, const char* device, uint32 flags,
	const char* args, ino_t* _rootID)
{
	Volume* volume = new(std::nothrow) Volume(_volume);
	if (volume == NULL)
		return B_NO_MEMORY;

	// TODO: this is a bit hacky: we can't use publish_vnode() to publish
	// the root node, or else its file cache cannot be created (we could
	// create it later, though). Therefore we're using get_vnode() in Mount(),
	// but that requires us to export our volume data before calling it.
	_volume->private_volume = volume;
	_volume->ops = &gExfatVolumeOps;

	status_t status = volume->Mount(device, flags);
	if (status != B_OK) {
		ERROR("Failed mounting the volume. Error: %s\n", strerror(status));
		delete volume;
		return status;
	}

	*_rootID = volume->RootNode()->ID();
	return B_OK;
}


static status_t
exfat_unmount(fs_volume *_volume)
{
	Volume* volume = (Volume *)_volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;

	return status;
}


static status_t
exfat_read_fs_info(fs_volume* _volume, struct fs_info* info)
{
	Volume* volume = (Volume*)_volume->private_volume;

	// File system flags
	info->flags = B_FS_IS_PERSISTENT
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);
	info->io_size = EXFAT_IO_SIZE;
	info->block_size = volume->BlockSize();
	info->total_blocks = volume->SuperBlock().NumBlocks();
	info->free_blocks = 0; //volume->NumFreeBlocks();

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	strlcpy(info->fsh_name, "exfat", sizeof(info->fsh_name));

	return B_OK;
}


//	#pragma mark -


static status_t
exfat_get_vnode(fs_volume* _volume, ino_t id, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	TRACE("get_vnode %" B_PRIdINO "\n", id);
	Volume* volume = (Volume*)_volume->private_volume;

	Inode* inode = new(std::nothrow) Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status_t status = inode->InitCheck();
	if (status != B_OK)
		delete inode;

	if (status == B_OK) {
		_node->private_node = inode;
		_node->ops = &gExfatVnodeOps;
		*_type = inode->Mode();
		*_flags = 0;
	} else
		ERROR("get_vnode: InitCheck() failed. Error: %s\n", strerror(status));

	return status;
}


static status_t
exfat_put_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	delete (Inode*)_node->private_node;
	return B_OK;
}


static bool
exfat_can_page(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	return true;
}


static status_t
exfat_read_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (inode->FileCache() == NULL)
		return B_BAD_VALUE;

	rw_lock_read_lock(inode->Lock());

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	while (true) {
		file_io_vec fileVecs[8];
		size_t fileVecCount = 8;

		status = file_map_translate(inode->Map(), pos, bytesLeft, fileVecs,
			&fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bool bufferOverflow = status == B_BUFFER_OVERFLOW;

		size_t bytes = bytesLeft;
		status = read_file_io_vec_pages(volume->Device(), fileVecs,
			fileVecCount, vecs, count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	rw_lock_read_unlock(inode->Lock());

	return status;
}


static status_t
exfat_io(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	io_request* request)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

#ifndef EXFAT_SHELL
	if (io_request_is_write(request) && volume->IsReadOnly()) {
		notify_io_request(request, B_READ_ONLY_DEVICE);
		return B_READ_ONLY_DEVICE;
	}
#endif

	if (inode->FileCache() == NULL) {
#ifndef EXFAT_SHELL
		notify_io_request(request, B_BAD_VALUE);
#endif
		return B_BAD_VALUE;
	}

	// We lock the node here and will unlock it in the "finished" hook.
	rw_lock_read_lock(inode->Lock());

	return do_iterative_fd_io(volume->Device(), request,
		iterative_io_get_vecs_hook, iterative_io_finished_hook, inode);
}


static status_t
exfat_get_file_map(fs_volume* _volume, fs_vnode* _node, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	TRACE("exfat_get_file_map()\n");
	Inode* inode = (Inode*)_node->private_node;
	size_t index = 0, max = *_count;

	while (true) {
		off_t blockOffset;
		off_t blockLength;
		status_t status = inode->FindBlock(offset, blockOffset, &blockLength);
		if (status != B_OK)
			return status;

		if (index > 0 && (vecs[index - 1].offset
				== blockOffset - vecs[index - 1].length)) {
			vecs[index - 1].length += blockLength;
		} else {
			if (index >= max) {
				// we're out of file_io_vecs; let's bail out
				*_count = index;
				return B_BUFFER_OVERFLOW;
			}

			vecs[index].offset = blockOffset;
			vecs[index].length = blockLength;
			index++;
		}

		offset += blockLength;
		size -= blockLength;

		if ((off_t)size <= vecs[index - 1].length || offset >= inode->Size()) {
			// We're done!
			*_count = index;
			TRACE("exfat_get_file_map for inode %" B_PRIdINO"\n", inode->ID());
			return B_OK;
		}
	}

	// can never get here
	return B_ERROR;
}


//	#pragma mark -


static status_t
exfat_lookup(fs_volume* _volume, fs_vnode* _directory, const char* name,
	ino_t* _vnodeID)
{
	TRACE("exfat_lookup: name address: %p (%s)\n", name, name);
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		return status;

	status = DirectoryIterator(directory).Lookup(name, strlen(name), _vnodeID);
	if (status != B_OK) {
		ERROR("exfat_lookup: name %s (%s)\n", name, strerror(status));
		return status;
	}

	TRACE("exfat_lookup: ID %" B_PRIdINO "\n", *_vnodeID);

	return get_vnode(volume->FSVolume(), *_vnodeID, NULL);
}


static status_t
exfat_ioctl(fs_volume* _volume, fs_vnode* _node, void* _cookie, uint32 cmd,
	void* buffer, size_t bufferLength)
{
	TRACE("ioctl: %" B_PRIu32 "\n", cmd);

	/*Volume* volume = (Volume*)_volume->private_volume;*/
	return B_DEV_INVALID_IOCTL;
}


static status_t
exfat_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat* stat)
{
	Inode* inode = (Inode*)_node->private_node;

	stat->st_dev = inode->GetVolume()->ID();
	stat->st_ino = inode->ID();
	stat->st_nlink = 1;
	stat->st_blksize = EXFAT_IO_SIZE;

	stat->st_uid = inode->UserID();
	stat->st_gid = inode->GroupID();
	stat->st_mode = inode->Mode();
	stat->st_type = 0;

	inode->GetAccessTime(stat->st_atim);
	inode->GetModificationTime(stat->st_mtim);
	inode->GetChangeTime(stat->st_ctim);
	inode->GetCreationTime(stat->st_crtim);

	stat->st_size = inode->Size();
	stat->st_blocks = (inode->Size() + 511) / 512;

	return B_OK;
}


static status_t
exfat_open(fs_volume* /*_volume*/, fs_vnode* _node, int openMode,
	void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && (openMode & O_RWMASK) != 0)
		return B_IS_A_DIRECTORY;

	status_t status =  inode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
	if (status != B_OK)
		return status;

	// Prepare the cookie
	file_cookie* cookie = new(std::nothrow) file_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	cookie->open_mode = openMode & EXFAT_OPEN_MODE_USER_MASK;
	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

	if ((openMode & O_NOCACHE) != 0 && inode->FileCache() != NULL) {
		// Disable the file cache, if requested?
		status = file_cache_disable(inode->FileCache());
		if (status != B_OK)
			return status;
	}

	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


static status_t
exfat_read(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	void* buffer, size_t* _length)
{
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsFile()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	return inode->ReadAt(pos, (uint8*)buffer, _length);
}


static status_t
exfat_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_OK;
}


static status_t
exfat_free_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (inode->Size() != cookie->last_size)
		notify_stat_changed(volume->ID(), -1, inode->ID(), B_STAT_SIZE);

	delete cookie;
	return B_OK;
}


static status_t
exfat_access(fs_volume* _volume, fs_vnode* _node, int accessMode)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->CheckPermissions(accessMode);
}


static status_t
exfat_read_link(fs_volume *_volume, fs_vnode *_node, char *buffer,
	size_t *_bufferSize)
{
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsSymLink())
		return B_BAD_VALUE;

	status_t result = inode->ReadAt(0, reinterpret_cast<uint8*>(buffer),
		_bufferSize);
	if (result != B_OK)
		return result;

	*_bufferSize = inode->Size();

	return B_OK;
}


//	#pragma mark - Directory functions


static status_t
exfat_open_dir(fs_volume* /*_volume*/, fs_vnode* _node, void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;
	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	if (!inode->IsDirectory())
		return B_NOT_A_DIRECTORY;

	DirectoryIterator* iterator = new(std::nothrow) DirectoryIterator(inode);
	if (iterator == NULL || iterator->InitCheck() != B_OK) {
		delete iterator;
		return B_NO_MEMORY;
	}

	*_cookie = iterator;
	return B_OK;
}


static status_t
exfat_read_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	TRACE("exfat_read_dir\n");
	DirectoryIterator* iterator = (DirectoryIterator*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;

	uint32 maxCount = *_num;
	uint32 count = 0;

	while (count < maxCount && bufferSize > sizeof(struct dirent)) {
		ino_t id;
		size_t length = bufferSize - sizeof(struct dirent) + 1;

		status_t status = iterator->GetNext(dirent->d_name, &length, &id);
		if (status == B_ENTRY_NOT_FOUND)
			break;

		if (status == B_BUFFER_OVERFLOW) {
			// the remaining name buffer length was too small
			if (count == 0)
				return B_BUFFER_OVERFLOW;
			break;
		}

		if (status != B_OK)
			return status;

		dirent->d_dev = volume->ID();
		dirent->d_ino = id;
		dirent->d_reclen = sizeof(struct dirent) + length;

		bufferSize -= dirent->d_reclen;
		dirent = (struct dirent*)((uint8*)dirent + dirent->d_reclen);
		count++;
	}

	*_num = count;
	TRACE("exfat_read_dir end\n");
	return B_OK;
}


static status_t
exfat_rewind_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void *_cookie)
{
	DirectoryIterator* iterator = (DirectoryIterator*)_cookie;

	return iterator->Rewind();
}


static status_t
exfat_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void * /*_cookie*/)
{
	return B_OK;
}


static status_t
exfat_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	delete (DirectoryIterator*)_cookie;
	return B_OK;
}


fs_volume_ops gExfatVolumeOps = {
	&exfat_unmount,
	&exfat_read_fs_info,
	NULL,	// write_fs_info()
	NULL,	// fs_sync,
	&exfat_get_vnode,
};


fs_vnode_ops gExfatVnodeOps = {
	/* vnode operations */
	&exfat_lookup,
	NULL,
	&exfat_put_vnode,
	NULL,	// exfat_remove_vnode,

	/* VM file access */
	&exfat_can_page,
	&exfat_read_pages,
	NULL,	// exfat_write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

	&exfat_get_file_map,

	&exfat_ioctl,
	NULL,
	NULL,	// fs_select
	NULL,	// fs_deselect
	NULL,	// fs_fsync,

	&exfat_read_link,
	NULL,	// fs_create_symlink,

	NULL,	// fs_link,
	NULL,	// fs_unlink,
	NULL,	// fs_rename,

	&exfat_access,
	&exfat_read_stat,
	NULL,	// fs_write_stat,
	NULL,	// fs_preallocate

	/* file operations */
	NULL,	// fs_create,
	&exfat_open,
	&exfat_close,
	&exfat_free_cookie,
	&exfat_read,
	NULL,	//	fs_write,

	/* directory operations */
	NULL, 	// fs_create_dir,
	NULL, 	// fs_remove_dir,
	&exfat_open_dir,
	&exfat_close_dir,
	&exfat_free_dir_cookie,
	&exfat_read_dir,
	&exfat_rewind_dir,

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


static file_system_module_info sExfatFileSystem = {
	{
		"file_systems/exfat" B_CURRENT_FS_API_VERSION,
		0,
		NULL,
	},

	"exfat",						// short_name
	"ExFAT File System",			// pretty_name
	0,								// DDM flags

	// scanning
	exfat_identify_partition,
	exfat_scan_partition,
	exfat_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&exfat_mount,

	NULL,
};


module_info *modules[] = {
	(module_info *)&sExfatFileSystem,
	NULL,
};
