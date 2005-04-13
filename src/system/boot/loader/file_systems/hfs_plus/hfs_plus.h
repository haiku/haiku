/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HFS_PLUS_H
#define HFS_PLUS_H


#include <SupportDefs.h>


namespace HFSPlus {

struct hfs_extent_descriptor {
	uint32				start_block;
	uint32				block_count;
};

typedef hfs_extent_descriptor hfs_extent_record[8];

struct hfs_fork_data {
	uint64				logical_size;
	uint32				clump_size;
	uint32				total_blocks;
    hfs_extent_record	extents;
};


#define HFS_VOLUME_SIGNATURE 'H+'

struct hfs_volume_header {
	uint16				signature;
	uint16				version;
	uint32				attributes;
	uint32				last_mounted_version;
	uint32				journal_info_block;

	uint32				creation_date;
	uint32				modification_date;
	uint32				backup_date;
	uint32				checked_date;

	uint32				file_count;
	uint32				folder_count;

	uint32				block_size;
	uint32				total_blocks;
	uint32				free_blocks;

	uint32				next_allocation;
	uint32				resource_clump_size;
	uint32				data_clump_size;
	uint32				next_catalog_id;

	uint32				write_count;
	uint64				encodings_bitmap;

	uint32				finder_info[8];

	hfs_fork_data		allocation_file;
	hfs_fork_data		extents_file;
	hfs_fork_data		catalog_file;
	hfs_fork_data		attributes_file;
	hfs_fork_data		startup_file;
};

}	// namespace HFSPlus

#endif	/* HFS_PLUS_H */
