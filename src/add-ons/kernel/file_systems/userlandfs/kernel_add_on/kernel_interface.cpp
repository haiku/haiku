/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "kernel_interface.h"

#include <dirent.h>

#include <KernelExport.h>
#include <fs_interface.h>

#include "Debug.h"
#include "FileSystem.h"
#include "String.h"
#include "UserlandFS.h"
#include "Volume.h"


// #pragma mark - general


// parse_parameters
static status_t
parse_parameters(const char *parameters, String &fsName,
	const char **fsParameters)
{
	// check parameters
	if (!parameters)
		return B_BAD_VALUE;

	int32 len = strlen(parameters);

	// skip leading white space
	for (; len > 0; parameters++, len--) {
		if (*parameters != ' ' && *parameters != '\t' && *parameters != '\n')
			break;
	}
	if (len == 0)
		return B_BAD_VALUE;

	// get the file system name
	int32 fsNameLen = len;
	for (int32 i = 0; i < len; i++) {
		if (parameters[i] == ' ' || parameters[i] == '\t'
			|| parameters[i] == '\n') {
			fsNameLen = i;
			break;
		}
	}

	fsName.SetTo(parameters, fsNameLen);
	if (fsName.GetLength() == 0) {
		exit_debugging();
		return B_NO_MEMORY;
	}
	parameters += fsNameLen;
	len -= fsNameLen;

	// skip leading white space of the FS parameters
	for (; len > 0; parameters++, len--) {
		if (*parameters != ' ' && *parameters != '\t' && *parameters != '\n')
			break;
	}
	*fsParameters = parameters;

	return B_OK;
}

// userlandfs_mount
static status_t
userlandfs_mount(fs_volume* fsVolume, const char* device, uint32 flags,
	const char* args, ino_t* rootVnodeID)
{
	PRINT(("userlandfs_mount(%p (%ld), %s, 0x%lx, %s, %p)\n", fsVolume,
		fsVolume->id, device, flags, args, rootVnodeID));

	status_t error = B_OK;

	// get the parameters
// TODO: The parameters are in driver settings format now.
	String fsName;
	const char* fsParameters;
	error = parse_parameters(args, fsName, &fsParameters);
	if (error != B_OK)
		RETURN_ERROR(error);

	// get the UserlandFS object
	UserlandFS* userlandFS = UserlandFS::GetUserlandFS();
	if (!userlandFS)
		RETURN_ERROR(B_ERROR);

	// get the file system
	FileSystem* fileSystem = NULL;
	error = userlandFS->RegisterFileSystem(fsName.GetString(), &fileSystem);
	if (error != B_OK)
		RETURN_ERROR(error);

	// mount the volume
	Volume* volume = NULL;
	error = fileSystem->Mount(fsVolume, device, flags, fsParameters, &volume);
	if (error != B_OK) {
		userlandFS->UnregisterFileSystem(fileSystem);
		RETURN_ERROR(error);
	}

	fsVolume->private_volume = volume;
	fsVolume->ops = volume->GetVolumeOps();
	*rootVnodeID = volume->GetRootID();

	PRINT(("userlandfs_mount() done: %p, %lld\n", fsVolume->private_volume,
		*rootVnodeID));

	return error;
}

// userlandfs_unmount
static status_t
userlandfs_unmount(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_unmount(%p)\n", volume));

	FileSystem* fileSystem = volume->GetFileSystem();
	status_t error = volume->Unmount();
	// The error code the FS's unmount hook returns is completely irrelevant to
	// the VFS. It considers the volume unmounted in any case.
	volume->ReleaseReference();
	UserlandFS::GetUserlandFS()->UnregisterFileSystem(fileSystem);

	PRINT(("userlandfs_unmount() done: %lx\n", error));
	return error;
}

// userlandfs_sync
static status_t
userlandfs_sync(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_sync(%p)\n", volume));
	status_t error = volume->Sync();
	PRINT(("userlandfs_sync() done: %lx \n", error));
	return error;
}

// userlandfs_read_fs_info
static status_t
userlandfs_read_fs_info(fs_volume* fsVolume, struct fs_info* info)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_fs_info(%p, %p)\n", volume, info));
	status_t error = volume->ReadFSInfo(info);
	PRINT(("userlandfs_read_fs_info() done: %lx \n", error));
	return error;
}

// userlandfs_write_fs_info
static status_t
userlandfs_write_fs_info(fs_volume* fsVolume, const struct fs_info* info,
	uint32 mask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_write_fs_info(%p, %p, 0x%lx)\n", volume, info, mask));
	status_t error = volume->WriteFSInfo(info, mask);
	PRINT(("userlandfs_write_fs_info() done: %lx \n", error));
	return error;
}


