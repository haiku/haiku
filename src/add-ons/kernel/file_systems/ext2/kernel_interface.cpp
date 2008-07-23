/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <dirent.h>
#include <util/kernel_cpp.h>
#include <string.h>

#include <fs_cache.h>
#include <fs_info.h>

#include "DirectoryIterator.h"
#include "ext2.h"
#include "Inode.h"


#define EXT2_IO_SIZE	65536


struct identify_cookie {
	ext2_super_block super_block;
};


/*!	Converts the open mode, the open flags given to bfs_open(), into
	access modes, e.g. since O_RDONLY requires read access to the
	file, it will be converted to R_OK.
*/
int
open_mode_to_access(int openMode)
{
	openMode &= O_RWMASK;
	if (openMode == O_RDONLY)
		return R_OK;
	else if (openMode == O_WRONLY)
		return W_OK;

	return R_OK | W_OK;
}


//	#pragma mark - Scanning


static float
ext2_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	ext2_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return status;

	identify_cookie *cookie = new identify_cookie;
	memcpy(&cookie->super_block, &superBlock, sizeof(ext2_super_block));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
ext2_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.NumBlocks()
		<< cookie->super_block.BlockShift();
	partition->block_size = 1UL << cookie->super_block.BlockShift();
	partition->content_name = strdup(cookie->super_block.name);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
ext2_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	delete (identify_cookie*)_cookie;
}


//	#pragma mark -


static status_t
ext2_mount(fs_volume* _volume, const char* device, uint32 flags,
	const char* args, ino_t* _rootID)
{
	Volume* volume = new Volume(_volume);
	if (volume == NULL)
		return B_NO_MEMORY;

	// TODO: this is a bit hacky: we can't use publish_vnode() to publish
	// the root node, or else its file cache cannot be created (we could
	// create it later, though). Therefore we're using get_vnode() in Mount(),
	// but that requires us to export our volume data before calling it.
	_volume->private_volume = volume;
	_volume->ops = &gExt2VolumeOps;

	status_t status = volume->Mount(device, flags);
	if (status != B_OK) {
		delete volume;
		return status;
	}

	*_rootID = volume->RootNode()->ID();
	return B_OK;
}


static status_t
ext2_unmount(fs_volume *_volume)
{
	Volume* volume = (Volume *)_volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;

	return status;
}


static status_t
ext2_read_fs_info(fs_volume* _volume, struct fs_info* info)
{
	Volume* volume = (Volume*)_volume->private_volume;

	// File system flags
	info->flags = B_FS_IS_PERSISTENT
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);
	info->io_size = EXT2_IO_SIZE;
	info->block_size = volume->BlockSize();
	info->total_blocks = volume->NumBlocks();
	info->free_blocks = volume->FreeBlocks();

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	strlcpy(info->fsh_name, "ext2", sizeof(info->fsh_name));

	return B_OK;
}


//	#pragma mark -


static status_t
ext2_get_vnode(fs_volume* _volume, ino_t id, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)_volume->private_volume;

	if (id < 2 || id > volume->NumBlocks()) {
		dprintf("ext2: inode at %Ld requested!\n", id);
		return B_ERROR;
	}

	Inode* inode = new Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status_t status = inode->InitCheck();
	if (status < B_OK)
		delete inode;

	if (status == B_OK) {
		_node->private_node = inode;
		_node->ops = &gExt2VnodeOps;
		*_type = inode->Mode();
		*_flags = 0;
	}

	return status;
}


static status_t
ext2_put_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	delete (Inode*)_node->private_node;
	return B_OK;
}


static bool
ext2_can_page(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	return true;
}


static status_t
ext2_read_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
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
		uint32 fileVecCount = 8;

		status = file_map_translate(inode->Map(), pos, bytesLeft, fileVecs,
			&fileVecCount);
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
ext2_get_file_map(fs_volume* _volume, fs_vnode* _node, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;
	size_t index = 0, max = *_count;

	while (true) {
		uint32 block;
		status_t status = inode->FindBlock(offset, block);
		if (status != B_OK)
			return status;

		off_t blockOffset = (off_t)block << volume->BlockShift();
		uint32 blockLength = volume->BlockSize();

		if (index > 0 && vecs[index - 1].offset == blockOffset - blockLength) {
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

		if (size <= vecs[index - 1].length || offset >= inode->Size()) {
			// We're done!
			*_count = index;
			return B_OK;
		}
	}

	// can never get here
	return B_ERROR;
}


//	#pragma mark -


static status_t
ext2_lookup(fs_volume* _volume, fs_vnode* _directory, const char* name,
	ino_t* _vnodeID)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		return status;

	DirectoryIterator iterator(directory);
	while (true) {
		char buffer[B_FILE_NAME_LENGTH];
		size_t length = sizeof(buffer);
		status = iterator.GetNext(buffer, &length, _vnodeID);
		if (status != B_OK)
			return status;

		if (!strcmp(buffer, name))
			break;
	}

	return get_vnode(volume->FSVolume(), *_vnodeID, NULL);
}


