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


static RPC::ProgramData*
sCreateNFS4Server(RPC::Server* serv)
{
	return new NFS4Server(serv);
}


static status_t
nfs4_mount(fs_volume* volume, const char* device, uint32 flags,
			const char* args, ino_t* _rootVnodeID)
{
	status_t result;

	RPC::Server *server;
	// hardcoded ip 192.168.1.70
	result = gRPCServerManager->Acquire(&server, 0xc0a80146, 2049, ProtocolUDP,
		sCreateNFS4Server);
	if (result != B_OK)
		return result;
	
	Filesystem* fs;
	// hardcoded path
	result = Filesystem::Mount(&fs, server, "haiku/src/add-ons/kernel",
				volume->id);
	if (result != B_OK) {
		gRPCServerManager->Release(server);
		return result;
	}

	Inode* inode = fs->CreateRootInode();
	if (inode == NULL) {
		delete fs;
		gRPCServerManager->Release(server);

		return B_IO_ERROR;
	}

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
nfs4_get_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type,
	uint32* _flags, bool reenter)
{
	Filesystem* fs = reinterpret_cast<Filesystem*>(volume->private_volume);
	Inode* inode;	
	status_t result = fs->GetInode(id, &inode);
	if (result != B_OK)
		return result;

	vnode->ops = &gNFSv4VnodeOps;
	vnode->private_node = inode;

	*_type = inode->Type();
	*_flags = 0;

	return B_OK;
}


static status_t
nfs4_unmount(fs_volume* volume)
{
	Filesystem* fs = reinterpret_cast<Filesystem*>(volume->private_volume);
	RPC::Server* server = fs->Server();

	delete fs;
	gRPCServerManager->Release(server);

	return B_OK;
}


static status_t
nfs4_lookup(fs_volume* volume, fs_vnode* dir, const char* name, ino_t* _id)
{
	Inode* inode = reinterpret_cast<Inode*>(dir->private_node);
	status_t result = inode->LookUp(name, _id);
	if (result != B_OK)
		return result;

	void* ptr;
	return get_vnode(volume, *_id, &ptr);
}


static status_t
nfs4_get_vnode_name(fs_volume* volume, fs_vnode* vnode, char* buffer,
	size_t bufferSize)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	strncpy(buffer, inode->Name(), bufferSize);
	return B_OK;
}


static status_t
nfs4_put_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	delete inode;

	return B_OK;
}


static status_t
nfs4_read_symlink(fs_volume* volume, fs_vnode* link, char* buffer,
	size_t* _bufferSize)
{
	Inode* inode = reinterpret_cast<Inode*>(link->private_node);
	return inode->ReadLink(buffer, _bufferSize);
}


static status_t
nfs4_access(fs_volume* volume, fs_vnode* vnode, int mode)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	return inode->Access(mode);
}


static status_t
nfs4_read_stat(fs_volume* volume, fs_vnode* vnode, struct stat* stat)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	return inode->Stat(stat);
}


static status_t
nfs4_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR) {
		*_cookie = NULL;
		return B_OK;
	}

	OpenFileCookie* cookie = new OpenFileCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	*_cookie = cookie;


	status_t result = inode->Open(openMode, cookie);
	if (result != B_OK)
		delete cookie;

	return result;
}


static status_t
nfs4_close(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
nfs4_free_cookie(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR)
		return B_OK;

	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);
	inode->Close(cookie);
	delete cookie;

	return B_OK;
}


static status_t
nfs4_read(fs_volume* volume, fs_vnode* vnode, void* _cookie, off_t pos,
	void* buffer, size_t* length)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR)
		return B_IS_A_DIRECTORY;

	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);

	return inode->Read(cookie, pos, buffer, length);
}


static status_t
nfs4_open_dir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	uint64* cookie = new(std::nothrow) uint64[2];
	if (cookie == NULL)
		return B_NO_MEMORY;
	*_cookie = cookie;

	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	status_t result = inode->OpenDir(cookie);
	if (result != B_OK)
		delete cookie;

	return result;
}


static status_t 
nfs4_close_dir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
nfs4_free_dir_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	delete[] reinterpret_cast<uint64*>(cookie);
	return B_OK;
}


static status_t
nfs4_read_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie,
				struct dirent* buffer, size_t bufferSize, uint32* _num)
{
	uint64* cookie = reinterpret_cast<uint64*>(_cookie);
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	return inode->ReadDir(buffer, bufferSize, _num, cookie);
}


static status_t
nfs4_rewind_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	uint64* cookie = reinterpret_cast<uint64*>(_cookie);
	cookie[0] = 0;
	cookie[1] = 2;

	return B_OK;
}

status_t
nfs4_init()
{
	dprintf("NFS4 Init\n");

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
	nfs4_unmount,
	NULL,
	NULL,
	NULL,
	nfs4_get_vnode,
};

fs_vnode_ops gNFSv4VnodeOps = {
	nfs4_lookup,
	nfs4_get_vnode_name,
	nfs4_put_vnode,
	NULL,	// remove_vnode()

	/* VM file access */
	NULL,	// can_page()
	NULL,	// read_pages()
	NULL,	// write_pages()

	NULL,	// io()
	NULL,	// cancel_io()

	NULL,	// get_file_map()

	NULL,	// ioctl()
	NULL,
	NULL,	// fs_select()
	NULL,	// fs_deselect()
	NULL,	// fsync()

	nfs4_read_symlink,
	NULL,	// create_symlink()

	NULL,	// link()
	NULL,	// unlink()
	NULL,	// rename()

	nfs4_access,
	nfs4_read_stat,
	NULL,	// write_stat()
	NULL,	// fs_preallocate()

	/* file operations */
	NULL,	// create()
	nfs4_open,
	nfs4_close,
	nfs4_free_cookie,
	nfs4_read,
	NULL,	// write,

	/* directory operations */
	NULL,	// create_dir()
	NULL,	// remove_dir()
	nfs4_open_dir,
	nfs4_close_dir,
	nfs4_free_dir_cookie,
	nfs4_read_dir,
	nfs4_rewind_dir,
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

	nfs4_mount,
};

module_info* modules[] = {
	(module_info* )&sNFSv4ModuleInfo,
	NULL,
};

