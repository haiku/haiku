/*
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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

#include <vfs_types.h>


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
status_t vfs_register_file_system(const char *name, struct fs_ops *calls);
void *vfs_new_io_context(void *parent_ioctx);
int vfs_free_io_context(void *ioctx);

struct rlimit;
int vfs_getrlimit(int resource, struct rlimit * rlp);
int vfs_setrlimit(int resource, const struct rlimit * rlp);

// ToDo: for now; this prototype should be in os/drivers/Drivers.h
//	or similar places.
extern status_t notify_select_event(struct selectsync *sync, uint32 ref, uint8 event);

/* calls needed by the VM for paging */
int vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode);
status_t vfs_get_vnode_from_path(const char *path, bool kernel, void **vnode);
int vfs_put_vnode_ptr(void *vnode);
void vfs_vnode_acquire_ref(void *vnode);
void vfs_vnode_release_ref(void *vnode);
ssize_t vfs_can_page(void *vnode);
ssize_t vfs_read_page(void *vnode, iovecs *vecs, off_t pos);
ssize_t vfs_write_page(void *vnode, iovecs *vecs, off_t pos);
void *vfs_get_cache_ptr(void *vnode);
int vfs_set_cache_ptr(void *vnode, void *cache);

/* special module convenience call */
status_t vfs_get_module_path(const char *basePath, const char *moduleName, char *pathBuffer, size_t bufferSize);

/* calls kernel code should make for file I/O */
int sys_mount(const char *path, const char *device, const char *fs_name, void *args);
int sys_unmount(const char *path);
status_t _kern_read_fs_info(dev_t device, struct fs_info *info);
status_t _kern_write_fs_info(dev_t device, const struct fs_info *info, int mask);
int sys_sync(void);
int sys_open_entry_ref(dev_t device, ino_t inode, const char *name, int omode);
int sys_open(const char *path, int omode);
int sys_open_dir_node_ref(dev_t device, ino_t inode);
int sys_open_dir_entry_ref(dev_t device, ino_t inode, const char *name);
int sys_open_dir(const char *path);
int sys_fsync(int fd);
off_t sys_seek(int fd, off_t pos, int seekType);
int sys_create_entry_ref(dev_t device, ino_t inode, const char *uname, int omode, int perms);
int sys_create(const char *path, int omode, int perms);
int sys_create_dir_entry_ref(dev_t device, ino_t inode, const char *name, int perms);
int sys_create_dir(const char *path, int perms);
int sys_remove_dir(const char *path);
int sys_read_link(const char *path, char *buffer, size_t bufferSize);
int sys_write_link(const char *path, const char *toPath);
int sys_create_symlink(const char *path, const char *toPath, int mode);
int sys_create_link(const char *path, const char *toPath);
int sys_unlink(const char *path);
int sys_rename(const char *oldpath, const char *newpath);
int sys_access(const char *path, int mode);
status_t _kern_read_path_stat(const char *path, bool traverseLink, struct stat *stat, size_t statSize);
status_t _kern_write_path_stat(const char *path, bool traverseLink, const struct stat *stat, size_t statSize, int statMask);
int sys_select(int numfds, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
	bigtime_t timeout, const sigset_t *sigMask);
int sys_poll(struct pollfd *fds, int numfds, bigtime_t timeout);
int sys_open_attr_dir(int fd, const char *path);
int sys_create_attr(int fd, const char *name, uint32 type, int openMode);
int sys_open_attr(int fd, const char *name, int openMode);
int sys_remove_attr(int fd, const char *name);
int sys_rename_attr(int fromFile, const char *fromName, int toFile, const char *toName);
int sys_open_index_dir(dev_t device);
int sys_create_index(dev_t device, const char *name, uint32 type, uint32 flags);
int sys_read_index_stat(dev_t device, const char *name, struct stat *stat);
int sys_remove_index(dev_t device, const char *name);
int sys_getcwd(char *buffer, size_t size);
int sys_setcwd(int fd, const char *path);