// #pragma mark - vnodes


// userlandfs_lookup
static status_t
userlandfs_lookup(fs_volume* fsVolume, fs_vnode* fsDir, const char* entryName,
	ino_t* vnid)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_lookup(%p, %p, `%s', %p)\n", volume, fsDir->private_node,
		entryName, vnid));
	status_t error = volume->Lookup(fsDir->private_node, entryName, vnid);
	PRINT(("userlandfs_lookup() done: (%lx, %lld)\n", error, *vnid));
	return error;
}

// userlandfs_get_vnode_name
static status_t
userlandfs_get_vnode_name(fs_volume* fsVolume, fs_vnode* fsNode, char* buffer,
	size_t bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_get_vnode_name(%p, %p, %p, %lu)\n", volume,
		fsNode->private_node, buffer, bufferSize));
	status_t error = volume->GetVNodeName(fsNode->private_node, buffer,
		bufferSize);
	PRINT(("userlandfs_get_vnode_name() done: (%lx, \"%.*s\")\n", error,
		(int)bufferSize, (error == B_OK ? buffer : NULL)));
	return error;
}

// userlandfs_get_vnode
static status_t
userlandfs_get_vnode(fs_volume* fsVolume, ino_t vnid, fs_vnode* fsNode,
	int* _type, uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_get_vnode(%p, %lld, %p, %d)\n", volume, vnid,
		fsNode->private_node, reenter));
	void* node;
	fs_vnode_ops* ops;
	status_t error = volume->ReadVNode(vnid, reenter, &node, &ops, _type,
		_flags);
	if (error == B_OK) {
		fsNode->private_node = node;
		fsNode->ops = ops;
	}

	PRINT(("userlandfs_get_vnode() done: (%lx, %p, %#x, %#lx)\n", error, node,
		*_type, *_flags));
	return error;
}

// userlandfs_put_vnode
static status_t
userlandfs_put_vnode(fs_volume* fsVolume, fs_vnode* fsNode, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	PRINT(("userlandfs_put_vnode(%p, %p, %d)\n", volume, fsNode->private_node,
//		reenter));
	status_t error = volume->WriteVNode(fsNode->private_node, reenter);
//	PRINT(("userlandfs_put_vnode() done: %lx\n", error));
	return error;
}

// userlandfs_remove_vnode
static status_t
userlandfs_remove_vnode(fs_volume* fsVolume, fs_vnode* fsNode, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
// DANGER: See userlandfs_write_vnode().
//	PRINT(("userlandfs_remove_vnode(%p, %p, %d)\n", volume,
//		fsNode->private_node, reenter));
	status_t error = volume->RemoveVNode(fsNode->private_node, reenter);
//	PRINT(("userlandfs_remove_vnode() done: %lx\n", error));
	return error;
}


// #pragma mark - asynchronous I/O


// userlandfs_io
status_t
userlandfs_io(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	io_request* request)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_io(%p, %p, %p, %p)\n", volume, fsNode->private_node,
		cookie, request));
	status_t error = volume->DoIO(fsNode->private_node, cookie, request);
	PRINT(("userlandfs_io() done: (%lx)\n", error));
	return error;
}


// userlandfs_cancel_io
status_t
userlandfs_cancel_io(fs_volume* fsVolume, fs_vnode* fsNode, void *cookie,
	io_request *request)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_cancel_io(%p, %p, %p, %p)\n", volume,
		fsNode->private_node, cookie, request));
	status_t error = volume->CancelIO(fsNode->private_node, cookie, request);
	PRINT(("userlandfs_cancel_io() done: (%lx)\n", error));
	return error;
}


// #pragma mark - common


// userlandfs_ioctl
static status_t
userlandfs_ioctl(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie, uint32 op,
	void* buffer, size_t length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_ioctl(%p, %p, %p, %lu, %p, %lu)\n", volume,
		fsNode->private_node, cookie, op, buffer, length));
	status_t error = volume->IOCtl(fsNode->private_node, cookie, op, buffer,
		length);
	PRINT(("userlandfs_ioctl() done: (%lx)\n", error));
	return error;
}

// userlandfs_set_flags
static status_t
userlandfs_set_flags(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	int flags)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_set_flags(%p, %p, %p, %d)\n", volume,
		fsNode->private_node, cookie, flags));
	status_t error = volume->SetFlags(fsNode->private_node, cookie, flags);
	PRINT(("userlandfs_set_flags() done: (%lx)\n", error));
	return error;
}

// userlandfs_select
static status_t
userlandfs_select(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	uint8 event, selectsync* sync)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_select(%p, %p, %p, %hhd, %p)\n", volume,
		fsNode->private_node, cookie, event, sync));
	status_t error = volume->Select(fsNode->private_node, cookie, event, sync);
	PRINT(("userlandfs_select() done: (%lx)\n", error));
	return error;
}

// userlandfs_deselect
static status_t
userlandfs_deselect(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	uint8 event, selectsync* sync)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_deselect(%p, %p, %p, %hhd, %p)\n", volume,
		fsNode->private_node, cookie, event, sync));
	status_t error = volume->Deselect(fsNode->private_node, cookie, event,
		sync);
	PRINT(("userlandfs_deselect() done: (%lx)\n", error));
	return error;
}

// userlandfs_fsync
static status_t
userlandfs_fsync(fs_volume* fsVolume, fs_vnode* fsNode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_fsync(%p, %p)\n", volume, fsNode->private_node));
	status_t error = volume->FSync(fsNode->private_node);
	PRINT(("userlandfs_fsync() done: %lx\n", error));
	return error;
}

// userlandfs_read_symlink
static status_t
userlandfs_read_symlink(fs_volume* fsVolume, fs_vnode* fsLink, char* buffer,
	size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_symlink(%p, %p, %p, %lu)\n", volume,
		fsLink->private_node, buffer, *bufferSize));
	status_t error = volume->ReadSymlink(fsLink->private_node, buffer,
		*bufferSize, bufferSize);
	PRINT(("userlandfs_read_symlink() done: (%lx, %lu)\n", error, *bufferSize));
	return error;
}

// userlandfs_create_symlink
static status_t
userlandfs_create_symlink(fs_volume* fsVolume, fs_vnode* fsDir,
	const char* name, const char* path, int mode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_create_symlink(%p, %p, `%s', `%s', %d)\n", volume,
		fsDir->private_node, name, path, mode));
	status_t error = volume->CreateSymlink(fsDir->private_node, name, path,
		mode);
	PRINT(("userlandfs_create_symlink() done: (%lx)\n", error));
	return error;
}

