/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <string.h>

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#endif
#include <file_systems/fs_ops_support.h>

#include "DirectoryIterator.h"
#include "Inode.h"
#include "system_dependencies.h"
#include "ufs2.h"
#include "Volume.h"

#define TRACE_UFS2
#ifdef TRACE_UFS2
#define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mufs2:\33[0m " x)


struct identify_cookie
{
	ufs2_super_block super_block;
	int cookie;
};


//	#pragma mark - Scanning
static float
ufs2_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	ufs2_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return -1;

	identify_cookie* cookie = new identify_cookie;
	memcpy(&cookie->super_block, &superBlock, sizeof(ufs2_super_block));
	*_cookie = cookie;

	return 0.8f;
}


static status_t
ufs2_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie* cookie = (identify_cookie*)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM | B_PARTITION_READ_ONLY;
	partition->block_size = cookie->super_block.fs_bsize;
	partition->content_size = cookie->super_block.fs_fsize
		* cookie->super_block.fs_size;
	partition->content_name = strdup(cookie->super_block.fs_volname);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
ufs2_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	identify_cookie* cookie = (identify_cookie*)_cookie;
	delete cookie;
	return;
}


//	#pragma mark -
static status_t
ufs2_mount(fs_volume *_volume, const char *device, uint32 flags,
		  const char *args, ino_t *_rootID)
{
	TRACE("Tracing mount()\n");
	Volume* volume = new(std::nothrow) Volume(_volume);
	if (volume == NULL)
		return B_NO_MEMORY;

	_volume->private_volume = volume;
	_volume->ops = &gUfs2VolumeOps;
	*_rootID = UFS2_ROOT;
	status_t status = volume->Mount(device, flags);
	if (status != B_OK){
		ERROR("Failed mounting the volume. Error: %s\n", strerror(status));
		delete volume;
		return status;
	}
	return B_OK;
}


static status_t
ufs2_unmount(fs_volume *_volume)
{
	Volume* volume = (Volume *)_volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;

	return status;
}


static status_t
ufs2_read_fs_info(fs_volume *_volume, struct fs_info *info)
{
	Volume* volume = (Volume*)_volume->private_volume;

	// File system flags
	info->flags = B_FS_IS_PERSISTENT
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);
	info->io_size = 65536;
	info->block_size = volume->SuperBlock().fs_fsize;
	info->total_blocks = volume->SuperBlock().fs_size;
	info->free_blocks = volume->SuperBlock().fs_cstotal.cs_nbfree;

	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));
	strlcpy(info->fsh_name, "UFS2", sizeof(info->fsh_name));

	return B_OK;
}


//	#pragma mark -

static status_t
ufs2_get_vnode(fs_volume *_volume, ino_t id, fs_vnode *_node, int *_type,
			  uint32 *_flags, bool reenter)
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
	_node->ops = &gufs2VnodeOps;
	*_type = inode->Mode();
	*_flags = 0;

	return B_OK;

}


static status_t
ufs2_put_vnode(fs_volume *_volume, fs_vnode *_node, bool reenter)
{
	return B_NOT_SUPPORTED;
}


static bool
ufs2_can_page(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return false;
}


static status_t
ufs2_read_pages(fs_volume *_volume, fs_vnode *_node, void *_cookie,
			   off_t pos, const iovec *vecs, size_t count, size_t *_numBytes)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_io(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	   io_request *request)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_get_file_map(fs_volume *_volume, fs_vnode *_node, off_t offset,
				 size_t size, struct file_io_vec *vecs, size_t *_count)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark -

static status_t
ufs2_lookup(fs_volume *_volume, fs_vnode *_directory, const char *name,
		   ino_t *_vnodeID)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		return status;

	status = DirectoryIterator(directory).Lookup(name, _vnodeID);
	if (status != B_OK)
		return status;

	status = get_vnode(volume->FSVolume(), *_vnodeID, NULL);
	return status;
}


static status_t
ufs2_ioctl(fs_volume *_volume, fs_vnode *_node, void *_cookie, uint32 cmd,
		  void *buffer, size_t bufferLength)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_read_stat(fs_volume *_volume, fs_vnode *_node, struct stat *stat)
{
	Inode* inode = (Inode*)_node->private_node;
	stat->st_dev = inode->GetVolume()->ID();
	stat->st_ino = inode->ID();
	stat->st_nlink = inode->LinkCount();
	stat->st_blksize = 65536;

	stat->st_uid = inode->UserID();
	stat->st_gid = inode->GroupID();
	stat->st_mode = inode->Mode();
	stat->st_type = 0;

	inode->GetAccessTime(stat->st_atim);
	inode->GetModificationTime(stat->st_mtim);
	inode->GetChangeTime(stat->st_ctim);
	inode->GetCreationTime(stat->st_crtim);

	stat->st_size = inode->Size();
	stat->st_blocks = inode->NumBlocks();

	return B_OK;
}


