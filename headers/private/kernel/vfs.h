/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <kernel.h>
#include <stage2.h>
#include <sys/stat.h>
#include <dirent.h>

#define DEFAULT_FD_TABLE_SIZE	128
#define MAX_FD_TABLE_SIZE		2048

#include <vfs_types.h>

/* macro to allocate a iovec array on the stack */
#define IOVECS(name, size) \
	uint8 _##name[sizeof(iovecs) + (size)*sizeof(iovec)]; \
	iovecs *name = (iovecs *)_##name

#ifdef __cplusplus
extern "C" {
#endif 


#if 0
struct fs_calls {
	int (*fs_mount)(void **fs_cookie, void *flags, void *covered_vnode, fs_id id, void **priv_vnode_root);
	int (*fs_unmount)(void *fs_cookie);
	int (*fs_register_mountpoint)(void *fs_cookie, void *vnode, void *redir_vnode);
	int (*fs_unregister_mountpoint)(void *fs_cookie, void *vnode);
	int (*fs_dispose_vnode)(void *fs_cookie, void *vnode);
	int (*fs_open)(void *fs_cookie, void *base_vnode, const char *path, const char *stream, stream_type stream_type, void **vnode, void **cookie, struct redir_struct *redir);
	int (*fs_seek)(void *fs_cookie, void *vnode, void *cookie, off_t pos, int seek_type);
	int (*fs_read)(void *fs_cookie, void *vnode, void *cookie, void *buf, off_t pos, size_t *len);
	int (*fs_write)(void *fs_cookie, void *vnode, void *cookie, const void *buf, off_t pos, size_t *len);
	int (*fs_ioctl)(void *fs_cookie, void *vnode, void *cookie, ulong op, void *buf, size_t len);
	int (*fs_close)(void *fs_cookie, void *vnode, void *cookie);
	int (*fs_create)(void *fs_cookie, void *base_vnode, const char *path, const char *stream, stream_type stream_type, struct redir_struct *redir);
	int (*fs_stat)(void *fs_cookie, void *base_vnode, const char *path, const char *stream, stream_type stream_type, struct vnode_stat *stat, struct redir_struct *redir);
};
#endif

struct fs_calls {
	int (*fs_mount)(fs_cookie *fs, fs_id id, const char *device, void *args, vnode_id *root_vnid);
	int (*fs_unmount)(fs_cookie fs);
	int (*fs_sync)(fs_cookie fs);

	int (*fs_lookup)(fs_cookie fs, fs_vnode dir, const char *name, vnode_id *id);

	int (*fs_getvnode)(fs_cookie fs, vnode_id id, fs_vnode *v, bool r);
	int (*fs_putvnode)(fs_cookie fs, fs_vnode v, bool r);
	int (*fs_removevnode)(fs_cookie fs, fs_vnode v, bool r);

	int (*fs_open)(fs_cookie fs, fs_vnode v, file_cookie *cookie, stream_type st, int oflags);
	int (*fs_close)(fs_cookie fs, fs_vnode v, file_cookie cookie);
	int (*fs_freecookie)(fs_cookie fs, fs_vnode v, file_cookie cookie);
	int (*fs_fsync)(fs_cookie fs, fs_vnode v);

	ssize_t (*fs_read)(fs_cookie fs, fs_vnode v, file_cookie cookie, void *buf, off_t pos, size_t *len);
	ssize_t (*fs_write)(fs_cookie fs, fs_vnode v, file_cookie cookie, const void *buf, off_t pos, size_t *len);
	int (*fs_seek)(fs_cookie fs, fs_vnode v, file_cookie cookie, off_t pos, int st);

	int (*fs_read_dir)(fs_cookie fs, fs_vnode v, file_cookie cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num);
	int (*fs_rewind_dir)(fs_cookie fs, fs_vnode v, file_cookie cookie);
	int (*fs_ioctl)(fs_cookie fs, fs_vnode v, file_cookie cookie, ulong op, void *buf, size_t len);

	int (*fs_canpage)(fs_cookie fs, fs_vnode v);
	ssize_t (*fs_readpage)(fs_cookie fs, fs_vnode v, iovecs *vecs, off_t pos);
	ssize_t (*fs_writepage)(fs_cookie fs, fs_vnode v, iovecs *vecs, off_t pos);

	int (*fs_create)(fs_cookie fs, fs_vnode dir, const char *name, stream_type st, void *create_args, vnode_id *new_vnid);
	int (*fs_unlink)(fs_cookie fs, fs_vnode dir, const char *name);
	int (*fs_rename)(fs_cookie fs, fs_vnode olddir, const char *oldname, fs_vnode newdir, const char *newname);

	int (*fs_rstat)(fs_cookie fs, fs_vnode v, struct stat *stat);
	int (*fs_wstat)(fs_cookie fs, fs_vnode v, struct stat *stat, int stat_mask);
};

int vfs_init(kernel_args *ka);
int vfs_bootstrap_all_filesystems(void);
int vfs_register_filesystem(const char *name, struct fs_calls *calls);
void *vfs_new_io_context(void *parent_ioctx);
int vfs_free_io_context(void *ioctx);
int vfs_test(void);

struct rlimit;
int vfs_getrlimit(int resource, struct rlimit * rlp);
int vfs_setrlimit(int resource, const struct rlimit * rlp);

image_id vfs_load_fs_module(const char *path);

/* calls needed by fs internals */
int vfs_get_vnode(fs_id fsid, vnode_id vnid, fs_vnode *v);
int vfs_put_vnode(fs_id fsid, vnode_id vnid);
int vfs_remove_vnode(fs_id fsid, vnode_id vnid);

/* calls needed by the VM for paging */
int vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode);
int vfs_get_vnode_from_path(const char *path, bool kernel, void **vnode);
int vfs_put_vnode_ptr(void *vnode);
void vfs_vnode_acquire_ref(void *vnode);
void vfs_vnode_release_ref(void *vnode);
ssize_t vfs_canpage(void *vnode);
ssize_t vfs_readpage(void *vnode, iovecs *vecs, off_t pos);
ssize_t vfs_writepage(void *vnode, iovecs *vecs, off_t pos);
void *vfs_get_cache_ptr(void *vnode);
int vfs_set_cache_ptr(void *vnode, void *cache);

