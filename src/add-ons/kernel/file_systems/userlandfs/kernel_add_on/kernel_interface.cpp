// kernel_interface.cpp

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
userlandfs_mount(mount_id id, const char *device, uint32 flags, 
	const char *args, fs_volume *fsCookie, vnode_id *rootVnodeID)
{
	status_t error = B_OK;

	// get the parameters
// TODO: The parameters are in driver settings format now.
	String fsName;
	const char* fsParameters;
	error = parse_parameters(args, fsName, &fsParameters);
	if (error != B_OK)
		return error;

	// get the UserlandFS object
	UserlandFS* userlandFS = UserlandFS::GetUserlandFS();
	if (!userlandFS)
		return B_ERROR;

	// get the file system
	FileSystem* fileSystem = NULL;
	error = userlandFS->RegisterFileSystem(fsName.GetString(), &fileSystem);
	if (error != B_OK)
		return error;

	// mount the volume
	Volume* volume = NULL;
	error = fileSystem->Mount(id, device, flags, fsParameters, &volume);
	if (error != B_OK) {
		userlandFS->UnregisterFileSystem(fileSystem);
		return error;
	}

	*fsCookie = volume;
	*rootVnodeID = volume->GetRootID();

	return error;
}

// userlandfs_unmount
static status_t
userlandfs_unmount(fs_volume ns)
{
	Volume* volume = (Volume*)ns;
	FileSystem* fileSystem = volume->GetFileSystem();
	status_t error = volume->Unmount();
	// The error code the FS's unmount hook returns is completely irrelevant to
	// the VFS. It considers the volume unmounted in any case.
	volume->RemoveReference();
	UserlandFS::GetUserlandFS()->UnregisterFileSystem(fileSystem);
	return error;
}

// userlandfs_sync
static status_t
userlandfs_sync(fs_volume fs)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_sync(%p)\n", fs));
	status_t error = volume->Sync();
	PRINT(("userlandfs_sync() done: %lx \n", error));
	return error;
}

// userlandfs_read_fs_info
static status_t
userlandfs_read_fs_info(fs_volume fs, struct fs_info *info)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_fs_info(%p, %p)\n", fs, info));
	status_t error = volume->ReadFSInfo(info);
	PRINT(("userlandfs_read_fs_info() done: %lx \n", error));
	return error;
}

// userlandfs_write_fs_info
static status_t
userlandfs_write_fs_info(fs_volume fs, const struct fs_info *info, uint32 mask)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_write_fs_info(%p, %p, 0x%lx)\n", fs, info, mask));
	status_t error = volume->WriteFSInfo(info, mask);
	PRINT(("userlandfs_write_fs_info() done: %lx \n", error));
	return error;
}


// #pragma mark - vnodes


