/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "fatfs.h"

#include <boot/partitions.h>
#include <boot/platform.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


using namespace FATFS;

#if 0
status_t
FATFS::get_root_block(int fDevice, char *buffer, int32 blockSize, off_t partitionSize)
{
	// calculate root block position (it depends on the block size)

	// ToDo: get the number of reserved blocks out of the disk_environment structure??
	//		(from the amiga_rdb module)
	int32 reservedBlocks = 2;
	off_t offset = (((partitionSize / blockSize) - 1 - reservedBlocks) / 2) + reservedBlocks;
		// ToDo: this calculation might be incorrect for certain cases.

	if (read_pos(fDevice, offset * blockSize, buffer, blockSize) < B_OK)
		return B_ERROR;

	RootBlock root(buffer, blockSize);
	if (root.ValidateCheckSum() < B_OK)
		return B_BAD_DATA;

	//printf("primary = %ld, secondary = %ld\n", root.PrimaryType(), root.SecondaryType());
	if (!root.IsRootBlock())
		return B_BAD_TYPE;

	return B_OK;
}

#endif