// userlandfs_link
static status_t
userlandfs_link(fs_volume* fsVolume, fs_vnode* fsDir, const char* name,
	fs_vnode* fsNode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_link(%p, %p, `%s', %p)\n", volume,
		fsDir->private_node, name, fsNode->private_node));
	status_t error = volume->Link(fsDir->private_node, name,
		fsNode->private_node);
	PRINT(("userlandfs_link() done: (%lx)\n", error));
	return error;
}

// userlandfs_unlink
static status_t
userlandfs_unlink(fs_volume* fsVolume, fs_vnode* fsDir, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_unlink(%p, %p, `%s')\n", volume, fsDir->private_node,
		name));
	status_t error = volume->Unlink(fsDir->private_node, name);
	PRINT(("userlandfs_unlink() done: (%lx)\n", error));
	return error;
}

// userlandfs_rename
static status_t
userlandfs_rename(fs_volume* fsVolume, fs_vnode* fsFromDir,
	const char *fromName, fs_vnode* fsToDir, const char *toName)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_rename(%p, %p, `%s', %p, `%s')\n", volume,
		fsFromDir->private_node, fromName, fsToDir->private_node, toName));
	status_t error = volume->Rename(fsFromDir->private_node, fromName,
		fsToDir->private_node, toName);
	PRINT(("userlandfs_rename() done: (%lx)\n", error));
	return error;
}

// userlandfs_access
static status_t
userlandfs_access(fs_volume* fsVolume, fs_vnode* fsNode, int mode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_access(%p, %p, %d)\n", volume, fsNode->private_node,
		mode));
	status_t error = volume->Access(fsNode->private_node, mode);
	PRINT(("userlandfs_access() done: %lx\n", error));
	return error;
}

// userlandfs_read_stat
static status_t
userlandfs_read_stat(fs_volume* fsVolume, fs_vnode* fsNode, struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_stat(%p, %p, %p)\n", volume, fsNode->private_node,
		st));
	status_t error = volume->ReadStat(fsNode->private_node, st);
	PRINT(("userlandfs_read_stat() done: %lx\n", error));
	return error;
}

// userlandfs_write_stat
static status_t
userlandfs_write_stat(fs_volume* fsVolume, fs_vnode* fsNode,
	const struct stat* st, uint32 mask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_write_stat(%p, %p, %p, %ld)\n", volume,
		fsNode->private_node, st, mask));
	status_t error = volume->WriteStat(fsNode->private_node, st, mask);
	PRINT(("userlandfs_write_stat() done: %lx\n", error));
	return error;
}


