/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Volume.h"
#include "Directory.h"

#include <boot/partitions.h>
#include <boot/platform.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


using namespace FFS;


Volume::Volume(boot::Partition *partition)
	:
	fRoot(NULL)
{
	if ((fDevice = open_node(partition, O_RDONLY)) < B_OK)
		return;

	if (read_pos(fDevice, 0, &fType, sizeof(int32)) < B_OK)
		return;

	fType = B_BENDIAN_TO_HOST_INT32(fType);

	switch (fType) {
		case DT_AMIGA_FFS:
		case DT_AMIGA_FFS_INTL:
		case DT_AMIGA_FFS_DCACHE:
			break;

		case DT_AMIGA_OFS:
			printf("The Amiga OFS is not yet supported.\n");
			return;
		default:
			// unsupported file system
			//printf("amiga_ffs: unsupported: %08lx\n", fType);
			return;
	}

	char *buffer = (char *)malloc(4096);
	if (buffer == NULL)
		return;

	int32 blockSize = partition->block_size;
	if (get_root_block(fDevice, buffer, blockSize, partition->size) != B_OK) {
		// try to get the root block at different sizes, if the
		// block size was incorrectly passed from the partitioning
		// system
		for (int32 size = 512; size <= 4096; size <<= 1) {
			if (get_root_block(fDevice, buffer, size, partition->size) == B_OK) {
				blockSize = size;
				break;
			} else if (size >= 4096) {
				puts("Could not find root block\n");
				free(buffer);
				return;
			}
		}
	}

	char *newBuffer = (char *)realloc(buffer, blockSize);
		// if reallocation fails, we keep the old buffer
	if (newBuffer != NULL)
		buffer = newBuffer;

	fRootNode.SetTo(buffer, blockSize);
	fRoot = new(nothrow) Directory(*this, fRootNode);
		// fRoot will free the buffer for us upon destruction
}


Volume::~Volume()
{
	delete fRoot;
	close(fDevice);
}


status_t 
Volume::InitCheck()
{
	if (fRoot != NULL)
		return fRootNode.ValidateCheckSum();

	return B_ERROR;
}


//	#pragma mark -


float
amiga_ffs_identify_file_system(boot::Partition *partition)
{
	Volume volume(partition);

	return volume.InitCheck() < B_OK ? 0 : 0.8;
}


static status_t
amiga_ffs_get_file_system(boot::Partition *partition, ::Directory **_root)
{
	Volume *volume = new(nothrow) Volume(partition);
	if (volume == NULL)
		return B_NO_MEMORY;

	if (volume->InitCheck() < B_OK) {
		delete volume;
		return B_ERROR;
	}

	*_root = volume->Root();
	return B_OK;
}


file_system_module_info gAmigaFFSFileSystemModule = {
	"file_systems/amiga_ffs/v1",
	kPartitionTypeAmigaFFS,
	amiga_ffs_identify_file_system,
	amiga_ffs_get_file_system
};