static status_t
ufs2_open(fs_volume * _volume, fs_vnode *_node, int openMode,
		 void **_cookie)
{
	//Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && (openMode & O_RWMASK) != O_RDONLY)
		return B_IS_A_DIRECTORY;
	if ((openMode & O_DIRECTORY) != 0 && !inode->IsDirectory())
		return B_NOT_A_DIRECTORY;

	status_t status =  inode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
	if (status != B_OK)
		return status;

	file_cookie* cookie = new(std::nothrow) file_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

//	fileCacheEnabler.Detach();
	cookieDeleter.Detach();
	*_cookie = cookie;
	return B_OK;
}


static status_t
ufs2_read(fs_volume *_volume, fs_vnode *_node, void *_cookie, off_t pos,
		 void *buffer, size_t *_length)
{
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsFile()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	return inode->ReadAt(pos, (uint8*)buffer, _length);
}


static status_t
ufs2_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_OK;
}


static status_t
ufs2_free_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	file_cookie* cookie = (file_cookie*)_cookie;

	delete cookie;
	return B_OK;
}

static status_t
ufs2_access(fs_volume *_volume, fs_vnode *_node, int accessMode)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->CheckPermissions(accessMode);
}


static status_t
ufs2_read_link(fs_volume *_volume, fs_vnode *_node, char *buffer,
			  size_t *_bufferSize)
{
	Inode* inode = (Inode*)_node->private_node;

	return inode->ReadLink(buffer, _bufferSize);
}


status_t
ufs2_unlink(fs_volume *_volume, fs_vnode *_directory, const char *name)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark - Directory functions

static status_t
ufs2_create_dir(fs_volume *_volume, fs_vnode *_directory, const char *name,
			   int mode)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_remove_dir(fs_volume *_volume, fs_vnode *_directory, const char *name)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_open_dir(fs_volume * /*_volume*/, fs_vnode *_node, void **_cookie)
{
	Inode* inode = (Inode*)_node->private_node;

	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	if (!inode->IsDirectory())
		return B_NOT_A_DIRECTORY;

	DirectoryIterator* iterator = new(std::nothrow) DirectoryIterator(inode);
	if (iterator == NULL)
		return B_NO_MEMORY;

	*_cookie = iterator;
	return B_OK;
}


static status_t
ufs2_read_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
			 struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	DirectoryIterator* iterator = (DirectoryIterator*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;

	uint32 maxCount = *_num;
	uint32 count = 0;

	while (count < maxCount
			&& (bufferSize >= sizeof(struct dirent) + B_FILE_NAME_LENGTH)) {
		size_t length = bufferSize - offsetof(struct dirent, d_name);
		ino_t iNodeNo;

		status_t status = iterator->GetNext(dirent->d_name, &length, &iNodeNo);
		if (status == B_ENTRY_NOT_FOUND)
			break;
		if (status == B_BUFFER_OVERFLOW) {
			if (count == 0)
				return status;
			break;
		}
		if (status != B_OK)
			return status;

		dirent->d_dev = volume->ID();
		dirent->d_ino = iNodeNo;
		dirent->d_reclen = offsetof(struct dirent, d_name) + length + 1;
		bufferSize -= dirent->d_reclen;
		dirent = (struct dirent*)((uint8*)dirent + dirent->d_reclen);
		count++;
	}

	*_num = count;
	return B_OK;

}


static status_t
ufs2_rewind_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void *_cookie)
{
	DirectoryIterator *iterator = (DirectoryIterator *)_cookie;
	return iterator->Rewind();
}


static status_t
ufs2_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/,
			  void * /*_cookie*/)
{
	return B_OK;
}


static status_t
ufs2_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	delete (DirectoryIterator*)_cookie;
	return B_OK;
}