// #pragma mark - files


// userlandfs_create
static status_t
userlandfs_create(fs_volume* fsVolume, fs_vnode* fsDir, const char* name,
	int openMode, int perms, void** cookie, ino_t* vnid)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_create(%p, %p, `%s', %d, %d, %p, %p)\n", volume,
		fsDir->private_node, name, openMode, perms, cookie, vnid));
	status_t error = volume->Create(fsDir->private_node, name, openMode, perms,
		cookie, vnid);
	PRINT(("userlandfs_create() done: (%lx, %lld, %p)\n", error, *vnid,
		*cookie));
	return error;
}

// userlandfs_open
static status_t
userlandfs_open(fs_volume* fsVolume, fs_vnode* fsNode, int openMode,
	void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_open(%p, %p, %d)\n", volume, fsNode->private_node,
		openMode));
	status_t error = volume->Open(fsNode->private_node, openMode, cookie);
	PRINT(("userlandfs_open() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close
static status_t
userlandfs_close(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_close(%p, %p, %p)\n", volume, fsNode->private_node,
		cookie));
	status_t error = volume->Close(fsNode->private_node, cookie);
	PRINT(("userlandfs_close() done: %lx\n", error));
	return error;
}

// userlandfs_free_cookie
static status_t
userlandfs_free_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_free_cookie(%p, %p, %p)\n", volume, fsNode->private_node,
		cookie));
	status_t error = volume->FreeCookie(fsNode->private_node, cookie);
	PRINT(("userlandfs_free_cookie() done: %lx\n", error));
	return error;
}

// userlandfs_read
static status_t
userlandfs_read(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie, off_t pos,
	void* buffer, size_t* length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read(%p, %p, %p, %Ld, %p, %lu)\n", volume,
		fsNode->private_node, cookie, pos, buffer, *length));
	status_t error = volume->Read(fsNode->private_node, cookie, pos, buffer,
		*length, length);
	PRINT(("userlandfs_read() done: (%lx, %lu)\n", error, *length));
	return error;
}

// userlandfs_write
static status_t
userlandfs_write(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie, off_t pos,
	const void* buffer, size_t* length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_write(%p, %p, %p, %Ld, %p, %lu)\n", volume,
		fsNode->private_node, cookie, pos, buffer, *length));
	status_t error = volume->Write(fsNode->private_node, cookie, pos, buffer,
		*length, length);
	PRINT(("userlandfs_write() done: (%lx, %lu)\n", error, *length));
	return error;
}


// #pragma mark - directories


// userlandfs_create_dir
static status_t
userlandfs_create_dir(fs_volume* fsVolume, fs_vnode* fsParent, const char* name,
	int perms)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_create_dir(%p, %p, `%s', %#x)\n", volume,
		fsParent->private_node, name, perms));
	status_t error = volume->CreateDir(fsParent->private_node, name, perms);
	PRINT(("userlandfs_create_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_remove_dir
static status_t
userlandfs_remove_dir(fs_volume* fsVolume, fs_vnode* fsParent, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_remove_dir(%p, %p, `%s')\n", volume,
		fsParent->private_node, name));
	status_t error = volume->RemoveDir(fsParent->private_node, name);
	PRINT(("userlandfs_remove_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_open_dir
static status_t
userlandfs_open_dir(fs_volume* fsVolume, fs_vnode* fsNode, void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_open_dir(%p, %p)\n", volume, fsNode->private_node));
	status_t error = volume->OpenDir(fsNode->private_node, cookie);
	PRINT(("userlandfs_open_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_dir
static status_t
userlandfs_close_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_close_dir(%p, %p, %p)\n", volume, fsNode->private_node,
		cookie));
	status_t error = volume->CloseDir(fsNode->private_node, cookie);
	PRINT(("userlandfs_close_dir() done: %lx\n", error));
	return error;
}

// userlandfs_free_dir_cookie
static status_t
userlandfs_free_dir_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_free_dir_cookie(%p, %p, %p)\n", volume,
		fsNode->private_node, cookie));
	status_t error = volume->FreeDirCookie(fsNode->private_node, cookie);
	PRINT(("userlandfs_free_dir_cookie() done: %lx \n", error));
	return error;
}

// userlandfs_read_dir
static status_t
userlandfs_read_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_dir(%p, %p, %p, %p, %lu, %lu)\n", volume,
		fsNode->private_node, cookie, buffer, bufferSize, *count));
	status_t error = volume->ReadDir(fsNode->private_node, cookie, buffer,
		bufferSize, *count, count);
	PRINT(("userlandfs_read_dir() done: (%lx, %lu)\n", error, *count));
	#if DEBUG
		dirent* entry = buffer;
		for (uint32 i = 0; error == B_OK && i < *count; i++) {
			// R5's kernel vsprintf() doesn't seem to know `%.<number>s', so
			// we need to work around.
			char name[B_FILE_NAME_LENGTH];
			int nameLen = strnlen(entry->d_name, B_FILE_NAME_LENGTH - 1);
			strncpy(name, entry->d_name, nameLen);
			name[nameLen] = '\0';
			PRINT(("  entry: d_dev: %ld, d_pdev: %ld, d_ino: %Ld, d_pino: %Ld, "
				"d_reclen: %hu, d_name: `%s'\n",
				entry->d_dev, entry->d_pdev, entry->d_ino, entry->d_pino,
				entry->d_reclen, name));
			entry = (dirent*)((char*)entry + entry->d_reclen);
		}
	#endif
	return error;
}

// userlandfs_rewind_dir
static status_t
userlandfs_rewind_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_rewind_dir(%p, %p, %p)\n", volume, fsNode->private_node,
		cookie));
	status_t error = volume->RewindDir(fsNode->private_node, cookie);
	PRINT(("userlandfs_rewind_dir() done: %lx\n", error));
	return error;
}


