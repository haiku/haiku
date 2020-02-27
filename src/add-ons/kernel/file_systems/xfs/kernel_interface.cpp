/*
 * Copyright 2001-2017, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "system_dependencies.h"
#include "Volume.h"


struct identify_cookie
{
	/*	super_block_struct super_block;
	 *	No structure yet implemented.
	 */
	int cookie;
};


//!	xfs_io() callback hook
static status_t
iterative_io_get_vecs_hook(void *cookie, io_request *request, off_t offset,
	size_t size, struct file_io_vec *vecs, size_t *_count)
{
	return B_NOT_SUPPORTED;
}


//!	xfs_io() callback hook
static status_t
iterative_io_finished_hook(void *cookie, io_request *request, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark - Scanning


static float
xfs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static void
xfs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	dprintf("Unsupported in XFS currently.\n");
	return;
}


//	#pragma mark -


static status_t
xfs_mount(fs_volume *_volume, const char *device, uint32 flags,
	const char *args, ino_t *_rootID)
{
	TRACE("xfs_mount(): Trying to mount\n");

	Volume *volume = new (std::nothrow) Volume(_volume);
	if (volume == NULL)
		return B_NO_MEMORY;

	_volume->private_volume = volume;
	_volume->ops = &gxfsVolumeOps;

	status_t status = volume->Mount(device, flags);
	if (status != B_OK) {
		ERROR("Failed mounting the volume. Error: %s\n", strerror(status));
		delete volume;
		_volume->private_volume = NULL;
		return status;
	}

/* Don't have Inodes yet */
#if 0
	 *_rootID = volume->Root()->ID();
#endif

	return B_OK;
}


static status_t
xfs_unmount(fs_volume *_volume)
{
	Volume* volume = (Volume*) _volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;
	
	TRACE("xfs_unmount(): Deleted volume");
	return status;
}


static status_t
xfs_read_fs_info(fs_volume *_volume, struct fs_info *info)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark -


static status_t
xfs_get_vnode(fs_volume *_volume, ino_t id, fs_vnode *_node, int *_type,
	uint32 *_flags, bool reenter)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_put_vnode(fs_volume *_volume, fs_vnode *_node, bool reenter)
{
	return B_NOT_SUPPORTED;
}


