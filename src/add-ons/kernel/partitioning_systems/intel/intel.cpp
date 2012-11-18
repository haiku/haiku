/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 *		Tomas Kucera, kucerat@centrum.cz
 */

/*!
	\file intel.cpp
	\brief partitioning system module for "intel" style partitions.
*/

// TODO: The implementation is very strict right now. It rejects a partition
// completely, if it finds an error in its partition tables. We should see,
// what error can be handled gracefully, e.g. by ignoring the partition
// descriptor or the whole partition table sector.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <KernelExport.h>

#include <AutoDeleter.h>
#include <disk_device_manager/ddm_modules.h>

#include "intel.h"
#include "PartitionLocker.h"
#include "PartitionMap.h"
#include "PartitionMapParser.h"

#ifndef _BOOT_MODE
#	include <DiskDeviceTypes.h>
#	include "write_support.h"
#	define TRACE(x) dprintf x
#else
#	include <boot/partitions.h>
#	include <util/kernel_cpp.h>
#	define TRACE(x) ;
#endif


// module names
#define INTEL_PARTITION_MODULE_NAME "partitioning_systems/intel/map/v1"
#define INTEL_EXTENDED_PARTITION_MODULE_NAME \
	"partitioning_systems/intel/extended/v1"


using std::nothrow;

// TODO: This doesn't belong here!
// no atomic_add() in the boot loader
#ifdef _BOOT_MODE

inline int32
atomic_add(int32* a, int32 num)
{
	int32 oldA = *a;
	*a += num;
	return oldA;
}

#endif


#ifndef _BOOT_MODE

// get_type_for_content_type (for both pm_* and ep_*)
static status_t
get_type_for_content_type(const char* contentType, char* type)
{
	TRACE(("intel: get_type_for_content_type(%s)\n",
		   contentType));

	if (!contentType || !type)
		return B_BAD_VALUE;

	PartitionType ptype;
	ptype.SetContentType(contentType);
	if (!ptype.IsValid())
		return B_NAME_NOT_FOUND;

	ptype.GetTypeString(type);
	return B_OK;
}

#endif


// #pragma mark - Intel Partition Map Module


