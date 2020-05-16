/*
 * Copyright (C) 2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */


#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
#endif

#include <ByteOrder.h>
#include <ddm_modules.h>
#include <util/kernel_cpp.h>
#include <KernelExport.h>
#include <stdint.h>
#include <string.h>


//#define TRACE_SUN_PARTITION
#ifdef TRACE_SUN_PARTITION
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define SUN_PARTITION_MODULE_NAME "partitioning_systems/sun/v1"
#define SUN_PARTITION_NAME "Sun volume table of contents"


static uint16_t kMainSignature = 0xDABE;
static uint32_t kVtocSignature = 0x600DDEEE;
static uint32_t kVtocVersion = 1;
static off_t kSectorSize = 512;


struct sun_vtoc {
	char diskName[128];
	struct {
		uint32_t version;
		char volumeName[8];
		uint16_t partitionCount;
		struct {
			uint16_t type;
			uint16_t flags;
		} partitions[8];
		uint8_t bootinfo[12];
		uint8_t reserved[2];
		uint32_t signature;
		uint8_t reserved2[38];
		uint32_t timestamp[8];
	} __attribute__((packed)) vtoc;
	uint16_t write_skip;
	uint16_t read_skip;
	uint8_t reserved[154];
	uint16_t diskSpeed;
	uint16_t cylinders;
	uint16_t alternates;
	uint8_t reserved2[4];
	uint16_t interleave;
	uint16_t dataCylinders;
	uint16_t alternateCylinders;
	uint16_t heads;
	uint16_t sectorsPerTrack;
	uint8_t reserved3[4];
	struct {
		uint32_t startCylinder;
		uint32_t sectorCount;
	} partitions[8];
	uint16_t signature;
	uint16_t checksum;
} __attribute__((packed));


//	#pragma mark -
//	Sun VTOC public module interface


static status_t
sun_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


static float
sun_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	uint8 buffer[512];
	ssize_t bytesRead = read_pos(fd, 0, buffer, sizeof(buffer));
	sun_vtoc *vtoc = (sun_vtoc *)buffer;
	if (bytesRead < (ssize_t)sizeof(buffer)) {
		TRACE(("%s: read error: %ld\n", __FUNCTION__, bytesRead));
		return B_ERROR;
	}

	if (vtoc->signature == B_HOST_TO_BENDIAN_INT16(kMainSignature)
		&& vtoc->vtoc.signature == B_HOST_TO_BENDIAN_INT32(kVtocSignature)
		&& vtoc->vtoc.version == B_HOST_TO_BENDIAN_INT32(kVtocVersion)) {
		// TODO verify the checksum

		uint16_t partitionCount
			= B_BENDIAN_TO_HOST_INT16(vtoc->vtoc.partitionCount);
		TRACE(("Found %d partitions\n", partitionCount));

		if (partitionCount > 8)
			return B_ERROR;

		vtoc = new sun_vtoc();
		memcpy(vtoc, buffer, sizeof(sun_vtoc));
		*_cookie = (void *)vtoc;

		bool hasParent = (get_parent_partition(partition->id) != NULL);
		if (!hasParent)
			return 0.61; // Larger than iso9660
		else
			return 0.3;
	}

	return B_ERROR;
}


static status_t
sun_scan_partition(int fd, partition_data *partition, void *cookie)
{
	sun_vtoc *vtoc = (sun_vtoc *)cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY;
	partition->content_size = partition->size;

	off_t headCount = B_BENDIAN_TO_HOST_INT16(vtoc->heads);
	off_t sectorsPerTrack = B_BENDIAN_TO_HOST_INT16(vtoc->sectorsPerTrack);
	off_t cylinderSize = kSectorSize * headCount * sectorsPerTrack;

	TRACE(("%" B_PRIdOFF " heads, %" B_PRIdOFF " sectors, cylindersize %"
		B_PRIdOFF "\n", headCount, sectorsPerTrack, cylinderSize));

	for (int i = 0; i < B_BENDIAN_TO_HOST_INT16(vtoc->vtoc.partitionCount);
		i++) {
		uint16_t type = B_BENDIAN_TO_HOST_INT16(vtoc->vtoc.partitions[i].type);
		// Ignore unused and "whole disk" entries
		if (type == 0 || type == 5)
			continue;

		off_t start = B_BENDIAN_TO_HOST_INT32(
			vtoc->partitions[i].startCylinder) * cylinderSize;
		off_t size = B_BENDIAN_TO_HOST_INT32(vtoc->partitions[i].sectorCount)
			* kSectorSize;
		TRACE(("Part %d type %d start %" B_PRIdOFF " size %" B_PRIdOFF "\n", i,
			type, start, size));
		partition_data *child = create_child_partition(partition->id, i,
			start, size, -1);
		if (child == NULL) {
			TRACE(("sun: Creating child at index %d failed\n", i));
			return B_ERROR;
		}
		child->block_size = partition->block_size;
	}

	return B_OK;
}


static void
sun_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	delete (sun_vtoc *)_cookie;
}


#ifndef _BOOT_MODE
static partition_module_info sSunPartitionModule = {
#else
partition_module_info gSunPartitionModule = {
#endif
	{
		SUN_PARTITION_MODULE_NAME,
		0,
		sun_std_ops
	},
	"sun",							// short_name
	SUN_PARTITION_NAME,				// pretty_name
	0,									// flags

	// scanning
	sun_identify_partition,		// identify_partition
	sun_scan_partition,			// scan_partition
	sun_free_identify_partition_cookie,	// free_identify_partition_cookie
	NULL,
//	atari_free_partition_content_cookie,	// free_partition_content_cookie
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sSunPartitionModule,
	NULL
};
#endif
