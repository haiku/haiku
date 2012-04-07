// netfs.cpp

#include <new>

#include <KernelExport.h>
#include <fsproto.h>

#include "DebugSupport.h"
#include "Node.h"
#include "ObjectTracker.h"
#include "QueryManager.h"
#include "RootVolume.h"
#include "VolumeManager.h"

// #pragma mark -
// #pragma mark ----- prototypes -----

extern "C" {

// fs
static int netfs_mount(nspace_id nsid, const char *device, ulong flags,
				void *parameters, size_t len, void **data, vnode_id *rootID);
static int netfs_unmount(void *ns);
//static int netfs_sync(void *ns);
static int netfs_read_fs_stat(void *ns, struct fs_info *info);
//static int netfs_write_fs_stat(void *ns, struct fs_info *info, long mask);

// vnodes
static int netfs_read_vnode(void *ns, vnode_id vnid, char reenter,
				void **node);
static int netfs_write_vnode(void *ns, void *node, char reenter);
static int netfs_remove_vnode(void *ns, void *node, char reenter);

// nodes
//static int netfs_fsync(void *ns, void *node);
static int netfs_read_stat(void *ns, void *node, struct stat *st);
static int netfs_write_stat(void *ns, void *node, struct stat *st,
				long mask);
static int netfs_access(void *ns, void *node, int mode);

// files
static int netfs_create(void *ns, void *dir, const char *name,
				int openMode, int mode, vnode_id *vnid, void **cookie);
static int netfs_open(void *ns, void *node, int openMode, void **cookie);
static int netfs_close(void *ns, void *node, void *cookie);
static int netfs_free_cookie(void *ns, void *node, void *cookie);
static int netfs_read(void *ns, void *node, void *cookie, off_t pos,
				void *buffer, size_t *bufferSize);
static int netfs_write(void *ns, void *node, void *cookie, off_t pos,
				const void *buffer, size_t *bufferSize);
static int netfs_ioctl(void *ns, void *node, void *cookie, int cmd,
				void *buffer, size_t bufferSize);
//static int netfs_setflags(void *ns, void *node, void *cookie, int flags);

// hard links / symlinks
static int netfs_link(void *ns, void *dir, const char *name, void *node);
static int netfs_unlink(void *ns, void *dir, const char *name);
static int netfs_symlink(void *ns, void *dir, const char *name,
				const char *path);
static int netfs_read_link(void *ns, void *node, char *buffer,
				size_t *bufferSize);
static int netfs_rename(void *ns, void *oldDir, const char *oldName,
				void *newDir, const char *newName);

// directories
static int netfs_mkdir(void *ns, void *dir, const char *name, int mode);
static int netfs_rmdir(void *ns, void *dir, const char *name);
static int netfs_open_dir(void *ns, void *node, void **cookie);
static int netfs_close_dir(void *ns, void *node, void *cookie);
static int netfs_free_dir_cookie(void *ns, void *node, void *cookie);
static int netfs_read_dir(void *ns, void *node, void *cookie,
				long *count, struct dirent *buffer, size_t bufferSize);
static int netfs_rewind_dir(void *ns, void *node, void *cookie);
static int netfs_walk(void *ns, void *dir, const char *entryName,
				char **resolvedPath, vnode_id *vnid);

// attributes
static int netfs_open_attrdir(void *ns, void *node, void **cookie);
static int netfs_close_attrdir(void *ns, void *node, void *cookie);
static int netfs_free_attrdir_cookie(void *ns, void *node, void *cookie);
static int netfs_read_attrdir(void *ns, void *node, void *cookie,
				long *count, struct dirent *buffer, size_t bufferSize);
static int netfs_read_attr(void *ns, void *node, const char *name,
				int type, void *buffer, size_t *bufferSize, off_t pos);
static int netfs_rewind_attrdir(void *ns, void *node, void *cookie);
static int netfs_write_attr(void *ns, void *node, const char *name,
				int type, const void *buffer, size_t *bufferSize, off_t pos);
static int netfs_remove_attr(void *ns, void *node, const char *name);
static int netfs_rename_attr(void *ns, void *node, const char *oldName,
				const char *newName);
static int netfs_stat_attr(void *ns, void *node, const char *name,
				struct attr_info *attrInfo);

// queries
static int netfs_open_query(void *ns, const char *queryString, ulong flags,
				port_id port, long token, void **cookie);
static int netfs_close_query(void *ns, void *cookie);
static int netfs_free_query_cookie(void *ns, void *node, void *cookie);
static int netfs_read_query(void *ns, void *cookie, long *count,
				struct dirent *buffer, size_t bufferSize);

} // extern "C"

