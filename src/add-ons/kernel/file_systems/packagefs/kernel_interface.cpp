/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>


extern fs_volume_ops gPackageFSVolumeOps;
extern fs_vnode_ops gPackageFSVnodeOps;


//	#pragma mark - Volume


static status_t
packagefs_mount(fs_volume* _volume, const char* device, uint32 flags,
	const char* parameters, ino_t* rootID)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_unmount(fs_volume* fs)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_read_fs_info(fs_volume* fs, struct fs_info* info)
{
	return B_UNSUPPORTED;
}


// #pragma mark - VNodes


static status_t
packagefs_lookup(fs_volume* fs, fs_vnode* _dir, const char* entryName,
	ino_t* vnid)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_get_vnode(fs_volume* fs, ino_t vnid, fs_vnode* node, int* _type,
	uint32* _flags, bool reenter)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_put_vnode(fs_volume* fs, fs_vnode* _node, bool reenter)
{
	return B_UNSUPPORTED;
}


// #pragma mark - Nodes


static status_t
packagefs_read_symlink(fs_volume* fs, fs_vnode* _node, char* buffer,
	size_t* bufferSize)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_access(fs_volume* fs, fs_vnode* _node, int mode)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_read_stat(fs_volume* fs, fs_vnode* _node, struct stat* st)
{
	return B_UNSUPPORTED;
}


// #pragma mark - Files


static status_t
packagefs_open(fs_volume* fs, fs_vnode* _node, int openMode, void** cookie)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_close(fs_volume* fs, fs_vnode* _node, void* cookie)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_free_cookie(fs_volume* fs, fs_vnode* _node, void* cookie)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_read(fs_volume* fs, fs_vnode* _node, void* cookie, off_t pos,
	void* buffer, size_t* bufferSize)
{
	return B_UNSUPPORTED;
}


// #pragma mark - Directories


static status_t
packagefs_open_dir(fs_volume* fs, fs_vnode* _node, void** cookie)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_close_dir(fs_volume* fs, fs_vnode* _node, void* cookie)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_free_dir_cookie(fs_volume* fs, fs_vnode* _node, void* cookie)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_read_dir(fs_volume* fs, fs_vnode* _node, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* count)
{
	return B_UNSUPPORTED;
}


static status_t
packagefs_rewind_dir(fs_volume* fs, fs_vnode* _node, void* cookie)
{
	return B_UNSUPPORTED;
}


// #pragma mark - Module Interface


static status_t
packagefs_std_ops(int32 op, ...)
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


static file_system_module_info sPackageFSModuleInfo = {
	{
		"file_systems/packagefs" B_CURRENT_FS_API_VERSION,
		0,
		packagefs_std_ops,
	},

	"packagefs",				// short_name
	"Package File System",		// pretty_name
	0,							// DDM flags


	// scanning
	NULL,	// identify_partition,
	NULL,	// scan_partition,
	NULL,	// free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&packagefs_mount
};


fs_volume_ops gPackageFSVolumeOps = {
	&packagefs_unmount,
	&packagefs_read_fs_info,
	NULL,	// write_fs_info,
	NULL,	// sync,

	&packagefs_get_vnode

	// TODO: index operations
	// TODO: query operations
	// TODO: FS layer operations
};


fs_vnode_ops gPackageFSVnodeOps = {
	/* vnode operations */
	&packagefs_lookup,
	NULL,	// get_vnode_name,
	&packagefs_put_vnode,
	NULL,	// remove_vnode,

	/* VM file access */
	NULL,	// can_page,
	NULL,	// read_pages,
	NULL,	// write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

	NULL,	// get_file_map,

	NULL,	// ioctl,
	NULL,	// set_flags,
	NULL,	// select,
	NULL,	// deselect,
	NULL,	// fsync,

	&packagefs_read_symlink,
	NULL,	// create_symlink,

	NULL,	// link,
	NULL,	// unlink,
	NULL,	// rename,

	&packagefs_access,
	&packagefs_read_stat,
	NULL,	// write_stat,

	/* file operations */
	NULL,	// create,
	&packagefs_open,
	&packagefs_close,
	&packagefs_free_cookie,
	&packagefs_read,
	NULL,	// write,

	/* directory operations */
	NULL,	// create_dir,
	NULL,	// remove_dir,
	&packagefs_open_dir,
	&packagefs_close_dir,
	&packagefs_free_dir_cookie,
	&packagefs_read_dir,
	&packagefs_rewind_dir

	// TODO: attribute directory operations
	// TODO: attribute operations
	// TODO: FS layer operations
};


module_info *modules[] = {
	(module_info *)&sPackageFSModuleInfo,
	NULL,
};
