/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/partitions.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <ddm_modules.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>


static partition_module_info *sPartitionModules[] = {
#ifdef BOOT_SUPPORT_PARTITION_AMIGA
	&gAmigaPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_INTEL
	&gIntelPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_APPLE
	&gApplePartitionModule,
#endif
};
static const int32 sNumPartitionModules = sizeof(sPartitionModules) / sizeof(partition_module_info *);

extern list gPartitions;


Partition::Partition(int fd)
{
	// it's safe to close the file
	fFD = dup(fd);
}


Partition::~Partition()
{
	close(fFD);
}


ssize_t 
Partition::ReadAt(void *cookie, off_t position, void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		position = 0;

	if (position + bufferSize > this->size)
		bufferSize = this->size - position;

	return read_pos(fFD, this->offset + position, buffer, bufferSize);
}


ssize_t 
Partition::WriteAt(void *cookie, off_t position, const void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		position = 0;

	if (position + bufferSize > this->size)
		bufferSize = this->size - position;

	return write_pos(fFD, this->offset + position, buffer, bufferSize);
}


//	#pragma mark -


status_t
add_partitions_for(int fd)
{
	struct partition_data partition;
	memset(&partition, 0, sizeof(partition_data));

	// set some magic/default values
	partition.id = (partition_id)&partition;
	partition.block_size = 512;
	partition.size = 1024*1024*1024; // ToDo: fix this!
	partition.volume = (dev_t)fd;

	for (int32 i = 0; i < sNumPartitionModules; i++) {
		partition_module_info *module = sPartitionModules[i];
		void *cookie;

		puts(module->pretty_name);
		if (module->identify_partition(fd, &partition, &cookie) <= 0.0)
			continue;

		status_t status = module->scan_partition(fd, &partition, cookie);
		module->free_identify_partition_cookie(&partition, cookie);

		if (status == B_OK)
			return B_OK;
	}
	return B_ENTRY_NOT_FOUND; 
}


partition_data *
create_child_partition(partition_id id, int32 index, partition_id childID)
{
	partition_data &partition = *(partition_data *)id;
	Partition *child = new Partition(partition.volume);
	if (child == NULL) {
		printf("creating partition failed: no memory\n");
		return NULL;
	}

	list_add_item(&gPartitions, (void *)child);

	return child;
}


