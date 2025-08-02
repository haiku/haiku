/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef DOSFS_H_
#define DOSFS_H_


#ifdef FS_SHELL
#include "fssh_defines.h"
#else
#include <SupportDefs.h>
#endif


#define BUF_CACHE_SIZE 20

// notify every second if the file size has changed
#define INODE_NOTIFICATION_INTERVAL 1000000LL

// the value given to the VFS as the preferred length of a read and write
#define FAT_IO_SIZE 65536

#define read16(buffer, off) B_LENDIAN_TO_HOST_INT16(*(uint16*)&buffer[off])

#define read32(buffer, off) B_LENDIAN_TO_HOST_INT32(*(uint32*)&buffer[off])

// true only for the root directory in a FAT12/FAT16 volume
#define IS_FIXED_ROOT(fatNode) ((fatNode)->de_StartCluster == 0)

#define SECTORS_PER_CLUSTER(fat_volume) \
	((fat_volume->pm_bpcluster) / (fat_volume->pm_BlkPerSec * DEV_BSIZE))
#define BLOCKS_PER_CLUSTER(fatVolume) (fatVolume->pm_bpcluster / DEV_BSIZE)

// convert a block number from DEV_BSIZE units to volume-specific sector units
#define BLOCK_TO_SECTOR(fat_volume, block) ((block * DEV_BSIZE) / fat_volume->pm_BytesPerSec)

#define vIS_DATA_CLUSTER(fatVolume, cluster) \
	(((cluster) >= 2) && ((uint32)(cluster) <= fatVolume->pm_maxcluster))
#define IS_DATA_CLUSTER(cluster) vIS_DATA_CLUSTER(fatVolume, cluster)

#define MOUNTED_READ_ONLY(fatVolume) \
	((fatVolume->pm_flags & MSDOSFSMNT_RONLY) != 0 \
		|| (fatVolume->pm_mountp->mnt_flag & MNT_RDONLY) != 0)
// these 2 flags are redundant (a side effect of simulating pieces of the FreeBSD VFS);
// both are checked to err on the side of caution

/* Unfortunately, ino_t's are defined as signed. This causes problems with
 * programs (notably cp) that use the modulo of a ino_t as a
 * hash function to index an array. This means the high bit of every ino_t
 * is off-limits. Luckily, FAT32 is actually FAT28, so dosfs can make do with
 * only 63 bits.
 */
#define INVALID_VNID_BITS_MASK (0x9LL << 60)
// the lower limit of the range of artificial inode numbers (the numbers used
// when the location-based inode is already taken by a renamed or deleted file)
#define ARTIFICIAL_VNID_BITS (0x6LL << 60)
#define DIR_INDEX_VNID_BITS 0

#define IS_DIR_INDEX_VNID(vnid) (((vnid) & ARTIFICIAL_VNID_BITS) == DIR_INDEX_VNID_BITS)
#define IS_ARTIFICIAL_VNID(vnid) (((vnid) & ARTIFICIAL_VNID_BITS) == ARTIFICIAL_VNID_BITS)
#define IS_INVALID_VNID(vnid) \
	((!IS_DIR_INDEX_VNID(vnid) && !IS_ARTIFICIAL_VNID(vnid)) || ((vnid) & INVALID_VNID_BITS_MASK))

// FreeBSD macros, modified to be compatible with Haiku's struct dirent.
// GENERIC_DIRSIZ returns an appropriate value for dirent::d_reclen.
#define _GENERIC_DIRLEN(namlen) ((offsetof(struct dirent, d_name) + (namlen) + 1 + 7) & ~7)
#define GENERIC_DIRSIZ(dp) _GENERIC_DIRLEN(strlen((dp)->d_name))

struct mount;
struct vnode;
struct fs_vnode_ops;
struct fs_volume_ops;


enum FatType { fat12, fat16, fat32 };


extern const char* LABEL_ILLEGAL;
extern struct fs_vnode_ops gFATVnodeOps;
extern struct fs_volume_ops gFATVolumeOps;


#ifdef __cplusplus
extern "C"
{
#endif
status_t _dosfs_access(const struct mount* bsdVolume, const struct vnode* bsdNode,
	const int mode);
status_t assign_inode(struct mount* volume, ino_t* inode);
bool node_exists(struct mount* bsdVolume, uint64_t inode);
status_t discard_clusters(struct vnode* bsdNode, off_t newLength);
#ifdef __cplusplus
}
#endif


#endif // DOSFS_H_
