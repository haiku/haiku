/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the Haiku License.
*/

/* This file contains the module interface to the disk device manager.
 * Currently, only the part for identifying and scanning a volume is implemented.
 */


#include "Volume.h"

#include <disk_device_manager/ddm_modules.h>
#include <util/kernel_cpp.h>

#include <string.h>


struct identify_cookie {
	disk_super_block super_block;
};


//	Scanning


static float
bfs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	disk_super_block superBlock;
	if (Volume::Identify(fd, &superBlock) != B_OK)
		return 0.0f;

	identify_cookie *cookie = new identify_cookie;
	memcpy(&cookie->super_block, &superBlock, sizeof(disk_super_block));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
bfs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.NumBlocks() * cookie->super_block.BlockSize();
	partition->block_size = cookie->super_block.BlockSize();

	partition->content_name = strdup(cookie->super_block.name);

	return B_OK;
}


static void
bfs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	delete cookie;
}


//	#pragma mark -


static status_t
bfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_BAD_VALUE;
}


struct fs_module_info gDiskDeviceInfo = {
	{
		"file_systems/bfs/disk_device/v1",
		0,
		bfs_std_ops,
	},

	"Be File System",
	0,	/* flags ? */

	// scanning
	bfs_identify_partition,
	bfs_scan_partition,
	bfs_free_identify_partition_cookie,
	NULL,

	// querying
	// shadow partition modification
	// writing
	NULL
};

