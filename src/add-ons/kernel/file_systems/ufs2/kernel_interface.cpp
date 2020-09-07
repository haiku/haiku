/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

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


#if 0
//!	ufs2_io() callback hook
static status_t
iterative_io_get_vecs_hook(void *cookie, io_request *request, off_t offset,
						size_t size, struct file_io_vec *vecs, size_t *_count)
{
	return B_NOT_SUPPORTED;
}


//!	ufs2_io() callback hook
static status_t
iterative_io_finished_hook(void *cookie, io_request *request, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	return B_NOT_SUPPORTED;
}
#endif


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
	partition->content_size = partition->block_size
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
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_read_fs_info(fs_volume *_volume, struct fs_info *info)
{
	Volume* volume = (Volume*)_volume->private_volume;

	// File system flags
	info->flags = B_FS_IS_PERSISTENT
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);
	info->io_size = 65536;
	info->block_size = volume->SuperBlock().fs_sbsize;
	info->total_blocks = volume->SuperBlock().fs_size;

	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

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
	return B_NOT_SUPPORTED;
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

	status_t status = DirectoryIterator(directory).Lookup(name, strlen(name),
		(ino_t*)_vnodeID);

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
//	TODO handle hardlinks which will have nlink > 1. Maybe linkCount in inode
//	structure may help?
	stat->st_nlink = 1;
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
	stat->st_blocks = (inode->Size() + 511) / 512;

	return B_OK;
}


static status_t
ufs2_open(fs_volume * _volume, fs_vnode *_node, int openMode,
		 void **_cookie)
{
	//Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;
	if (inode->IsDirectory())
		return B_IS_A_DIRECTORY;

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
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_free_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}

static status_t
ufs2_access(fs_volume *_volume, fs_vnode *_node, int accessMode)
{
	return B_OK;
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
		size_t length = bufferSize;
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
		dirent->d_reclen = sizeof(struct dirent) + length;
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
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/,
			  void * /*_cookie*/)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
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


static uint32
ufs2_get_supported_operations(partition_data *partition, uint32 mask)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_initialize(int fd, partition_id partitionID, const char *name,
			const char *parameterString, off_t partitionSize, disk_job_id job)
{
	return B_NOT_SUPPORTED;
}


static status_t
ufs2_uninitialize(int fd, partition_id partitionID, off_t partitionSize,
				 uint32 blockSize, disk_job_id job)
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
	0| B_DISK_SYSTEM_SUPPORTS_INITIALIZING |B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
	//	| B_DISK_SYSTEM_SUPPORTS_WRITING
	,

	// scanning
	ufs2_identify_partition,
	ufs2_scan_partition,
	ufs2_free_identify_partition_cookie,
	NULL, // free_partition_content_cookie()

	&ufs2_mount,

	/* capability querying operations */
	&ufs2_get_supported_operations,

	NULL, // validate_resize
	NULL, // validate_move
	NULL, // validate_set_content_name
	NULL, // validate_set_content_parameters
	NULL, // validate_initialize,

	/* shadow partition modification */
	NULL, // shadow_changed

	/* writing */
	NULL, // defragment
	NULL, // repair
	NULL, // resize
	NULL, // move
	NULL, // set_content_name
	NULL, // set_content_parameters
	ufs2_initialize,
	ufs2_uninitialize};

module_info *modules[] = {
	(module_info *)&sufs2FileSystem,
	NULL,
};
