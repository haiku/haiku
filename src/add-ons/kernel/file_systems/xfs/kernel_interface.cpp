/*
 * Copyright 2001-2017, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "system_dependencies.h"
#include "Directory.h"
#include "Inode.h"
#include "ShortAttribute.h"
#include "Symlink.h"
#include "Volume.h"

#include <file_systems/fs_ops_support.h>


#define XFS_IO_SIZE	65536

struct identify_cookie {
	/*	super_block_struct super_block;
	*	No structure yet implemented.
	*/
	int cookie;
};

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

	*_rootID = volume->Root();
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
	TRACE("XFS_READ_FS_INFO:\n");
	Volume* volume = (Volume*)_volume->private_volume;

	info->flags = B_FS_IS_READONLY
					| B_FS_HAS_ATTR | B_FS_IS_PERSISTENT;

	info->io_size = XFS_IO_SIZE;
	info->block_size = volume->SuperBlock().BlockSize();
	info->total_blocks = volume->SuperBlock().TotalBlocks();
	info->free_blocks = volume->SuperBlock().FreeBlocks();

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// Filesystem name
	if(volume->IsVersion5())
		strlcpy(info->fsh_name, "xfs V5", sizeof(info->fsh_name));
	else
		strlcpy(info->fsh_name, "xfs V4", sizeof(info->fsh_name));

	return B_OK;
}


//	#pragma mark -


static status_t
xfs_get_vnode(fs_volume *_volume, ino_t id, fs_vnode *_node, int *_type,
	uint32 *_flags, bool reenter)
{
	TRACE("XFS_GET_VNODE:\n");
	Volume* volume = (Volume*)_volume->private_volume;

	Inode* inode = new(std::nothrow) Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status_t status = inode->Init();
	if (status != B_OK) {
		delete inode;
		ERROR("get_vnode: InitCheck() failed. Error: %s\n", strerror(status));
		return B_NO_INIT;
	}

	_node->private_node = inode;
	_node->ops = &gxfsVnodeOps;
	*_type = inode->Mode();
	*_flags = 0;
	TRACE("(%ld)\n", inode->ID());
	return B_OK;
}


static status_t
xfs_put_vnode(fs_volume *_volume, fs_vnode *_node, bool reenter)
{
	TRACE("XFS_PUT_VNODE:\n");
	delete (Inode*)_node->private_node;
	return B_OK;
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
	TRACE("XFS_LOOKUP: %p (%s)\n", name, name);
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (!directory->IsDirectory())
		return B_NOT_A_DIRECTORY;

	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		return status;

	DirectoryIterator* iterator = DirectoryIterator::Init(directory);
	if (iterator == NULL)
		return B_BAD_VALUE;

	status = iterator->Lookup(name, strlen(name), (xfs_ino_t*)_vnodeID);
	if (status != B_OK) {
		delete iterator;
		return status;
	}

	TRACE("XFS_LOOKUP: ID: (%ld)\n", *_vnodeID);
	status = get_vnode(volume->FSVolume(), *_vnodeID, NULL);
	TRACE("get_vnode status: (%d)\n", status);
	return status;
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
	Inode* inode = (Inode*)_node->private_node;
	TRACE("XFS_READ_STAT: id: (%ld)\n", inode->ID());
	stat->st_dev = inode->GetVolume()->ID();
	stat->st_ino = inode->ID();
	stat->st_nlink = 1;
	stat->st_blksize = XFS_IO_SIZE;

	stat->st_uid = inode->UserId();
	stat->st_gid = inode->GroupId();
	stat->st_mode = inode->Mode();
	stat->st_type = 0;	// TODO

	stat->st_size = inode->Size();
	stat->st_blocks = inode->BlockCount();

	inode->GetAccessTime(stat->st_atim);
	inode->GetModificationTime(stat->st_mtim);
	inode->GetChangeTime(stat->st_ctim);

	// Only version 3 Inodes has creation time
	if(inode->Version() == 3)
		inode->GetCreationTime(stat->st_crtim);
	else
		inode->GetChangeTime(stat->st_crtim);

	return B_OK;
}


static status_t
xfs_open(fs_volume * /*_volume*/, fs_vnode *_node, int openMode,
	void **_cookie)
{
	TRACE("XFS_OPEN:\n");
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

	cookie->open_mode = openMode & XFS_OPEN_MODE_USER_MASK;
	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


static status_t
xfs_read(fs_volume *_volume, fs_vnode *_node, void *_cookie, off_t pos,
	void *buffer, size_t *_length)
{
	TRACE("Inode::ReadAt: pos:(%ld), *length:(%ld)\n", pos, *_length);
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsFile()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	return inode->ReadAt(pos, (uint8*)buffer, _length);
}


static status_t
xfs_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_OK;
}


static status_t
xfs_free_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	TRACE("XFS_FREE_COOKIE:\n");
	file_cookie* cookie = (file_cookie*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (inode->Size() != cookie->last_size)
		notify_stat_changed(volume->ID(), -1, inode->ID(), B_STAT_SIZE);

	delete cookie;
	return B_OK;
}


