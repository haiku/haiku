/* This file contains the prototypes for various functions copied from "lowntfs-3g.c"
 * and then modified for use in Haiku's filesystem driver. (The implementations are GPL.) */

#ifndef LOWNTFS_H
#define LOWNTFS_H

#include <StorageDefs.h>

#include "libntfs/inode.h"
#include "libntfs/security.h"


struct lowntfs_context {
	ntfs_volume* vol;
	char* abs_mnt_point;
	unsigned int dmask, fmask;
	BOOL posix_nlink;
};


u64 ntfs_fuse_inode_lookup(struct lowntfs_context *ctx, u64 parent, const char* name);

int ntfs_fuse_getstat(struct lowntfs_context *ctx, struct SECURITY_CONTEXT *scx,
	ntfs_inode *ni, struct stat *stbuf);

int ntfs_fuse_readlink(struct lowntfs_context *ctx, u64 ino, void* buffer, size_t* bufferSize);

int ntfs_fuse_read(ntfs_inode* ni, off_t offset, char* buffer, size_t size);


#endif	// LOWNTFS_H
