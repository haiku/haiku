/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <arpa/inet.h>
#include <stdio.h>

#include <net/dns_resolver.h>
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


RPC::ProgramData*
CreateNFS4Server(RPC::Server* serv)
{
	return new NFS4Server(serv);
}


static status_t
sParseArguments(const char* _args, uint32* _ip, char* _path)
{
	if (_args == NULL)
		return B_BAD_VALUE;

	char* args = strdup(_args);
	char* path = strpbrk(args, ":");
	if (path == NULL) {
		free(args);
		return B_MISMATCHED_VALUES;
	}
	*path++ = '\0';

	status_t result;
	struct in_addr addr;
	if (inet_aton(args, &addr) == 0) {
			dns_resolver_module* dns;
			result = get_module(DNS_RESOLVER_MODULE_NAME,
				reinterpret_cast<module_info**>(&dns));
			if (result != B_OK) {
				free(args);
				return result;
			}

			result = dns->dns_resolve(args, _ip);
			put_module(DNS_RESOLVER_MODULE_NAME);
			if (result != B_OK) {
				free(args);
				return result;
			}
	} else
		*_ip = addr.s_addr;
	*_ip = ntohl(*_ip);

	_path[255] = '\0';
	strncpy(_path, path, 255);

	free(args);
	return B_OK;
}


static status_t
nfs4_mount(fs_volume* volume, const char* device, uint32 flags,
			const char* args, ino_t* _rootVnodeID)
{
	status_t result;

	uint32 ip;
	char path[256];
	result = sParseArguments(args, &ip, path);
	if (result != B_OK)
		return result;

	RPC::Server *server;
	result = gRPCServerManager->Acquire(&server, ip, 2049, ProtocolUDP,
		CreateNFS4Server);
	if (result != B_OK)
		return result;
	
	Filesystem* fs;
	result = Filesystem::Mount(&fs, server, path, volume->id);
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
nfs4_read_fs_info(fs_volume* volume, struct fs_info* info)
{
	Filesystem* fs = reinterpret_cast<Filesystem*>(volume->private_volume);
	return fs->ReadInfo(info);
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
	inode->FileSystem()->InoIdMap()->RemoveEntry(inode->ID());
	delete inode;

	return B_OK;
}


static status_t
nfs4_set_flags(fs_volume* volume, fs_vnode* vnode, void* _cookie, int flags)
{
	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);
	cookie->fMode = (cookie->fMode & ~(O_APPEND | O_NONBLOCK)) | flags;
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
nfs4_create_symlink(fs_volume* volume, fs_vnode* dir, const char* name,
	const char* path, int mode)
{
	Inode* inode = reinterpret_cast<Inode*>(dir->private_node);
	return inode->CreateLink(name, path, mode);
}


static status_t
nfs4_link(fs_volume* volume, fs_vnode* dir, const char* name, fs_vnode* vnode)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	Inode* dirInode = reinterpret_cast<Inode*>(dir->private_node);
	return inode->Link(dirInode, name);
}


static status_t
nfs4_unlink(fs_volume* volume, fs_vnode* dir, const char* name)
{
	Inode* inode = reinterpret_cast<Inode*>(dir->private_node);
	return inode->Remove(name, NF4REG);
}


static status_t
nfs4_rename(fs_volume* volume, fs_vnode* fromDir, const char* fromName,
	fs_vnode* toDir, const char* toName)
{
	Inode* fromInode = reinterpret_cast<Inode*>(fromDir->private_node);
	Inode* toInode = reinterpret_cast<Inode*>(toDir->private_node);
	return Inode::Rename(fromInode, toInode, fromName, toName);
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
nfs4_write_stat(fs_volume* volume, fs_vnode* vnode, const struct stat* stat,
	uint32 statMask)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	return inode->WriteStat(stat, statMask);
}


static status_t
nfs4_create(fs_volume* volume, fs_vnode* dir, const char* name, int openMode,
	int perms, void** _cookie, ino_t* _newVnodeID)
{
	OpenFileCookie* cookie = new OpenFileCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	*_cookie = cookie;

	Inode* inode = reinterpret_cast<Inode*>(dir->private_node);
	status_t result = inode->Create(name, openMode, perms, cookie, _newVnodeID);
	if (result != B_OK)
		delete cookie;

	void* ptr;
	result = get_vnode(volume, *_newVnodeID, &ptr);
	if (result != B_OK)
		delete cookie;

	return result;
}


