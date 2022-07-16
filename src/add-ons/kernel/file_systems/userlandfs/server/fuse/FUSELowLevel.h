/*
 * Copyright 2022, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#ifndef FUSELOWLEVEL_H
#define FUSELOWLEVEL_H

#include <semaphore.h>
#include <stdlib.h>

#include "fuse_api.h"


typedef	int	(*ReadDirBufferFiller) (void* buffer, char* buf, size_t bufsize, const char* name,
	const struct stat* st, off_t offset);

void fuse_ll_init(const fuse_lowlevel_ops* ops, void* userdata, struct fuse_conn_info* conn);
void fuse_ll_destroy(const fuse_lowlevel_ops* ops, void *userdata);
int fuse_ll_lookup(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	struct stat* st);
int fuse_ll_getattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct stat* st);
int fuse_ll_setattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, const struct stat *attr,
	int to_set);
int fuse_ll_readlink(const fuse_lowlevel_ops* ops, fuse_ino_t ino, char* buffer, size_t size);
int fuse_ll_mkdir(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	mode_t mode);
int fuse_ll_unlink(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name);
int fuse_ll_rmdir(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name);
int fuse_ll_symlink(const fuse_lowlevel_ops* ops, const char* link, fuse_ino_t parent,
	const char* name);
int fuse_ll_rename(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	fuse_ino_t newparent, const char *newname);
int fuse_ll_link(const fuse_lowlevel_ops* ops, fuse_ino_t ino, fuse_ino_t newparent,
	const char *newname);
int fuse_ll_open(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct fuse_file_info *fi);
int fuse_ll_read(const fuse_lowlevel_ops* ops, fuse_ino_t ino, char* buffer, size_t bufferSize,
	off_t position, fuse_file_info* ffi);
int fuse_ll_write(const fuse_lowlevel_ops* ops, fuse_ino_t ino, const char *buf,
	size_t size, off_t off, struct fuse_file_info *fi);
int fuse_ll_flush(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct fuse_file_info *fi);
int fuse_ll_release(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct fuse_file_info *fi);
int fuse_ll_fsync(const fuse_lowlevel_ops* ops, fuse_ino_t ino, int datasync,
	struct fuse_file_info *fi);
int fuse_ll_opendir(const fuse_lowlevel_ops* ops, fuse_ino_t inode, struct fuse_file_info* ffi);
int fuse_ll_readdir(const fuse_lowlevel_ops* ops, fuse_ino_t ino, void* cookie,
	char* buffer, size_t bufferSize, ReadDirBufferFiller filler, off_t pos, fuse_file_info* ffi);
int fuse_ll_releasedir(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct fuse_file_info *fi);
int fuse_ll_statfs(const fuse_lowlevel_ops* ops, fuse_ino_t inode, struct statvfs* stat);
int fuse_ll_getxattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, const char *name,
	char* buffer, size_t size);
int fuse_ll_listxattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, char* buffer, size_t size);
int fuse_ll_access(const fuse_lowlevel_ops* ops, fuse_ino_t ino, int mask);
int fuse_ll_create(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	mode_t mode, struct fuse_file_info *fi, fuse_ino_t& ino);



#endif /* !FUSELOWLEVEL_H */