// pm_std_ops
static status_t
pm_std_ops(int32 op, ...)
{
	TRACE(("intel: pm_std_ops(0x%" B_PRIx32 ")\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}


// pm_identify_partition
static float
pm_identify_partition(int fd, partition_data* partition, void** cookie)
{
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return -1;

	TRACE(("intel: pm_identify_partition(%d, %" B_PRId32 ": %" B_PRId64 ", "
		"%" B_PRId64 ", %" B_PRId32 ")\n", fd, partition->id, partition->offset,
		partition->size, partition->block_size));
	// reject extended partitions
	if (partition->type
		&& !strcmp(partition->type, kPartitionTypeIntelExtended)) {
		return -1;
	}

	// allocate a PartitionMap
	PartitionMapCookie* map = new(nothrow) PartitionMapCookie;
	if (!map)
		return -1;

	// read the partition structure
	PartitionMapParser parser(fd, 0, partition->size, partition->block_size);
	status_t error = parser.Parse(NULL, map);
	if (error != B_OK) {
		// cleanup, if not detected
		delete map;
		return -1;
	}

	*cookie = map;

	// Depending on whether we actually have recognized child partitions and
	// whether we are installed directly on a device (the by far most common
	// setup), we determine the priority.
	bool hasChildren = (map->CountNonEmptyPartitions() > 0);
	bool hasParent = (get_parent_partition(partition->id) != NULL);

	if (!hasParent) {
		if (hasChildren) {
			// This value overrides BFS.
			return 0.81;
		}

		// No children -- might be a freshly initialized disk. But it could
		// also be an image file. So we give BFS a chance to override us.
		return 0.5;
	}

// NOTE: It seems supporting nested partition maps makes more trouble than it
// has useful applications ATM. So it is disabled for the time being.
#if 0
	// We have a parent. That's a very unlikely setup.
	if (hasChildren)
		return 0.4;

	// No children. Extremely unlikely, that this is desired. But if no one
	// else claims the partition, we take it anyway.
	return 0.1;
#endif
	return -1;
}


// pm_scan_partition
static status_t
pm_scan_partition(int fd, partition_data* partition, void* cookie)
{
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return B_ERROR;

	TRACE(("intel: pm_scan_partition(%d, %" B_PRId32 ": %" B_PRId64 ", "
		"%" B_PRId64 ", %" B_PRId32 ")\n", fd, partition->id, partition->offset,
		partition->size, partition->block_size));

	PartitionMapCookie* map = (PartitionMapCookie*)cookie;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)

	partition->content_cookie = map;
	// children
	status_t error = B_OK;
	int32 index = 0;
	for (int32 i = 0; i < 4; i++) {
		PrimaryPartition* primary = map->PrimaryPartitionAt(i);
		if (!primary->IsEmpty()) {
			partition_data* child = create_child_partition(partition->id,
				index, partition->offset + primary->Offset(), primary->Size(),
				-1);
			index++;
			if (!child) {
				// something went wrong
				error = B_ERROR;
				break;
			}

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
		atomic_add(&map->ref_count, 1);
	} else {
		partition->content_cookie = NULL;
		for (int32 i = 0; i < partition->child_count; i++) {
			if (partition_data* child = get_child_partition(partition->id, i))
				child->cookie = NULL;
		}
	}

	return error;
}


// pm_free_identify_partition_cookie
static void
pm_free_identify_partition_cookie(partition_data*/* partition*/, void* cookie)
{
	if (cookie) {
		PartitionMapCookie* map = (PartitionMapCookie*)cookie;
		if (atomic_add(&map->ref_count, -1) == 1)
			delete map;
	}
}


// pm_free_partition_cookie
static void
pm_free_partition_cookie(partition_data* partition)
{
	// called for the primary partitions: the PrimaryPartition is allocated
	// by the partition containing the partition map
	if (partition)
		partition->cookie = NULL;
}


// pm_free_partition_content_cookie
static void
pm_free_partition_content_cookie(partition_data* partition)
{
	if (partition && partition->content_cookie) {
		pm_free_identify_partition_cookie(partition, partition->content_cookie);
		partition->content_cookie = NULL;
	}
}


// #pragma mark - Intel Extended Partition Module


// ep_std_ops
static status_t
ep_std_ops(int32 op, ...)
{
	TRACE(("intel: ep_std_ops(0x%" B_PRIx32 ")\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}


// ep_identify_partition
static float
ep_identify_partition(int fd, partition_data* partition, void** cookie)
{
	// check parameters
	if (fd < 0 || !partition || !cookie || !partition->cookie)
		return -1;

	TRACE(("intel: ep_identify_partition(%d, %" B_PRId64 ", %" B_PRId64 ", "
		"%" B_PRId32 ")\n", fd, partition->offset, partition->size,
		partition->block_size));

	// our parent must be a intel partition map partition and we must have
	// extended partition type
	if (!partition->type
		|| strcmp(partition->type, kPartitionTypeIntelExtended)) {
		return -1;
	}
	partition_data* parent = get_parent_partition(partition->id);
	if (!parent || !parent->content_type
		|| strcmp(parent->content_type, kPartitionTypeIntel)) {
		return -1;
	}

	// things seem to be in order
	return 0.95;
}


// ep_scan_partition
static status_t
ep_scan_partition(int fd, partition_data* partition, void* cookie)
{
	// check parameters
	if (fd < 0 || !partition || !partition->cookie)
		return B_ERROR;

	TRACE(("intel: ep_scan_partition(%d, %" B_PRId64 ", %" B_PRId64 ", "
		"%" B_PRId32 ")\n", fd, partition->offset, partition->size,
		partition->block_size));

	partition_data* parent = get_parent_partition(partition->id);
	if (!parent)
		return B_ERROR;

	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)

	partition->content_cookie = primary;
	// children
	status_t error = B_OK;
	int32 index = 0;
	for (int32 i = 0; i < primary->CountLogicalPartitions(); i++) {
		LogicalPartition* logical = primary->LogicalPartitionAt(i);
		partition_data* child = create_child_partition(partition->id, index,
			parent->offset + logical->Offset(), logical->Size(), -1);
		index++;
		if (!child) {
			// something went wrong
			TRACE(("intel: ep_scan_partition(): failed to create child "
				"partition\n"));
			error = B_ERROR;
			break;
		}
		child->block_size = partition->block_size;

		// (no name)
		char type[B_FILE_NAME_LENGTH];
		logical->GetTypeString(type);
		child->type = strdup(type);

		// parameters
		char buffer[128];
		sprintf(buffer, "active %s ;\npartition_table_offset %" B_PRId64 " ;\n",
			logical->Active() ? "true" : "false",
			logical->PartitionTableOffset());
		child->parameters = strdup(buffer);
		child->cookie = logical;
		// check for allocation problems
		if (!child->type || !child->parameters) {
			TRACE(("intel: ep_scan_partition(): failed to allocation type "
				"or parameters\n"));
			error = B_NO_MEMORY;
			break;
		}
	}

	// cleanup on error
	if (error != B_OK) {
		partition->content_cookie = NULL;
		for (int32 i = 0; i < partition->child_count; i++) {
			if (partition_data* child = get_child_partition(partition->id, i))
				child->cookie = NULL;
		}
	}
	return error;
}


// ep_free_identify_partition_cookie
static void
ep_free_identify_partition_cookie(partition_data* partition, void* cookie)
{
	// nothing to do
}


// ep_free_partition_cookie
static void
ep_free_partition_cookie(partition_data* partition)
{
	// the logical partition's cookie belongs to the partition map partition
	if (partition)
		partition->cookie = NULL;
}


// ep_free_partition_content_cookie
static void
ep_free_partition_content_cookie(partition_data* partition)
{
	// the extended partition's cookie belongs to the partition map partition
	if (partition)
		partition->content_cookie = NULL;
}


// #pragma mark - modules


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
	"intel",							// short_name
	INTEL_PARTITION_NAME,				// pretty_name

	// flags
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
	| B_DISK_SYSTEM_SUPPORTS_RESIZING
	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
//	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME

	| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_NAME
	,

	// scanning
	pm_identify_partition,				// identify_partition
	pm_scan_partition,					// scan_partition
	pm_free_identify_partition_cookie,	// free_identify_partition_cookie
	pm_free_partition_cookie,			// free_partition_cookie
	pm_free_partition_content_cookie,	// free_partition_content_cookie

#ifndef _BOOT_MODE
	// querying
	pm_get_supported_operations,		// get_supported_operations
	pm_get_supported_child_operations,	// get_supported_child_operations
	NULL,								// supports_initializing_child
	pm_is_sub_system_for,				// is_sub_system_for

	pm_validate_resize,					// validate_resize
	pm_validate_resize_child,			// validate_resize_child
	pm_validate_move,					// validate_move
	pm_validate_move_child,				// validate_move_child
	NULL,								// validate_set_name
	NULL,								// validate_set_content_name
	pm_validate_set_type,				// validate_set_type
	NULL,								// validate_set_parameters
	NULL,								// validate_set_content_parameters
	pm_validate_initialize,				// validate_initialize
	pm_validate_create_child,			// validate_create_child
	pm_get_partitionable_spaces,		// get_partitionable_spaces
	pm_get_next_supported_type,			// get_next_supported_type
	get_type_for_content_type,			// get_type_for_content_type

	// shadow partition modification
	pm_shadow_changed,					// shadow_changed

	// writing
	NULL,								// repair
	pm_resize,							// resize
	pm_resize_child,					// resize_child
	pm_move,							// move
	pm_move_child,						// move_child
	NULL,								// set_name
	NULL,								// set_content_name
	pm_set_type,						// set_type
	NULL,								// set_parameters
	NULL,								// set_content_parameters
	pm_initialize,						// initialize
	pm_uninitialize,					// uninitialize
	pm_create_child,					// create_child
	pm_delete_child,					// delete_child
#else
	NULL
#endif	// _BOOT_MODE
};


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
	"intel_extended",					// short_name
	INTEL_EXTENDED_PARTITION_NAME,		// pretty_name

	// flags
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
	| B_DISK_SYSTEM_SUPPORTS_RESIZING
	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
//	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME

	| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_NAME
	,

	// scanning
	ep_identify_partition,				// identify_partition
	ep_scan_partition,					// scan_partition
	ep_free_identify_partition_cookie,	// free_identify_partition_cookie
	ep_free_partition_cookie,			// free_partition_cookie
	ep_free_partition_content_cookie,	// free_partition_content_cookie

#ifndef _BOOT_MODE
	// querying
	ep_get_supported_operations,		// get_supported_operations
	ep_get_supported_child_operations,	// get_supported_child_operations
	NULL,								// supports_initializing_child
	ep_is_sub_system_for,				// is_sub_system_for

	ep_validate_resize,					// validate_resize
	ep_validate_resize_child,			// validate_resize_child
	ep_validate_move,					// validate_move
	ep_validate_move_child,				// validate_move_child
	NULL,								// validate_set_name
	NULL,								// validate_set_content_name
	ep_validate_set_type,				// validate_set_type
	NULL,								// validate_set_parameters
	NULL,								// validate_set_content_parameters
	ep_validate_initialize,				// validate_initialize
	ep_validate_create_child,			// validate_create_child
	ep_get_partitionable_spaces,		// get_partitionable_spaces
	ep_get_next_supported_type,			// get_next_supported_type
	get_type_for_content_type,			// get_type_for_content_type

	// shadow partition modification
	ep_shadow_changed,					// shadow_changed

	// writing
	NULL,								// repair
	ep_resize,							// resize
	ep_resize_child,					// resize_child
	ep_move,							// move
	ep_move_child,						// move_child
	NULL,								// set_name
	NULL,								// set_content_name
	ep_set_type,						// set_type
	NULL,								// set_parameters
	NULL,								// set_content_parameters
	ep_initialize,						// initialize
	NULL,								// uninitialize
	ep_create_child,					// create_child
	ep_delete_child,					// delete_child
#else	// _BOOT_MODE
	NULL
#endif	// _BOOT_MODE
};


#ifndef _BOOT_MODE
extern "C" partition_module_info* modules[];
_EXPORT partition_module_info* modules[] =
{
	&intel_partition_map_module,
	&intel_extended_partition_module,
	NULL
};
#endif
