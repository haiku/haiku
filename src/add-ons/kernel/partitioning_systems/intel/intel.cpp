//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//---------------------------------------------------------------------
/*!
	\file intel.cpp
	\brief disk_scanner partition module for "intel" style partitions.
*/

// TODO: The implementation is very strict right now. It rejects a partition
// completely, if it finds an error in its partition tables. We should see,
// what error can be handled gracefully, e.g. by ignoring the partition
// descriptor or the whole partition table sector.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/kernel_cpp.h>
#include <AutoDeleter.h>
#include <ddm_modules.h>
#ifndef _BOOT_MODE
#	include <DiskDeviceTypes.h>
#else
#	include <boot/partitions.h>
//#	include "DiskDeviceUtils.h"
#endif
#include <KernelExport.h>

#include "PartitionMap.h"
#include "PartitionMapParser.h"

#define TRACE(x) ;
//#define TRACE(x) dprintf x

// module names
#define INTEL_PARTITION_MODULE_NAME "partitioning_systems/intel/map/v1"
#define INTEL_EXTENDED_PARTITION_MODULE_NAME "partitioning_systems/intel/extended/v1"

// these match those in DiskDeviceTypes.cpp
#define INTEL_PARTITION_NAME "Intel Partition Map"
#define INTEL_EXTENDED_PARTITION_NAME "Intel Extended Partition"


// intel partition map module

// module
static status_t pm_std_ops(int32 op, ...);

// scanning
static float pm_identify_partition(int fd, partition_data *partition,
		void **cookie);
static status_t pm_scan_partition(int fd, partition_data *partition,
		void *cookie);
static void pm_free_identify_partition_cookie(partition_data *partition,
		void *cookie);
static void pm_free_partition_cookie(partition_data *partition);
static void pm_free_partition_content_cookie(partition_data *partition);

#ifndef _BOOT_MODE
// querying
static bool pm_supports_resizing_child(partition_data *partition,
									   partition_data *child);

static bool pm_validate_resize_child(partition_data *partition,
									 partition_data *child, off_t *size);

// writing
static status_t pm_resize_child(int fd, partition_id partition, off_t size,
								disk_job_id job);
#endif

#ifdef _BOOT_MODE
partition_module_info gIntelPartitionMapModule =
#else
static partition_module_info intel_partition_map_module =
#endif
{
	{
		INTEL_PARTITION_MODULE_NAME,
		0,
		pm_std_ops
	},
	INTEL_PARTITION_NAME,				// pretty_name
	0,									// flags

	// scanning
	pm_identify_partition,				// identify_partition
	pm_scan_partition,					// scan_partition
	pm_free_identify_partition_cookie,	// free_identify_partition_cookie
	pm_free_partition_cookie,			// free_partition_cookie
	pm_free_partition_content_cookie,	// free_partition_content_cookie

#ifndef _BOOT_MODE
	// querying
	NULL,								// supports_repairing
	NULL,								// supports_resizing
	pm_supports_resizing_child,			// supports_resizing_child
	NULL,								// supports_moving
	NULL,								// supports_moving_child
	NULL,								// supports_setting_name
	NULL,								// supports_setting_content_name
	NULL,								// supports_setting_type
	NULL,								// supports_setting_parameters
	NULL,								// supports_setting_content_parameters
	NULL,								// supports_initializing
	NULL,								// supports_initializing_child
	NULL,								// supports_creating_child
	NULL,								// supports_deleting_child
	NULL,								// is_sub_system_for

	NULL,								// validate_resize
	pm_validate_resize_child,			// validate_resize_child
	NULL,								// validate_move
	NULL,								// validate_move_child
	NULL,								// validate_set_name
	NULL,								// validate_set_content_name
	NULL,								// validate_set_type
	NULL,								// validate_set_parameters
	NULL,								// validate_set_content_parameters
	NULL,								// validate_initialize
	NULL,								// validate_create_child
	NULL,								// get_partitionable_spaces
	NULL,								// get_next_supported_type
	NULL,								// get_type_for_content_type

	// shadow partition modification
	NULL,								// shadow_changed

	// writing
	NULL,								// repair
	NULL,								// resize
	pm_resize_child,					// resize_child
	NULL,								// move
	NULL,								// move_child
	NULL,								// set_name
	NULL,								// set_content_name
	NULL,								// set_type
	NULL,								// set_parameters
	NULL,								// set_content_parameters
	NULL,								// initialize
	NULL,								// create_child
	NULL,								// delete_child
#else
	NULL
#endif	// _BOOT_MODE
};


