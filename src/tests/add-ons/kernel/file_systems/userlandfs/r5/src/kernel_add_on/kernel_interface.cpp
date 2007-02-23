// kernel_interface.cpp

#include <KernelExport.h>
#include <fsproto.h>

#include "Debug.h"
#include "FileSystem.h"
#include "String.h"
#include "UserlandFS.h"
#include "Volume.h"

#if USER

// disable_interrupts
static inline
cpu_status
disable_interrupts()
{
	return 0;
}

// restore_interrupts
static inline
void
restore_interrupts(cpu_status status)
{
}

#endif	// USER

// #pragma mark -
// #pragma mark ----- prototypes -----

extern "C" {

// fs
static int userlandfs_mount(nspace_id nsid, const char *device, ulong flags,
				void *parameters, size_t len, void **data, vnode_id *rootID);
static int userlandfs_unmount(void *ns);
static int userlandfs_initialize(const char *deviceName, void *parameters,
				size_t len);
static int userlandfs_sync(void *ns);
static int userlandfs_read_fs_stat(void *ns, struct fs_info *info);
static int userlandfs_write_fs_stat(void *ns, struct fs_info *info, long mask);

// vnodes
static int userlandfs_read_vnode(void *ns, vnode_id vnid, char reenter,
				void **node);
static int userlandfs_write_vnode(void *ns, void *node, char reenter);
static int userlandfs_remove_vnode(void *ns, void *node, char reenter);

// nodes
static int userlandfs_fsync(void *ns, void *node);
static int userlandfs_read_stat(void *ns, void *node, struct stat *st);
static int userlandfs_write_stat(void *ns, void *node, struct stat *st,
				long mask);
static int userlandfs_access(void *ns, void *node, int mode);

// files
static int userlandfs_create(void *ns, void *dir, const char *name,
				int openMode, int mode, vnode_id *vnid, void **cookie);
static int userlandfs_open(void *ns, void *node, int openMode, void **cookie);
static int userlandfs_close(void *ns, void *node, void *cookie);
static int userlandfs_free_cookie(void *ns, void *node, void *cookie);
static int userlandfs_read(void *ns, void *node, void *cookie, off_t pos,
				void *buffer, size_t *bufferSize);
static int userlandfs_write(void *ns, void *node, void *cookie, off_t pos,
				const void *buffer, size_t *bufferSize);
static int userlandfs_ioctl(void *ns, void *node, void *cookie, int cmd,
				void *buffer, size_t bufferSize);
static int userlandfs_setflags(void *ns, void *node, void *cookie, int flags);
static int userlandfs_select(void *ns, void *node, void *cookie, uint8 event,
				uint32 ref, selectsync *sync);
static int userlandfs_deselect(void *ns, void *node, void *cookie, uint8 event,
				selectsync *sync);

// hard links / symlinks
static int userlandfs_link(void *ns, void *dir, const char *name, void *node);
static int userlandfs_unlink(void *ns, void *dir, const char *name);
static int userlandfs_symlink(void *ns, void *dir, const char *name,
				const char *path);
static int userlandfs_read_link(void *ns, void *node, char *buffer,
				size_t *bufferSize);
static int userlandfs_rename(void *ns, void *oldDir, const char *oldName,
				void *newDir, const char *newName);

// directories
static int userlandfs_mkdir(void *ns, void *dir, const char *name, int mode);
static int userlandfs_rmdir(void *ns, void *dir, const char *name);
static int userlandfs_open_dir(void *ns, void *node, void **cookie);
static int userlandfs_close_dir(void *ns, void *node, void *cookie);
static int userlandfs_free_dir_cookie(void *ns, void *node, void *cookie);
static int userlandfs_read_dir(void *ns, void *node, void *cookie,
				long *count, struct dirent *buffer, size_t bufferSize);
static int userlandfs_rewind_dir(void *ns, void *node, void *cookie);
static int userlandfs_walk(void *ns, void *dir, const char *entryName,
				char **resolvedPath, vnode_id *vnid);

// attributes
static int userlandfs_open_attrdir(void *ns, void *node, void **cookie);
static int userlandfs_close_attrdir(void *ns, void *node, void *cookie);
static int userlandfs_free_attrdir_cookie(void *ns, void *node, void *cookie);
static int userlandfs_read_attrdir(void *ns, void *node, void *cookie,
				long *count, struct dirent *buffer, size_t bufferSize);
static int userlandfs_read_attr(void *ns, void *node, const char *name,
				int type, void *buffer, size_t *bufferSize, off_t pos);
static int userlandfs_rewind_attrdir(void *ns, void *node, void *cookie);
static int userlandfs_write_attr(void *ns, void *node, const char *name,
				int type, const void *buffer, size_t *bufferSize, off_t pos);
static int userlandfs_remove_attr(void *ns, void *node, const char *name);
static int userlandfs_rename_attr(void *ns, void *node, const char *oldName,
				const char *newName);
static int userlandfs_stat_attr(void *ns, void *node, const char *name,
				struct attr_info *attrInfo);

// indices
static int userlandfs_open_indexdir(void *ns, void **cookie);
static int userlandfs_close_indexdir(void *ns, void *cookie);
static int userlandfs_free_indexdir_cookie(void *ns, void *node, void *cookie);
static int userlandfs_read_indexdir(void *ns, void *cookie, long *count,
				struct dirent *buffer, size_t bufferSize);
static int userlandfs_rewind_indexdir(void *ns, void *cookie);
static int userlandfs_create_index(void *ns, const char *name, int type, int flags);
static int userlandfs_remove_index(void *ns, const char *name);
static int userlandfs_rename_index(void *ns, const char *oldName,
				const char *newName);
static int userlandfs_stat_index(void *ns, const char *name,
				struct index_info *indexInfo);

// queries
static int userlandfs_open_query(void *ns, const char *queryString, ulong flags,
				port_id port, long token, void **cookie);
static int userlandfs_close_query(void *ns, void *cookie);
static int userlandfs_free_query_cookie(void *ns, void *node, void *cookie);
static int userlandfs_read_query(void *ns, void *cookie, long *count,
				struct dirent *buffer, size_t bufferSize);

} // extern "C"

