/*
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <kernel.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <dirent.h>
#include <fs_interface.h>
#include <lock.h>
#include <util/list.h>

#define DEFAULT_FD_TABLE_SIZE	128
#define MAX_FD_TABLE_SIZE		2048
#define MAX_NODE_MONITORS		4096


struct kernel_args;
struct file_descriptor;
struct selectsync;
struct pollfd;


/** The I/O context of a process/team, holds the fd array among others */
typedef struct io_context {
	struct vnode *cwd;
	mutex		io_mutex;
	uint32		table_size;
	uint32		num_used_fds;
	struct file_descriptor **fds;
	struct list node_monitors;
	uint32		num_monitors;
	uint32		max_monitors;
} io_context;


/* macro to allocate a iovec array on the stack */
#define IOVECS(name, size) \
	uint8 _##name[sizeof(iovecs) + (size)*sizeof(iovec)]; \
	iovecs *name = (iovecs *)_##name


#ifdef __cplusplus
extern "C" {
#endif 

status_t vfs_init(struct kernel_args *ka);
status_t vfs_bootstrap_file_systems(void);
status_t vfs_mount_boot_file_system(void);
void vfs_exec_io_context(void *context);
void *vfs_new_io_context(void *parentContext);
int vfs_free_io_context(void *context);

struct rlimit;
int vfs_getrlimit(int resource, struct rlimit * rlp);
int vfs_setrlimit(int resource, const struct rlimit * rlp);

/* calls needed by the VM for paging and by the file cache */
int vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode);
status_t vfs_get_vnode_from_path(const char *path, bool kernel, void **vnode);
status_t vfs_get_vnode(mount_id mountID, vnode_id vnodeID, void **_vnode);
int vfs_put_vnode_ptr(void *vnode);
void vfs_vnode_acquire_ref(void *vnode);
void vfs_vnode_release_ref(void *vnode);
status_t vfs_get_cookie_from_fd(int fd, void **_cookie);
bool vfs_can_page(void *vnode, void *cookie);
status_t vfs_read_pages(void *vnode, void *cookie, off_t pos, const iovec *vecs, size_t count, size_t *_numBytes);
status_t vfs_write_pages(void *vnode, void *cookie, off_t pos, const iovec *vecs, size_t count, size_t *_numBytes);
status_t vfs_get_vnode_cache(void *vnode, void **_cache);
status_t vfs_get_file_map( void *_vnode, off_t offset, size_t size, struct file_io_vec *vecs, size_t *_count);
status_t vfs_get_fs_node_from_path(mount_id mountID, const char *path, bool kernel, void **_node);

/* special module convenience call */
status_t vfs_get_module_path(const char *basePath, const char *moduleName, char *pathBuffer, size_t bufferSize);

/* calls the syscall dispatcher should use for user file I/O */
status_t _user_mount(const char *path, const char *device, const char *fs_name,
			uint32 flags, void *args);
status_t _user_unmount(const char *path);
status_t _user_read_fs_info(dev_t device, struct fs_info *info);
status_t _user_write_fs_info(dev_t device, const struct fs_info *info, int mask);
dev_t _user_next_device(int32 *_cookie);
status_t _user_sync(void);
status_t _user_entry_ref_to_path(dev_t device, ino_t inode, const char *leaf,
			char *userPath, size_t pathLength);
int _user_open_entry_ref(dev_t device, ino_t inode, const char *name, int omode);
int _user_open(int fd, const char *path, int omode);
int _user_open_dir_node_ref(dev_t device, ino_t inode);
int _user_open_dir_entry_ref(dev_t device, ino_t inode, const char *uname);
int _user_open_dir(int fd, const char *path);
int _user_open_parent_dir(int fd, char *name, size_t nameLength);
status_t _user_fcntl(int fd, int op, uint32 argument);
status_t _user_fsync(int fd);
status_t _user_read_stat(int fd, const char *path, bool traverseLink,
			struct stat *stat, size_t statSize);
status_t _user_write_stat(int fd, const char *path, bool traverseLink,
			const struct stat *stat, size_t statSize, int statMask);
off_t _user_seek(int fd, off_t pos, int seekType);
int _user_create_entry_ref(dev_t device, ino_t inode, const char *uname, int omode, int perms);
int _user_create(const char *path, int omode, int perms);
status_t _user_create_dir_entry_ref(dev_t device, ino_t inode, const char *name, int perms);
status_t _user_create_dir(int fd, const char *path, int perms);
status_t _user_remove_dir(const char *path);
ssize_t _user_read_link(int fd, const char *path, char *buffer, size_t bufferSize);
status_t _user_write_link(const char *path, const char *toPath);
status_t _user_create_symlink(int fd, const char *path, const char *toPath,
			int mode);
status_t _user_create_link(const char *path, const char *toPath);
status_t _user_unlink(int fd, const char *path);
status_t _user_rename(int oldFD, const char *oldpath, int newFD,
			const char *newpath);
status_t _user_access(const char *path, int mode);
ssize_t _user_select(int numfds, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
			bigtime_t timeout, const sigset_t *sigMask);
ssize_t _user_poll(struct pollfd *fds, int numfds, bigtime_t timeout);
int _user_open_attr_dir(int fd, const char *path);
int _user_create_attr(int fd, const char *name, uint32 type, int openMode);
int _user_open_attr(int fd, const char *name, int openMode);
status_t _user_remove_attr(int fd, const char *name);
status_t _user_rename_attr(int fromFile, const char *fromName, int toFile, const char *toName);
int _user_open_index_dir(dev_t device);
status_t _user_create_index(dev_t device, const char *name, uint32 type, uint32 flags);
status_t _user_read_index_stat(dev_t device, const char *name, struct stat *stat);
status_t _user_remove_index(dev_t device, const char *name);
status_t _user_getcwd(char *buffer, size_t size);
status_t _user_setcwd(int fd, const char *path);
int _user_open_query(dev_t device, const char *query, uint32 flags,
			port_id port, int32 token);

/* fd user prototypes (implementation located in fd.c)  */
extern ssize_t _user_read(int fd, off_t pos, void *buffer, size_t bufferSize);
extern ssize_t _user_write(int fd, off_t pos, const void *buffer, size_t bufferSize);
extern status_t _user_ioctl(int fd, ulong cmd, void *data, size_t length);
extern ssize_t _user_read_dir(int fd, struct dirent *buffer, size_t bufferSize, uint32 maxCount);
extern status_t _user_rewind_dir(int fd);
extern status_t _user_close(int fd);
extern int _user_dup(int fd);
extern int _user_dup2(int ofd, int nfd);
extern status_t _user_lock_node(int fd);
extern status_t _user_unlock_node(int fd);

/* vfs entry points... */

#ifdef __cplusplus
}
#endif 

#endif	/* _KERNEL_VFS_H */