// userlandfs_lookup
static status_t
userlandfs_lookup(fs_volume fs, fs_vnode dir, const char *entryName,
	vnode_id *vnid, int *type)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_lookup(%p, %p, `%s', %p, %p)\n", fs, dir,
		entryName, vnid, type));
	status_t error = volume->Lookup(dir, entryName, vnid, type);
	PRINT(("userlandfs_lookup() done: (%lx, %lld, 0x%x)\n", error, *vnid,
		*type));
	return error;
}

// TODO: userlandfs_get_vnode_name()

// userlandfs_get_vnode
static status_t
userlandfs_get_vnode(fs_volume fs, vnode_id vnid, fs_vnode *node, bool reenter)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_get_vnode(%p, %lld, %p, %d)\n", fs, vnid, node,
		reenter));
	status_t error = volume->ReadVNode(vnid, reenter, node);
	PRINT(("userlandfs_get_vnode() done: (%lx, %p)\n", error, *node));
	return error;
}

// userlandfs_put_vnode
static status_t
userlandfs_put_vnode(fs_volume fs, fs_vnode node, bool reenter)
{
	Volume* volume = (Volume*)fs;
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	PRINT(("userlandfs_put_vnode(%p, %p, %d)\n", fs, node, reenter));
	status_t error = volume->WriteVNode(node, reenter);
//	PRINT(("userlandfs_put_vnode() done: %lx\n", error));
	return error;
}

// userlandfs_remove_vnode
static status_t
userlandfs_remove_vnode(fs_volume fs, fs_vnode node, bool reenter)
{
	Volume* volume = (Volume*)fs;
// DANGER: See userlandfs_write_vnode().
//	PRINT(("userlandfs_remove_vnode(%p, %p, %d)\n", fs, node, reenter));
	status_t error = volume->RemoveVNode(node, reenter);
//	PRINT(("userlandfs_remove_vnode() done: %lx\n", error));
	return error;
}


// #pragma mark - VM file access


// TODO: userlandfs_can_page()
// TODO: userlandfs_read_pages()
// TODO: userlandfs_write_pages()


// #pragma mark - cache file access


// TODO: userlandfs_get_file_map()


// #pragma mark - common


// userlandfs_ioctl
static status_t
userlandfs_ioctl(fs_volume fs, fs_vnode node, fs_cookie cookie, ulong op,
	void *buffer, size_t length)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_ioctl(%p, %p, %p, %lu, %p, %lu)\n", fs, node, cookie, op,
		buffer, length));
	status_t error = volume->IOCtl(node, cookie, op, buffer, length);
	PRINT(("userlandfs_ioctl() done: (%lx)\n", error));
	return error;
}

// userlandfs_set_flags
static status_t
userlandfs_set_flags(fs_volume fs, fs_vnode node, fs_cookie cookie, int flags)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_set_flags(%p, %p, %p, %d)\n", fs, node, cookie, flags));
	status_t error = volume->SetFlags(node, cookie, flags);
	PRINT(("userlandfs_set_flags() done: (%lx)\n", error));
	return error;
}

// userlandfs_select
static status_t
userlandfs_select(fs_volume fs, fs_vnode node, fs_cookie cookie,
	uint8 event, uint32 ref, selectsync *sync)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_select(%p, %p, %p, %hhd, %lu, %p)\n", fs, node, cookie,
		event, ref, sync));
	status_t error = volume->Select(node, cookie, event, ref, sync);
	PRINT(("userlandfs_select() done: (%lx)\n", error));
	return error;
}

// userlandfs_deselect
static status_t
userlandfs_deselect(fs_volume fs, fs_vnode node, fs_cookie cookie, uint8 event,
	selectsync *sync)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_deselect(%p, %p, %p, %hhd, %p)\n", fs, node, cookie,
		event, sync));
	status_t error = volume->Deselect(node, cookie, event, sync);
	PRINT(("userlandfs_deselect() done: (%lx)\n", error));
	return error;
}

// userlandfs_fsync
static status_t
userlandfs_fsync(fs_volume fs, fs_vnode node)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_fsync(%p, %p)\n", fs, node));
	status_t error = volume->FSync(node);
	PRINT(("userlandfs_fsync() done: %lx\n", error));
	return error;
}

// userlandfs_read_symlink
static status_t
userlandfs_read_symlink(fs_volume fs, fs_vnode link, char *buffer,
	size_t *bufferSize)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_symlink(%p, %p, %p, %lu)\n", fs, link, buffer,
		*bufferSize));
	status_t error = volume->ReadSymlink(link, buffer, *bufferSize, bufferSize);
	PRINT(("userlandfs_read_symlink() done: (%lx, %lu)\n", error, *bufferSize));
	return error;
}

// TODO: userlandfs_write_symlink

// userlandfs_create_symlink
static status_t
userlandfs_create_symlink(fs_volume fs, fs_vnode dir, const char *name,
	const char *path, int mode)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_create_symlink(%p, %p, `%s', `%s', %d)\n", fs, dir, name,
		path, mode));
	status_t error = volume->CreateSymlink(dir, name, path, mode);
	PRINT(("userlandfs_create_symlink() done: (%lx)\n", error));
	return error;
}

// userlandfs_link
static status_t
userlandfs_link(fs_volume fs, fs_vnode dir, const char *name, fs_vnode node)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_link(%p, %p, `%s', %p)\n", fs, dir, name, node));
	status_t error = volume->Link(dir, name, node);
	PRINT(("userlandfs_link() done: (%lx)\n", error));
	return error;
}

// userlandfs_unlink
static status_t
userlandfs_unlink(fs_volume fs, fs_vnode dir, const char *name)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_unlink(%p, %p, `%s')\n", fs, dir, name));
	status_t error = volume->Unlink(dir, name);
	PRINT(("userlandfs_unlink() done: (%lx)\n", error));
	return error;
}

// userlandfs_rename
static status_t
userlandfs_rename(fs_volume fs, fs_vnode fromDir, const char *fromName,
	fs_vnode toDir, const char *toName)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_rename(%p, %p, `%s', %p, `%s')\n", fs, fromDir, fromName,
		toDir, toName));
	status_t error = volume->Rename(fromDir, fromName, toDir, toName);
	PRINT(("userlandfs_rename() done: (%lx)\n", error));
	return error;
}