/* vnode_ops struct. Fill this in to tell the kernel how to call
	functions in your driver.
*/
vnode_ops fs_entry = {
	&netfs_read_vnode,				// read_vnode
	&netfs_write_vnode,				// write_vnode
	&netfs_remove_vnode,			// remove_vnode
	NULL,							// secure_vnode (not needed)
	&netfs_walk,					// walk
	&netfs_access,					// access
	&netfs_create,					// create
	&netfs_mkdir,					// mkdir
	&netfs_symlink,					// symlink
	&netfs_link,					// link
	&netfs_rename,					// rename
	&netfs_unlink,					// unlink
	&netfs_rmdir,					// rmdir
	&netfs_read_link,				// readlink
	&netfs_open_dir,				// opendir
	&netfs_close_dir,				// closedir
	&netfs_free_dir_cookie,			// free_dircookie
	&netfs_rewind_dir,				// rewinddir
	&netfs_read_dir,				// readdir
	&netfs_open,					// open file
	&netfs_close,					// close file
	&netfs_free_cookie,				// free cookie
	&netfs_read,					// read file
	&netfs_write,					// write file
	NULL,							// readv
	NULL,							// writev
	&netfs_ioctl,					// ioctl
	NULL,							// setflags file
	&netfs_read_stat,				// read stat
	&netfs_write_stat,				// write stat
	NULL,							// fsync
	NULL,							// initialize
	&netfs_mount,					// mount
	&netfs_unmount,					// unmount
	NULL,							// sync
	&netfs_read_fs_stat,			// read fs stat
	NULL,							// write fs stat
	NULL,							// select
	NULL,							// deselect

	NULL,							// open index dir
	NULL,							// close index dir
	NULL,							// free index dir cookie
	NULL,							// rewind index dir
	NULL,							// read index dir
	NULL,							// create index
	NULL,							// remove index
	NULL,							// rename index
	NULL,							// stat index

	&netfs_open_attrdir,			// open attr dir
	&netfs_close_attrdir,			// close attr dir
	&netfs_free_attrdir_cookie,		// free attr dir cookie
	&netfs_rewind_attrdir,			// rewind attr dir
	&netfs_read_attrdir,			// read attr dir
	&netfs_write_attr,				// write attr
	&netfs_read_attr,				// read attr
	&netfs_remove_attr,				// remove attr
	&netfs_rename_attr,				// rename attr
	&netfs_stat_attr,				// stat attr

	&netfs_open_query,				// open query
	&netfs_close_query,				// close query
	&netfs_free_query_cookie,		// free query cookie
	&netfs_read_query,				// read query
};

int32 api_version = B_CUR_FS_API_VERSION;

// #pragma mark -
// #pragma mark ----- fs -----

// netfs_mount
static
int
netfs_mount(nspace_id nsid, const char *device, ulong flags,
	void *parameters, size_t len, void **data, vnode_id *rootID)
{
	status_t error = B_OK;
	init_debugging();

	#ifdef DEBUG_OBJECT_TRACKING
		ObjectTracker::InitDefault();
	#endif

	// create and init the volume manager
	VolumeManager* volumeManager = new(std::nothrow) VolumeManager(nsid, flags);
	Volume* rootVolume = NULL;
	if (volumeManager) {
		error = volumeManager->MountRootVolume(device,
			(const char*)parameters, len, &rootVolume);
		if (error != B_OK) {
			delete volumeManager;
			volumeManager = NULL;
		}
	} else
		error = B_NO_MEMORY;
	VolumePutter _(rootVolume);

	// set results
	if (error == B_OK) {
		*data = volumeManager;
		*rootID = rootVolume->GetRootID();
	} else {
		#ifdef DEBUG_OBJECT_TRACKING
			ObjectTracker::ExitDefault();
		#endif
		exit_debugging();
	}
	return error;
}

