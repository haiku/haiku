/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _ZFS_FS_H
#define _ZFS_FS_H

#include <types.h>

typedef int64 inode_num;
typedef int64 block_num;

#define ZFS_SB_MAGIC1 0x23afb490
#define ZFS_SB_MAGIC2 0x343287f4

#define ZFS_SB_OFFSET 1024
#define ZFS_BLOCKSIZE 4096

#define ZFS_INODE_TABLE_INODE 0
#define ZFS_BOOT_INODE 1
#define ZFS_BITMAP_INODE 2
#define ZFS_ROOT_DIR_INODE 3

#define ZFS_RESERVED_INODES 32

#define ZFS_HOST_ENDIAN 0x12345678
#define ZFS_SWAPPED_ENDIAN 0x78563412

#define ZFS_CURRENT_VERSION 1

typedef struct zfs_superblock {
	int32 magic1;
	int32 version;
	int32 endian;
	int32 blocksize;

	int64 num_blocks;
	int64 used_blocks;

	block_num boot_code_start;
	block_num inode_table_start;

	char  name[32];

	int32 magic2;
} zfs_superblock;

#define ZFS_INODE_SIZE ZFS_BLOCKSIZE

#define ZFS_INODE_MAGIC 0x444f4e49 // 'INOD'

#define ZFS_INODE_FLAG_INUSE 0x1
#define ZFS_INODE_FLAG_PRIMARY 0x2

typedef struct zfs_inode_container {
	int32 magic;
	int16 flags;
	int16 num_attributes;
	inode_num num;
/* 0x10 */
	inode_num next_spillover_inode;
	inode_num primary_inode;
/* 0x20 */
} zfs_inode_container;

typedef struct zfs_attribute_header {
	uint32 type;
	uint32 len;
	uint8  non_resident;
	uint8  name_len;
	uint16 value_offset;
	uint32 filler;
/* 0x10 */
} zfs_attribute_header;

typedef struct zfs_attribute_resident {
	uint32 len;
	uint16 offset;
	uint16 filler;
/* 0x08 */
} zfs_attribute_resident;

typedef struct zfs_attribute_nonresident {
	block_num starting_fileblock;
	block_num ending_fileblock;
/* 0x10 */
	uint16 runlist_offset;
	uint16 num_runs;
	uint32 filler;
	uint64 len;
/* 0x20 */
} zfs_attribute_nonresident;

typedef struct zfs_run {
	block_num start;
	int64 len;
} zfs_run;

#define ZFS_ATTR_STD_INFO 0x10
#define ZFS_ATTR_DIR      0x20
#define ZFS_ATTR_DATA     0x30
#define ZFS_ATTR_BITMAP   0x40

typedef struct zfs_attr_std_info {
	uint64 create_time;
	uint64 last_mod_time;
	uint64 last_access_time;
} zfs_attr_std_info;

typedef struct zfs_dir_ent {
	inode_num inum;
	uint16 len;
	uint16 name_len;
	char name[0];
} zfs_dir_ent;

#endif