// userlandfs_access
static status_t
userlandfs_access(fs_volume fs, fs_vnode node, int mode)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_access(%p, %p, %d)\n", fs, node, mode));
	status_t error = volume->Access(node, mode);
	PRINT(("userlandfs_access() done: %lx\n", error));
	return error;
}

// userlandfs_read_stat
static status_t
userlandfs_read_stat(fs_volume fs, fs_vnode node, struct stat *st)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_stat(%p, %p, %p)\n", fs, node, st));
	status_t error = volume->ReadStat(node, st);
	PRINT(("userlandfs_read_stat() done: %lx\n", error));
	return error;
}

// userlandfs_write_stat
static status_t
userlandfs_write_stat(fs_volume fs, fs_vnode node, const struct stat *st,
	uint32 mask)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_write_stat(%p, %p, %p, %ld)\n", fs, node, st, mask));
	status_t error = volume->WriteStat(node, st, mask);
	PRINT(("userlandfs_write_stat() done: %lx\n", error));
	return error;
}


// #pragma mark - files


// userlandfs_create
static status_t
userlandfs_create(fs_volume fs, fs_vnode dir, const char *name, int openMode,
	int perms, fs_cookie *cookie, vnode_id *vnid)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_create(%p, %p, `%s', %d, %d, %p, %p)\n", fs, dir,
		name, openMode, perms, cookie, vnid));
	status_t error = volume->Create(dir, name, openMode, perms, cookie, vnid);
	PRINT(("userlandfs_create() done: (%lx, %lld, %p)\n", error, *vnid,
		*cookie));
	return error;
}

// userlandfs_open
static status_t
userlandfs_open(fs_volume fs, fs_vnode node, int openMode, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_open(%p, %p, %d)\n", fs, node, openMode));
	status_t error = volume->Open(node, openMode, cookie);
	PRINT(("userlandfs_open() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close
static status_t
userlandfs_close(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->Close(node, cookie);
	PRINT(("userlandfs_close() done: %lx\n", error));
	return error;
}

// userlandfs_free_cookie
static status_t
userlandfs_free_cookie(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_free_cookie(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->FreeCookie(node, cookie);
	PRINT(("userlandfs_free_cookie() done: %lx\n", error));
	return error;
}

// userlandfs_read
static status_t
userlandfs_read(fs_volume fs, fs_vnode node, fs_cookie cookie, off_t pos,
	void *buffer, size_t *length)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read(%p, %p, %p, %Ld, %p, %lu)\n", nf, node, cookie, pos,
		buffer, *length));
	status_t error = volume->Read(node, cookie, pos, buffer, *length,
		length);
	PRINT(("userlandfs_read() done: (%lx, %lu)\n", error, *length));
	return error;
}

// userlandfs_write
static status_t
userlandfs_write(fs_volume fs, fs_vnode node, fs_cookie cookie, off_t pos,
	const void *buffer, size_t *length)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_write(%p, %p, %p, %Ld, %p, %lu)\n", fs, node, cookie,
		pos, buffer, *length));
	status_t error = volume->Write(node, cookie, pos, buffer, *length, length);
	PRINT(("userlandfs_write() done: (%lx, %lu)\n", error, *length));
	return error;
}


// #pragma mark - directories


// userlandfs_create_dir
static status_t
userlandfs_create_dir(fs_volume fs, fs_vnode parent, const char *name,
	int perms, vnode_id *newDir)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_create_dir(%p, %p, `%s', %d, %p)\n", fs, parent, name,
		perms, newDir));
	status_t error = volume->CreateDir(parent, name, perms, newDir);
	PRINT(("userlandfs_create_dir() done: (%lx, %lld)\n", error, *newDir));
	return error;
}