// #pragma mark - attribute directories


// userlandfs_open_attr_dir
static status_t
userlandfs_open_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_open_attr_dir(%p, %p)\n", volume, fsNode->private_node));
	status_t error = volume->OpenAttrDir(fsNode->private_node, cookie);
	PRINT(("userlandfs_open_attr_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_attr_dir
static status_t
userlandfs_close_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_close_attr_dir(%p, %p, %p)\n", volume,
		fsNode->private_node, cookie));
	status_t error = volume->CloseAttrDir(fsNode->private_node, cookie);
	PRINT(("userlandfs_close_attr_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_attr_dir_cookie
static status_t
userlandfs_free_attr_dir_cookie(fs_volume* fsVolume, fs_vnode* fsNode,
	void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_free_attr_dir_cookie(%p, %p, %p)\n", volume,
		fsNode->private_node, cookie));
	status_t error = volume->FreeAttrDirCookie(fsNode->private_node, cookie);
	PRINT(("userlandfs_free_attr_dir_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_attr_dir
static status_t
userlandfs_read_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_attr_dir(%p, %p, %p, %p, %lu, %lu)\n", volume,
		fsNode->private_node, cookie, buffer, bufferSize, *count));
	status_t error = volume->ReadAttrDir(fsNode->private_node, cookie, buffer,
		bufferSize, *count, count);
	PRINT(("userlandfs_read_attr_dir() done: (%lx, %lu)\n", error, *count));
	return error;
}

// userlandfs_rewind_attr_dir
static status_t
userlandfs_rewind_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_rewind_attr_dir(%p, %p, %p)\n", volume,
		fsNode->private_node, cookie));
	status_t error = volume->RewindAttrDir(fsNode->private_node, cookie);
	PRINT(("userlandfs_rewind_attr_dir() done: (%lx)\n", error));
	return error;
}


// #pragma mark - attributes


// userlandfs_create_attr
status_t
userlandfs_create_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	uint32 type, int openMode, void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_create_attr(%p, %p, \"%s\", 0x%lx, %d, %p)\n", volume,
		fsNode->private_node, name, type, openMode, cookie));
	status_t error = volume->CreateAttr(fsNode->private_node, name, type,
		openMode, cookie);
	PRINT(("userlandfs_create_attr() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_open_attr
status_t
userlandfs_open_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	int openMode, void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_open_attr(%p, %p, \"%s\", %d, %p)\n", volume,
		fsNode->private_node, name, openMode, cookie));
	status_t error = volume->OpenAttr(fsNode->private_node, name, openMode,
		cookie);
	PRINT(("userlandfs_open_attr() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_attr
status_t
userlandfs_close_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_close_attr(%p, %p, %p)\n", volume, fsNode->private_node,
		cookie));
	status_t error = volume->CloseAttr(fsNode->private_node, cookie);
	PRINT(("userlandfs_close_attr() done: %lx\n", error));
	return error;
}

// userlandfs_free_attr_cookie
status_t
userlandfs_free_attr_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_free_attr_cookie(%p, %p, %p)\n", volume,
		fsNode->private_node, cookie));
	status_t error = volume->FreeAttrCookie(fsNode->private_node, cookie);
	PRINT(("userlandfs_free_attr_cookie() done: %lx\n", error));
	return error;
}

