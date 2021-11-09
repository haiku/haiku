/* This file contains the prototypes for various functions copied from "lowntfs-3g.c"
 * and then modified for use in Haiku's filesystem driver. (The implementations are GPL.) */

#ifndef LOWNTFS_H
#define LOWNTFS_H

#include <StorageDefs.h>

#include "libntfs/inode.h"
#include "libntfs/security.h"


struct lowntfs_context {
	void* haiku_fs_volume;
	void* current_close_state_vnode;

	ntfs_volume* vol;
	char* abs_mnt_point;
	unsigned int dmask, fmask;
	s64 dmtime;
	ntfs_volume_special_files special_files;
	BOOL posix_nlink, inherit, windows_names;

	u64 latest_ghost;
};

int* ntfs_haiku_get_close_state(struct lowntfs_context *ctx, u64 ino);
void ntfs_haiku_put_close_state(struct lowntfs_context *ctx, u64 ino, u64 ghost);

enum {
	CLOSE_GHOST = 1,
	CLOSE_COMPRESSED = 2,
	CLOSE_ENCRYPTED = 4,
	CLOSE_DMTIME = 8,
	CLOSE_REPARSE = 16
};

u64 ntfs_fuse_inode_lookup(struct lowntfs_context *ctx, u64 parent, const char* name);

int ntfs_fuse_getstat(struct lowntfs_context *ctx, struct SECURITY_CONTEXT *scx,
	ntfs_inode *ni, struct stat *stbuf);

int ntfs_fuse_readlink(struct lowntfs_context *ctx, u64 ino, void* buffer, size_t* bufferSize);

int ntfs_fuse_read(ntfs_inode* ni, off_t offset, char* buffer, size_t size);
int ntfs_fuse_write(struct lowntfs_context* ctx, ntfs_inode* ni, const char *buf,
	size_t size, off_t offset);

int ntfs_fuse_create(struct lowntfs_context *ctx, ino_t parent, const char *name,
	mode_t typemode, dev_t dev, const char *target, ino_t* ino);

enum RM_TYPES {
	RM_LINK = 0,
	RM_DIR,
	RM_ANY,

	RM_NO_CHECK_OPEN = 1 << 8
		// Haiku addition, so that ntfs_fuse_rm skips doing vnode lookups.
};

int ntfs_fuse_rm(struct lowntfs_context *ctx, ino_t parent, const char *name,
	enum RM_TYPES rm_type);

int ntfs_fuse_release(struct lowntfs_context *ctx, ino_t parent, ino_t ino, int state, u64 ghost);

int ntfs_fuse_rename(struct lowntfs_context *ctx, ino_t parent, const char *name,
	ino_t newparent, const char *newname);

#endif	// LOWNTFS_H