// userlandfs_remove_dir
static status_t
userlandfs_remove_dir(fs_volume fs, fs_vnode parent, const char *name)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_remove_dir(%p, %p, `%s')\n", fs, parent, name));
	status_t error = volume->RemoveDir(parent, name);
	PRINT(("userlandfs_remove_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_open_dir
static status_t
userlandfs_open_dir(fs_volume fs, fs_vnode node, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_open_dir(%p, %p)\n", fs, node));
	status_t error = volume->OpenDir(node, cookie);
	PRINT(("userlandfs_open_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_dir
static status_t
userlandfs_close_dir(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close_dir(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->CloseDir(node, cookie);
	PRINT(("userlandfs_close_dir() done: %lx\n", error));
	return error;
}

// userlandfs_free_dir_cookie
static status_t
userlandfs_free_dir_cookie(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_free_dir_cookie(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->FreeDirCookie(node, cookie);
	PRINT(("userlandfs_free_dir_cookie() done: %lx \n", error));
	return error;
}

// userlandfs_read_dir
static status_t
userlandfs_read_dir(fs_volume fs, fs_vnode node, fs_cookie cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_dir(%p, %p, %p, %p, %lu, %lu)\n", fs, node, cookie,
		buffer, bufferSize, *count));
	status_t error = volume->ReadDir(node, cookie, buffer, bufferSize, *count,
		count);
	PRINT(("userlandfs_read_dir() done: (%lx, %lu)\n", error, *count));
	#if DEBUG
		dirent* entry = buffer;
		for (int32 i = 0; i < *count; i++) {
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
userlandfs_rewind_dir(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_rewind_dir(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->RewindDir(node, cookie);
	PRINT(("userlandfs_rewind_dir() done: %lx\n", error));
	return error;
}


// #pragma mark - attribute directories


// userlandfs_open_attr_dir
static status_t
userlandfs_open_attr_dir(fs_volume fs, fs_vnode node, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_open_attr_dir(%p, %p)\n", fs, node));
	status_t error = volume->OpenAttrDir(node, cookie);
	PRINT(("userlandfs_open_attr_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_attr_dir
static status_t
userlandfs_close_attr_dir(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close_attr_dir(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->CloseAttrDir(node, cookie);
	PRINT(("userlandfs_close_attr_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_attr_dir_cookie
static status_t
userlandfs_free_attr_dir_cookie(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_free_attr_dir_cookie(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->FreeAttrDirCookie(node, cookie);
	PRINT(("userlandfs_free_attr_dir_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_attr_dir
static status_t
userlandfs_read_attr_dir(fs_volume fs, fs_vnode node, fs_cookie cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_attr_dir(%p, %p, %p, %p, %lu, %lu)\n", fs, node,
		cookie, buffer, bufferSize, *count));
	status_t error = volume->ReadAttrDir(node, cookie, buffer, bufferSize,
		*count, count);
	PRINT(("userlandfs_read_attr_dir() done: (%lx, %lu)\n", error, *count));
	return error;
}

// userlandfs_rewind_attr_dir
static status_t
userlandfs_rewind_attr_dir(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_rewind_attr_dir(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->RewindAttrDir(node, cookie);
	PRINT(("userlandfs_rewind_attr_dir() done: (%lx)\n", error));
	return error;
}


// #pragma mark - attributes


// userlandfs_create_attr
status_t
userlandfs_create_attr(fs_volume fs, fs_vnode node, const char *name,
	uint32 type, int openMode, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_create_attr(%p, %p, \"%s\", 0x%lx, %d, %p)\n", fs, node,
		name, type, openMode, cookie));
	status_t error = volume->CreateAttr(node, name, type, openMode, cookie);
	PRINT(("userlandfs_create_attr() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_open_attr
status_t
userlandfs_open_attr(fs_volume fs, fs_vnode node, const char *name,
	int openMode, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_open_attr(%p, %p, \"%s\", %d, %p)\n", fs, node, name,
		openMode, cookie));
	status_t error = volume->OpenAttr(node, name, openMode, cookie);
	PRINT(("userlandfs_open_attr() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_attr
status_t
userlandfs_close_attr(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close_attr(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->CloseAttr(node, cookie);
	PRINT(("userlandfs_close_attr() done: %lx\n", error));
	return error;
}

// userlandfs_free_attr_cookie
status_t
userlandfs_free_attr_cookie(fs_volume fs, fs_vnode node, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close_attr(%p, %p, %p)\n", fs, node, cookie));
	status_t error = volume->FreeAttrCookie(node, cookie);
	PRINT(("userlandfs_close_attr() done: %lx\n", error));
	return error;
}

// userlandfs_read_attr
static status_t
userlandfs_read_attr(fs_volume fs, fs_vnode node, fs_cookie cookie,
	off_t pos, void *buffer, size_t *length)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_attr(%p, %p, %p, %lld, %p, %lu)\n", fs, node,
		cookie, pos, buffer, *length));
	status_t error = volume->ReadAttr(node, cookie, pos, buffer, *length,
		length);
	PRINT(("userlandfs_read_attr() done: (%lx, %lu)\n", error, *length));
	return error;
}

// userlandfs_write_attr
static status_t
userlandfs_write_attr(fs_volume fs, fs_vnode node, fs_cookie cookie,
	off_t pos, const void *buffer, size_t *length)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_write_attr(%p, %p, %p, %lld, %p, %lu)\n", fs, node,
		cookie, pos, buffer, *length));
	status_t error = volume->WriteAttr(node, cookie, pos, buffer, *length,
		length);
	PRINT(("userlandfs_write_attr() done: (%lx, %lu)\n", error, *length));
	return error;
}

// userlandfs_read_attr_stat
static status_t
userlandfs_read_attr_stat(fs_volume fs, fs_vnode node, fs_cookie cookie,
	struct stat *st)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_attr_stat(%p, %p, %p', %p)\n", fs, node, cookie,
		st));
	status_t error = volume->ReadAttrStat(node, cookie, st);
	PRINT(("userlandfs_read_attr_stat() done: (%lx)\n", error));
	return error;
}

// TODO: userlandfs_write_attr_stat

// userlandfs_rename_attr
static status_t
userlandfs_rename_attr(fs_volume fs, fs_vnode fromNode, const char *fromName,
	fs_vnode toNode, const char *toName)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_rename_attr(%p, %p, `%s', %p, `%s')\n", fs, fromNode,
		fromName, toNode, toName));
	status_t error = volume->RenameAttr(fromNode, fromName, toNode, toName);
	PRINT(("userlandfs_rename_attr() done: (%lx)\n", error));
	return error;
}