// userlandfs_read_attr
static status_t
userlandfs_read_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t pos, void* buffer, size_t* length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_attr(%p, %p, %p, %lld, %p, %lu)\n", volume,
		fsNode->private_node, cookie, pos, buffer, *length));
	status_t error = volume->ReadAttr(fsNode->private_node, cookie, pos, buffer,
		*length, length);
	PRINT(("userlandfs_read_attr() done: (%lx, %lu)\n", error, *length));
	return error;
}

// userlandfs_write_attr
static status_t
userlandfs_write_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t pos, const void* buffer, size_t* length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_write_attr(%p, %p, %p, %lld, %p, %lu)\n", volume,
		fsNode->private_node, cookie, pos, buffer, *length));
	status_t error = volume->WriteAttr(fsNode->private_node, cookie, pos,
		buffer, *length, length);
	PRINT(("userlandfs_write_attr() done: (%lx, %lu)\n", error, *length));
	return error;
}

// userlandfs_read_attr_stat
static status_t
userlandfs_read_attr_stat(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_attr_stat(%p, %p, %p, %p)\n", volume,
		fsNode->private_node, cookie, st));
	status_t error = volume->ReadAttrStat(fsNode->private_node, cookie, st);
	PRINT(("userlandfs_read_attr_stat() done: (%lx)\n", error));
	return error;
}

// userlandfs_write_attr_stat
static status_t
userlandfs_write_attr_stat(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	const struct stat* st, int statMask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_write_attr_stat(%p, %p, %p, %p, 0x%x)\n", volume,
		fsNode->private_node, cookie, st, statMask));
	status_t error = volume->WriteAttrStat(fsNode->private_node, cookie, st,
		statMask);
	PRINT(("userlandfs_write_attr_stat() done: (%lx)\n", error));
	return error;
}

// userlandfs_rename_attr
static status_t
userlandfs_rename_attr(fs_volume* fsVolume, fs_vnode* fsFromNode,
	const char* fromName, fs_vnode* fsToNode, const char* toName)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_rename_attr(%p, %p, `%s', %p, `%s')\n", volume,
		fsFromNode->private_node, fromName, fsToNode->private_node, toName));
	status_t error = volume->RenameAttr(fsFromNode->private_node, fromName,
		fsToNode->private_node, toName);
	PRINT(("userlandfs_rename_attr() done: (%lx)\n", error));
	return error;
}