static bool
xfs_can_page(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_pages(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	off_t pos, const iovec *vecs, size_t count, size_t *_numBytes)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_io(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	io_request *request)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_get_file_map(fs_volume *_volume, fs_vnode *_node, off_t offset,
	size_t size, struct file_io_vec *vecs, size_t *_count)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark -


static status_t
xfs_lookup(fs_volume *_volume, fs_vnode *_directory, const char *name,
	ino_t *_vnodeID)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_ioctl(fs_volume *_volume, fs_vnode *_node, void *_cookie, uint32 cmd,
	void *buffer, size_t bufferLength)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_stat(fs_volume *_volume, fs_vnode *_node, struct stat *stat)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_open(fs_volume * /*_volume*/, fs_vnode *_node, int openMode,
	void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read(fs_volume *_volume, fs_vnode *_node, void *_cookie, off_t pos,
	void *buffer, size_t *_length)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_free_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_access(fs_volume *_volume, fs_vnode *_node, int accessMode)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_link(fs_volume *_volume, fs_vnode *_node, char *buffer,
	size_t *_bufferSize)
{
	return B_NOT_SUPPORTED;
}


status_t
xfs_unlink(fs_volume *_volume, fs_vnode *_directory, const char *name)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark - Directory functions


static status_t
xfs_create_dir(fs_volume *_volume, fs_vnode *_directory, const char *name,
	int mode)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_remove_dir(fs_volume *_volume, fs_vnode *_directory, const char *name)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_open_dir(fs_volume * /*_volume*/, fs_vnode *_node, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_rewind_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/,
	void * /*_cookie*/)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_open_attr_dir(fs_volume *_volume, fs_vnode *_node, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_close_attr_dir(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_free_attr_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_attr_dir(fs_volume *_volume, fs_vnode *_node,
	void *_cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_rewind_attr_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_NOT_SUPPORTED;
}


/* attribute operations */
static status_t
xfs_create_attr(fs_volume *_volume, fs_vnode *_node,
	const char *name, uint32 type, int openMode, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_open_attr(fs_volume *_volume, fs_vnode *_node, const char *name,
	int openMode, void **_cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_close_attr(fs_volume *_volume, fs_vnode *_node,
	void *cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_free_attr_cookie(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_attr(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	off_t pos, void *buffer, size_t *_length)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_write_attr(fs_volume *_volume, fs_vnode *_node, void *cookie,
	off_t pos, const void *buffer, size_t *length)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_read_attr_stat(fs_volume *_volume, fs_vnode *_node,
	void *_cookie, struct stat *stat)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_write_attr_stat(fs_volume *_volume, fs_vnode *_node,
	void *cookie, const struct stat *stat, int statMask)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_rename_attr(fs_volume *_volume, fs_vnode *fromVnode,
	const char *fromName, fs_vnode *toVnode, const char *toName)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_remove_attr(fs_volume *_volume, fs_vnode *vnode,
	const char *name)
{
	return B_NOT_SUPPORTED;
}


static uint32
xfs_get_supported_operations(partition_data *partition, uint32 mask)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_initialize(int fd, partition_id partitionID, const char *name,
	const char *parameterString, off_t partitionSize, disk_job_id job)
{
	return B_NOT_SUPPORTED;
}


static status_t
xfs_uninitialize(int fd, partition_id partitionID, off_t partitionSize,
	uint32 blockSize, disk_job_id job)
{
	return B_NOT_SUPPORTED;
}


//	#pragma mark -


static status_t
xfs_std_ops(int32 op, ...)
{
	switch (op)
	{
	case B_MODULE_INIT:
#ifdef XFS_DEBUGGER_COMMANDS
		// Perform nothing at the moment
		// add_debugger_commands();
#endif
		return B_OK;
	case B_MODULE_UNINIT:
#ifdef XFS_DEBUGGER_COMMANDS
		// Perform nothing at the moment
		// remove_debugger_commands();
#endif
		return B_OK;

	default:
		return B_ERROR;
	}
}


fs_volume_ops gxfsVolumeOps = {
	&xfs_unmount,
	&xfs_read_fs_info,
	NULL,				// write_fs_info()
	NULL,				// fs_sync,
	&xfs_get_vnode,
};


fs_vnode_ops gxfsVnodeOps = {
	/* vnode operations */
	&xfs_lookup,
	NULL,				// xfs_get_vnode_name- optional, and we can't do better
						// than the fallback implementation, so leave as NULL.
	&xfs_put_vnode,
	NULL, 				// xfs_remove_vnode,

	/* VM file access */
	&xfs_can_page,
	&xfs_read_pages,
	NULL,				// xfs_write_pages,

	&xfs_io,			// io()
	NULL,				// cancel_io()

	&xfs_get_file_map,

	&xfs_ioctl,
	NULL,
	NULL,				// fs_select
	NULL,				// fs_deselect
	NULL,				// fs_fsync,

	&xfs_read_link,
	NULL,				// fs_create_symlink,

	NULL,				// fs_link,
	&xfs_unlink,
	NULL,				// fs_rename,

	&xfs_access,
	&xfs_read_stat,
	NULL,				// fs_write_stat,
	NULL,				// fs_preallocate

	/* file operations */
	NULL,				// fs_create,
	&xfs_open,
	&xfs_close,
	&xfs_free_cookie,
	&xfs_read,
	NULL,				// fs_write,

	/* directory operations */
	&xfs_create_dir,
	&xfs_remove_dir,
	&xfs_open_dir,
	&xfs_close_dir,
	&xfs_free_dir_cookie,
	&xfs_read_dir,
	&xfs_rewind_dir,

	/* attribute directory operations */
	&xfs_open_attr_dir,
	&xfs_close_attr_dir,
	&xfs_free_attr_dir_cookie,
	&xfs_read_attr_dir,
	&xfs_rewind_attr_dir,

	/* attribute operations */
	&xfs_create_attr,
	&xfs_open_attr,
	&xfs_close_attr,
	&xfs_free_attr_cookie,
	&xfs_read_attr,
	&xfs_write_attr,
	&xfs_read_attr_stat,
	&xfs_write_attr_stat,
	&xfs_rename_attr,
	&xfs_remove_attr,
};


static
file_system_module_info sxfsFileSystem = {
	{
		"file_systems/xfs" B_CURRENT_FS_API_VERSION,
		0,
		xfs_std_ops,
	},

	"xfs",				// short_name
	"XFS File System",	// pretty_name

	// DDM flags
	0 |B_DISK_SYSTEM_SUPPORTS_INITIALIZING |B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
	//	| B_DISK_SYSTEM_SUPPORTS_WRITING
	,

	// scanning
	xfs_identify_partition,
	xfs_scan_partition,
	xfs_free_identify_partition_cookie,
	NULL,				// free_partition_content_cookie()

	&xfs_mount,

	/* capability querying operations */
	&xfs_get_supported_operations,

	NULL,				// validate_resize
	NULL,				// validate_move
	NULL,				// validate_set_content_name
	NULL,				// validate_set_content_parameters
	NULL,				// validate_initialize,

	/* shadow partition modification */
	NULL,				// shadow_changed

	/* writing */
	NULL,				// defragment
	NULL,				// repair
	NULL,				// resize
	NULL,				// move
	NULL,				// set_content_name
	NULL,				// set_content_parameters
	xfs_initialize,
	xfs_uninitialize};

module_info *modules[] = {
	(module_info *)&sxfsFileSystem,
	NULL,
};
