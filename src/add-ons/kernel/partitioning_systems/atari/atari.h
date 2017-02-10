/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef ATARI_PARTITION_H
#define ATARI_PARTITION_H

/*
 * An extented partition scheme exists, (several actually), cf.
 * http://people.debian.org/~smarenka/d-i/atari-fdisk-README
 * boot sector stuff:
 * http://leonard.oxg.free.fr/articles/multi_atari/multi_atari.html
 */

#include "SupportDefs.h"
#include "ByteOrder.h"

/* this one is BIG endian */
struct atari_partition_entry {
#define ATARI_PART_EXISTS	0x01
#define ATARI_PART_BOOTABLE	0x80
	uint8	flags;
	/* some known types */
#define ATARI_PART_TYPE_GEM 'G', 'E', 'M'
#define ATARI_PART_TYPE_LNX 'L', 'N', 'X'
#define ATARI_PART_TYPE_OS9 'O', 'S', '9'
	/* the one we'll use */
#define ATARI_PART_TYPE_BFS 'B', 'F', 'S'
	char	id[3];
	uint32	start;
	uint32	size;

	uint8 Flags() const { return flags; }
	uint32 Start() const { return B_BENDIAN_TO_HOST_INT32(start); }
	uint32 Size() const { return B_BENDIAN_TO_HOST_INT32(size); }
} _PACKED ;


/* this one is BIG endian */
struct atari_root_block {
	uint8	bootcode[0x1b6];
	uint16	cylinder_count;
	uint8	head_count;
	uint8	_reserved_1[1];
	uint16	reduced_write_current_cylinder;
	uint16	write_precomp_cynlinder;
	uint8	landing_zone;
	uint8	seek_rate_code;
	uint8	interleave_factor;
	uint8	sectors_per_track;
	uint32	maximum_partition_size; /* in sectors (in clusters) */
	struct atari_partition_entry	partitions[4];
	uint32	bad_sector_list_start;
	uint32	bad_sector_list_count;
#define ATARI_BOOTABLE_MAGIC	0x1234
	uint16	checksum; /* checksum? 0x1234 for bootable */

	uint32 MaxPartitionSize() const { return B_BENDIAN_TO_HOST_INT16(maximum_partition_size); }
	uint32 BadSectorsStart() const { return B_BENDIAN_TO_HOST_INT32(bad_sector_list_start); }
	uint32 BadSectorsCount() const { return B_BENDIAN_TO_HOST_INT32(bad_sector_list_count); }
	uint16 Checksum() const { return B_BENDIAN_TO_HOST_INT16(checksum); }
} _PACKED ;

/************* bad blocks *************/

struct bad_block_entry {
};

struct bad_block_block {
};

/************* partition block *************/

/* this one is BIG endian, but with LITTLE endian fields! marked LE */
/* this is part of the filesystem... */
struct atari_boot_block {
	uint16	branch; /* branch code */
	char	volume_id[6];
	uint8	disk_id[3];
	uint16	bytes_per_sector;	/* LE */
	uint8	sectors_per_cluster;
	uint16	reserved_sectors;	/* LE */
	uint8	fat_count;
	uint16	entries_in_root_dir;	/* LE */
	uint16	sector_count;		/* LE */
	uint8	media_descriptor;
	uint16	sectors_per_fat;	/* LE */
	uint16	sectors_per_track;	/* LE */
	uint16	side_count;		/* LE */
	uint16	hidden_sector_count;	/* LE */
	/* boot code... */
	uint16	execflag;
	uint16	loadmode;
	uint16	first_sector;
	uint16	num_sectors;
	uint32	load_addr;
	uint32	fat_buffer;
#define ATARI_BB_FILENAME_SZ (0x003a - 0x002e)
	char	filename[ATARI_BB_FILENAME_SZ];
#define ATARI_BB_BOOTCODE_SZ (0x01fe - 0x003a)
	uint8	bootcode[ATARI_BB_BOOTCODE_SZ];
	uint16	checksum;	/* 0x1234 if bootable */
};



#endif	/* ATARI_PARTITION_H */