/* vnode_ops struct. Fill this in to tell the kernel how to call
	functions in your driver.
*/
vnode_ops fs_entry =  {
	&userlandfs_read_vnode,			// read_vnode
	&userlandfs_write_vnode,		// write_vnode
	&userlandfs_remove_vnode,		// remove_vnode
	NULL,							// secure_vnode (not needed)
	&userlandfs_walk,				// walk
	&userlandfs_access,				// access
	&userlandfs_create,				// create
	&userlandfs_mkdir,				// mkdir
	&userlandfs_symlink,			// symlink
	&userlandfs_link,				// link
	&userlandfs_rename,				// rename
	&userlandfs_unlink,				// unlink
	&userlandfs_rmdir,				// rmdir
	&userlandfs_read_link,			// readlink
	&userlandfs_open_dir,			// opendir
	&userlandfs_close_dir,			// closedir
	&userlandfs_free_dir_cookie,	// free_dircookie
	&userlandfs_rewind_dir,			// rewinddir
	&userlandfs_read_dir,			// readdir
	&userlandfs_open,				// open file
	&userlandfs_close,				// close file
	&userlandfs_free_cookie,		// free cookie
	&userlandfs_read,				// read file
	&userlandfs_write,				// write file
	NULL,							// readv
	NULL,							// writev
	&userlandfs_ioctl,				// ioctl
	&userlandfs_setflags,			// setflags file
	&userlandfs_read_stat,			// read stat
	&userlandfs_write_stat,			// write stat
	&userlandfs_fsync,				// fsync
	&userlandfs_initialize,			// initialize
	&userlandfs_mount,				// mount
	&userlandfs_unmount,			// unmount
	&userlandfs_sync,				// sync
	&userlandfs_read_fs_stat,		// read fs stat
	&userlandfs_write_fs_stat,		// write fs stat
	&userlandfs_select,				// select
	&userlandfs_deselect,			// deselect

	&userlandfs_open_indexdir,		// open index dir
	&userlandfs_close_indexdir,		// close index dir
	&userlandfs_free_indexdir_cookie,	// free index dir cookie
	&userlandfs_rewind_indexdir,	// rewind index dir
	&userlandfs_read_indexdir,		// read index dir
	&userlandfs_create_index,		// create index
	&userlandfs_remove_index,		// remove index
	&userlandfs_rename_index,		// rename index
	&userlandfs_stat_index,			// stat index

	&userlandfs_open_attrdir,		// open attr dir
	&userlandfs_close_attrdir,		// close attr dir
	&userlandfs_free_attrdir_cookie,	// free attr dir cookie
	&userlandfs_rewind_attrdir,		// rewind attr dir
	&userlandfs_read_attrdir,		// read attr dir
	&userlandfs_write_attr,			// write attr
	&userlandfs_read_attr,			// read attr
	&userlandfs_remove_attr,		// remove attr
	&userlandfs_rename_attr,		// rename attr
	&userlandfs_stat_attr,			// stat attr

	&userlandfs_open_query,			// open query
	&userlandfs_close_query,		// close query
	&userlandfs_free_query_cookie,	// free query cookie
	&userlandfs_read_query,			// read query
};