// netfs_unmount
static
int
netfs_unmount(void *ns)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;

	PRINT("netfs_unmount()\n");

	volumeManager->UnmountRootVolume();
	delete volumeManager;

	#ifdef DEBUG_OBJECT_TRACKING
		ObjectTracker::ExitDefault();
	#endif

	PRINT("netfs_unmount() done\n");

	exit_debugging();
	return B_OK;
}

#if 0 // not used

// netfs_sync
static
int
netfs_sync(void *ns)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	Volume* volume = volumeManager->GetRootVolume();
	VolumePutter _(volume);

	PRINT("netfs_sync(%p)\n", ns);

	status_t error = B_BAD_VALUE;
	if (volume)
		error = volume->Sync();

	PRINT("netfs_sync() done: %lx \n", error);

	return error;
}

#endif

// netfs_read_fs_stat
static
int
netfs_read_fs_stat(void *ns, struct fs_info *info)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	Volume* volume = volumeManager->GetRootVolume();
	VolumePutter _(volume);

	PRINT("netfs_read_fs_stat(%p, %p)\n", ns, info);

	status_t error = B_BAD_VALUE;
	if (volume)
		error = volume->ReadFSStat(info);

	PRINT("netfs_read_fs_stat() done: %lx \n", error);

	return error;
}

#if 0 // not used

// netfs_write_fs_stat
static
int
netfs_write_fs_stat(void *ns, struct fs_info *info, long mask)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	Volume* volume = volumeManager->GetRootVolume();
	VolumePutter _(volume);

	PRINT("netfs_write_fs_stat(%p, %p, %ld)\n", ns, info, mask);

	status_t error = B_BAD_VALUE;
	if (volume)
		error = volume->WriteFSStat(info, mask);

	PRINT("netfs_write_fs_stat() done: %lx \n", error);

	return error;
}

#endif

// #pragma mark -
// #pragma mark ----- vnodes -----

// netfs_read_vnode
static
int
netfs_read_vnode(void *ns, vnode_id vnid, char reenter, void **node)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	Volume* volume = volumeManager->GetVolume(vnid);
	VolumePutter _(volume);

	PRINT("netfs_read_vnode(%p, %Ld, %d, %p)\n", ns, vnid, reenter, node);

	status_t error = B_BAD_VALUE;
	if (volume)
		error = volume->ReadVNode(vnid, reenter, (Node**)node);

	PRINT("netfs_read_vnode() done: (%lx, %p)\n", error, *node);

	return error;
}

// netfs_write_vnode
static
int
netfs_write_vnode(void *ns, void *_node, char reenter)
{
	Node* node = (Node*)_node;
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	PRINT(("netfs_write_vnode(%p, %p, %d)\n", ns, node, reenter));
	status_t error = node->GetVolume()->WriteVNode(node, reenter);
//	PRINT(("netfs_write_vnode() done: %lx\n", error));
	return error;
}

// netfs_remove_vnode
static
int
netfs_remove_vnode(void *ns, void *_node, char reenter)
{
	Node* node = (Node*)_node;
// DANGER: See netfs_write_vnode().
//	PRINT(("netfs_remove_vnode(%p, %p, %d)\n", ns, node, reenter));
	status_t error = node->GetVolume()->RemoveVNode(node, reenter);
//	PRINT(("netfs_remove_vnode() done: %lx\n", error));
	return error;
}

// #pragma mark -
// #pragma mark ----- nodes -----

#if 0 // not used

// netfs_fsync
static
int
netfs_fsync(void *ns, void *_node)
{
	Node* node = (Node*)_node;
	PRINT("netfs_fsync(%p, %p)\n", ns, node);
	status_t error = node->GetVolume()->FSync(node);
	PRINT("netfs_fsync() done: %lx\n", error);
	return error;
}

#endif

// netfs_read_stat
static
int
netfs_read_stat(void *ns, void *_node, struct stat *st)
{
	Node* node = (Node*)_node;
	PRINT("netfs_read_stat(%p, %p, %p)\n", ns, node, st);
	status_t error = node->GetVolume()->ReadStat(node, st);
	PRINT("netfs_read_stat() done: %lx\n", error);
	return error;
}