// intel extended partition module

// module
static status_t ep_std_ops(int32 op, ...);

// scanning
static float ep_identify_partition(int fd, partition_data *partition,
		void **cookie);
static status_t ep_scan_partition(int fd, partition_data *partition,
		void *cookie);
static void ep_free_identify_partition_cookie(partition_data *partition,
		void *cookie);
static void ep_free_partition_cookie(partition_data *partition);
static void ep_free_partition_content_cookie(partition_data *partition);

#ifdef _BOOT_MODE
partition_module_info gIntelExtendedPartitionModule =
#else
static partition_module_info intel_extended_partition_module =
#endif
{
	{
		INTEL_EXTENDED_PARTITION_MODULE_NAME,
		0,
		ep_std_ops
	},
	INTEL_EXTENDED_PARTITION_NAME,		// pretty_name
	0,									// flags

	// scanning
	ep_identify_partition,				// identify_partition
	ep_scan_partition,					// scan_partition
	ep_free_identify_partition_cookie,	// free_identify_partition_cookie
	ep_free_partition_cookie,			// free_partition_cookie
	ep_free_partition_content_cookie,	// free_partition_content_cookie

#ifndef _BOOT_MODE
	// querying
	NULL,								// supports_repairing
	NULL,								// supports_resizing
	NULL,								// supports_resizing_child
	NULL,								// supports_moving
	NULL,								// supports_moving_child
	NULL,								// supports_setting_name
	NULL,								// supports_setting_content_name
	NULL,								// supports_setting_type
	NULL,								// supports_setting_parameters
	NULL,								// supports_setting_content_parameters
	NULL,								// supports_initializing
	NULL,								// supports_initializing_child
	NULL,								// supports_creating_child
	NULL,								// supports_deleting_child
	NULL,								// is_sub_system_for

	NULL,								// validate_resize
	NULL,								// validate_resize_child
	NULL,								// validate_move
	NULL,								// validate_move_child
	NULL,								// validate_set_name
	NULL,								// validate_set_content_name
	NULL,								// validate_set_type
	NULL,								// validate_set_parameters
	NULL,								// validate_set_content_parameters
	NULL,								// validate_initialize
	NULL,								// validate_create_child
	NULL,								// get_partitionable_spaces
	NULL,								// get_next_supported_type
	NULL,								// get_type_for_content_type

	// shadow partition modification
	NULL,								// shadow_changed

	// writing
	NULL,								// repair
	NULL,								// resize
	NULL,								// resize_child
	NULL,								// move
	NULL,								// move_child
	NULL,								// set_name
	NULL,								// set_content_name
	NULL,								// set_type
	NULL,								// set_parameters
	NULL,								// set_content_parameters
	NULL,								// initialize
	NULL,								// create_child
	NULL,								// delete_child
#else
	NULL
#endif	// _BOOT_MODE
};


#ifndef _BOOT_MODE
extern "C" partition_module_info *modules[];
_EXPORT partition_module_info *modules[] =
{
	&intel_partition_map_module,
	&intel_extended_partition_module,
	NULL
};
#endif


// intel partition map module

