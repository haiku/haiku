/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "atari.h"

#include <ByteOrder.h>
#include <KernelExport.h>
#include <ddm_modules.h>
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
#endif
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define SECTSZ 512

//#define TRACE_ATARI_PARTITION
#ifdef TRACE_ATARI_PARTITION
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define ATARI_PARTITION_MODULE_NAME "partitioning_systems/atari/v1"
#define ATARI_PARTITION_NAME "Atari Partition Map"


#if 0
template<typename Type> bool
validate_check_sum(Type *type)
{
	if (type->SummedLongs() != sizeof(*type) / sizeof(uint32))
		return false;

	// check checksum
	uint32 *longs = (uint32 *)type;
	uint32 sum = 0;
	for (uint32 i = 0; i < type->SummedLongs(); i++)
		sum += B_BENDIAN_TO_HOST_INT32(longs[i]);

#ifdef TRACE_ATARI_PARTITION
	if (sum != 0)
		TRACE(("search_rdb: check sum is incorrect!\n"));
#endif

	return sum == 0;
}
#endif


//	#pragma mark -
//	Atari Root Block public module interface


static status_t
atari_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


static float
atari_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	uint8 buffer[512];
	atari_root_block *arb = (atari_root_block *)buffer;
	float weight = 0.5;
	int i;
	ssize_t bytesRead = read_pos(fd, 0, buffer, sizeof(buffer));
	if (bytesRead < (ssize_t)sizeof(buffer)) {
		TRACE(("%s: read error: %ld\n", __FUNCTION__, bytesRead));
		return B_ERROR;
	}
	if (partition->offset)
		return B_ERROR;

	if (arb->Checksum() == 0x55aa)
		weight -= 0.1; /* possible but likely a PC sector */
	if (arb->_reserved_1[0] != 0x00)
		weight -= 10;
	/* hope so */
	if (arb->MaxPartitionSize() < 10)
		weight -= 20;

	if ((arb->BadSectorsStart()+arb->BadSectorsCount())*(off_t)SECTSZ > partition->size)
		return B_ERROR;

	/* check each partition */
	for (i = 0; i < 4; i++) {
		struct atari_partition_entry *p = &arb->partitions[i];
		if (p->Flags() & ATARI_PART_EXISTS) {
			/* check for unknown flags */
			if (p->Flags() & ~ (ATARI_PART_EXISTS|ATARI_PART_BOOTABLE))
				weight -= 10.0;
			/* id should be readable */
			if (!isalnum(p->id[0]))
				weight -= 1.0;
			if (!isalnum(p->id[1]))
				weight -= 1.0;
			if (!isalnum(p->id[2]))
				weight -= 1.0;
			/* make sure partition doesn't overlap bad sector list */
			if ((arb->BadSectorsStart() < p->Start()) &&
				((arb->BadSectorsStart() + arb->BadSectorsCount()) > p->Start()))
				weight -= 10;
			if ((p->Start()+p->Size())*(off_t)SECTSZ > partition->size)
				return B_ERROR;
			/* should check partitions don't overlap each other... */
		} else {
			/* empty partition entry, then it must be all null */
			if (p->Flags() || p->id[0] || p->id[1] || p->id[2] ||
				p->Start() || p->Size())
				weight -= 10.0;
			else
				weight += 0.1;
		}
	}
	/* not exactly sure */
	if (arb->Checksum() != ATARI_BOOTABLE_MAGIC)
		weight -= 0.1;

	if (weight > 1.0)
		weight = 1.0;

	if (weight > 0.0) {
		// copy the root block to a new piece of memory
		arb = new atari_root_block();
		memcpy(arb, buffer, sizeof(atari_root_block));

		*_cookie = (void *)arb;
		return weight;
	}

	return B_ERROR;
}


static status_t
atari_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	TRACE(("atari_scan_partition(cookie = %p)\n", _cookie));

	atari_root_block &arb = *(atari_root_block *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY;
	partition->content_size = partition->size;

	// scan all children

	uint32 index = 0;
	status_t status = B_ENTRY_NOT_FOUND;

	for (index = 0; index < 4; index++) {
		struct atari_partition_entry *p = &arb.partitions[index];
		if (!(p->Flags() & ATARI_PART_EXISTS))
			continue;
		TRACE(("atari: file system: %.3s\n", p->id));
		if ((p->Start() + p->Size())*(uint64)SECTSZ > (uint64)partition->size) {
			TRACE(("atari: child partition exceeds existing space (%Ld bytes)\n", p->Size()*SECTSZ));
			continue;
		}
		if (!isalnum(p->id[0]))
			continue;
		if (!isalnum(p->id[1]))
			continue;
		if (!isalnum(p->id[2]))
			continue;

		partition_data *child = create_child_partition(partition->id, index,
			partition->offset + p->Start() * (uint64)SECTSZ,
			p->Size() * (uint64)SECTSZ, -1);
		if (child == NULL) {
			TRACE(("atari: Creating child at index %ld failed\n", index - 1));
			return B_ERROR;
		}
#warning M68K: use a lookup table ?
		char type[] = "??? Partition";
		memcpy(type, p->id, 3);
		child->type = strdup(type);
		child->block_size = SECTSZ;
		status = B_OK;
	}

	if (status == B_ENTRY_NOT_FOUND)
		return B_OK;

	return status;
}


static void
atari_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	delete (atari_root_block *)_cookie;
}


#ifndef _BOOT_MODE
static partition_module_info sAtariPartitionModule = {
#else
partition_module_info gAtariPartitionModule = {
#endif
	{
		ATARI_PARTITION_MODULE_NAME,
		0,
		atari_std_ops
	},
	"atari",							// short_name
	ATARI_PARTITION_NAME,				// pretty_name
	0,									// flags

	// scanning
	atari_identify_partition,		// identify_partition
	atari_scan_partition,			// scan_partition
	atari_free_identify_partition_cookie,	// free_identify_partition_cookie
	NULL,
//	atari_free_partition_cookie,			// free_partition_cookie
//	atari_free_partition_content_cookie,	// free_partition_content_cookie
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sAtariPartitionModule,
	NULL
};
#endif