static status_t
ufs2_open_attr_dir(fs_volume *_volume, fs_vnode *_node, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_close_attr_dir(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_free_attr_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_read_attr_dir(fs_volume *_volume, fs_vnode *_node,
				  void *_cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_rewind_attr_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


/* attribute operations */
static status_t
ufs2_create_attr(fs_volume *_volume, fs_vnode *_node,
				const char *name, uint32 type, int openMode, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_open_attr(fs_volume *_volume, fs_vnode *_node, const char *name,
			  int openMode, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_close_attr(fs_volume *_volume, fs_vnode *_node,
			   void *cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_free_attr_cookie(fs_volume *_volume, fs_vnode *_node,
					 void *cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_read_attr(fs_volume *_volume, fs_vnode *_node, void *_cookie,
			  off_t pos, void *buffer, size_t *_length)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_write_attr(fs_volume *_volume, fs_vnode *_node, void *cookie,
			   off_t pos, const void *buffer, size_t *length)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_read_attr_stat(fs_volume *_volume, fs_vnode *_node,
				   void *_cookie, struct stat *stat)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_write_attr_stat(fs_volume *_volume, fs_vnode *_node,
					void *cookie, const struct stat *stat, int statMask)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_rename_attr(fs_volume *_volume, fs_vnode *fromVnode,
				const char *fromName, fs_vnode *toVnode, const char *toName)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_remove_attr(fs_volume *_volume, fs_vnode *vnode,
				const char *name)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark -

static status_t
ufs2_std_ops(int32 op, ...)
{
	switch (op)
	{
	case B_MODULE_INIT:
#ifdef ufs2_DEBUGGER_COMMANDS
		// Perform nothing at the moment
		// add_debugger_commands();
#endif
		return B_OK;
	case B_MODULE_UNINIT:
#ifdef ufs2_DEBUGGER_COMMANDS
		// Perform nothing at the moment
		// remove_debugger_commands();
#endif
		return B_OK;

	default:
		return B_ERROR;
	}
}

fs_volume_ops gUfs2VolumeOps = {
	&ufs2_unmount,
	&ufs2_read_fs_info,
	NULL, //write_fs_info()
	NULL, // fs_sync,
	&ufs2_get_vnode,
};

fs_vnode_ops gufs2VnodeOps = {
	/* vnode operations */
	&ufs2_lookup,
	NULL, // ufs2_get_vnode_name - optional, and we can't do better than the
		  // fallback implementation, so leave as NULL.
	&ufs2_put_vnode,
	NULL, // ufs2_remove_vnode,

	/* VM file access */
	&ufs2_can_page,
	&ufs2_read_pages,
	NULL, // ufs2_write_pages,

	&ufs2_io, // io()
	NULL,	// cancel_io()

	&ufs2_get_file_map,

	&ufs2_ioctl,
	NULL,
	NULL, // fs_select
	NULL, // fs_deselect
	NULL, // fs_fsync,

	&ufs2_read_link,
	NULL, // fs_create_symlink,

	NULL, // fs_link,
	&ufs2_unlink,
	NULL, // fs_rename,

	&ufs2_access,
	&ufs2_read_stat,
	NULL, // fs_write_stat,
	NULL, // fs_preallocate

	/* file operations */
	NULL, // fs_create,
	&ufs2_open,
	&ufs2_close,
	&ufs2_free_cookie,
	&ufs2_read,
	NULL, //	fs_write,

	/* directory operations */
	&ufs2_create_dir,
	&ufs2_remove_dir,
	&ufs2_open_dir,
	&ufs2_close_dir,
	&ufs2_free_dir_cookie,
	&ufs2_read_dir,
	&ufs2_rewind_dir,

	/* attribute directory operations */
	&ufs2_open_attr_dir,
	&ufs2_close_attr_dir,
	&ufs2_free_attr_dir_cookie,
	&ufs2_read_attr_dir,
	&ufs2_rewind_attr_dir,

	/* attribute operations */
	&ufs2_create_attr,
	&ufs2_open_attr,
	&ufs2_close_attr,
	&ufs2_free_attr_cookie,
	&ufs2_read_attr,
	&ufs2_write_attr,
	&ufs2_read_attr_stat,
	&ufs2_write_attr_stat,
	&ufs2_rename_attr,
	&ufs2_remove_attr,
};


static file_system_module_info sufs2FileSystem = {
	{
		"file_systems/ufs2" B_CURRENT_FS_API_VERSION,
		0,
		ufs2_std_ops,
	},

	"ufs2",			   // short_name
	"Unix Filesystem 2", // pretty_name

	// DDM flags
	B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
	//	| B_DISK_SYSTEM_SUPPORTS_WRITING
	,

	// scanning
	ufs2_identify_partition,
	ufs2_scan_partition,
	ufs2_free_identify_partition_cookie,
	NULL, // free_partition_content_cookie()

	&ufs2_mount,
};

module_info *modules[] = {
	(module_info *)&sufs2FileSystem,
	NULL,
};