// userlandfs_remove_attr
static status_t
userlandfs_remove_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_remove_attr(%p, %p, `%s')\n", volume,
		fsNode->private_node, name));
	status_t error = volume->RemoveAttr(fsNode->private_node, name);
	PRINT(("userlandfs_remove_attr() done: (%lx)\n", error));
	return error;
}


// #pragma mark - indices


// userlandfs_open_index_dir
static status_t
userlandfs_open_index_dir(fs_volume* fsVolume, void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_open_index_dir(%p, %p)\n", volume, cookie));
	status_t error = volume->OpenIndexDir(cookie);
	PRINT(("userlandfs_open_index_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_index_dir
static status_t
userlandfs_close_index_dir(fs_volume* fsVolume, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_close_index_dir(%p, %p)\n", volume, cookie));
	status_t error = volume->CloseIndexDir(cookie);
	PRINT(("userlandfs_close_index_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_index_dir_cookie
static status_t
userlandfs_free_index_dir_cookie(fs_volume* fsVolume, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_free_index_dir_cookie(%p, %p)\n", volume, cookie));
	status_t error = volume->FreeIndexDirCookie(cookie);
	PRINT(("userlandfs_free_index_dir_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_index_dir
static status_t
userlandfs_read_index_dir(fs_volume* fsVolume, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_index_dir(%p, %p, %p, %lu, %lu)\n", volume, cookie,
		buffer, bufferSize, *count));
	status_t error = volume->ReadIndexDir(cookie, buffer, bufferSize,
		*count, count);
	PRINT(("userlandfs_read_index_dir() done: (%lx, %lu)\n", error, *count));
	return error;
}

// userlandfs_rewind_index_dir
static status_t
userlandfs_rewind_index_dir(fs_volume* fsVolume, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_rewind_index_dir(%p, %p)\n", volume, cookie));
	status_t error = volume->RewindIndexDir(cookie);
	PRINT(("userlandfs_rewind_index_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_create_index
static status_t
userlandfs_create_index(fs_volume* fsVolume, const char* name, uint32 type,
	uint32 flags)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_create_index(%p, `%s', 0x%lx, 0x%lx)\n", volume, name,
		type, flags));
	status_t error = volume->CreateIndex(name, type, flags);
	PRINT(("userlandfs_create_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_remove_index
static status_t
userlandfs_remove_index(fs_volume* fsVolume, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_remove_index(%p, `%s')\n", volume, name));
	status_t error = volume->RemoveIndex(name);
	PRINT(("userlandfs_remove_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_index_stat
static status_t
userlandfs_read_index_stat(fs_volume* fsVolume, const char* name,
	struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_index_stat(%p, `%s', %p)\n", volume, name, st));
	status_t error = volume->ReadIndexStat(name, st);
	PRINT(("userlandfs_read_index_stat() done: (%lx)\n", error));
	return error;
}


// #pragma mark - queries


// userlandfs_open_query
static status_t
userlandfs_open_query(fs_volume* fsVolume, const char *queryString,
	uint32 flags, port_id port, uint32 token, void** cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_open_query(%p, `%s', %lu, %ld, %lu, %p)\n", volume,
		queryString, flags, port, token, cookie));
	status_t error = volume->OpenQuery(queryString, flags, port, token, cookie);
	PRINT(("userlandfs_open_query() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_query
static status_t
userlandfs_close_query(fs_volume* fsVolume, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_close_query(%p, %p)\n", volume, cookie));
	status_t error = volume->CloseQuery(cookie);
	PRINT(("userlandfs_close_query() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_query_cookie
static status_t
userlandfs_free_query_cookie(fs_volume* fsVolume, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_free_query_cookie(%p, %p)\n", volume, cookie));
	status_t error = volume->FreeQueryCookie(cookie);
	PRINT(("userlandfs_free_query_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_query
static status_t
userlandfs_read_query(fs_volume* fsVolume, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_read_query(%p, %p, %p, %lu, %lu)\n", volume, cookie,
		buffer, bufferSize, *count));
	status_t error = volume->ReadQuery(cookie, buffer, bufferSize, *count,
		count);
	PRINT(("userlandfs_read_query() done: (%lx, %ld)\n", error, *count));
	#if DEBUG
		if (error == B_OK && *count > 0) {
			// R5's kernel vsprintf() doesn't seem to know `%.<number>s', so
			// we need to work around.
			char name[B_FILE_NAME_LENGTH];
			int nameLen = strnlen(buffer->d_name, B_FILE_NAME_LENGTH - 1);
			strncpy(name, buffer->d_name, nameLen);
			name[nameLen] = '\0';
			PRINT(("  entry: d_dev: %ld, d_pdev: %ld, d_ino: %Ld, d_pino: %Ld, "
				"d_reclen: %hu, d_name: `%s'\n",
				buffer->d_dev, buffer->d_pdev, buffer->d_ino, buffer->d_pino,
				buffer->d_reclen, name));
		}
	#endif
	return error;
}

// userlandfs_rewind_query
static status_t
userlandfs_rewind_query(fs_volume* fsVolume, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	PRINT(("userlandfs_rewind_query(%p, %p)\n", volume, cookie));
	status_t error = volume->RewindQuery(cookie);
	PRINT(("userlandfs_rewind_query() done: (%lx)\n", error));
	return error;
}


// userlandfs_initialize
/*
static status_t
userlandfs_initialize(const char *deviceName, void *parameters,
	size_t len)
{
	// get the parameters
	String fsName;
	const char* fsParameters;
	int32 fsParameterLength;
	status_t error = parse_parameters(parameters, len, fsName, &fsParameters,
		&fsParameterLength);
	// make sure there is a UserlandFS we can work with
	UserlandFS* userlandFS = NULL;
	error = UserlandFS::RegisterUserlandFS(&userlandFS);
	if (error != B_OK) {
		exit_debugging();
		return error;
	}
	// get the file system
	FileSystem* fileSystem = NULL;
	if (error == B_OK)
		error = userlandFS->RegisterFileSystem(fsName.GetString(), &fileSystem);
	// initialize the volume
	if (error == B_OK) {
		error = fileSystem->Initialize(deviceName, fsParameters,
			fsParameterLength);
	}
	// cleanup
	if (fileSystem)
		userlandFS->UnregisterFileSystem(fileSystem);
	UserlandFS::UnregisterUserlandFS();
	return error;
}*/



// #pragma mark ----- module -----


static status_t
userlandfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			init_debugging();
			PRINT(("userlandfs_std_ops(): B_MODULE_INIT\n"));

			// make sure there is a UserlandFS we can work with
			UserlandFS* userlandFS = NULL;
			status_t error = UserlandFS::InitUserlandFS(&userlandFS);
			if (error != B_OK) {
				exit_debugging();
				return error;
			}

			return B_OK;
		}

		case B_MODULE_UNINIT:
			PRINT(("userlandfs_std_ops(): B_MODULE_UNINIT\n"));
			UserlandFS::UninitUserlandFS();
			exit_debugging();
			return B_OK;

		default:
			return B_ERROR;
	}
}


