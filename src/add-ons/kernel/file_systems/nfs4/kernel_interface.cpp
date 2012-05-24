/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <fs_interface.h>


extern fs_volume_ops gNFSv4VolumeOps;
extern fs_vnode_ops gNFSv4VnodeOps;

extern "C" void
dprintf(const char* format, ...);


static status_t
nfs4_mount(fs_volume* volume, const char* device, uint32 flags,
			const char* args, ino_t* _rootVnodeID)
{
	dprintf("NFS4 Mounting...\n");

	volume->ops = &gNFSv4VolumeOps;

	status_t error = publish_vnode(volume, 0, (void*)0xdeadbeef,
									&gNFSv4VnodeOps, S_IFDIR, 0);
	if (error != B_OK)
		return error;

	*_rootVnodeID = 0;

	dprintf("NFS4 Mounted\n");

	return B_OK;
}


static status_t
nfs4_unmount(fs_volume* volume)
{
	dprintf("NFS4 Unmounting...\n");
	return B_OK;
}


static status_t
nfs4_put_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	return B_OK;
}


static status_t
nfs4_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			dprintf("NFS4 Init\n");
			return B_OK;

		case B_MODULE_UNINIT:
			dprintf("NFS4 Uninit\n");
			return B_OK;

		default:
			return B_ERROR;
	}
}


fs_volume_ops gNFSv4VolumeOps = {
	&nfs4_unmount
};

fs_vnode_ops gNFSv4VnodeOps = {
	NULL,	// lookup()
	NULL,	// get_vnode_name()
	&nfs4_put_vnode
};

static file_system_module_info sNFSv4ModuleInfo = {
	{
		"file_systems/nfs4" B_CURRENT_FS_API_VERSION,
		0,
		nfs4_std_ops,
	},

	"nfs4",								// short_name
	"Network File System version 4",	// pretty_name

	// DDM flags
	0,

	// scanning
	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	&nfs4_mount,
};

module_info* modules[] = {
	(module_info* )&sNFSv4ModuleInfo,
	NULL,
};