/* calls kernel code should make for file I/O */
int sys_mount(const char *path, const char *device, const char *fs_name, void *args);
int sys_unmount(const char *path);
int sys_sync(void);
int sys_open(const char *path, stream_type st, int omode);
int sys_fsync(int fd);
int sys_seek(int fd, off_t pos, int seek_type);
int sys_create(const char *path, stream_type stream_type);
int sys_unlink(const char *path);
int sys_rename(const char *oldpath, const char *newpath);
int sys_wstat(const char *path, struct stat *stat, int stat_mask);
char *sys_getcwd(char *buf, size_t size);
int sys_setcwd(const char* path);
int sys_dup(int fd);
int sys_dup2(int ofd, int nfd);

/* calls the syscall dispatcher should use for user file I/O */
int user_mount(const char *path, const char *device, const char *fs_name, void *args);
int user_unmount(const char *path);
int user_sync(void);
int user_open(const char *path, stream_type st, int omode);
int user_fsync(int fd);
int user_seek(int fd, off_t pos, int seek_type);
int user_create(const char *path, stream_type stream_type);
int user_unlink(const char *path);
int user_rename(const char *oldpath, const char *newpath);
int user_rstat(const char *path, struct stat *stat);
int user_wstat(const char *path, struct stat *stat, int stat_mask);
int user_getcwd(char *buf, size_t size);
int user_setcwd(const char* path);
int user_dup(int fd);
int user_dup2(int ofd, int nfd);

/* fd kernel prototypes (implementation located in fd.c) */
extern ssize_t sys_read(int fd, void *buf, off_t pos, size_t len);
extern ssize_t sys_write(int fd, const void *buf, off_t pos, size_t len);
extern int sys_ioctl(int fd, ulong cmd, void *data, size_t length);
extern ssize_t sys_read_dir(int fd, struct dirent *buffer, size_t bufferSize);
extern status_t sys_rewind_dir(int fd);
extern int sys_rstat(const char *path, struct stat *stat);
extern int sys_close(int fd);


/* fd user prototypes (implementation located in fd.c)  */
extern ssize_t user_read(int fd, void *buf, off_t pos, size_t len);
extern ssize_t user_write(int fd, const void *buf, off_t pos, size_t len);
extern int user_ioctl(int fd, ulong cmd, void *data, size_t length);
extern ssize_t user_read_dir(int fd, struct dirent *buffer, size_t bufferSize);
extern status_t user_rewind_dir(int fd);
extern int user_fstat(int, struct stat *);
extern int user_close(int fd);

/* vfs entry points... */

#ifdef __cplusplus
}
#endif 

#endif