static status_t
xfs_access(fs_volume *_volume, fs_vnode *_node, int accessMode)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->CheckPermissions(accessMode);
}


static status_t
xfs_read_link(fs_volume *_volume, fs_vnode *_node, char *buffer,
	size_t *_bufferSize)
{
	TRACE("XFS_READ_SYMLINK\n");

	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsSymLink())
		return B_BAD_VALUE;

	Symlink symlink(inode);

	status_t result = symlink.ReadLink(0, buffer, _bufferSize);

	return result;
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
	Inode* inode = (Inode*)_node->private_node;
	TRACE("XFS_OPEN_DIR: (%ld)\n", inode->ID());

	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	if (!inode->IsDirectory())
		return B_NOT_A_DIRECTORY;

	DirectoryIterator* iterator = DirectoryIterator::Init(inode);
	if (iterator == NULL)
		return B_BAD_VALUE;

	*_cookie = iterator;
	return status;
}


static status_t
xfs_read_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *_num)
{
	TRACE("XFS_READ_DIR\n");
	DirectoryIterator* iterator = (DirectoryIterator*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;

	uint32 maxCount = *_num;
	uint32 count = 0;

	while (count < maxCount && (bufferSize > sizeof(struct dirent))) {
		size_t length = bufferSize - sizeof(struct dirent);
		xfs_ino_t ino;

		status_t status = iterator->GetNext(buffer->d_name, &length, &ino);
		if (status == B_ENTRY_NOT_FOUND)
			break;
		if (status == B_BUFFER_OVERFLOW) {
			if (count == 0)
				return status;
			break;
		}
		if (status != B_OK)
			return status;

		buffer->d_dev = volume->ID();
		buffer->d_ino = ino;

		buffer = next_dirent(buffer, length, bufferSize);
		count++;
	}

	*_num = count;
	TRACE("Count: (%d)\n", count);
	return B_OK;
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
	return B_OK;
}


static status_t
xfs_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	delete (DirectoryIterator*)_cookie;
	return B_NOT_SUPPORTED;
}


static status_t
xfs_open_attr_dir(fs_volume *_volume, fs_vnode *_node, void **_cookie)
{
	Inode* inode = (Inode*)_node->private_node;
	TRACE("%s()\n", __FUNCTION__);

	// Attributes are only on files
	if (!inode->IsFile())
		return B_NOT_SUPPORTED;

	Attribute* iterator = Attribute::Init(inode);
	if (iterator == NULL)
		return B_BAD_VALUE;

	*_cookie = iterator;
	return B_OK;
}


static status_t
xfs_close_attr_dir(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	TRACE("%s()\n", __FUNCTION__);
	return B_OK;
}


static status_t
xfs_free_attr_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	delete (Attribute*)_cookie;
	return B_OK;
}


static status_t
xfs_read_attr_dir(fs_volume *_volume, fs_vnode *_node,
	void *_cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	TRACE("%s()\n", __FUNCTION__);
	Attribute* iterator = (Attribute*)_cookie;

	size_t length = bufferSize;
	status_t status = iterator->GetNext(dirent->d_name, &length);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	}

	if (status != B_OK)
		return status;

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;
	dirent->d_dev = volume->ID();
	dirent->d_ino = inode->ID();
	dirent->d_reclen = offsetof(struct dirent, d_name) + length + 1;
	*_num = 1;

	return B_OK;
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
	TRACE("%s()\n", __FUNCTION__);

	status_t status;

	Inode* inode = (Inode*)_node->private_node;

	int accessMode = open_mode_to_access(openMode) | (openMode & O_TRUNC ? W_OK : 0);
	status = inode->CheckPermissions(accessMode);
	if (status < B_OK)
		return status;

	Attribute* attribute = Attribute::Init(inode);

	if (attribute == NULL)
		return B_BAD_VALUE;

	status = attribute->Open(name, openMode, (attr_cookie**)_cookie);
	delete attribute;

	return status;
}


static status_t
xfs_close_attr(fs_volume *_volume, fs_vnode *_node,
	void *cookie)
{
	return B_OK;
}


static status_t
xfs_free_attr_cookie(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	delete (attr_cookie*)cookie;
	return B_OK;
}


static status_t
xfs_read_attr(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	off_t pos, void *buffer, size_t *_length)
{
	TRACE("%s()\n", __FUNCTION__);

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	Attribute* attribute = Attribute::Init(inode);

	if (attribute == NULL)
		return B_BAD_VALUE;

	status_t status = attribute->Read(cookie, pos, (uint8*)buffer, _length);
	delete attribute;

	return status;
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
	TRACE("%s()\n", __FUNCTION__);

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	Attribute* attribute = Attribute::Init(inode);

	if (attribute == NULL)
		return B_BAD_VALUE;

	status_t status = attribute->Stat(cookie, *stat);
	delete attribute;

	return status;
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