// netfs_write_stat
static
int
netfs_write_stat(void *ns, void *_node, struct stat *st, long mask)
{
	Node* node = (Node*)_node;
	PRINT("netfs_write_stat(%p, %p, %p, %ld)\n", ns, node, st, mask);
	status_t error = node->GetVolume()->WriteStat(node, st, mask);
	PRINT("netfs_write_stat() done: %lx\n", error);
	return error;
}

// netfs_access
static
int
netfs_access(void *ns, void *_node, int mode)
{
	Node* node = (Node*)_node;
	PRINT("netfs_access(%p, %p, %d)\n", ns, node, mode);
	status_t error = node->GetVolume()->Access(node, mode);
	PRINT("netfs_access() done: %lx\n", error);
	return error;
}

// #pragma mark -
// #pragma mark ----- files -----

// netfs_create
static
int
netfs_create(void *ns, void *_dir, const char *name, int openMode, int mode,
	vnode_id *vnid, void **cookie)
{
	Node* dir = (Node*)_dir;
	PRINT("netfs_create(%p, %p, `%s', %d, %d, %p, %p)\n", ns, dir,
		name, openMode, mode, vnid, cookie);
	status_t error = dir->GetVolume()->Create(dir, name, openMode, mode, vnid,
		cookie);
	PRINT("netfs_create() done: (%lx, %Ld, %p)\n", error, *vnid,
		*cookie);
	return error;
}

// netfs_open
static
int
netfs_open(void *ns, void *_node, int openMode, void **cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_open(%p, %p, %d)\n", ns, node, openMode);
	status_t error = node->GetVolume()->Open(node, openMode, cookie);
	PRINT("netfs_open() done: (%lx, %p)\n", error, *cookie);
	return error;
}