// pm_std_ops
static
status_t
pm_std_ops(int32 op, ...)
{
	TRACE(("intel: pm_std_ops(0x%lx)\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// pm_identify_partition
static
float
pm_identify_partition(int fd, partition_data *partition, void **cookie)
{
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return -1;
	TRACE(("intel: pm_identify_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	// reject extended partitions
	if (partition->type
		&& !strcmp(partition->type, kPartitionTypeIntelExtended)) {
		return -1;
	}
	// check block size
	uint32 blockSize = partition->block_size;
	if (blockSize < sizeof(partition_table_sector)) {
		TRACE(("intel: read_partition_map: bad block size: %ld, should be "
			   ">= %ld\n", blockSize, sizeof(partition_table_sector)));
		return -1;
	}
	// allocate a PartitionMap
	PartitionMap *map = new(nothrow) PartitionMap;
	if (!map)
		return -1;
	// read the partition structure
	PartitionMapParser parser(fd, 0, partition->size, blockSize);
	status_t error = parser.Parse(NULL, map);
	if (error == B_OK) {
		*cookie = map;
		return 0.5;
	}
	// cleanup, if not detected
	delete map;
	return -1;
}

// pm_scan_partition
static
status_t
pm_scan_partition(int fd, partition_data *partition, void *cookie)
{
	ObjectDeleter<PartitionMap> deleter((PartitionMap*)cookie);
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return B_ERROR;
	TRACE(("intel: pm_scan_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	PartitionMap *map = (PartitionMap*)cookie;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY
						| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD;
		// TODO: Update when write functionality is implemented.
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)
	partition->content_cookie = map;
	// children
	status_t error = B_OK;
	int32 index = 0;
	for (int32 i = 0; i < 4; i++) {
		PrimaryPartition *primary = map->PrimaryPartitionAt(i);
		if (!primary->IsEmpty()) {
			partition_data *child = create_child_partition(partition->id,
														   index, -1);
			index++;
			if (!child) {
TRACE(("Creating child at index %ld failed\n", index - 1));
				// something went wrong
				error = B_ERROR;
				break;
			}
			child->offset = partition->offset + primary->Offset();
			child->size = primary->Size();
			child->block_size = partition->block_size;
			// (no name)
			char type[B_FILE_NAME_LENGTH];
			primary->GetTypeString(type);
			child->type = strdup(type);
			// parameters
			char buffer[128];
			sprintf(buffer, "type = %u ; active = %d", primary->Type(),
					primary->Active());
			child->parameters = strdup(buffer);
			child->cookie = primary;
			// check for allocation problems
			if (!child->type || !child->parameters) {
				error = B_NO_MEMORY;
				break;
			}
		}
	}
	// keep map on success or cleanup on error
	if (error == B_OK) {
		deleter.Detach();
	} else {
		partition->content_cookie = NULL;
		for (int32 i = 0; i < partition->child_count; i++) {
			if (partition_data *child = get_child_partition(partition->id, i))
				child->cookie = NULL;
		}
	}
	return error;
}

// pm_free_identify_partition_cookie
static
void
pm_free_identify_partition_cookie(partition_data */*partition*/, void *cookie)
{
	if (cookie)
		delete (PartitionMap*)cookie;
}

// pm_free_partition_cookie
static
void
pm_free_partition_cookie(partition_data *partition)
{
	// called for the primary partitions: the PrimaryPartition is allocated
	// by the partition containing the partition map
	if (partition)
		partition->cookie = NULL;
}

// pm_free_partition_content_cookie
static
void
pm_free_partition_content_cookie(partition_data *partition)
{
	if (partition && partition->content_cookie) {
		delete (PartitionMap*)partition->content_cookie;
		partition->content_cookie = NULL;
	}
}

#ifndef _BOOT_MODE
// pm_supports_resizing_child
static
bool
pm_supports_resizing_child(partition_data *partition, partition_data *child)
{
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}

// pm_validate_resize_child
static
bool
pm_validate_resize_child(partition_data *partition, partition_data *child,
						 off_t *size)
{
	if (!partition || !child || !partition->content_type
		|| strcmp(partition->content_type, kPartitionTypeIntel)
		|| !size) {
		return false;
	}
	// size remains the same?
	if (*size == child->size)
		return true;
	// shrink partition?
	if (*size < child->size) {
		if (*size < 0)
			*size = 0;
		// make the size a multiple of the block size
		*size = *size / partition->block_size * partition->block_size;
		return true;
	}
	// grow partition
	// child must completely lie within the parent partition
	if (child->offset + *size > partition->offset + partition->size)
		*size = partition->offset + partition->size - child->offset;
	// child must not intersect with sibling partitions
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *sibling = get_child_partition(partition->id, i);
		if (sibling && sibling != child && sibling->offset > child->offset) {
			if (child->offset + *size > sibling->offset)
				*size = sibling->offset - child->offset;
		}
	}
	// make the size a multiple of the block size
	*size = *size / partition->block_size * partition->block_size;
	return true;
}

// pm_resize_child
static
status_t
pm_resize_child(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
/*
	disk_device_data *device = read_lock_disk_device(partitionID);
	if (!device)
		return B_BAD_VALUE;
	DeviceStructWriteLocker locker(device, true);
	// get partition and child and out partition map structure
	partition_data *partition = get_parent_partition(partitionID);
	partition_data *child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition *primary = child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;
	// validate the new size
	off_t validatedSize = size;
	if (!pm_validate_resize_child(partition, child, validatedSize)
		return B_BAD_VALUE;
	if (child->size == validatedSize)
		return B_OK;
	// write the changes to disk
	primary->SetSize(validatedSize);
	// TODO: Write...
	// TODO: PartitionMapParser into separate file,
	//       implement PartitionMapWriter
*/
set_disk_device_job_error_message(job, "Resize not implemented yet.");
return B_ERROR;
}
#endif	// _BOOT_MODE


// intel extended partition module

// ep_std_ops
static
status_t
ep_std_ops(int32 op, ...)
{
	TRACE(("intel: ep_std_ops(0x%lx)\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// ep_identify_partition
static
float
ep_identify_partition(int fd, partition_data *partition, void **cookie)
{
	// check parameters
	if (fd < 0 || !partition || !cookie || !partition->cookie)
		return -1;
	TRACE(("intel: ep_identify_partition(%d, %lld, %lld, %ld)\n", fd,
		   partition->offset, partition->size, partition->block_size));
	// our parent must be a intel partition map partition and we must have
	// extended partition type
	if (!partition->type
		|| strcmp(partition->type, kPartitionTypeIntelExtended)) {
		return -1;
	}
	partition_data *parent = get_parent_partition(partition->id);
	if (!parent || !parent->content_type
		|| strcmp(parent->content_type, kPartitionTypeIntel)) {
		return -1;
	}
	// things seem to be in order
	return 0.5;
}

// ep_scan_partition
static
status_t
ep_scan_partition(int fd, partition_data *partition, void *cookie)
{
	// check parameters
	if (fd < 0 || !partition || !partition->cookie)
		return B_ERROR;
	partition_data *parent = get_parent_partition(partition->id);
	if (!parent)
		return B_ERROR;
	PrimaryPartition *primary = (PrimaryPartition*)partition->cookie;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY;
		// TODO: Update when write functionality is implemented.
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)
	partition->content_cookie = primary;
	// children
	status_t error = B_OK;
	int32 index = 0;
	for (int32 i = 0; i < primary->CountLogicalPartitions(); i++) {
		LogicalPartition *logical = primary->LogicalPartitionAt(i);
		partition_data *child = create_child_partition(partition->id,
													   index, -1);
		index++;
		if (!child) {
			// something went wrong
			error = B_ERROR;
			break;
		}
		child->offset = parent->offset + logical->Offset();
		child->size = logical->Size();
		child->block_size = partition->block_size;
		// (no name)
		char type[B_FILE_NAME_LENGTH];
		logical->GetTypeString(type);
		child->type = strdup(type);
		// parameters
		char buffer[128];
		sprintf(buffer, "type = %u ; active = %d", logical->Type(),
				logical->Active());
		child->parameters = strdup(buffer);
		child->cookie = logical;
		// check for allocation problems
		if (!child->type || !child->parameters) {
			error = B_NO_MEMORY;
			break;
		}
	}
	// cleanup on error
	if (error != B_OK) {
		partition->content_cookie = NULL;
		for (int32 i = 0; i < partition->child_count; i++) {
			if (partition_data *child = get_child_partition(partition->id, i))
				child->cookie = NULL;
		}
	}
	return error;
}

// ep_free_identify_partition_cookie
static
void
ep_free_identify_partition_cookie(partition_data *partition, void *cookie)
{
	// nothing to do
}

// ep_free_partition_cookie
static
void
ep_free_partition_cookie(partition_data *partition)
{
	// the logical partition's cookie belongs to the partition map partition
	if (partition)
		partition->cookie = NULL;
}

// ep_free_partition_content_cookie
static
void
ep_free_partition_content_cookie(partition_data *partition)
{
	// the extended partition's cookie belongs to the partition map partition
	if (partition)
		partition->content_cookie = NULL;
}