// userlandfs_remove_attr
static status_t
userlandfs_remove_attr(fs_volume fs, fs_vnode node, const char *name)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_remove_attr(%p, %p, `%s')\n", fs, node, name));
	status_t error = volume->RemoveAttr(node, name);
	PRINT(("userlandfs_remove_attr() done: (%lx)\n", error));
	return error;
}


// #pragma mark - indices


// userlandfs_open_index_dir
static status_t
userlandfs_open_index_dir(fs_volume fs, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_open_index_dir(%p, %p)\n", fs, cookie));
	status_t error = volume->OpenIndexDir(cookie);
	PRINT(("userlandfs_open_index_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_index_dir
static status_t
userlandfs_close_index_dir(fs_volume fs, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close_index_dir(%p, %p)\n", fs, cookie));
	status_t error = volume->CloseIndexDir(cookie);
	PRINT(("userlandfs_close_index_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_index_dir_cookie
static status_t
userlandfs_free_index_dir_cookie(fs_volume fs, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_free_index_dir_cookie(%p, %p)\n", fs, cookie));
	status_t error = volume->FreeIndexDirCookie(cookie);
	PRINT(("userlandfs_free_index_dir_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_index_dir
static status_t
userlandfs_read_index_dir(fs_volume fs, fs_cookie cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_index_dir(%p, %p, %p, %lu, %lu)\n", fs, cookie,
		buffer, bufferSize, *count));
	status_t error = volume->ReadIndexDir(cookie, buffer, bufferSize,
		*count, count);
	PRINT(("userlandfs_read_index_dir() done: (%lx, %lu)\n", error, *count));
	return error;
}

// userlandfs_rewind_index_dir
static status_t
userlandfs_rewind_index_dir(fs_volume fs, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_rewind_index_dir(%p, %p)\n", fs, cookie));
	status_t error = volume->RewindIndexDir(cookie);
	PRINT(("userlandfs_rewind_index_dir() done: (%lx)\n", error));
	return error;
}

// userlandfs_create_index
static status_t
userlandfs_create_index(fs_volume fs, const char *name, uint32 type,
	uint32 flags)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_create_index(%p, `%s', 0x%lx, 0x%lx)\n", fs, name, type,
		flags));
	status_t error = volume->CreateIndex(name, type, flags);
	PRINT(("userlandfs_create_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_remove_index
static status_t
userlandfs_remove_index(fs_volume fs, const char *name)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_remove_index(%p, `%s')\n", fs, name));
	status_t error = volume->RemoveIndex(name);
	PRINT(("userlandfs_remove_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_index_stat
static status_t
userlandfs_read_index_stat(fs_volume fs, const char *name, struct stat *st)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_index_stat(%p, `%s', %p)\n", s, name, st));
	status_t error = volume->ReadIndexStat(name, st);
	PRINT(("userlandfs_read_index_stat() done: (%lx)\n", error));
	return error;
}


// #pragma mark - queries


// userlandfs_open_query
static status_t
userlandfs_open_query(fs_volume fs, const char *queryString, uint32 flags,
	port_id port, uint32 token, fs_cookie *cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_open_query(%p, `%s', %lu, %ld, %lu, %p)\n", fs,
		queryString, flags, port, token, cookie));
	status_t error = volume->OpenQuery(queryString, flags, port, token, cookie);
	PRINT(("userlandfs_open_query() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_query
static status_t
userlandfs_close_query(fs_volume fs, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_close_query(%p, %p)\n", fs, cookie));
	status_t error = volume->CloseQuery(cookie);
	PRINT(("userlandfs_close_query() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_query_cookie
static status_t
userlandfs_free_query_cookie(fs_volume fs, fs_cookie cookie)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_free_query_cookie(%p, %p)\n", fs, cookie));
	status_t error = volume->FreeQueryCookie(cookie);
	PRINT(("userlandfs_free_query_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_query
static status_t
userlandfs_read_query(fs_volume fs, fs_cookie cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
	Volume* volume = (Volume*)fs;
	PRINT(("userlandfs_read_query(%p, %p, %p, %lu, %lu)\n", fs, cookie,
		buffer, bufferSize, *count));
	status_t error = volume->ReadQuery(cookie, buffer, bufferSize, *count,
		count);
	PRINT(("userlandfs_read_query() done: (%lx, %ld)\n", error, *count));
	#if DEBUG
		if (*count > 0) {
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

// TODO: userlandfs_rewind_query()


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

	"Userland File System",

	// scanning
	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	&userlandfs_mount,
	&userlandfs_unmount,
	&userlandfs_read_fs_info,
	&userlandfs_write_fs_info,
	&userlandfs_sync,

	/* vnode operations */
	&userlandfs_lookup,
	NULL,	// &userlandfs_get_vnode_name,
	&userlandfs_get_vnode,
	&userlandfs_put_vnode,
	&userlandfs_remove_vnode,

	/* VM file access */
	NULL,	// &userlandfs_can_page,
	NULL,	// &userlandfs_read_pages,
	NULL,	// &userlandfs_write_pages,

	NULL,	// &userlandfs_get_file_map,

	&userlandfs_ioctl,
	&userlandfs_set_flags,
	&userlandfs_select,
	&userlandfs_deselect,
	&userlandfs_fsync,

	&userlandfs_read_symlink,
	NULL,						// write link
	&userlandfs_create_symlink,

	&userlandfs_link,
	&userlandfs_unlink,
	&userlandfs_rename,

	&userlandfs_access,
	&userlandfs_read_stat,
	&userlandfs_write_stat,

	/* file operations */
	&userlandfs_create,
	&userlandfs_open,
	&userlandfs_close,
	&userlandfs_free_cookie,
	&userlandfs_read,
	&userlandfs_write,

	/* directory operations */
	&userlandfs_create_dir,
	&userlandfs_remove_dir,
	&userlandfs_open_dir,
	&userlandfs_close_dir,
	&userlandfs_free_dir_cookie,
	&userlandfs_read_dir,
	&userlandfs_rewind_dir,
	
	/* attribute directory operations */
	&userlandfs_open_attr_dir,
	&userlandfs_close_attr_dir,
	&userlandfs_free_attr_dir_cookie,
	&userlandfs_read_attr_dir,
	&userlandfs_rewind_attr_dir,

	/* attribute operations */
	&userlandfs_create_attr,
	&userlandfs_open_attr,
	&userlandfs_close_attr,
	&userlandfs_free_attr_cookie,
	&userlandfs_read_attr,
	&userlandfs_write_attr,

	&userlandfs_read_attr_stat,
	NULL,	// &userlandfs_write_attr_stat,
	&userlandfs_rename_attr,
	&userlandfs_remove_attr,

	/* index directory & index operations */
	&userlandfs_open_index_dir,
	&userlandfs_close_index_dir,
	&userlandfs_free_index_dir_cookie,
	&userlandfs_read_index_dir,
	&userlandfs_rewind_index_dir,

	&userlandfs_create_index,
	&userlandfs_remove_index,
	&userlandfs_read_index_stat,

	/* query operations */
	&userlandfs_open_query,
	&userlandfs_close_query,
	&userlandfs_free_query_cookie,
	&userlandfs_read_query,
	NULL,	// &userlandfs_rewind_query,
};

module_info *modules[] = {
	(module_info *)&sUserlandFSModuleInfo,
	NULL,
};
