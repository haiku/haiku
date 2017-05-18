/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Attribute.h"
#include "AttributeIterator.h"
#include "btrfs.h"
#include "DirectoryIterator.h"
#include "Inode.h"
#include "Utility.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


#define BTRFS_IO_SIZE	65536


struct identify_cookie {
	btrfs_super_block super_block;
};


//!	btrfs_io() callback hook
static status_t
iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	Inode* inode = (Inode*)cookie;

	return file_map_translate(inode->Map(), offset, size, vecs, _count,
		inode->GetVolume()->BlockSize());
}


//!	btrfs_io() callback hook
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
btrfs_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	btrfs_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return -1;

	identify_cookie* cookie = new identify_cookie;
	memcpy(&cookie->super_block, &superBlock, sizeof(btrfs_super_block));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
btrfs_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	identify_cookie* cookie = (identify_cookie*)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.TotalSize();
	partition->block_size = cookie->super_block.BlockSize();
	partition->content_name = strdup(cookie->super_block.label);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
btrfs_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	delete (identify_cookie*)_cookie;
}


//	#pragma mark -


static status_t
btrfs_mount(fs_volume* _volume, const char* device, uint32 flags,
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
	_volume->ops = &gBtrfsVolumeOps;

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
btrfs_unmount(fs_volume* _volume)
{
	Volume* volume = (Volume*)_volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;

	return status;
}


static status_t
btrfs_read_fs_info(fs_volume* _volume, struct fs_info* info)
{
	Volume* volume = (Volume*)_volume->private_volume;

	// File system flags
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);
	info->io_size = BTRFS_IO_SIZE;
	info->block_size = volume->BlockSize();
	info->total_blocks = volume->SuperBlock().TotalSize() / volume->BlockSize();
	info->free_blocks = 0; //volume->NumFreeBlocks();

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	strlcpy(info->fsh_name, "btrfs", sizeof(info->fsh_name));

	return B_OK;
}


//	#pragma mark -


static status_t
btrfs_get_vnode(fs_volume* _volume, ino_t id, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)_volume->private_volume;

	Inode* inode = new(std::nothrow) Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status_t status = inode->InitCheck();
	if (status != B_OK) {
		delete inode;
		ERROR("get_vnode: InitCheck() failed. Error: %s\n", strerror(status));
		return status;
	}

	_node->private_node = inode;
	_node->ops = &gBtrfsVnodeOps;
	*_type = inode->Mode();
	*_flags = 0;

	return B_OK;
}


static status_t
btrfs_put_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	delete (Inode*)_node->private_node;
	return B_OK;
}


static bool
btrfs_can_page(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	return true;
}


static status_t
btrfs_read_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
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
btrfs_io(fs_volume* _volume, fs_vnode* _node, void* _cookie, io_request* request)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

#ifndef FS_SHELL
	if (io_request_is_write(request) && volume->IsReadOnly()) {
		notify_io_request(request, B_READ_ONLY_DEVICE);
		return B_READ_ONLY_DEVICE;
	}
#endif

	if (inode->FileCache() == NULL) {
#ifndef FS_SHELL
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
btrfs_get_file_map(fs_volume* _volume, fs_vnode* _node, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	TRACE("btrfs_get_file_map()\n");
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
			TRACE("btrfs_get_file_map for inode %" B_PRIdINO "\n", inode->ID());
			return B_OK;
		}
	}

	// can never get here
	return B_ERROR;
}


//	#pragma mark -


static status_t
btrfs_lookup(fs_volume* _volume, fs_vnode* _directory, const char* name,
	ino_t* _vnodeID)
{
	TRACE("btrfs_lookup: name address: %p (%s)\n", name, name);
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		return status;

	status = DirectoryIterator(directory).Lookup(name, strlen(name), _vnodeID);
	if (status != B_OK)
		return status;

	return get_vnode(volume->FSVolume(), *_vnodeID, NULL);
}


static status_t
btrfs_ioctl(fs_volume* _volume, fs_vnode* _node, void* _cookie, uint32 cmd,
	void* buffer, size_t bufferLength)
{
	TRACE("ioctl: %" B_PRIu32 "\n", cmd);

	/*Volume* volume = (Volume*)_volume->private_volume;*/
	return B_DEV_INVALID_IOCTL;
}