int32 api_version = B_CUR_FS_API_VERSION;

// #pragma mark -
// #pragma mark ----- fs -----

// parse_parameters
static
status_t
parse_parameters(const void *_parameters, int32 _len, String &fsName,
	const char **fsParameters, int32 *fsParameterLength)
{
	// check parameters
	if (!_parameters || _len <= 0)
		return B_BAD_VALUE;
	const char* parameters = (const char*)_parameters;
	int32 len = strnlen(parameters, _len);
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
	*fsParameterLength = len;
	return B_OK;
}

// userlandfs_mount
static
int
userlandfs_mount(nspace_id nsid, const char *device, ulong flags,
	void *parameters, size_t len, void **data, vnode_id *rootID)
{
	status_t error = B_OK;
	init_debugging();
	// get the parameters
	String fsName;
	const char* fsParameters;
	int32 fsParameterLength;
	error = parse_parameters(parameters, len, fsName, &fsParameters,
		&fsParameterLength);
	if (error != B_OK) {
		exit_debugging();
		return error;
	}
	// make sure there is a UserlandFS we can work with
	UserlandFS* userlandFS = NULL;
	error = UserlandFS::RegisterUserlandFS(&userlandFS);
	if (error != B_OK) {
		exit_debugging();
		return error;
	}
	// get the file system
	FileSystem* fileSystem = NULL;
	error = userlandFS->RegisterFileSystem(fsName.GetString(), &fileSystem);
	if (error != B_OK) {
		UserlandFS::UnregisterUserlandFS();
		exit_debugging();
		return error;
	}
	// mount the volume
	Volume* volume = NULL;
	error = fileSystem->Mount(nsid, device, flags, fsParameters,
		fsParameterLength, &volume);
	if (error != B_OK) {
		userlandFS->UnregisterFileSystem(fileSystem);
		UserlandFS::UnregisterUserlandFS();
		exit_debugging();
		return error;
	}
	*data = volume;
	*rootID = volume->GetRootID();
	return error;
}

// userlandfs_unmount
static
int
userlandfs_unmount(void *ns)
{
	Volume* volume = (Volume*)ns;
	FileSystem* fileSystem = volume->GetFileSystem();
	status_t error = volume->Unmount();
	// The error code the FS's unmount hook returns is completely irrelevant to
	// the VFS. It considers the volume unmounted in any case.
	volume->RemoveReference();
	UserlandFS::GetUserlandFS()->UnregisterFileSystem(fileSystem);
	UserlandFS::UnregisterUserlandFS();
	exit_debugging();
	return error;
}

