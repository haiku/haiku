/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "kernel_interface.h"

#include <new>

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"
#include "Directory.h"
#include "Volume.h"


static const uint32 kOptimalIOSize = 64 * 1024;


//	#pragma mark - Volume


static status_t
packagefs_mount(fs_volume* fsVolume, const char* device, uint32 flags,
	const char* parameters, ino_t* _rootID)
{
	FUNCTION("fsVolume: %p, device: \"%s\", flags: %#lx, parameters: \"%s\"\n",
		fsVolume, device, flags, parameters);

	// create a Volume object
	Volume* volume = new(std::nothrow) Volume(fsVolume);
	if (volume == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Volume> volumeDeleter(volume);

	status_t error = volume->Mount();
	if (error != B_OK)
		return error;

	// set return values
	*_rootID = volume->RootDirectory()->ID();
	fsVolume->private_volume = volumeDeleter.Detach();
	fsVolume->ops = &gPackageFSVolumeOps;

	return B_OK;
}


static status_t
packagefs_unmount(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p\n", volume);

	volume->Unmount();
	delete volume;

	return B_OK;
}


static status_t
packagefs_read_fs_info(fs_volume* fsVolume, struct fs_info* info)
{
	FUNCTION("volume: %p, info: %p\n", fsVolume->private_volume, info);

	info->flags = B_FS_IS_READONLY;
	info->block_size = 4096;
	info->io_size = kOptimalIOSize;
	info->total_blocks = info->free_blocks = 1;
	strlcpy(info->volume_name, "Package FS", sizeof(info->volume_name));
	return B_OK;
}


// #pragma mark - VNodes


static status_t
packagefs_lookup(fs_volume* fsVolume, fs_vnode* fsDir, const char* entryName,
	ino_t* _vnid)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* dir = (Node*)fsDir->private_node;

	FUNCTION("volume: %p, dir: %p (%lld), entry: \"%s\"\n", volume, dir,
		dir->ID(), entryName);

	return B_UNSUPPORTED;
}


static status_t
packagefs_get_vnode(fs_volume* fsVolume, ino_t vnid, fs_vnode* fsNode,
	int* _type, uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p, vnid: %lld\n", volume, vnid);

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
packagefs_read_stat(fs_volume* fsVolume, fs_vnode* fsNode, struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%lld)\n", volume, node, node->ID());

// TODO: Fill in correctly!
	st->st_mode = S_IFDIR;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_size = 0;
	st->st_blksize = kOptimalIOSize;
	st->st_atime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;
	st->st_crtime = 0;

	return B_OK;
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
			init_debugging();
			PRINT("package_std_ops(): B_MODULE_INIT\n");
			return B_OK;

		case B_MODULE_UNINIT:
			PRINT("package_std_ops(): B_MODULE_UNINIT\n");
			exit_debugging();
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