static status_t
btrfs_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat* stat)
{
	Inode* inode = (Inode*)_node->private_node;

	stat->st_dev = inode->GetVolume()->ID();
	stat->st_ino = inode->ID();
	stat->st_nlink = 1;
	stat->st_blksize = BTRFS_IO_SIZE;

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
btrfs_open(fs_volume* /*_volume*/, fs_vnode* _node, int openMode,
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

	cookie->open_mode = openMode & BTRFS_OPEN_MODE_USER_MASK;
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
btrfs_read(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
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
btrfs_close(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	return B_OK;
}


static status_t
btrfs_free_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
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
btrfs_access(fs_volume* _volume, fs_vnode* _node, int accessMode)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->CheckPermissions(accessMode);
}


static status_t
btrfs_read_link(fs_volume* _volume, fs_vnode* _node, char* buffer,
	size_t* _bufferSize)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->ReadAt(0, (uint8*)buffer, _bufferSize);
}


//	#pragma mark - Directory functions


static status_t
btrfs_open_dir(fs_volume* /*_volume*/, fs_vnode* _node, void** _cookie)
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
btrfs_read_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	struct dirent* dirent, size_t bufferSize, uint32* _num)
{
	DirectoryIterator* iterator = (DirectoryIterator*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;

	uint32 maxCount = *_num;
	uint32 count = 0;

	while (count < maxCount && bufferSize > sizeof(struct dirent)) {
		ino_t id;
		size_t length = bufferSize - sizeof(struct dirent) + 1;

		status_t status = iterator->GetNext(dirent->d_name, &length,
			&id);

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
	return B_OK;
}


static status_t
btrfs_rewind_dir(fs_volume* /*_volume*/, fs_vnode* /*node*/, void* _cookie)
{
	DirectoryIterator* iterator = (DirectoryIterator*)_cookie;

	return iterator->Rewind();
}


static status_t
btrfs_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void * /*_cookie*/)
{
	return B_OK;
}


static status_t
btrfs_free_dir_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	delete (DirectoryIterator*)_cookie;
	return B_OK;
}


static status_t
btrfs_open_attr_dir(fs_volume* _volume, fs_vnode* _node, void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;
	TRACE("%s()\n", __FUNCTION__);

	// on directories too ?
	if (!inode->IsFile())
		return EINVAL;

	AttributeIterator* iterator = new(std::nothrow) AttributeIterator(inode);
	if (iterator == NULL || iterator->InitCheck() != B_OK) {
		delete iterator;
		return B_NO_MEMORY;
	}

	*_cookie = iterator;
	return B_OK;
}


static status_t
btrfs_close_attr_dir(fs_volume* _volume, fs_vnode* _node, void* cookie)
{
	TRACE("%s()\n", __FUNCTION__);
	return B_OK;
}


static status_t
btrfs_free_attr_dir_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	TRACE("%s()\n", __FUNCTION__);
	delete (AttributeIterator*)_cookie;
	return B_OK;
}


static status_t
btrfs_read_attr_dir(fs_volume* _volume, fs_vnode* _node,
				void* _cookie, struct dirent* dirent, size_t bufferSize,
				uint32* _num)
{
	TRACE("%s()\n", __FUNCTION__);
	AttributeIterator* iterator = (AttributeIterator*)_cookie;

	size_t length = bufferSize;
	status_t status = iterator->GetNext(dirent->d_name, &length);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	}

	if (status != B_OK)
		return status;

	Volume* volume = (Volume*)_volume->private_volume;
	dirent->d_dev = volume->ID();
	dirent->d_reclen = sizeof(struct dirent) + length;
	*_num = 1;

	return B_OK;
}


static status_t
btrfs_rewind_attr_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	AttributeIterator* iterator = (AttributeIterator*)_cookie;
	return iterator->Rewind();
}


	/* attribute operations */
static status_t
btrfs_create_attr(fs_volume* _volume, fs_vnode* _node,
	const char* name, uint32 type, int openMode, void** _cookie)
{
	return EROFS;
}


static status_t
btrfs_open_attr(fs_volume* _volume, fs_vnode* _node, const char* name,
	int openMode, void** _cookie)
{
	TRACE("%s()\n", __FUNCTION__);

	Inode* inode = (Inode*)_node->private_node;
	Attribute attribute(inode);

	return attribute.Open(name, openMode, (attr_cookie**)_cookie);
}