// userlandfs_initialize
static
int
userlandfs_initialize(const char *deviceName, void *parameters,
	size_t len)
{
	init_debugging();
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
	exit_debugging();
	return error;
}

// userlandfs_sync
static
int
userlandfs_sync(void *ns)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_sync(%p)\n", ns));
	status_t error = volume->Sync();
	PRINT(("userlandfs_sync() done: %lx \n", error));
	return error;
}

// userlandfs_read_fs_stat
static
int
userlandfs_read_fs_stat(void *ns, struct fs_info *info)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_fs_stat(%p, %p)\n", ns, info));
	status_t error = volume->ReadFSStat(info);
	PRINT(("userlandfs_read_fs_stat() done: %lx \n", error));
	return error;
}

// userlandfs_write_fs_stat
static
int
userlandfs_write_fs_stat(void *ns, struct fs_info *info, long mask)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_write_fs_stat(%p, %p, %ld)\n", ns, info, mask));
	status_t error = volume->WriteFSStat(info, mask);
	PRINT(("userlandfs_write_fs_stat() done: %lx \n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- vnodes -----

// userlandfs_read_vnode
static
int
userlandfs_read_vnode(void *ns, vnode_id vnid, char reenter, void **node)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_vnode(%p, %Ld, %d, %p)\n", ns, vnid, reenter,
		node));
	status_t error = volume->ReadVNode(vnid, reenter, node);
	PRINT(("userlandfs_read_vnode() done: (%lx, %p)\n", error, *node));
	return error;
}

// userlandfs_write_vnode
static
int
userlandfs_write_vnode(void *ns, void *node, char reenter)
{
	Volume* volume = (Volume*)ns;
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	PRINT(("userlandfs_write_vnode(%p, %p, %d)\n", ns, node, reenter));
	status_t error = volume->WriteVNode(node, reenter);
//	PRINT(("userlandfs_write_vnode() done: %lx\n", error));
	return error;
}

// userlandfs_remove_vnode
static
int
userlandfs_remove_vnode(void *ns, void *node, char reenter)
{
	Volume* volume = (Volume*)ns;
// DANGER: See userlandfs_write_vnode().
//	PRINT(("userlandfs_remove_vnode(%p, %p, %d)\n", ns, node, reenter));
	status_t error = volume->RemoveVNode(node, reenter);
//	PRINT(("userlandfs_remove_vnode() done: %lx\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- nodes -----

// userlandfs_fsync
static
int
userlandfs_fsync(void *ns, void *node)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_fsync(%p, %p)\n", ns, node));
	status_t error = volume->FSync(node);
	PRINT(("userlandfs_fsync() done: %lx\n", error));
	return error;
}

// userlandfs_read_stat
static
int
userlandfs_read_stat(void *ns, void *node, struct stat *st)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_stat(%p, %p, %p)\n", ns, node, st));
	status_t error = volume->ReadStat(node, st);
	PRINT(("userlandfs_read_stat() done: %lx\n", error));
	return error;
}

// userlandfs_write_stat
static
int
userlandfs_write_stat(void *ns, void *node, struct stat *st, long mask)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_write_stat(%p, %p, %p, %ld)\n", ns, node, st, mask));
	status_t error = volume->WriteStat(node, st, mask);
	PRINT(("userlandfs_write_stat() done: %lx\n", error));
	return error;
}