/* calls the syscall dispatcher should use for user file I/O */
int user_mount(const char *path, const char *device, const char *fs_name, void *args);
int user_unmount(const char *path);
status_t _user_read_fs_info(dev_t device, struct fs_info *info);
status_t _user_write_fs_info(dev_t device, const struct fs_info *info, int mask);
int user_sync(void);
int user_open_entry_ref(dev_t device, ino_t inode, const char *name, int omode);
int user_open(const char *path, int omode);
int user_open_dir_node_ref(dev_t device, ino_t inode);
int user_open_dir_entry_ref(dev_t device, ino_t inode, const char *uname);
int user_open_dir(const char *path);
int user_fsync(int fd);
off_t user_seek(int fd, off_t pos, int seekType);
int user_create_entry_ref(dev_t device, ino_t inode, const char *uname, int omode, int perms);
int user_create(const char *path, int omode, int perms);
int user_create_dir_entry_ref(dev_t device, ino_t inode, const char *name, int perms);
int user_create_dir(const char *path, int perms);
int user_remove_dir(const char *path);
int user_read_link(const char *path, char *buffer, size_t bufferSize);
int user_write_link(const char *path, const char *toPath);
int user_create_symlink(const char *path, const char *toPath, int mode);
int user_create_link(const char *path, const char *toPath);
int user_unlink(const char *path);
int user_rename(const char *oldpath, const char *newpath);
int user_access(const char *path, int mode);
status_t _user_read_path_stat(const char *path, bool traverseLink, struct stat *stat, size_t statSize);
status_t _user_write_path_stat(const char *path, bool traverseLink, const struct stat *stat, size_t statSize, int statMask);
int user_select(int numfds, fd_set *readSet, fd_set *writeSet, fd_set *errorSet,
	bigtime_t timeout, const sigset_t *sigMask);
int user_poll(struct pollfd *fds, int numfds, bigtime_t timeout);
int user_open_attr_dir(int fd, const char *path);
int user_create_attr(int fd, const char *name, uint32 type, int openMode);
int user_open_attr(int fd, const char *name, int openMode);
int user_remove_attr(int fd, const char *name);
int user_rename_attr(int fromFile, const char *fromName, int toFile, const char *toName);
int user_open_index_dir(dev_t device);
int user_create_index(dev_t device, const char *name, uint32 type, uint32 flags);
int user_read_index_stat(dev_t device, const char *name, struct stat *stat);
int user_remove_index(dev_t device, const char *name);
int user_getcwd(char *buffer, size_t size);
int user_setcwd(int fd, const char *path);

/* fd kernel prototypes (implementation located in fd.c) */
extern ssize_t sys_read(int fd, off_t pos, void *buffer, size_t bufferSize);
extern ssize_t sys_write(int fd, off_t pos, const void *buffer, size_t bufferSize);
extern int sys_ioctl(int fd, ulong cmd, void *data, size_t length);
extern ssize_t sys_read_dir(int fd, struct dirent *buffer, size_t bufferSize, uint32 maxCount);
extern status_t sys_rewind_dir(int fd);
extern status_t _kern_read_stat(int fd, struct stat *stat, size_t statSize);
extern status_t _kern_write_stat(int fd, const struct stat *stat, size_t statSize, int statMask);
extern int sys_close(int fd);
extern int sys_dup(int fd);
extern int sys_dup2(int ofd, int nfd);

/* fd user prototypes (implementation located in fd.c)  */
extern ssize_t user_read(int fd, off_t pos, void *buffer, size_t bufferSize);
extern ssize_t user_write(int fd, off_t pos, const void *buffer, size_t bufferSize);
extern int user_ioctl(int fd, ulong cmd, void *data, size_t length);
extern ssize_t user_read_dir(int fd, struct dirent *buffer, size_t bufferSize, uint32 maxCount);
extern status_t user_rewind_dir(int fd);
extern status_t _user_read_stat(int fd, struct stat *stat, size_t statSize);
extern status_t _user_write_stat(int fd, const struct stat *stat, size_t statSize, int statMask);
extern int user_close(int fd);
extern int user_dup(int fd);
extern int user_dup2(int ofd, int nfd);

/* vfs entry points... */

#ifdef __cplusplus
}
#endif 

#endif	/* _KERNEL_VFS_H */