// netfs_close
static
int
netfs_close(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_close(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->Close(node, cookie);
	PRINT("netfs_close() done: %lx\n", error);
	return error;
}

// netfs_free_cookie
static
int
netfs_free_cookie(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_free_cookie(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->FreeCookie(node, cookie);
	PRINT("netfs_free_cookie() done: %lx\n", error);
	return error;
}

// netfs_read
static
int
netfs_read(void *ns, void *_node, void *cookie, off_t pos, void *buffer,
	size_t *bufferSize)
{
	Node* node = (Node*)_node;
	PRINT("netfs_read(%p, %p, %p, %Ld, %p, %lu)\n", ns, node, cookie, pos,
		buffer, *bufferSize);
	status_t error = node->GetVolume()->Read(node, cookie, pos, buffer,
		*bufferSize, bufferSize);
	PRINT("netfs_read() done: (%lx, %lu)\n", error, *bufferSize);
	return error;
}

// netfs_write
static
int
netfs_write(void *ns, void *_node, void *cookie, off_t pos,
	const void *buffer, size_t *bufferSize)
{
	Node* node = (Node*)_node;
	PRINT("netfs_write(%p, %p, %p, %Ld, %p, %lu)\n", ns, node, cookie, pos,
		buffer, *bufferSize);
	status_t error = node->GetVolume()->Write(node, cookie, pos, buffer,
		*bufferSize, bufferSize);
	PRINT("netfs_write() done: (%lx, %lu)\n", error, *bufferSize);
	return error;
}

// netfs_ioctl
static
int
netfs_ioctl(void *ns, void *_node, void *cookie, int cmd, void *buffer,
	size_t bufferSize)
{
	Node* node = (Node*)_node;
	PRINT("netfs_ioctl(%p, %p, %p, %d, %p, %lu)\n", ns, node, cookie, cmd,
		buffer, bufferSize);
	status_t error = node->GetVolume()->IOCtl(node, cookie, cmd, buffer,
		bufferSize);
	PRINT("netfs_ioctl() done: (%lx)\n", error);
	return error;
}

// netfs_setflags
//static
//int
//netfs_setflags(void *ns, void *_node, void *cookie, int flags)
//{
//	Node* node = (Node*)_node;
//	PRINT(("netfs_setflags(%p, %p, %p, %d)\n", ns, node, cookie, flags));
//	status_t error = node->GetVolume()->SetFlags(node, cookie, flags);
//	PRINT(("netfs_setflags() done: (%lx)\n", error));
//	return error;
//}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// netfs_link
static
int
netfs_link(void *ns, void *_dir, const char *name, void *_node)
{
	Node* dir = (Node*)_dir;
	Node* node = (Node*)_node;
	PRINT("netfs_link(%p, %p, `%s', %p)\n", ns, dir, name, node);
	status_t error = dir->GetVolume()->Link(dir, name, node);
	PRINT("netfs_link() done: (%lx)\n", error);
	return error;
}

// netfs_unlink
static
int
netfs_unlink(void *ns, void *_dir, const char *name)
{
	Node* dir = (Node*)_dir;
	PRINT("netfs_unlink(%p, %p, `%s')\n", ns, dir, name);
	status_t error = dir->GetVolume()->Unlink(dir, name);
	PRINT("netfs_unlink() done: (%lx)\n", error);
	return error;
}

// netfs_symlink
static
int
netfs_symlink(void *ns, void *_dir, const char *name, const char *path)
{
	Node* dir = (Node*)_dir;
	PRINT("netfs_symlink(%p, %p, `%s', `%s')\n", ns, dir, name, path);
	status_t error = dir->GetVolume()->Symlink(dir, name, path);
	PRINT("netfs_symlink() done: (%lx)\n", error);
	return error;
}

// netfs_read_link
static
int
netfs_read_link(void *ns, void *_node, char *buffer, size_t *bufferSize)
{
	Node* node = (Node*)_node;
	PRINT("netfs_read_link(%p, %p, %p, %lu)\n", ns, node, buffer,
		*bufferSize);
	status_t error = node->GetVolume()->ReadLink(node, buffer, *bufferSize,
		bufferSize);
	PRINT("netfs_read_link() done: (%lx, %lu)\n", error, *bufferSize);
	return error;
}

// netfs_rename
static
int
netfs_rename(void *ns, void *_oldDir, const char *oldName, void *_newDir,
	const char *newName)
{
	Node* oldDir = (Node*)_oldDir;
	Node* newDir = (Node*)_newDir;
	PRINT("netfs_rename(%p, %p, `%s', %p, `%s')\n", ns, oldDir, oldName,
		newDir, newName);
	status_t error = oldDir->GetVolume()->Rename(oldDir, oldName,
		newDir, newName);
	PRINT("netfs_rename() done: (%lx)\n", error);
	return error;
}

// #pragma mark -
// #pragma mark ----- directories -----

// netfs_mkdir
static
int
netfs_mkdir(void *ns, void *_dir, const char *name, int mode)
{
	Node* dir = (Node*)_dir;
	PRINT("netfs_mkdir(%p, %p, `%s', %d)\n", ns, dir, name, mode);
	status_t error = dir->GetVolume()->MkDir(dir, name, mode);
	PRINT("netfs_mkdir() done: (%lx)\n", error);
	return error;
}

// netfs_rmdir
static
int
netfs_rmdir(void *ns, void *_dir, const char *name)
{
	Node* dir = (Node*)_dir;
	PRINT("netfs_rmdir(%p, %p, `%s')\n", ns, dir, name);
	status_t error = dir->GetVolume()->RmDir(dir, name);
	PRINT("netfs_rmdir() done: (%lx)\n", error);
	return error;
}

// netfs_open_dir
static
int
netfs_open_dir(void *ns, void *_node, void **cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_open_dir(%p, %p)\n", ns, node);
	status_t error = node->GetVolume()->OpenDir(node, cookie);
	PRINT("netfs_open_dir() done: (%lx, %p)\n", error, *cookie);
	return error;
}

// netfs_close_dir
static
int
netfs_close_dir(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_close_dir(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->CloseDir(node, cookie);
	PRINT("netfs_close_dir() done: %lx\n", error);
	return error;
}

// netfs_free_dir_cookie
static
int
netfs_free_dir_cookie(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_free_dir_cookie(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->FreeDirCookie(node, cookie);
	PRINT("netfs_free_dir_cookie() done: %lx \n", error);
	return error;
}

// netfs_read_dir
static
int
netfs_read_dir(void *ns, void *_node, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	Node* node = (Node*)_node;
	PRINT("netfs_read_dir(%p, %p, %p, %ld, %p, %lu)\n", ns, node, cookie,
		*count, buffer, bufferSize);
	status_t error = node->GetVolume()->ReadDir(node, cookie, buffer,
		bufferSize, *count, count);
	PRINT("netfs_read_dir() done: (%lx, %ld)\n", error, *count);
	#if DEBUG
		dirent* entry = buffer;
		for (int32 i = 0; i < *count; i++) {
			// R5's kernel vsprintf() doesn't seem to know `%.<number>s', so
			// we need to work around.
			char name[B_FILE_NAME_LENGTH];
			int nameLen = strnlen(entry->d_name, B_FILE_NAME_LENGTH - 1);
			strncpy(name, entry->d_name, nameLen);
			name[nameLen] = '\0';
			PRINT("  entry: d_dev: %ld, d_pdev: %ld, d_ino: %Ld,"
				" d_pino: %Ld, d_reclen: %hu, d_name: `%s'\n",
				entry->d_dev, entry->d_pdev, entry->d_ino,
				entry->d_pino, entry->d_reclen, name);
			entry = (dirent*)((char*)entry + entry->d_reclen);
		}
	#endif

	return error;
}

// netfs_rewind_dir
static
int
netfs_rewind_dir(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_rewind_dir(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->RewindDir(node, cookie);
	PRINT("netfs_rewind_dir() done: %lx\n", error);
	return error;
}

// netfs_walk
static
int
netfs_walk(void *ns, void *_dir, const char *entryName,
	char **resolvedPath, vnode_id *vnid)
{
	Node* dir = (Node*)_dir;
	PRINT("netfs_walk(%p, %p, `%s', %p, %p)\n", ns, dir,
		entryName, resolvedPath, vnid);
	status_t error = dir->GetVolume()->Walk(dir, entryName, resolvedPath, vnid);
	PRINT("netfs_walk() done: (%lx, `%s', %Ld)\n", error,
		(resolvedPath ? *resolvedPath : NULL), *vnid);
	return error;
}

// #pragma mark -
// #pragma mark ----- attributes -----

// netfs_open_attrdir
static
int
netfs_open_attrdir(void *ns, void *_node, void **cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_open_attrdir(%p, %p)\n", ns, node);
	status_t error = node->GetVolume()->OpenAttrDir(node, cookie);
	PRINT("netfs_open_attrdir() done: (%lx, %p)\n", error, *cookie);
	return error;
}

// netfs_close_attrdir
static
int
netfs_close_attrdir(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_close_attrdir(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->CloseAttrDir(node, cookie);
	PRINT("netfs_close_attrdir() done: (%lx)\n", error);
	return error;
}

// netfs_free_attrdir_cookie
static
int
netfs_free_attrdir_cookie(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_free_attrdir_cookie(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->FreeAttrDirCookie(node, cookie);
	PRINT("netfs_free_attrdir_cookie() done: (%lx)\n", error);
	return error;
}

// netfs_read_attrdir
static
int
netfs_read_attrdir(void *ns, void *_node, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	Node* node = (Node*)_node;
	PRINT("netfs_read_attrdir(%p, %p, %p, %ld, %p, %lu)\n", ns, node,
		cookie, *count, buffer, bufferSize);
	status_t error = node->GetVolume()->ReadAttrDir(node, cookie, buffer,
		bufferSize, *count, count);
	PRINT("netfs_read_attrdir() done: (%lx, %ld)\n", error, *count);
	return error;
}

// netfs_rewind_attrdir
static
int
netfs_rewind_attrdir(void *ns, void *_node, void *cookie)
{
	Node* node = (Node*)_node;
	PRINT("netfs_rewind_attrdir(%p, %p, %p)\n", ns, node, cookie);
	status_t error = node->GetVolume()->RewindAttrDir(node, cookie);
	PRINT("netfs_rewind_attrdir() done: (%lx)\n", error);
	return error;
}

// netfs_read_attr
static
int
netfs_read_attr(void *ns, void *_node, const char *name, int type,
	void *buffer, size_t *bufferSize, off_t pos)
{
	Node* node = (Node*)_node;
	PRINT("netfs_read_attr(%p, %p, `%s', %d, %p, %lu, %Ld)\n", ns, node,
		name, type, buffer, *bufferSize, pos);
	status_t error = node->GetVolume()->ReadAttr(node, name, type, pos, buffer,
		*bufferSize, bufferSize);
	PRINT("netfs_read_attr() done: (%lx, %ld)\n", error, *bufferSize);
	return error;
}

// netfs_write_attr
static
int
netfs_write_attr(void *ns, void *_node, const char *name, int type,
	const void *buffer, size_t *bufferSize, off_t pos)
{
	Node* node = (Node*)_node;
	PRINT("netfs_write_attr(%p, %p, `%s', %d, %p, %lu, %Ld)\n", ns, node,
		name, type, buffer, *bufferSize, pos);
	status_t error = node->GetVolume()->WriteAttr(node, name, type, pos, buffer,
		*bufferSize, bufferSize);
	PRINT("netfs_write_attr() done: (%lx, %ld)\n", error, *bufferSize);
	return error;
}

// netfs_remove_attr
static
int
netfs_remove_attr(void *ns, void *_node, const char *name)
{
	Node* node = (Node*)_node;
	PRINT("netfs_remove_attr(%p, %p, `%s')\n", ns, node, name);
	status_t error = node->GetVolume()->RemoveAttr(node, name);
	PRINT("netfs_remove_attr() done: (%lx)\n", error);
	return error;
}

// netfs_rename_attr
static
int
netfs_rename_attr(void *ns, void *_node, const char *oldName,
	const char *newName)
{
	Node* node = (Node*)_node;
	PRINT("netfs_rename_attr(%p, %p, `%s', `%s')\n", ns, node, oldName,
		newName);
	status_t error = node->GetVolume()->RenameAttr(node, oldName, newName);
	PRINT("netfs_rename_attr() done: (%lx)\n", error);
	return error;
}

// netfs_stat_attr
static
int
netfs_stat_attr(void *ns, void *_node, const char *name,
	struct attr_info *attrInfo)
{
	Node* node = (Node*)_node;
	PRINT("netfs_stat_attr(%p, %p, `%s', %p)\n", ns, node, name,
		attrInfo);
	status_t error = node->GetVolume()->StatAttr(node, name, attrInfo);
	PRINT("netfs_stat_attr() done: (%lx)\n", error);
	return error;
}

// #pragma mark -
// #pragma mark ----- queries -----

// netfs_open_query
static
int
netfs_open_query(void *ns, const char *queryString, ulong flags,
	port_id port, long token, void **cookie)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	Volume* volume = volumeManager->GetRootVolume();
	VolumePutter _(volume);

	PRINT("netfs_open_query(%p, `%s', %lu, %ld, %ld, %p)\n", ns,
		queryString, flags, port, token, cookie);

	status_t error = B_BAD_VALUE;
	if (volume) {
		error = volume->OpenQuery(queryString, flags, port, token,
			(QueryIterator**)cookie);
	}

	PRINT("netfs_open_query() done: (%lx, %p)\n", error, *cookie);
	return error;
}

// netfs_close_query
static
int
netfs_close_query(void *ns, void *cookie)
{
	PRINT("netfs_close_query(%p, %p)\n", ns, cookie);

	status_t error = B_OK;
	// no-op: we don't use this hook

	PRINT("netfs_close_query() done: (%lx)\n", error);
	return error;
}

// netfs_free_query_cookie
static
int
netfs_free_query_cookie(void *ns, void *node, void *cookie)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	QueryIterator* iterator = (QueryIterator*)cookie;

	PRINT("netfs_free_query_cookie(%p, %p)\n", ns, cookie);

	status_t error = B_OK;
	volumeManager->GetQueryManager()->PutIterator(iterator);

	PRINT("netfs_free_query_cookie() done: (%lx)\n", error);
	return error;
}

// netfs_read_query
static
int
netfs_read_query(void *ns, void *cookie, long *count,
	struct dirent *buffer, size_t bufferSize)
{
	VolumeManager* volumeManager = (VolumeManager*)ns;
	Volume* volume = volumeManager->GetRootVolume();
	QueryIterator* iterator = (QueryIterator*)cookie;
	VolumePutter _(volume);

	PRINT("netfs_read_query(%p, %p, %ld, %p, %lu)\n", ns, cookie,
		*count, buffer, bufferSize);

	status_t error = B_BAD_VALUE;
	if (volume) {
		error = volume->ReadQuery(iterator, buffer, bufferSize,
		*count, count);
	}

	PRINT("netfs_read_query() done: (%lx, %ld)\n", error, *count);
	return error;
}