static status_t
btrfs_close_attr(fs_volume* _volume, fs_vnode* _node,
	void* cookie)
{
	return B_OK;
}


static status_t
btrfs_free_attr_cookie(fs_volume* _volume, fs_vnode* _node,
	void* cookie)
{
	delete (attr_cookie*)cookie;
	return B_OK;
}


static status_t
btrfs_read_attr(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, void* buffer, size_t* _length)
{
	TRACE("%s()\n", __FUNCTION__);

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	Attribute attribute(inode, cookie);

	return attribute.Read(cookie, pos, (uint8*)buffer, _length);
}


static status_t
btrfs_write_attr(fs_volume* _volume, fs_vnode* _node, void* cookie,
	off_t pos, const void* buffer, size_t* length)
{
	return EROFS;
}


static status_t
btrfs_read_attr_stat(fs_volume* _volume, fs_vnode* _node,
	void* _cookie, struct stat* stat)
{
	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	Attribute attribute(inode, cookie);

	return attribute.Stat(*stat);
}


static status_t
btrfs_write_attr_stat(fs_volume* _volume, fs_vnode* _node,
	void* cookie, const struct stat* stat, int statMask)
{
	return EROFS;
}


static status_t
btrfs_rename_attr(fs_volume* _volume, fs_vnode* fromVnode,
	const char* fromName, fs_vnode* toVnode, const char* toName)
{
	return EROFS;
}


static status_t
btrfs_remove_attr(fs_volume* _volume, fs_vnode* vnode,
	const char* name)
{
	return EROFS;
}

//	#pragma mark -


static status_t
btrfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


fs_volume_ops gBtrfsVolumeOps = {
	&btrfs_unmount,
	&btrfs_read_fs_info,
	NULL,	// write_fs_info()
	NULL,	// fs_sync,
	&btrfs_get_vnode,
};


fs_vnode_ops gBtrfsVnodeOps = {
	/* vnode operations */
	&btrfs_lookup,
	NULL,
	&btrfs_put_vnode,
	NULL,	// btrfs_remove_vnode,

	/* VM file access */
	&btrfs_can_page,
	&btrfs_read_pages,
	NULL,	// btrfs_write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

	&btrfs_get_file_map,

	&btrfs_ioctl,
	NULL,
	NULL,	// fs_select
	NULL,	// fs_deselect
	NULL,	// fs_fsync,

	&btrfs_read_link,
	NULL,	// fs_create_symlink,

	NULL,	// fs_link,
	NULL,	// fs_unlink,
	NULL,	// fs_rename,

	&btrfs_access,
	&btrfs_read_stat,
	NULL,	// fs_write_stat,
	NULL,	// fs_preallocate

	/* file operations */
	NULL,	// fs_create,
	&btrfs_open,
	&btrfs_close,
	&btrfs_free_cookie,
	&btrfs_read,
	NULL,	//	fs_write,

	/* directory operations */
	NULL, 	// fs_create_dir,
	NULL, 	// fs_remove_dir,
	&btrfs_open_dir,
	&btrfs_close_dir,
	&btrfs_free_dir_cookie,
	&btrfs_read_dir,
	&btrfs_rewind_dir,

	/* attribute directory operations */
	&btrfs_open_attr_dir,
	&btrfs_close_attr_dir,
	&btrfs_free_attr_dir_cookie,
	&btrfs_read_attr_dir,
	&btrfs_rewind_attr_dir,

	/* attribute operations */
	&btrfs_create_attr,
	&btrfs_open_attr,
	&btrfs_close_attr,
	&btrfs_free_attr_cookie,
	&btrfs_read_attr,
	&btrfs_write_attr,
	&btrfs_read_attr_stat,
	&btrfs_write_attr_stat,
	&btrfs_rename_attr,
	&btrfs_remove_attr,
};


static file_system_module_info sBtrfsFileSystem = {
	{
		"file_systems/btrfs" B_CURRENT_FS_API_VERSION,
		0,
		btrfs_std_ops,
	},

	"btrfs",						// short_name
	"btrfs File System",			// pretty_name
	0,								// DDM flags

	// scanning
	btrfs_identify_partition,
	btrfs_scan_partition,
	btrfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&btrfs_mount,


	/* capability querying operations */
	NULL,

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
	NULL,	// initialize
	NULL	// unitialize
};


module_info* modules[] = {
	(module_info*)&sBtrfsFileSystem,
	NULL,
};
