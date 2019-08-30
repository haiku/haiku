/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "apple.h"

#include <ddm_modules.h>
#include <disk_device_types.h>
#include <KernelExport.h>
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
#endif
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>


#define TRACE_APPLE 0
#if TRACE_APPLE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define APPLE_PARTITION_MODULE_NAME "partitioning_systems/apple/v1"

static const char *kApplePartitionTypes[] = {
	"partition_map",	// the partition map itself
	"Driver",			// contains a device driver
	"Driver43",			// the SCSI 4.3 manager
	"MFS",				// Macintosh File System
	"HFS",				// Hierarchical File System (HFS/HFS+)
	"Unix_SVR2",		// UFS
	"PRODOS",
	"Free",				// unused partition
	"Scratch",			// empty partition
	"Driver_ATA",		// the device driver for an ATA device
	"Driver_ATAPI",		// the device driver for an ATAPI device
	"Driver43_CD",		// an SCSI CD-ROM driver suitable for booting
	"FWDriver",			// a FireWire driver for the device
	"Void",				// dummy partition map entry (used to align entries for CD-ROM)
	"Patches",
	NULL
};
#if 0
static const char *kOtherPartitionTypes[] = {
	"Be_BFS",			// Be's BFS (not specified endian)
};
#endif

static status_t
get_next_partition(int fd, apple_driver_descriptor &descriptor, uint32 &cookie,
	apple_partition_map &partition)
{
	uint32 block = cookie;

	// find first partition map if this is the first call,
	// or else, just load the next block
	do {
		ssize_t bytesRead = read_pos(fd, (off_t)block * descriptor.BlockSize(),
					(void *)&partition, sizeof(apple_partition_map));
		if (bytesRead < (ssize_t)sizeof(apple_partition_map))
			return B_ERROR;

		block++;
	} while (cookie == 0 && block < 64 && !partition.HasValidSignature());

	if (!partition.HasValidSignature()) {
		if (cookie)
			return B_ENTRY_NOT_FOUND;

		// we searched for the first partition map entry and failed
		return B_ERROR;
	}

	// the first partition map entry must be of type Apple_partition_map
	if (!cookie && (strncmp(partition.type, "Apple_", 6)
		|| strcmp(partition.type + 6, kApplePartitionTypes[0])))
		return B_ERROR;

	// ToDo: warn about unknown types?

	cookie = block;
	return B_OK;
}


//	#pragma mark -
//	Apple public module interface


static status_t
apple_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


static float
apple_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	struct apple_driver_descriptor *descriptor;
	uint8 buffer[512];

	if (read_pos(fd, 0, buffer, sizeof(buffer)) < (ssize_t)sizeof(buffer))
		return B_ERROR;

	descriptor = (apple_driver_descriptor *)buffer;

	TRACE(("apple: read first chunk (signature = %x)\n", descriptor->signature));

	if (!descriptor->HasValidSignature())
		return B_ERROR;

	TRACE(("apple: valid partition descriptor!\n"));

	// ToDo: Should probably call get_next_partition() once to know if there
	//		are any partitions on this disk

	// copy the relevant part of the first block
	descriptor = new apple_driver_descriptor();
	memcpy(descriptor, buffer, sizeof(apple_driver_descriptor));

	*_cookie = (void *)descriptor;

	// ToDo: reevaluate the priority with ISO-9660 and others in mind
	//		(for CD-ROM only, as far as I can tell)
	return 0.5f;
}


static status_t
apple_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	TRACE(("apple_scan_partition(cookie = %p)\n", _cookie));

	apple_driver_descriptor &descriptor = *(apple_driver_descriptor *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY;
	partition->content_size = descriptor.BlockSize() * descriptor.BlockCount();

	// scan all children

	apple_partition_map partitionMap;
	uint32 index = 0, cookie = 0;
	status_t status;

	while ((status = get_next_partition(fd, descriptor, cookie, partitionMap)) == B_OK) {
		TRACE(("apple: found partition: name = \"%s\", type = \"%s\"\n",
			partitionMap.name, partitionMap.type));

		if (partitionMap.Start(descriptor) + partitionMap.Size(descriptor) > (uint64)partition->size) {
			TRACE(("apple: child partition exceeds existing space (%Ld bytes)\n",
				partitionMap.Size(descriptor)));
			continue;
		}

		partition_data *child = create_child_partition(partition->id, index++,
			partition->offset + partitionMap.Start(descriptor),
			partitionMap.Size(descriptor), -1);
		if (child == NULL) {
			TRACE(("apple: Creating child at index %ld failed\n", index - 1));
			return B_ERROR;
		}

		child->block_size = partition->block_size;
	}

	if (status == B_ENTRY_NOT_FOUND)
		return B_OK;

	return status;
}


static void
apple_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	delete (apple_driver_descriptor *)_cookie;
}


#ifndef _BOOT_MODE
static partition_module_info sApplePartitionModule = {
#else
partition_module_info gApplePartitionModule = {
#endif
	{
		APPLE_PARTITION_MODULE_NAME,
		0,
		apple_std_ops
	},
	"apple",							// short_name
	APPLE_PARTITION_NAME,				// pretty_name
	0,									// flags

	// scanning
	apple_identify_partition,			// identify_partition
	apple_scan_partition,				// scan_partition
	apple_free_identify_partition_cookie,	// free_identify_partition_cookie
	NULL,
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sApplePartitionModule,
	NULL
};
#endif
