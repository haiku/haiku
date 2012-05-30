/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <fs_interface.h>

#include "Connection.h"
#include "RPCServer.h"
#include "NFS4Defs.h"
#include "Inode.h"
#include "RequestBuilder.h"
#include "ReplyInterpreter.h"
#include "Filesystem.h"


extern fs_volume_ops gNFSv4VolumeOps;
extern fs_vnode_ops gNFSv4VnodeOps;

extern "C" void
dprintf(const char* format, ...);


RPC::ServerManager* gRPCServerManager;


static status_t
nfs4_mount(fs_volume* volume, const char* device, uint32 flags,
			const char* args, ino_t* _rootVnodeID)
{
	status_t result;

	RPC::Server *server;
	// hardcoded ip 192.168.1.70
	result = gRPCServerManager->Acquire(&server, 0xc0a80146, 2049, ProtocolUDP);
	if (result != B_OK)
		return result;
	
	Filesystem* fs;
	// hardcoded path
	result = Filesystem::Mount(&fs, server, "haiku/src/add-ons/kernel");
	if (result != B_OK) {
		gRPCServerManager->Release(server);
		return result;
	}

	Inode* inode = fs->CreateRootInode();

	volume->private_volume = fs;
	volume->ops = &gNFSv4VolumeOps;

	result = publish_vnode(volume, inode->ID(), inode, &gNFSv4VnodeOps,
							inode->Type(), 0);
	if (result != B_OK)
		return result;

	*_rootVnodeID = inode->ID();

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


status_t
nfs4_init()
{
	dprintf("NFS4 Init\n");

	status_t result = Connection::Init();
	if (result != B_OK)
		return result;

	gRPCServerManager = new(std::nothrow) RPC::ServerManager;
	if (gRPCServerManager == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


status_t
nfs4_uninit()
{
	dprintf("NFS4 Uninit\n");

	delete gRPCServerManager;

	status_t result = Connection::CleanUp();
	if (result != B_OK)
		return result;
	return B_OK;
}


static status_t
nfs4_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return nfs4_init();
		case B_MODULE_UNINIT:
			return nfs4_uninit();
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