static file_system_module_info sUserlandFSModuleInfo = {
	{
		"file_systems/userlandfs" B_CURRENT_FS_API_VERSION,
		0,
		userlandfs_std_ops,
	},

	"userlandfs",				// short name
	"Userland File System",		// pretty name
	0,	// DDM flags

	// scanning
	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	// general operations
	&userlandfs_mount,

	// capability querying
	NULL,	// get_supported_operations()
	NULL,	// validate_resize()
	NULL,	// validate_move()
	NULL,	// validate_set_content_name()
	NULL,	// validate_set_content_parameters()
	NULL,	// validate_initialize()

	// shadow partition modification
	NULL,	// shadow_changed()

	// writing
	NULL,	// defragment()
	NULL,	// repair()
	NULL,	// resize()
	NULL,	// move()
	NULL,	// set_content_name()
	NULL,	// set_content_parameters()
	NULL	// initialize()
};


fs_volume_ops gUserlandFSVolumeOps = {
	// general operations
	&userlandfs_unmount,
	&userlandfs_read_fs_info,
	&userlandfs_write_fs_info,
	&userlandfs_sync,

	&userlandfs_get_vnode,

	// index directory & index operations
	&userlandfs_open_index_dir,
	&userlandfs_close_index_dir,
	&userlandfs_free_index_dir_cookie,
	&userlandfs_read_index_dir,
	&userlandfs_rewind_index_dir,

	&userlandfs_create_index,
	&userlandfs_remove_index,
	&userlandfs_read_index_stat,

	// query operations
	&userlandfs_open_query,
	&userlandfs_close_query,
	&userlandfs_free_query_cookie,
	&userlandfs_read_query,
	&userlandfs_rewind_query,

	/* support for FS layers */
	NULL,	// all_layers_mounted()
	NULL,	// create_sub_vnode()
	NULL	// delete_sub_vnode()
};


fs_vnode_ops gUserlandFSVnodeOps = {
	// vnode operations
	&userlandfs_lookup,
	&userlandfs_get_vnode_name,
	&userlandfs_put_vnode,
	&userlandfs_remove_vnode,

	// VM file access
	NULL,	// can_page() -- obsolete
	NULL,	// read_pages() -- obsolete
	NULL,	// write_pages() -- obsolete

	// asynchronous I/O
	&userlandfs_io,
	&userlandfs_cancel_io,

	// cache file access
	NULL,	// get_file_map() -- not needed

	// common operations
	&userlandfs_ioctl,
	&userlandfs_set_flags,
	&userlandfs_select,
	&userlandfs_deselect,
	&userlandfs_fsync,

	&userlandfs_read_symlink,
	&userlandfs_create_symlink,

	&userlandfs_link,
	&userlandfs_unlink,
	&userlandfs_rename,

	&userlandfs_access,
	&userlandfs_read_stat,
	&userlandfs_write_stat,
	NULL,	// preallocate()

	// file operations
	&userlandfs_create,
	&userlandfs_open,
	&userlandfs_close,
	&userlandfs_free_cookie,
	&userlandfs_read,
	&userlandfs_write,

	// directory operations
	&userlandfs_create_dir,
	&userlandfs_remove_dir,
	&userlandfs_open_dir,
	&userlandfs_close_dir,
	&userlandfs_free_dir_cookie,
	&userlandfs_read_dir,
	&userlandfs_rewind_dir,

	// attribute directory operations
	&userlandfs_open_attr_dir,
	&userlandfs_close_attr_dir,
	&userlandfs_free_attr_dir_cookie,
	&userlandfs_read_attr_dir,
	&userlandfs_rewind_attr_dir,

	// attribute operations
	&userlandfs_create_attr,
	&userlandfs_open_attr,
	&userlandfs_close_attr,
	&userlandfs_free_attr_cookie,
	&userlandfs_read_attr,
	&userlandfs_write_attr,

	&userlandfs_read_attr_stat,
	&userlandfs_write_attr_stat,
	&userlandfs_rename_attr,
	&userlandfs_remove_attr,

	// support for node and FS layers
	NULL,	// create_special_node()
	NULL	// get_super_vnode()
};


module_info *modules[] = {
	(module_info *)&sUserlandFSModuleInfo,
	NULL,
};
