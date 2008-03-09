/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_SYSCALLS_H
#define _FSSH_SYSCALLS_H

#include "fssh_defs.h"
#include "fssh_os.h"


struct fssh_iovec;


namespace FSShell {


// defined in vfs.cpp
fssh_dev_t 		_kern_mount(const char *path, const char *device,
					const char *fsName, uint32_t flags, const char *args,
					fssh_size_t argsLength);
fssh_status_t	_kern_unmount(const char *path, uint32_t flags);
fssh_status_t 	_kern_read_fs_info(fssh_dev_t device,
					struct fssh_fs_info *info);
fssh_status_t	_kern_write_fs_info(fssh_dev_t device,
					const struct fssh_fs_info *info, int mask);
fssh_status_t	_kern_sync(void);
fssh_status_t	_kern_entry_ref_to_path(fssh_dev_t device, fssh_ino_t inode,
					const char *leaf, char *userPath, fssh_size_t pathLength);
fssh_dev_t		_kern_next_device(int32_t *_cookie);
int				_kern_open_entry_ref(fssh_dev_t device, fssh_ino_t inode,
					const char *name, int openMode, int perms);
int				_kern_open(int fd, const char *path, int openMode, int perms);
int				_kern_open_dir_entry_ref(fssh_dev_t device, fssh_ino_t inode,
					const char *name);
int				_kern_open_dir(int fd, const char *path);
fssh_status_t 	_kern_fcntl(int fd, int op, uint32_t argument);
fssh_status_t	_kern_fsync(int fd);
fssh_status_t	_kern_lock_node(int fd);
fssh_status_t	_kern_unlock_node(int fd);
fssh_status_t	_kern_create_dir_entry_ref(fssh_dev_t device, fssh_ino_t inode,
					const char *name, int perms);
fssh_status_t	_kern_create_dir(int fd, const char *path, int perms);
fssh_status_t	_kern_remove_dir(int fd, const char *path);
fssh_status_t	_kern_read_link(int fd, const char *path, char *buffer,
					fssh_size_t *_bufferSize);
fssh_status_t	_kern_create_symlink(int fd, const char *path,
					const char *toPath, int mode);
fssh_status_t	_kern_create_link(const char *path, const char *toPath);
fssh_status_t	_kern_unlink(int fd, const char *path);
fssh_status_t	_kern_rename(int oldFD, const char *oldPath, int newFD,
					const char *newPath);
fssh_status_t	_kern_access(const char *path, int mode);
fssh_status_t	_kern_read_stat(int fd, const char *path, bool traverseLeafLink,
					struct fssh_stat *stat, fssh_size_t statSize);
fssh_status_t	_kern_write_stat(int fd, const char *path,
					bool traverseLeafLink, const struct fssh_stat *stat,
					fssh_size_t statSize, int statMask);
int				_kern_open_attr_dir(int fd, const char *path);
int				_kern_create_attr(int fd, const char *name, uint32_t type,
					int openMode);
int				_kern_open_attr(int fd, const char *name, int openMode);
fssh_status_t	_kern_remove_attr(int fd, const char *name);
fssh_status_t	_kern_rename_attr(int fromFile, const char *fromName,
					int toFile, const char *toName);
int				_kern_open_index_dir(fssh_dev_t device);
fssh_status_t	_kern_create_index(fssh_dev_t device, const char *name,
					uint32_t type, uint32_t flags);
fssh_status_t	_kern_read_index_stat(fssh_dev_t device, const char *name,
					struct fssh_stat *stat);
fssh_status_t	_kern_remove_index(fssh_dev_t device, const char *name);
fssh_status_t	_kern_getcwd(char *buffer, fssh_size_t size);
fssh_status_t	_kern_setcwd(int fd, const char *path);

fssh_status_t	_kern_initialize_volume(const char* fileSystem,
					const char *partition, const char *name,
					const char *parameters);

extern int		_kern_open_query(fssh_dev_t device, const char* query,
					fssh_size_t queryLength, uint32_t flags, fssh_port_id port,
					int32_t token);

// defined in fd.cpp
fssh_ssize_t	_kern_read(int fd, fssh_off_t pos, void *buffer,
					fssh_size_t length);
fssh_ssize_t	_kern_readv(int fd, fssh_off_t pos, const fssh_iovec *vecs,
					fssh_size_t count);
fssh_ssize_t	_kern_write(int fd, fssh_off_t pos, const void *buffer,
					fssh_size_t length);
fssh_ssize_t	_kern_writev(int fd, fssh_off_t pos, const fssh_iovec *vecs,
					fssh_size_t count);
fssh_off_t		_kern_seek(int fd, fssh_off_t pos, int seekType);
fssh_status_t	_kern_ioctl(int fd, uint32_t op, void *buffer,
					fssh_size_t length);
fssh_ssize_t	_kern_read_dir(int fd, struct fssh_dirent *buffer,
					fssh_size_t bufferSize, uint32_t maxCount);
fssh_status_t	_kern_rewind_dir(int fd);
fssh_status_t	_kern_close(int fd);
int				_kern_dup(int fd);
int				_kern_dup2(int ofd, int nfd);


}	// namespace FSShell


#endif	// _FSSH_SYSCALLS_H
