/*
 * Copyright 2004-2017, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SYSCALLS_H
#define _KERNEL_SYSCALLS_H


#include <OS.h>
#include <image.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Since libroot_build is also used on Haiku and linked against the real
 * libroot which also has the syscall functions, these must be shadowed. */
#define _kern_entry_ref_to_path		_kernbuild_entry_ref_to_path
#define _kern_open_entry_ref		_kernbuild_open_entry_ref
#define _kern_open					_kernbuild_open
#define _kern_open_dir_entry_ref	_kernbuild_open_dir_entry_ref
#define _kern_open_dir				_kernbuild_open_dir
#define _kern_open_parent_dir		_kernbuild_open_parent_dir
#define _kern_fsync					_kernbuild_fsync
#define _kern_seek					_kernbuild_seek
#define _kern_create_dir_entry_ref	_kernbuild_create_dir_entry_ref
#define _kern_create_dir			_kernbuild_create_dir
#define _kern_read_link				_kernbuild_read_link
#define _kern_create_symlink		_kernbuild_create_symlink
#define _kern_unlink				_kernbuild_unlink
#define _kern_rename				_kernbuild_rename
#define _kern_open_attr_dir			_kernbuild_open_attr_dir
#define _kern_remove_attr			_kernbuild_remove_attr
#define _kern_rename_attr			_kernbuild_rename_attr
#define _kern_read					_kernbuild_read
#define _kern_readv					_kernbuild_readv
#define _kern_write					_kernbuild_write
#define _kern_writev				_kernbuild_writev
#define _kern_read_dir				_kernbuild_read_dir
#define _kern_rewind_dir			_kernbuild_rewind_dir
#define _kern_read_stat				_kernbuild_read_stat
#define _kern_write_stat			_kernbuild_write_stat
#define _kern_close					_kernbuild_close
#define _kern_dup					_kernbuild_dup
#define _kern_lock_node				_kernbuild_lock_node
#define _kern_unlock_node			_kernbuild_unlock_node


struct stat;
struct dirent;

extern status_t		_kern_entry_ref_to_path(dev_t device, ino_t inode,
						const char *leaf, char *userPath, size_t pathLength);
extern int			_kern_open_entry_ref(dev_t device, ino_t inode,
						const char *name, int openMode, int perms);
extern int			_kern_open(int fd, const char *path, int openMode,
						int perms);
extern int			_kern_open_dir_entry_ref(dev_t device, ino_t inode,
						const char *name);
extern int			_kern_open_dir(int fd, const char *path);
extern int			_kern_open_parent_dir(int fd, char *name,
						size_t nameLength);
extern status_t		_kern_fsync(int fd);
extern off_t		_kern_seek(int fd, off_t pos, int seekType);
extern status_t		_kern_create_dir_entry_ref(dev_t device, ino_t inode,
						const char *name, int perms);
extern status_t		_kern_create_dir(int fd, const char *path, int perms);
extern status_t		_kern_read_link(int fd, const char *path, char *buffer,
						size_t *_bufferSize);
extern status_t		_kern_create_symlink(int fd, const char *path,
						const char *toPath, int mode);
extern status_t		_kern_unlink(int fd, const char *path);
extern status_t		_kern_rename(int oldDir, const char *oldpath, int newDir,
						const char *newpath);
extern int			_kern_open_attr_dir(int fd, const char *path);
extern status_t		_kern_remove_attr(int fd, const char *name);
extern status_t		_kern_rename_attr(int fromFile, const char *fromName,
						int toFile, const char *toName);

// file descriptor functions
extern ssize_t		_kern_read(int fd, off_t pos, void *buffer,
						size_t bufferSize);
extern ssize_t		_kern_readv(int fd, off_t pos, const struct iovec *vecs,
						size_t count);
extern ssize_t		_kern_write(int fd, off_t pos, const void *buffer,
						size_t bufferSize);
extern ssize_t		_kern_writev(int fd, off_t pos, const struct iovec *vecs,
						size_t count);
extern ssize_t		_kern_read_dir(int fd, struct dirent *buffer,
						size_t bufferSize, uint32 maxCount);
extern status_t		_kern_rewind_dir(int fd);
extern status_t		_kern_read_stat(int fd, const char *path,
						bool traverseLink, struct stat *stat, size_t statSize);
extern status_t		_kern_write_stat(int fd, const char *path,
						bool traverseLink, const struct stat *stat,
						size_t statSize, int statMask);
extern status_t		_kern_close(int fd);
extern int			_kern_dup(int fd);
extern status_t		_kern_lock_node(int fd);
extern status_t		_kern_unlock_node(int fd);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_SYSCALLS_H */
