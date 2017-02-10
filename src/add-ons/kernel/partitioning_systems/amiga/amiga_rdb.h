/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef AMIGA_RDB_H
#define AMIGA_RDB_H


#include "SupportDefs.h"
#include "ByteOrder.h"


struct rigid_disk_block {
	uint32	id;
	uint32	summed_longs;
	int32	check_sum;
	uint32	host_id;
	uint32	block_size;
	uint32	flags;

	/* block list heads */
	
	uint32	bad_block_list;
	uint32	partition_list;
	uint32	fs_header_list;
	uint32	drive_init;

	uint32	__reserved1[6];

	/* physical drive characteristics */
	
	uint32	cylinders;
	uint32	sectors;
	uint32	heads;
	uint32	interleave;
	uint32	park;
	uint32	__reserved2[3];
	uint32	write_precompensation;
	uint32	reduced_write;
	uint32	step_rate;
	uint32	__reserved3[5];

	/* logical drive characteristics */
	
	uint32	rdb_blocks_low;
	uint32	rdb_blocks_high;
	uint32	low_cylinder;
	uint32	high_cylinder;
	uint32	cylinder_blocks;
	uint32	auto_park_seconds;

	uint32	__reserved4[2];

	/* drive identification */
	
	char	disk_vendor[8];
	char	disk_product[16];
	char	disk_revision[4];
	char	controller_vendor[8];
	char	controller_product[16];
	char	controller_revision[4];

	uint32	__reserved5[10];

	uint32 ID() const { return B_BENDIAN_TO_HOST_INT32(id); }
	uint32 SummedLongs() const { return B_BENDIAN_TO_HOST_INT32(summed_longs); }
	uint32 BlockSize() const { return B_BENDIAN_TO_HOST_INT32(block_size); }
	uint32 FirstPartition() const { return B_BENDIAN_TO_HOST_INT32(partition_list); }
};

#define RDB_DISK_ID			'RDSK'

#define RDB_LOCATION_LIMIT	16

enum rdb_flags {
	RDB_LAST				= 0x01,
	RDB_LAST_LUN			= 0x02,
	RDB_LAST_TID			= 0x04,
	RDB_NO_RESELECT			= 0x08,
	RDB_HAS_DISK_ID			= 0x10,
	RDB_HAS_CONTROLLER_ID	= 0x20,
	RDB_SUPPORTS_SYNCHRONOUS = 0x40,
};


/************* bad blocks *************/

struct bad_block_entry {
};

struct bad_block_block {
};

#define RDB_BAD_BLOCK_ID	'BADB'


/************* partition block *************/

struct partition_block {
	uint32	id;
	uint32	summed_longs;
	int32	check_sum;
	uint32	host_id;
	uint32	next;
	uint32	flags;
	uint32	__reserved1[2];
	uint32	open_device_flags;
	uint8	drive_name[32];		// BSTR form (Pascal like string)

	uint32	__reserved2[15];
	uint32	environment[17];
	uint32	__reserved3[15];

	uint32 ID() const { return B_BENDIAN_TO_HOST_INT32(id); }
	uint32 SummedLongs() const { return B_BENDIAN_TO_HOST_INT32(summed_longs); }
	uint32 Next() const { return B_BENDIAN_TO_HOST_INT32(next); }
};

#define RDB_PARTITION_ID	'PART'

enum rdb_partition_flags {
		RDB_PARTITION_BOOTABLE	= 0x01,
		RDB_PARTITION_NO_MOUNT	= 0x02,
};


/************* disk environment *************/

struct disk_environment {
	uint32	table_size;			// size of this environment
	uint32	long_block_size;	// block size in longs (128 == 512 byte)
	uint32	sec_org;			// always 0
	uint32	surfaces;
	uint32	sectors_per_block;
	uint32	blocks_per_track;
	uint32	reserved_blocks_at_start;
	uint32	reserved_blocks_at_end;
	uint32	interleave;
	uint32	first_cylinder;
	uint32	last_cylinder;
	uint32	num_buffers;
	uint32	buffer_mem_type;
	uint32	max_transfer;
	uint32	dma_mask;
	int32	boot_priority;
	uint32	dos_type;
	uint32	baud_rate;
	uint32	control;
	uint32	boot_blocks;

	uint32 FirstCylinder() const { return B_BENDIAN_TO_HOST_INT32(first_cylinder); }
	uint32 LastCylinder() const { return B_BENDIAN_TO_HOST_INT32(last_cylinder); }
	uint32 Surfaces() const { return B_BENDIAN_TO_HOST_INT32(surfaces); }
	uint32 BlocksPerTrack() const { return B_BENDIAN_TO_HOST_INT32(blocks_per_track); }
	uint32 LongBlockSize() const { return B_BENDIAN_TO_HOST_INT32(long_block_size); }
	uint32 BlockSize() const { return LongBlockSize() << 2; }

	uint64 Start() 
	{ 
		return uint64(FirstCylinder()) * BlocksPerTrack() * Surfaces() * BlockSize();
	}

	uint64 Size() 
	{ 
		return uint64(LastCylinder() + 1 - FirstCylinder()) * BlocksPerTrack() * Surfaces()
			* BlockSize();
	}
};


/************* file system header block *************/

struct fs_header_block {
};

#define RDB_FS_HEADER_ID	'FSHD'

struct load_seg_block {
};

#define RDB_LOAD_SEG_ID		'LSEG'

#endif	/* AMIGA_RDB_H */