static status_t
nfs4_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR || inode->Type() == S_IFLNK) {
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
nfs4_close(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR || inode->Type() == S_IFLNK)
		return B_OK;

	Cookie* cookie = reinterpret_cast<Cookie*>(_cookie);
	return cookie->CancelAll();
}


static status_t
nfs4_free_cookie(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR || inode->Type() == S_IFLNK)
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

	if (inode->Type() == S_IFLNK)
		return B_BAD_VALUE;

	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);

	return inode->Read(cookie, pos, buffer, length);
}


static status_t
nfs4_write(fs_volume* volume, fs_vnode* vnode, void* _cookie, off_t pos,
	const void* buffer, size_t* length)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);

	if (inode->Type() == S_IFDIR)
		return B_IS_A_DIRECTORY;

	if (inode->Type() == S_IFLNK)
		return B_BAD_VALUE;

	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);

	return inode->Write(cookie, pos, buffer, length);
}


static status_t
nfs4_create_dir(fs_volume* volume, fs_vnode* parent, const char* name,
	int mode)
{
	Inode* inode = reinterpret_cast<Inode*>(parent->private_node);
	return inode->CreateDir(name, mode);
}


static status_t
nfs4_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	Inode* inode = reinterpret_cast<Inode*>(parent->private_node);
	return inode->Remove(name, NF4DIR);
}


static status_t
nfs4_open_dir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	OpenDirCookie* cookie = new(std::nothrow) OpenDirCookie;
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
nfs4_close_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	Cookie* cookie = reinterpret_cast<Cookie*>(_cookie);
	return cookie->CancelAll();
}


static status_t
nfs4_free_dir_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	delete reinterpret_cast<OpenDirCookie*>(cookie);
	return B_OK;
}


static status_t
nfs4_read_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie,
				struct dirent* buffer, size_t bufferSize, uint32* _num)
{
	OpenDirCookie* cookie = reinterpret_cast<OpenDirCookie*>(_cookie);
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


static status_t
nfs4_test_lock(fs_volume* volume, fs_vnode* vnode, void* _cookie,
	struct flock* lock)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);
	return inode->TestLock(cookie, lock);
}


static status_t
nfs4_acquire_lock(fs_volume* volume, fs_vnode* vnode, void* _cookie,
			const struct flock* lock, bool wait)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);
	return inode->AcquireLock(cookie, lock, wait);
}


static status_t
nfs4_release_lock(fs_volume* volume, fs_vnode* vnode, void* _cookie,
			const struct flock* lock)
{
	Inode* inode = reinterpret_cast<Inode*>(vnode->private_node);
	OpenFileCookie* cookie = reinterpret_cast<OpenFileCookie*>(_cookie);
	if (lock != NULL)
		return inode->ReleaseLock(cookie, lock);
	else
		return inode->ReleaseAllLocks(cookie);
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
	nfs4_read_fs_info,
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
	nfs4_set_flags,
	NULL,	// fs_select()
	NULL,	// fs_deselect()
	NULL,	// fsync()

	nfs4_read_symlink,
	nfs4_create_symlink,

	nfs4_link,
	nfs4_unlink,
	nfs4_rename,

	nfs4_access,
	nfs4_read_stat,
	nfs4_write_stat,
	NULL,	// fs_preallocate()

	/* file operations */
	nfs4_create,
	nfs4_open,
	nfs4_close,
	nfs4_free_cookie,
	nfs4_read,
	nfs4_write,

	/* directory operations */
	nfs4_create_dir,
	nfs4_remove_dir,
	nfs4_open_dir,
	nfs4_close_dir,
	nfs4_free_dir_cookie,
	nfs4_read_dir,
	nfs4_rewind_dir,

	/* attribute directory operations */
	NULL,	// open_attr_dir
	NULL,	// close_attr_dir
	NULL,	// free_attr_dir_cookie
	NULL,	// read_attr_dir
	NULL,	// rewind_attr_dir

	/* attribute operations */
	NULL,	// create_attr
	NULL,	// open_attr
	NULL,	// close_attr
	NULL,	// free_attr_cookie
	NULL,	// read_attr
	NULL,	// write_attr

	NULL,	// read_attr_stat
	NULL,	// write_attr_stat
	NULL,	// rename_attr
	NULL,	// remove_attr

	/* support for node and FS layers */
	NULL,	// create_special_node
	NULL,	// get_super_vnode

	/* lock operations */
	nfs4_test_lock,
	nfs4_acquire_lock,
	nfs4_release_lock,
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

