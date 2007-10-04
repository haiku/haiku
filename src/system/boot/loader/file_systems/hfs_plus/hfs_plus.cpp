/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "hfs_plus.h"

#include <boot/partitions.h>
#include <boot/platform.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


using namespace HFSPlus;

#if 0
status_t
HFSPlus::get_root_block(int fDevice, char *buffer, int32 blockSize, off_t partitionSize)
{
	hfs_volume_header header;
	if (read_pos(fDevice, 1024, &header, sizeof(header)) < B_OK)
		return B_ERROR;


	return B_OK;
}
#endif

//	#pragma mark -


static status_t
hfs_plus_get_file_system(boot::Partition *partition, ::Directory **_root)
{
/*	Volume *volume = new(nothrow) Volume(partition);
	if (volume == NULL)
		return B_NO_MEMORY;

	if (volume->InitCheck() < B_OK) {
		delete volume;
		return B_ERROR;
	}

	*_root = volume->Root();
*/	return B_OK;
}


file_system_module_info gAmigaFFSFileSystemModule = {
	"file_systems/hfs_plus/v1",
	kPartitionTypeHFSPlus,
	NULL,
	hfs_plus_get_file_system
};