static status_t
ext2_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat* stat)
{
	Inode* inode = (Inode*)_node->private_node;
	const ext2_inode& node = inode->Node();

	stat->st_dev = inode->GetVolume()->ID();
	stat->st_ino = inode->ID();
	stat->st_nlink = node.NumLinks();
	stat->st_blksize = EXT2_IO_SIZE;

	stat->st_uid = node.UserID();
	stat->st_gid = node.GroupID();
	stat->st_mode = node.Mode();
	stat->st_type = 0;

	stat->st_atime = node.AccessTime();
	stat->st_mtime = stat->st_ctime = node.ModificationTime();
	stat->st_crtime = node.CreationTime();

	stat->st_size = inode->Size();
	return B_OK;
}


static status_t
ext2_open(fs_volume* _volume, fs_vnode* _node, int openMode, void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && (openMode & O_RWMASK) != 0)
		return B_IS_A_DIRECTORY;

	if ((openMode & O_TRUNC) != 0)
		return B_READ_ONLY_DEVICE;

	return inode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
}


static status_t
ext2_read(fs_volume *_volume, fs_vnode *_node, void *_cookie, off_t pos,
	void *buffer, size_t *_length)
{
	Inode *inode = (Inode *)_node->private_node;

	if (!inode->IsFile()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	return inode->ReadAt(pos, (uint8 *)buffer, _length);
}


static status_t
ext2_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_OK;
}


static status_t
ext2_free_cookie(fs_volume* _volume, fs_vnode* _node, void* cookie)
{
	return B_OK;
}


static status_t
ext2_access(fs_volume* _volume, fs_vnode* _node, int accessMode)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->CheckPermissions(accessMode);
}


static status_t
ext2_read_link(fs_volume *_volume, fs_vnode *_node, char *buffer,
	size_t *_bufferSize)
{
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsSymLink())
		return B_BAD_VALUE;

	if (inode->Size() < *_bufferSize)
		*_bufferSize = inode->Size();

	if (inode->Size() > EXT2_SHORT_SYMLINK_LENGTH)
		return inode->ReadAt(0, (uint8 *)buffer, _bufferSize);

	memcpy(buffer, inode->Node().symlink, *_bufferSize);
	return B_OK;
}


//	#pragma mark - Directory functions


static status_t
ext2_open_dir(fs_volume *_volume, fs_vnode *_node, void **_cookie)
{
	Inode *inode = (Inode *)_node->private_node;
	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	if (!inode->IsDirectory())
		return B_BAD_VALUE;

	DirectoryIterator* iterator = new(std::nothrow) DirectoryIterator(inode);
	if (iterator == NULL)
		return B_NO_MEMORY;

	*_cookie = iterator;
	return B_OK;
}


static status_t
ext2_read_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	DirectoryIterator *iterator = (DirectoryIterator *)_cookie;

	size_t length = bufferSize;
	ino_t id;
	status_t status = iterator->GetNext(dirent->d_name, &length, &id);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	} else if (status != B_OK)
		return status;

	Volume* volume = (Volume*)_volume->private_volume;

	dirent->d_dev = volume->ID();
	dirent->d_ino = id;
	dirent->d_reclen = sizeof(struct dirent) + length;

	*_num = 1;
	return B_OK;
}


static status_t
ext2_rewind_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void *_cookie)
{
	DirectoryIterator *iterator = (DirectoryIterator *)_cookie;
	return iterator->Rewind();
}


static status_t
ext2_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void * /*_cookie*/)
{
	return B_OK;
}


static status_t
ext2_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	delete (DirectoryIterator *)_cookie;
	return B_OK;
}


fs_volume_ops gExt2VolumeOps = {
	&ext2_unmount,
	&ext2_read_fs_info,
	NULL,	// write_fs_info()
	NULL,	// sync()
	&ext2_get_vnode,
};

fs_vnode_ops gExt2VnodeOps = {
	/* vnode operations */
	&ext2_lookup,
	NULL,
	&ext2_put_vnode,
	NULL,

	/* VM file access */
	&ext2_can_page,
	&ext2_read_pages,
	NULL,

	&ext2_get_file_map,

	NULL,
	NULL,
	NULL,	// fs_select
	NULL,	// fs_deselect
	NULL,

	&ext2_read_link,
	NULL,

	NULL,
	NULL,
	NULL,

	&ext2_access,
	&ext2_read_stat,
	NULL,

	/* file operations */
	NULL,
	&ext2_open,
	&ext2_close,
	&ext2_free_cookie,
	&ext2_read,
	NULL,

	/* directory operations */
	NULL,
	NULL,
	&ext2_open_dir,
	&ext2_close_dir,
	&ext2_free_dir_cookie,
	&ext2_read_dir,
	&ext2_rewind_dir,

	NULL,
};

static file_system_module_info sExt2FileSystem = {
	{
		"file_systems/ext2" B_CURRENT_FS_API_VERSION,
		0,
		NULL,
	},

	"ext2",						// short_name
	"Ext2 File System",			// pretty_name
	0,							// DDM flags

	// scanning
	ext2_identify_partition,
	ext2_scan_partition,
	ext2_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&ext2_mount,

	NULL,
};

module_info *modules[] = {
	(module_info *)&sExt2FileSystem,
	NULL,
};