// userlandfs_access
static
int
userlandfs_access(void *ns, void *node, int mode)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_access(%p, %p, %d)\n", ns, node, mode));
	status_t error = volume->Access(node, mode);
	PRINT(("userlandfs_access() done: %lx\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- files -----

// userlandfs_create
static
int
userlandfs_create(void *ns, void *dir, const char *name, int openMode, int mode,
	vnode_id *vnid, void **cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_create(%p, %p, `%s', %d, %d, %p, %p)\n", ns, dir,
		name, openMode, mode, vnid, cookie));
	status_t error = volume->Create(dir, name, openMode, mode, vnid, cookie);
	PRINT(("userlandfs_create() done: (%lx, %Ld, %p)\n", error, *vnid,
		*cookie));
	return error;
}

// userlandfs_open
static
int
userlandfs_open(void *ns, void *node, int openMode, void **cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_open(%p, %p, %d)\n", ns, node, openMode));
	status_t error = volume->Open(node, openMode, cookie);
	PRINT(("userlandfs_open() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close
static
int
userlandfs_close(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_close(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->Close(node, cookie);
	PRINT(("userlandfs_close() done: %lx\n", error));
	return error;
}

// userlandfs_free_cookie
static
int
userlandfs_free_cookie(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_free_cookie(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->FreeCookie(node, cookie);
	PRINT(("userlandfs_free_cookie() done: %lx\n", error));
	return error;
}

// userlandfs_read
static
int
userlandfs_read(void *ns, void *node, void *cookie, off_t pos, void *buffer,
	size_t *bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read(%p, %p, %p, %Ld, %p, %lu)\n", ns, node, cookie, pos,
		buffer, *bufferSize));
	status_t error = volume->Read(node, cookie, pos, buffer, *bufferSize,
		bufferSize);
	PRINT(("userlandfs_read() done: (%lx, %lu)\n", error, *bufferSize));
	return error;
}

// userlandfs_write
static
int
userlandfs_write(void *ns, void *node, void *cookie, off_t pos,
	const void *buffer, size_t *bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_write(%p, %p, %p, %Ld, %p, %lu)\n", ns, node, cookie, pos,
		buffer, *bufferSize));
	status_t error = volume->Write(node, cookie, pos, buffer, *bufferSize,
		bufferSize);
	PRINT(("userlandfs_write() done: (%lx, %lu)\n", error, *bufferSize));
	return error;
}

// userlandfs_ioctl
static
int
userlandfs_ioctl(void *ns, void *node, void *cookie, int cmd, void *buffer,
	size_t bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_ioctl(%p, %p, %p, %d, %p, %lu)\n", ns, node, cookie, cmd,
		buffer, bufferSize));
	status_t error = volume->IOCtl(node, cookie, cmd, buffer, bufferSize);
	PRINT(("userlandfs_ioctl() done: (%lx)\n", error));
	return error;
}

// userlandfs_setflags
static
int
userlandfs_setflags(void *ns, void *node, void *cookie, int flags)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_setflags(%p, %p, %p, %d)\n", ns, node, cookie, flags));
	status_t error = volume->SetFlags(node, cookie, flags);
	PRINT(("userlandfs_setflags() done: (%lx)\n", error));
	return error;
}

// userlandfs_select
static
int
userlandfs_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref,
	selectsync *sync)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_select(%p, %p, %p, %hhd, %lu, %p)\n", ns, node, cookie,
		event, ref, sync));
	status_t error = volume->Select(node, cookie, event, ref, sync);
	PRINT(("userlandfs_select() done: (%lx)\n", error));
	return error;
}

// userlandfs_deselect
static
int
userlandfs_deselect(void *ns, void *node, void *cookie, uint8 event,
	selectsync *sync)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_deselect(%p, %p, %p, %hhd, %p)\n", ns, node, cookie,
		event, sync));
	status_t error = volume->Deselect(node, cookie, event, sync);
	PRINT(("userlandfs_deselect() done: (%lx)\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// userlandfs_link
static
int
userlandfs_link(void *ns, void *dir, const char *name, void *node)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_link(%p, %p, `%s', %p)\n", ns, dir, name, node));
	status_t error = volume->Link(dir, name, node);
	PRINT(("userlandfs_link() done: (%lx)\n", error));
	return error;
}

// userlandfs_unlink
static
int
userlandfs_unlink(void *ns, void *dir, const char *name)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_unlink(%p, %p, `%s')\n", ns, dir, name));
	status_t error = volume->Unlink(dir, name);
	PRINT(("userlandfs_unlink() done: (%lx)\n", error));
	return error;
}

// userlandfs_symlink
static
int
userlandfs_symlink(void *ns, void *dir, const char *name, const char *path)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_symlink(%p, %p, `%s', `%s')\n", ns, dir, name, path));
	status_t error = volume->Symlink(dir, name, path);
	PRINT(("userlandfs_symlink() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_link
static
int
userlandfs_read_link(void *ns, void *node, char *buffer, size_t *bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_link(%p, %p, %p, %lu)\n", ns, node, buffer,
		*bufferSize));
	status_t error = volume->ReadLink(node, buffer, *bufferSize, bufferSize);
	PRINT(("userlandfs_read_link() done: (%lx, %lu)\n", error, *bufferSize));
	return error;
}

// userlandfs_rename
static
int
userlandfs_rename(void *ns, void *oldDir, const char *oldName, void *newDir,
	const char *newName)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rename(%p, %p, `%s', %p, `%s')\n", ns, oldDir, oldName,
		newDir, newName));
	status_t error = volume->Rename(oldDir, oldName, newDir, newName);
	PRINT(("userlandfs_rename() done: (%lx)\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- directories -----

// userlandfs_mkdir
static
int
userlandfs_mkdir(void *ns, void *dir, const char *name, int mode)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_mkdir(%p, %p, `%s', %d)\n", ns, dir, name, mode));
	status_t error = volume->MkDir(dir, name, mode);
	PRINT(("userlandfs_mkdir() done: (%lx)\n", error));
	return error;
}

// userlandfs_rmdir
static
int
userlandfs_rmdir(void *ns, void *dir, const char *name)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rmdir(%p, %p, `%s')\n", ns, dir, name));
	status_t error = volume->RmDir(dir, name);
	PRINT(("userlandfs_rmdir() done: (%lx)\n", error));
	return error;
}

// userlandfs_open_dir
static
int
userlandfs_open_dir(void *ns, void *node, void **cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_open_dir(%p, %p)\n", ns, node));
	status_t error = volume->OpenDir(node, cookie);
	PRINT(("userlandfs_open_dir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_dir
static
int
userlandfs_close_dir(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_close_dir(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->CloseDir(node, cookie);
	PRINT(("userlandfs_close_dir() done: %lx\n", error));
	return error;
}

// userlandfs_free_dir_cookie
static
int
userlandfs_free_dir_cookie(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_free_dir_cookie(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->FreeDirCookie(node, cookie);
	PRINT(("userlandfs_free_dir_cookie() done: %lx \n", error));
	return error;
}

// userlandfs_read_dir
static
int
userlandfs_read_dir(void *ns, void *node, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_dir(%p, %p, %p, %ld, %p, %lu)\n", ns, node, cookie,
		*count, buffer, bufferSize));
	status_t error = volume->ReadDir(node, cookie, buffer, bufferSize, *count,
		count);
	PRINT(("userlandfs_read_dir() done: (%lx, %ld)\n", error, *count));
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
static
int
userlandfs_rewind_dir(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rewind_dir(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->RewindDir(node, cookie);
	PRINT(("userlandfs_rewind_dir() done: %lx\n", error));
	return error;
}

// userlandfs_walk
static
int
userlandfs_walk(void *ns, void *dir, const char *entryName,
	char **resolvedPath, vnode_id *vnid)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_walk(%p, %p, `%s', %p, %p)\n", ns, dir,
		entryName, resolvedPath, vnid));
	status_t error = volume->Walk(dir, entryName, resolvedPath, vnid);
	PRINT(("userlandfs_walk() done: (%lx, `%s', %Ld)\n", error,
		(resolvedPath ? *resolvedPath : NULL), *vnid));
	return error;
}

// #pragma mark -
// #pragma mark ----- attributes -----

// userlandfs_open_attrdir
static
int
userlandfs_open_attrdir(void *ns, void *node, void **cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_open_attrdir(%p, %p)\n", ns, node));
	status_t error = volume->OpenAttrDir(node, cookie);
	PRINT(("userlandfs_open_attrdir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_attrdir
static
int
userlandfs_close_attrdir(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_close_attrdir(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->CloseAttrDir(node, cookie);
	PRINT(("userlandfs_close_attrdir() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_attrdir_cookie
static
int
userlandfs_free_attrdir_cookie(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_free_attrdir_cookie(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->FreeAttrDirCookie(node, cookie);
	PRINT(("userlandfs_free_attrdir_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_attrdir
static
int
userlandfs_read_attrdir(void *ns, void *node, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_attrdir(%p, %p, %p, %ld, %p, %lu)\n", ns, node,
		cookie, *count, buffer, bufferSize));
	status_t error = volume->ReadAttrDir(node, cookie, buffer, bufferSize,
		*count, count);
	PRINT(("userlandfs_read_attrdir() done: (%lx, %ld)\n", error, *count));
	return error;
}

// userlandfs_rewind_attrdir
static
int
userlandfs_rewind_attrdir(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rewind_attrdir(%p, %p, %p)\n", ns, node, cookie));
	status_t error = volume->RewindAttrDir(node, cookie);
	PRINT(("userlandfs_rewind_attrdir() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_attr
static
int
userlandfs_read_attr(void *ns, void *node, const char *name, int type,
	void *buffer, size_t *bufferSize, off_t pos)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_attr(%p, %p, `%s', %d, %p, %lu, %Ld)\n", ns, node,
		name, type, buffer, *bufferSize, pos));
	status_t error = volume->ReadAttr(node, name, type, pos, buffer,
		*bufferSize, bufferSize);
	PRINT(("userlandfs_read_attr() done: (%lx, %ld)\n", error, *bufferSize));
	return error;
}

// userlandfs_write_attr
static
int
userlandfs_write_attr(void *ns, void *node, const char *name, int type,
	const void *buffer, size_t *bufferSize, off_t pos)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_write_attr(%p, %p, `%s', %d, %p, %lu, %Ld)\n", ns, node,
		name, type, buffer, *bufferSize, pos));
	status_t error = volume->WriteAttr(node, name, type, pos, buffer,
		*bufferSize, bufferSize);
	PRINT(("userlandfs_write_attr() done: (%lx, %ld)\n", error, *bufferSize));
	return error;
}

// userlandfs_remove_attr
static
int
userlandfs_remove_attr(void *ns, void *node, const char *name)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_remove_attr(%p, %p, `%s')\n", ns, node, name));
	status_t error = volume->RemoveAttr(node, name);
	PRINT(("userlandfs_remove_attr() done: (%lx)\n", error));
	return error;
}

// userlandfs_rename_attr
static
int
userlandfs_rename_attr(void *ns, void *node, const char *oldName,
	const char *newName)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rename_attr(%p, %p, `%s', `%s')\n", ns, node, oldName,
		newName));
	status_t error = volume->RenameAttr(node, oldName, newName);
	PRINT(("userlandfs_rename_attr() done: (%lx)\n", error));
	return error;
}

// userlandfs_stat_attr
static
int
userlandfs_stat_attr(void *ns, void *node, const char *name,
	struct attr_info *attrInfo)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_stat_attr(%p, %p, `%s', %p)\n", ns, node, name,
		attrInfo));
	status_t error = volume->StatAttr(node, name, attrInfo);
	PRINT(("userlandfs_stat_attr() done: (%lx)\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- indices -----

// userlandfs_open_indexdir
static
int
userlandfs_open_indexdir(void *ns, void **cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_open_indexdir(%p, %p)\n", ns, cookie));
	status_t error = volume->OpenIndexDir(cookie);
	PRINT(("userlandfs_open_indexdir() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_indexdir
static
int
userlandfs_close_indexdir(void *ns, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_close_indexdir(%p, %p)\n", ns, cookie));
	status_t error = volume->CloseIndexDir(cookie);
	PRINT(("userlandfs_close_indexdir() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_indexdir_cookie
static
int
userlandfs_free_indexdir_cookie(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_free_indexdir_cookie(%p, %p)\n", ns, cookie));
	status_t error = volume->FreeIndexDirCookie(cookie);
	PRINT(("userlandfs_free_indexdir_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_indexdir
static
int
userlandfs_read_indexdir(void *ns, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_indexdir(%p, %p, %ld, %p, %lu)\n", ns, cookie,
		*count, buffer, bufferSize));
	status_t error = volume->ReadIndexDir(cookie, buffer, bufferSize,
		*count, count);
	PRINT(("userlandfs_read_indexdir() done: (%lx, %ld)\n", error, *count));
	return error;
}

// userlandfs_rewind_indexdir
static
int
userlandfs_rewind_indexdir(void *ns, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rewind_indexdir(%p, %p)\n", ns, cookie));
	status_t error = volume->RewindIndexDir(cookie);
	PRINT(("userlandfs_rewind_indexdir() done: (%lx)\n", error));
	return error;
}

// userlandfs_create_index
static
int
userlandfs_create_index(void *ns, const char *name, int type, int flags)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_create_index(%p, `%s', %d, %d)\n", ns, name, type,
		flags));
	status_t error = volume->CreateIndex(name, type, flags);
	PRINT(("userlandfs_create_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_remove_index
static
int
userlandfs_remove_index(void *ns, const char *name)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_remove_index(%p, `%s')\n", ns, name));
	status_t error = volume->RemoveIndex(name);
	PRINT(("userlandfs_remove_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_rename_index
static
int
userlandfs_rename_index(void *ns, const char *oldName, const char *newName)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_rename_index(%p, `%s', `%s')\n", ns, oldName, newName));
	status_t error = volume->RenameIndex(oldName, newName);
	PRINT(("userlandfs_rename_index() done: (%lx)\n", error));
	return error;
}

// userlandfs_stat_index
static
int
userlandfs_stat_index(void *ns, const char *name, struct index_info *indexInfo)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_stat_index(%p, `%s', %p)\n", ns, name, indexInfo));
	status_t error = volume->StatIndex(name, indexInfo);
	PRINT(("userlandfs_stat_index() done: (%lx)\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- queries -----

// userlandfs_open_query
static
int
userlandfs_open_query(void *ns, const char *queryString, ulong flags,
	port_id port, long token, void **cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_open_query(%p, `%s', %lu, %ld, %ld, %p)\n", ns,
		queryString, flags, port, token, cookie));
	status_t error = volume->OpenQuery(queryString, flags, port, token, cookie);
	PRINT(("userlandfs_open_query() done: (%lx, %p)\n", error, *cookie));
	return error;
}

// userlandfs_close_query
static
int
userlandfs_close_query(void *ns, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_close_query(%p, %p)\n", ns, cookie));
	status_t error = volume->CloseQuery(cookie);
	PRINT(("userlandfs_close_query() done: (%lx)\n", error));
	return error;
}

// userlandfs_free_query_cookie
static
int
userlandfs_free_query_cookie(void *ns, void *node, void *cookie)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_free_query_cookie(%p, %p)\n", ns, cookie));
	status_t error = volume->FreeQueryCookie(cookie);
	PRINT(("userlandfs_free_query_cookie() done: (%lx)\n", error));
	return error;
}

// userlandfs_read_query
static
int
userlandfs_read_query(void *ns, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	Volume* volume = (Volume*)ns;
	PRINT(("userlandfs_read_query(%p, %p, %ld, %p, %lu)\n", ns, cookie,
		*count, buffer, bufferSize));
	status_t error = volume->ReadQuery(cookie, buffer, bufferSize,
		*count, count);
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

