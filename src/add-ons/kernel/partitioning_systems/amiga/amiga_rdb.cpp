/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "amiga_rdb.h"

#include <ByteOrder.h>
#include <KernelExport.h>
#include <disk_device_manager/ddm_modules.h>
#include <disk_device_types.h>
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
#endif
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>


//#define TRACE_AMIGA_RDB
#ifdef TRACE_AMIGA_RDB
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define AMIGA_PARTITION_MODULE_NAME "partitioning_systems/amiga_rdb/v1"


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

#ifdef TRACE_AMIGA_RDB
	if (sum != 0)
		TRACE(("search_rdb: check sum is incorrect!\n"));
#endif

	return sum == 0;
}


#ifdef TRACE_AMIGA_RDB
static char *
get_tupel(uint32 id)
{
	static unsigned char tupel[5];

	tupel[0] = 0xff & (id >> 24);
	tupel[1] = 0xff & (id >> 16);
	tupel[2] = 0xff & (id >> 8);
	tupel[3] = 0xff & (id);
	tupel[4] = 0;
	for (int16 i = 0;i < 4;i++) {
		if (tupel[i] < ' ' || tupel[i] > 128)
			tupel[i] = '.';
	}

	return (char *)tupel;
}
#endif


static status_t
get_next_partition(int fd, rigid_disk_block &rdb, uint32 &cookie,
	partition_block &partition)
{
	if (cookie == 0) {
		// first entry
		cookie = rdb.FirstPartition();
	} else if (cookie == 0xffffffff) {
		// last entry
		return B_ENTRY_NOT_FOUND;
	}

	ssize_t bytesRead = read_pos(fd, (off_t)cookie * rdb.BlockSize(),
		(void *)&partition, sizeof(partition_block));
	if (bytesRead < (ssize_t)sizeof(partition_block))
		return B_ERROR;

	// TODO: Should we retry with the next block if the following test fails, as
	// long as this we find partition_blocks within a reasonable range?

	if (partition.ID() != RDB_PARTITION_ID
		|| !validate_check_sum<partition_block>(&partition))
		return B_BAD_DATA;

	cookie = partition.Next();
	return B_OK;
}


static bool
search_rdb(int fd, rigid_disk_block **_rdb)
{
	for (int32 sector = 0; sector < RDB_LOCATION_LIMIT; sector++) {
		uint8 buffer[512];
		ssize_t bytesRead = read_pos(fd, sector * 512, buffer, sizeof(buffer));
		if (bytesRead < (ssize_t)sizeof(buffer)) {
			TRACE(("search_rdb: read error: %ld\n", bytesRead));
			return false;
		}

		rigid_disk_block *rdb = (rigid_disk_block *)buffer;
		if (rdb->ID() == RDB_DISK_ID
			&& validate_check_sum<rigid_disk_block>(rdb)) {
			// copy the RDB to a new piece of memory
			rdb = new rigid_disk_block();
			memcpy(rdb, buffer, sizeof(rigid_disk_block));

			*_rdb = rdb;
			return true;
		}
	}

	return false;
}


// #pragma mark - public module interface


static status_t
amiga_rdb_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


static float
amiga_rdb_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	rigid_disk_block *rdb;
	if (!search_rdb(fd, &rdb))
		return B_ERROR;

	*_cookie = (void *)rdb;
	return 0.5f;
}


static status_t
amiga_rdb_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	TRACE(("amiga_rdb_scan_partition(cookie = %p)\n", _cookie));

	rigid_disk_block &rdb = *(rigid_disk_block *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY;
	partition->content_size = partition->size;

	// scan all children

	partition_block partitionBlock;
	uint32 index = 0, cookie = 0;
	status_t status;

	while ((status = get_next_partition(fd, rdb, cookie, partitionBlock))
			== B_OK) {
		disk_environment &environment
			= *(disk_environment *)&partitionBlock.environment[0];
		TRACE(("amiga_rdb: file system: %s\n",
			get_tupel(B_BENDIAN_TO_HOST_INT32(environment.dos_type))));

		if (environment.Start() + environment.Size()
				> (uint64)partition->size) {
			TRACE(("amiga_rdb: child partition exceeds existing space (%Ld "
				"bytes)\n", environment.Size()));
			continue;
		}

		partition_data *child = create_child_partition(partition->id, index++,
			partition->offset + environment.Start(), environment.Size(), -1);
		if (child == NULL) {
			TRACE(("amiga_rdb: Creating child at index %ld failed\n",
				index - 1));
			return B_ERROR;
		}

		child->block_size = environment.BlockSize();
	}

	if (status == B_ENTRY_NOT_FOUND)
		return B_OK;

	return status;
}


static void
amiga_rdb_free_identify_partition_cookie(partition_data *partition,
	void *_cookie)
{
	delete (rigid_disk_block *)_cookie;
}


#ifndef _BOOT_MODE
static partition_module_info sAmigaPartitionModule = {
#else
partition_module_info gAmigaPartitionModule = {
#endif
	{
		AMIGA_PARTITION_MODULE_NAME,
		0,
		amiga_rdb_std_ops
	},
	"amiga",							// short_name
	AMIGA_PARTITION_NAME,				// pretty_name
	0,									// flags

	// scanning
	amiga_rdb_identify_partition,
	amiga_rdb_scan_partition,
	amiga_rdb_free_identify_partition_cookie,
	NULL,
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sAmigaPartitionModule,
	NULL
};
#endif

