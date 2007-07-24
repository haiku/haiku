/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 *		Tomas Kucera, kucerat@centrum.cz
 */

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

#include "PartitionLocker.h"
#include "PartitionMap.h"
#include "PartitionMapParser.h"
#include "PartitionMapWriter.h"

//#define TRACE(x) ;
#define TRACE(x) dprintf x


// this macro is just for testing purpose
// keep it undefined, if you are not testing !!!
// #define TESTING_INTEL_MODULE

// module names
#define INTEL_PARTITION_MODULE_NAME "partitioning_systems/intel/map/v1"
#define INTEL_EXTENDED_PARTITION_MODULE_NAME "partitioning_systems/intel/extended/v1"

// no atomic_add() in the boot loader
#ifdef _BOOT_MODE

inline int32
atomic_add(int32 *a, int32 num)
{
	int32 oldA = *a;
	*a += num;
	return oldA;
}

#endif


// A PartitionMap with reference count.
struct PartitionMapCookie : PartitionMap {
	int32	ref_count;
};


#ifndef _BOOT_MODE

// Maximal size of move buffer (in sectors).
static const int32 MAX_MOVE_BUFFER = 2 * 1024 * 4;

// for logical partitions in Intel Extended Partition
// Count of free sectors after Partition Table Sector (at logical partition).
static const uint32 FREE_SEKTORS_AFTER_PTS = 0;
// Count of free sectors after Master Boot Record.
static const uint32 FREE_SEKTORS_AFTER_MBR = 0;
// size of logical partition header in blocks
static const uint32 PTS_OFFSET = FREE_SEKTORS_AFTER_PTS + 1;
static const uint32 MBR_OFFSET = FREE_SEKTORS_AFTER_MBR + 1;


typedef partitionable_space_data PartitionPosition;

typedef void (*fc_get_sibling_partitions)(partition_data *partition,
		partition_data *child, off_t child_offset, partition_data **prec,
		partition_data **follow, off_t *prec_offset, off_t *prec_size,
		off_t *follow_offset, off_t *follow_size);

typedef int32 (*fc_fill_partitionable_spaces_buffer)(partition_data *partition,
		PartitionPosition *p_positions);


#endif	//_BOOT_MODE




//////////////////////////////////////////
//     Intel Partition Map Module       //
//////////////////////////////////////////





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
static bool pm_supports_resizing(partition_data *partition);
static bool pm_supports_resizing_child(partition_data *partition,
									   partition_data *child);
static bool pm_supports_moving(partition_data *partition, bool *isNoOp);
static bool pm_supports_moving_child(partition_data *partition,
									 partition_data *child);
static bool pm_supports_setting_name(partition_data *partition);
static bool pm_supports_setting_content_name(partition_data *partition);
static bool pm_supports_setting_type(partition_data *partition);
static bool pm_supports_initializing(partition_data *partition);
static bool pm_supports_creating_child(partition_data *partition);
static bool pm_supports_deleting_child(partition_data *partition,
									   partition_data *child);
static bool pm_is_sub_system_for(partition_data *partition);

static bool pm_validate_resize(partition_data *partition, off_t *size);
static bool pm_validate_resize_child(partition_data *partition,
									 partition_data *child, off_t *size);
static bool pm_validate_move(partition_data *partition, off_t *start);
static bool pm_validate_move_child(partition_data *partition,
								   partition_data *child, off_t *start);
static bool pm_validate_set_type(partition_data *partition, const char *type);
static bool pm_validate_initialize(partition_data *partition,
								   char *name, const char *parameters);
static bool pm_validate_create_child(partition_data *partition,
									 off_t *start, off_t *size,
									 const char *type, const char *parameters,
									 int32 *index);
static status_t pm_get_partitionable_spaces(partition_data *partition,
											partitionable_space_data *buffer,
											int32 count, int32 *actualCount);
static status_t pm_get_next_supported_type(partition_data *partition,
										   int32 *cookie, char *type);
static status_t get_type_for_content_type(const char *contentType,
										  char *type);

// shadow partition modification
static status_t pm_shadow_changed(partition_data *partition, uint32 operation);

// writing
static status_t pm_resize(int fd, partition_id partition, off_t size,
						  disk_job_id job);
static status_t pm_resize_child(int fd, partition_id partition, off_t size,
								disk_job_id job);
static status_t pm_move(int fd, partition_id partition, off_t offset,
						disk_job_id job);
static status_t pm_move_child(int fd, partition_id partition, partition_id child,
							  off_t offset, disk_job_id job);
static status_t pm_set_type(int fd, partition_id partition, const char *type,
							disk_job_id job);
static status_t pm_initialize(int fd, partition_id partition, const char *name,
							  const char *parameters, disk_job_id job);
static status_t pm_create_child(int fd, partition_id partition, off_t offset,
								off_t size, const char *type,
								const char *parameters, disk_job_id job,
								partition_id *childID);
static status_t pm_delete_child(int fd, partition_id partition,
								partition_id child, disk_job_id job);

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
	pm_supports_resizing,				// supports_resizing
	pm_supports_resizing_child,			// supports_resizing_child
	pm_supports_moving,					// supports_moving
	pm_supports_moving_child,			// supports_moving_child
	pm_supports_setting_name,			// supports_setting_name
	pm_supports_setting_content_name,	// supports_setting_content_name
	pm_supports_setting_type,			// supports_setting_type
	NULL,								// supports_setting_parameters
	NULL,								// supports_setting_content_parameters
	pm_supports_initializing,			// supports_initializing
	NULL,								// supports_initializing_child
	pm_supports_creating_child,			// supports_creating_child
	pm_supports_deleting_child,			// supports_deleting_child
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
	pm_create_child,					// create_child
	pm_delete_child,					// delete_child
#else
	NULL
#endif	// _BOOT_MODE
};






///////////////////////////////////////////////
//      Intel Extended Partition Module      //
///////////////////////////////////////////////





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

#ifndef _BOOT_MODE
// querying
static bool ep_supports_resizing(partition_data *partition);
static bool ep_supports_resizing_child(partition_data *partition,
									   partition_data *child);
static bool ep_supports_moving(partition_data *partition, bool *isNoOp);
static bool ep_supports_moving_child(partition_data *partition,
									 partition_data *child);
static bool ep_supports_setting_name(partition_data *partition);
static bool ep_supports_setting_content_name(partition_data *partition);
static bool ep_supports_setting_type(partition_data *partition);
static bool ep_supports_initializing(partition_data *partition);
static bool ep_supports_creating_child(partition_data *partition);
static bool ep_supports_deleting_child(partition_data *partition,
									   partition_data *child);
static bool ep_is_sub_system_for(partition_data *partition);

static bool ep_validate_resize(partition_data *partition, off_t *size);
static bool ep_validate_resize_child(partition_data *partition,
									 partition_data *child, off_t *size);
static bool ep_validate_move(partition_data *partition, off_t *start);
static bool ep_validate_move_child(partition_data *partition,
								   partition_data *child, off_t *start);
static bool ep_validate_set_type(partition_data *partition, const char *type);
static bool ep_validate_initialize(partition_data *partition,
								   char *name, const char *parameters);
static bool ep_validate_create_child(partition_data *partition,
									 off_t *start, off_t *size,
									 const char *type, const char *parameters,
									 int32 *index);
static status_t ep_get_partitionable_spaces(partition_data *partition,
											partitionable_space_data *buffer,
											int32 count, int32 *actualCount);
static status_t ep_get_next_supported_type(partition_data *partition,
										   int32 *cookie, char *type);

// shadow partition modification
static status_t ep_shadow_changed(partition_data *partition, uint32 operation);

// writing
static status_t ep_resize(int fd, partition_id partition, off_t size,
						  disk_job_id job);
static status_t ep_resize_child(int fd, partition_id partition, off_t size,
								disk_job_id job);
static status_t ep_move(int fd, partition_id partition, off_t offset,
						disk_job_id job);
static status_t ep_move_child(int fd, partition_id partition, partition_id child,
							  off_t offset, disk_job_id job);
static status_t ep_set_type(int fd, partition_id partition, const char *type,
							disk_job_id job);
static status_t ep_initialize(int fd, partition_id partition, const char *name,
							  const char *parameters, disk_job_id job);
static status_t ep_create_child(int fd, partition_id partition, off_t offset,
								off_t size, const char *type,
								const char *parameters, disk_job_id job,
								partition_id *childID);
static status_t ep_delete_child(int fd, partition_id partition,
								partition_id child, disk_job_id job);

#endif


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
	ep_supports_resizing,				// supports_resizing
	ep_supports_resizing_child,			// supports_resizing_child
	ep_supports_moving,					// supports_moving
	ep_supports_moving_child,			// supports_moving_child
	ep_supports_setting_name,			// supports_setting_name
	ep_supports_setting_content_name,	// supports_setting_content_name
	ep_supports_setting_type,			// supports_setting_type
	NULL,								// supports_setting_parameters
	NULL,								// supports_setting_content_parameters
	ep_supports_initializing,			// supports_initializing
	NULL,								// supports_initializing_child
	ep_supports_creating_child,			// supports_creating_child
	ep_supports_deleting_child,			// supports_deleting_child
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
	ep_create_child,					// create_child
	ep_delete_child,					// delete_child
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







//////////////////////////////////////////
//     Intel Partition Map Module       //
//////////////////////////////////////////





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
	PartitionMapCookie *map = new(nothrow) PartitionMapCookie;
	if (!map)
		return -1;
	map->ref_count = 1;

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
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return B_ERROR;

	TRACE(("intel: pm_scan_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	
	PartitionMapCookie *map = (PartitionMapCookie*)cookie;
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
		PrimaryPartition *primary = map->PrimaryPartitionAt(i);
		if (!primary->IsEmpty()) {
			partition_data *child = create_child_partition(partition->id,
														   index, -1);
			index++;
			if (!child) {
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
		atomic_add(&map->ref_count, 1);
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
	if (cookie) {
		PartitionMapCookie *map = (PartitionMapCookie*)cookie;
		if (atomic_add(&map->ref_count, -1) == 1)
			delete map;
	}
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
		pm_free_identify_partition_cookie(partition, partition->content_cookie);
		partition->content_cookie = NULL;
	}
}

#ifndef _BOOT_MODE





/////////////////////////////////////////////
// Intel Partition Map - support functions //
/////////////////////////////////////////////




// pm_supports_resizing
static
bool
pm_supports_resizing(partition_data *partition)
{
	TRACE(("intel: pm_supports_resizing(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}

// pm_supports_resizing_child
static
bool
pm_supports_resizing_child(partition_data *partition, partition_data *child)
{
	TRACE(("intel: pm_supports_resizing_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}

// pm_supports_moving
static
bool
pm_supports_moving(partition_data *partition, bool *isNoOp)
{
	TRACE(("intel: pm_supports_moving(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	*isNoOp = true;
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}

// pm_supports_moving_child
static
bool
pm_supports_moving_child(partition_data *partition, partition_data *child)
{
	TRACE(("intel: pm_supports_moving_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}

// pm_supports_setting_name
static
bool
pm_supports_setting_name(partition_data *partition)
{
	return false;
}

// pm_supports_setting_content_name
static
bool
pm_supports_setting_content_name(partition_data *partition)
{
	return false;
}

// pm_supports_setting_type
static
bool
pm_supports_setting_type(partition_data *partition)
{
	TRACE(("intel: pm_supports_setting_type(%ld: %lld, %lld, %ld)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	
	// partition should be child of "Intel Partition Map"
	partition_data *parent;
	return (partition
			&& (parent = get_parent_partition(partition->id))
			&& parent->content_type
			&& !strcmp(parent->content_type, kPartitionTypeIntel)
		   );
}

// pm_supports_initializing
static
bool
pm_supports_initializing(partition_data *partition)
{
	TRACE(("intel: pm_supports_initializing(%ld: %lld, %lld, %ld, %s, %d, %d)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type,
		   partition->child_count == 0,
		   partition->content_cookie == NULL));

	return (partition
			&& !partition->content_type
			&& partition->child_count == 0
			&& partition->content_cookie == NULL
		   );
}

// pm_supports_creating_child
static
bool
pm_supports_creating_child(partition_data *partition)
{
	TRACE(("intel: pm_supports_creating_child(%ld: %lld, %lld, %ld, %s, %d)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type,
		   partition->child_count));

	int32 count_spaces = 0;
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel)
			&& partition->child_count < 4
			// free space check
			&& pm_get_partitionable_spaces(partition, NULL, 0,
										   &count_spaces) == B_OK
			&& count_spaces
		   );
}

// pm_supports_deleting_child
static
bool
pm_supports_deleting_child(partition_data *partition, partition_data *child)
{
	TRACE(("intel: pm_supports_deleting_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}

// pm_is_sub_system_for
static
bool
pm_is_sub_system_for(partition_data *partition)
{
	// primary partition map doesn't naturally live in any other child partition
	return false;
}






//////////////////////////////////////////////
// Intel Partition Map - validate functions //
//////////////////////////////////////////////








// block_align (auxiliary function)
static inline
off_t
block_align(off_t offset, uint32 block_size)
{
	return offset / block_size * block_size;
}

// block_align_up (auxiliary function)
static inline
off_t
block_align_up(off_t offset, uint32 block_size)
{
	return (offset + block_size - 1) / block_size * block_size;
}

// validate_resize (auxiliary function)
static
bool
validate_resize(partition_data *partition, off_t *size)
{
	off_t new_size = *size;
	// size remains the same?
	if (new_size == partition->size)
		return true;

	if (new_size < 0)
		new_size = 0;
	else
		new_size = block_align(new_size, partition->block_size);

	// grow partition?
	if (new_size > partition->size) {
		*size = new_size;
		return true;
	}

	// shrink partition
	// no child has to be over the new size of the parent partition
	// TODO: shouldn't be just: off_t current_end = new_size; ??? probably not
	off_t current_end = partition->offset + new_size;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *child = get_child_partition(partition->id, i);
		if (child && child->offset + child->size > current_end)
			current_end = child->offset + child->size;
	}
	new_size = current_end - partition->offset;
	// make the size a multiple of the block size (greater one)
	new_size = block_align_up(new_size, partition->block_size);
	*size = new_size;
	return true;
}

// pm_validate_resize
static
bool
pm_validate_resize(partition_data *partition, off_t *size)
{
	TRACE(("intel: pm_validate_resize\n"));
	
	if (!partition || !pm_supports_resizing(partition) || !size)
		return false;

	return validate_resize(partition, size);
}

// get_offset_ep (auxiliary function)
static inline
off_t
get_offset_ep(const partition_data *partition)
{
	LogicalPartition *logical = (LogicalPartition *)partition->cookie;
	off_t diff_offset = logical->Offset() - logical->PTSOffset();
	return partition->offset - diff_offset;
}

// get_size_ep (auxiliary function)
static inline
off_t
get_size_ep(const partition_data *partition)
{
	LogicalPartition *logical = (LogicalPartition *)partition->cookie;
	off_t diff_offset = logical->Offset() - logical->PTSOffset();
	return partition->size + diff_offset;
}


// get_sibling_partitions_pm (auxiliary function)
static
void
get_sibling_partitions_pm(partition_data *partition,
						  partition_data *child, off_t child_offset,
						  partition_data **prec, partition_data **follow,
						  off_t *prec_offset, off_t *prec_size,
						  off_t *follow_offset, off_t *follow_size)
// according to child_offset returns preceding and following sibling or NULL
// prec, follow output parameters
// partition - Intel Partition Map
{
	// finding out sibling partitions
	partition_data *prec_sibling = NULL;
	partition_data *follow_sibling = NULL;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *sibling = get_child_partition(partition->id, i);
		if (sibling && sibling != child)
			if (sibling->offset <= child_offset) {
				if (!prec_sibling || prec_sibling->offset < sibling->offset)
					prec_sibling = sibling;
			} else {
				// sibling->offset > child_offset
				if (!follow_sibling || follow_sibling->offset > sibling->offset)
					follow_sibling = sibling;
			}
	}
	*prec = prec_sibling;
	*follow = follow_sibling;
	if (prec_sibling) {
		*prec_offset = prec_sibling->offset;
		*prec_size = prec_sibling->size;
	}
	if (follow_sibling) {
		*follow_offset = follow_sibling->offset;
		*follow_size = follow_sibling->size;
	}
}

// get_sibling_partitions_ep (auxiliary function)
static
void
get_sibling_partitions_ep(partition_data *partition,
						  partition_data *child, off_t child_offset,
						  partition_data **prec, partition_data **follow,
						  off_t *prec_offset, off_t *prec_size,
						  off_t *follow_offset, off_t *follow_size)
// according to child_offset returns preceding and following sibling or NULL
// prec, follow output parameters
// partition - Intel Extended Partition
{
	// finding out sibling partitions
	partition_data *prec_sibling = NULL;
	partition_data *follow_sibling = NULL;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *sibling = get_child_partition(partition->id, i);
		if (sibling && sibling != child)
			if (get_offset_ep(sibling) <= child_offset) {
				if (!prec_sibling || prec_sibling->offset < sibling->offset)
					prec_sibling = sibling;
			} else {
				// get_offset_ep(sibling) > child_offset
				if (!follow_sibling || follow_sibling->offset > sibling->offset)
					follow_sibling = sibling;
			}
	}
	*prec = prec_sibling;
	*follow = follow_sibling;
	if (prec_sibling) {
		*prec_offset = get_offset_ep(prec_sibling);
		*prec_size = get_size_ep(prec_sibling);
	}
	if (follow_sibling) {
		*follow_offset = get_offset_ep(follow_sibling);
		*follow_size = get_size_ep(follow_sibling);
	}
}

// validate_resize_child (auxiliary function)
static
bool
validate_resize_child(partition_data *partition, partition_data *child,
					  off_t child_offset, off_t child_size, off_t *size,
					  fc_get_sibling_partitions get_sibling_partitions)
{
	// size remains the same?
	if (*size == child_size)
		return true;
	// shrink partition?
	if (*size < child_size) {
		if (*size < 0)
			*size = 0;
		// make the size a multiple of the block size
		*size = block_align(*size, partition->block_size);
		return true;
	}
	// grow partition
	// child must completely lie within the parent partition
	if (child_offset + *size > partition->offset + partition->size)
		*size = partition->offset + partition->size - child_offset;

	// child must not intersect with sibling partitions
	// finding out sibling partitions
	partition_data *prec_sibling = NULL;
	partition_data *follow_sibling = NULL;
	off_t prec_offset = 0, prec_size = 0, follow_offset = 0, follow_size = 0;

	get_sibling_partitions(partition, child, child_offset, &prec_sibling,
						   &follow_sibling, &prec_offset, &prec_size,
						   &follow_offset, &follow_size);

	if (follow_sibling && (follow_offset < child_offset + *size))
		*size = follow_offset - child_offset;
	*size = block_align(*size, partition->block_size);
	return true;
}

// pm_validate_resize_child
static
bool
pm_validate_resize_child(partition_data *partition, partition_data *child,
						 off_t *size)
{
	TRACE(("intel: pm_validate_resize_child\n"));
	
	if (!partition || !child || !size
		|| !pm_supports_resizing_child(partition, child))
		return false;

	return validate_resize_child(partition, child, child->offset,
					child->size, size, get_sibling_partitions_pm);
}

// pm_validate_move
static
bool
pm_validate_move(partition_data *partition, off_t *start)
{
	TRACE(("intel: pm_validate_move\n"));
	
	bool isNoOp;
	if (!partition || !start || !pm_supports_moving(partition, &isNoOp))
		return false;
	// nothing to do here
	return true;
}

// validate_move_child (auxiliary function)
static
bool
validate_move_child(partition_data *partition, partition_data *child,
					off_t child_offset, off_t child_size, off_t *start,
					fc_get_sibling_partitions get_sibling_partitions)
{
	off_t start_ = *start;

	if (start_ < 0)
		start_ = 0;
	else if (start_ + child_size > partition->size)
		start_ = partition->size - child_size;

	start_ = block_align(start_, partition->block_size);

	// finding out sibling partitions
	partition_data *prec_sibling = NULL;
	partition_data *follow_sibling = NULL;
	off_t prec_offset = 0, prec_size = 0, follow_offset = 0, follow_size = 0;

	get_sibling_partitions(partition, child, child_offset, &prec_sibling,
						   &follow_sibling, &prec_offset, &prec_size,
						   &follow_offset, &follow_size);

	// we cannot move child over sibling partition
	if (start_ < child_offset) {
		// moving left
		if (prec_sibling && prec_offset + prec_size > start_) {
			start_ = prec_offset + prec_size;
			start_ = block_align_up(start_, partition->block_size);
		}
	} else {
		// moving right
		if (follow_sibling && follow_offset < start_ + child_size) {
			start_ = follow_offset - child_size;
			start_ = block_align(start_, partition->block_size);
		}
	}
	*start = start_;
	return true;
}


// pm_validate_move_child
static
bool
pm_validate_move_child(partition_data *partition, partition_data *child,
					   off_t *start)
{
	TRACE(("intel: pm_validate_move_child\n"));
	
	if (!partition || !child || !start
		|| !pm_supports_moving_child(partition, child))
		return false;
	if (*start == child->offset)
		return true;

	return validate_move_child(partition, child, child->offset,
						child->size, start, get_sibling_partitions_pm);
}

// is_type_valid_pm (auxiliary function)
static
bool
is_type_valid_pm(const char *type, partition_data *partition, PrimaryPartition *child = NULL)
// type has to be known, only one extended partition is allowed
// partition - intel partition map
// child can be NULL
{
	// validity check of the type
	PartitionType ptype;
	ptype.SetType(type);
	if (!ptype.IsValid() || ptype.IsEmpty())
		return false;

	// only one extended partition is allowed
	if (ptype.IsExtended()) {
		PartitionMap *map = (PartitionMap*)partition->content_cookie;
		if (!map)
			return false;
		for (int32 i = 0; i < partition->child_count; i++) {
			PrimaryPartition *primary = map->PrimaryPartitionAt(i);
			if (primary && primary->IsExtended() && primary != child)
				return false;
		}
	}
	return true;
}

// pm_validate_set_type
static
bool
pm_validate_set_type(partition_data *partition, const char *type)
{
	TRACE(("intel: pm_validate_set_type\n"));
	
	if (!partition || !pm_supports_setting_type(partition) || !type)
		return false;
	
	partition_data *father = get_parent_partition(partition->id);
	if (!father)
		return false;
	PrimaryPartition *child = (PrimaryPartition*)partition->cookie;
	if (!child)
		return false;

	// validity check of the type
	return is_type_valid_pm(type, father, child);
}

// pm_validate_initialize
static
bool
pm_validate_initialize(partition_data *partition, char *name, const char *parameters)
{
	TRACE(("intel: pm_validate_initialize\n"));
	
	if (!partition || !pm_supports_initializing(partition))
		return false;
	// name is ignored - we cannot set it to the intel partitioning map
	// TODO: check parameters - don't know whether any parameters could be set
	//		 to the intel partition map
	return true;
}

// validate_create_child_part (auxiliary function)
static
bool
validate_create_child_part(partition_data *partition, off_t *start, off_t *size,
		fc_get_sibling_partitions get_sibling_partitions)
{
	// make the start and size a multiple of the block size
	*start = block_align(*start, partition->block_size);
	if (*size < 0)
		*size = 0;
	else
		*size = block_align(*size, partition->block_size);

	// child must completely lie within the parent partition
	if (*start >= partition->offset + partition->size)
		return false;
	if (*start + *size > partition->offset + partition->size)
		*size = partition->offset + partition->size - *start;

	// new child must not intersect with sibling partitions
	// finding out sibling partitions
	partition_data *prec_sibling = NULL;
	partition_data *follow_sibling = NULL;
	off_t prec_offset = 0, prec_size = 0, follow_offset = 0, follow_size = 0;

	get_sibling_partitions(partition, NULL, *start, &prec_sibling,
						   &follow_sibling, &prec_offset, &prec_size,
						   &follow_offset, &follow_size);

	
	// position check of the new partition
	if (prec_sibling && (prec_offset + prec_size > *start)) {
		*start = prec_offset + prec_size;
		*start = block_align_up(*start, partition->block_size);
	}
	
	if (follow_sibling && (follow_offset < *start + *size))
		*size = follow_offset - *start;
	*size = block_align(*size, partition->block_size);
	if (*size == 0)
		return false;

	return true;
}

// pm_validate_create_child
static
bool
pm_validate_create_child(partition_data *partition, off_t *start, off_t *size,
						 const char *type, const char *parameters, int32 *index)
	// index - returns position of the new partition (first free record in MBR)
{
	TRACE(("intel: pm_validate_create_child\n"));
	
	if (!partition || !pm_supports_creating_child(partition)
			|| !start || !size || !type || !index) {
		return false;
	}

	// TODO: check parameters
	// type check
	if (!is_type_valid_pm(type, partition))
		return false;

	// finding out index of the new partition (first free record in MBR)
	// at least one record has to be free
	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	if (!map)
		return false;
	int32 new_index = -1;
	for (int32 i = 0; i < 4; i++) {
		PrimaryPartition *primary = map->PrimaryPartitionAt(i);
		if (primary->IsEmpty()) {
			new_index = i;
			break;
		}
	}
	// this cannot happen
	if (new_index < 0)
		return false;
	*index = new_index;

	if (*start < partition->offset + MBR_OFFSET * partition->block_size) {
		*start = partition->offset + MBR_OFFSET * partition->block_size;
		*start = block_align_up(*start, partition->block_size);
	}

	return validate_create_child_part(partition, start, size,
									  get_sibling_partitions_pm);
}

// cmp_partition_position
static
int
cmp_partition_position(const void *o1, const void *o2) {
	off_t offset1 = ((PartitionPosition*)o1)->offset;
	off_t offset2 = ((PartitionPosition*)o2)->offset;
	if (offset1 < offset2)
		return -1;
	else if (offset1 > offset2)
		return 1;
	return 0;
}


// fill_partitionable_spaces_buffer_pm
static
int32
fill_partitionable_spaces_buffer_pm(partition_data *partition,
									PartitionPosition *p_positions)
// p_positions - output buffer with sufficient size
// returns partition count
{
	int32 partition_count = 0;
	for (int32 i = 0; i < partition->child_count; i++) {
		const partition_data *child = get_child_partition(partition->id, i);
		if (child) {
			p_positions[partition_count].offset = child->offset;
			p_positions[partition_count].size = child->size;
			partition_count++;
		}
	}
	return partition_count;
}

// fill_partitionable_spaces_buffer_ep
static
int32
fill_partitionable_spaces_buffer_ep(partition_data *partition,
									PartitionPosition *p_positions)
// p_positions - output buffer with sufficient size
// returns partition count
{
	int32 partition_count = 0;
	for (int32 i = 0; i < partition->child_count; i++) {
		const partition_data *child = get_child_partition(partition->id, i);
		if (child) {
			p_positions[partition_count].offset = get_offset_ep(child);
			p_positions[partition_count].size = get_size_ep(child);
			partition_count++;
		}
	}
	return partition_count;
}

// get_partitionable_spaces (auxiliary function)
static
status_t
get_partitionable_spaces(partition_data *partition,
		partitionable_space_data *buffer, int32 count, int32 *actualCount,
		fc_fill_partitionable_spaces_buffer fill_buffer,
		off_t start_offset, off_t limit_size = 0, off_t header_size = 0)
{
	PartitionPosition *p_positions = new(nothrow) PartitionPosition[partition->child_count];
	if (!p_positions)
		return B_NO_MEMORY;
	// fill the array
	int32 partition_count = fill_buffer(partition, p_positions);
	// sort the array
	qsort(p_positions, partition_count, sizeof(PartitionPosition), cmp_partition_position);
	
	// first sektor is MBR or EBR
	off_t offset = start_offset + header_size;
	off_t size = 0;
	int32 actual_count = 0;

	// offset alignment (to upper bound)
	offset = block_align_up(offset, partition->block_size);

	// finding out all partitionable spaces
	for (int32 i = 0; i < partition_count; i++) {
		size = p_positions[i].offset - offset;
		size = block_align(size, partition->block_size);
		if (size > limit_size) {
			if (actual_count < count) {
				buffer[actual_count].offset = offset;
				buffer[actual_count].size = size;
			}
			actual_count++;
		}
		offset = p_positions[i].offset + p_positions[i].size + header_size;
		offset = block_align_up(offset, partition->block_size);
	}
	// space in the end of partition
	size = partition->offset + partition->size - offset;
	size = block_align(size, partition->block_size);
	if (size > 0) {
		if (actual_count < count) {
			buffer[actual_count].offset = offset;
			buffer[actual_count].size = size;
		}
		actual_count++;
	}

	// cleanup
	if (p_positions)
		delete[] p_positions;

	*actualCount = actual_count;
	TRACE(("intel: get_partitionable_spaces - found: %d\n", actual_count));
	return B_OK;
}

// pm_get_partitionable_spaces
static
status_t
pm_get_partitionable_spaces(partition_data *partition,
							partitionable_space_data *buffer, int32 count,
							int32 *actualCount)
{
	TRACE(("intel: pm_get_partitionable_spaces\n"));
	
	if (!partition || !partition->content_type
			|| strcmp(partition->content_type, kPartitionTypeIntel)
			|| !actualCount)
		return B_BAD_VALUE;
	if (count > 0 && !buffer)
		return B_BAD_VALUE;

	return get_partitionable_spaces(partition, buffer, count, actualCount,
									fill_partitionable_spaces_buffer_pm,
									MBR_OFFSET * partition->block_size, 0, 0);
}

// pm_get_next_supported_type
static
status_t
pm_get_next_supported_type(partition_data *partition, int32 *cookie, char *type)
{
	TRACE(("intel: pm_get_next_supported_type\n"));
	
	if (!partition || !partition->content_type
			|| strcmp(partition->content_type, kPartitionTypeIntel)
			|| !cookie || !type)
		return B_BAD_VALUE;

	if (*cookie < 1)
		*cookie = 1;
	else if (*cookie > 255)
		return B_BAD_INDEX;

	// finding out supported type
	PartitionType ptype;
	ptype.SetType(*cookie);
	if (!ptype.IsValid())
		ptype.FindNext();
	if (!ptype.IsValid())
		return B_BAD_INDEX;

	ptype.GetTypeString(type);
	*cookie = ptype.Type();
	return B_OK;
}

// get_type_for_content_type (for both pm_* and ep_*)
static
status_t
get_type_for_content_type(const char *contentType, char *type)
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

// pm_shadow_changed
static
status_t
pm_shadow_changed(partition_data *partition, uint32 operation)
{
	TRACE(("intel: pm_shadow_changed\n"));
	
	if (!partition)
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}







/////////////////////////////////////////////
// Intel Partition Map - writing functions //
/////////////////////////////////////////////







// pm_resize
static
status_t
pm_resize(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: pm_resize\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;
	
	// get out partition
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new size
	off_t validatedSize = size;
	if (!pm_validate_resize(partition, &validatedSize))
		return B_BAD_VALUE;

	// update data stuctures
	update_disk_device_job_progress(job, 0.0);
	
	partition->size = validatedSize;
	partition->content_size = validatedSize;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// pm_resize_child
static
status_t
pm_resize_child(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: pm_resize_child\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get out partition, child and partition map structure
	partition_data *partition = get_parent_partition(partitionID);
	partition_data *child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition *primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	// validate the new size
	off_t validatedSize = size;
	if (!pm_validate_resize_child(partition, child, &validatedSize))
		return B_BAD_VALUE;
	if (child->size == validatedSize)
		return B_OK;

	// update data stuctures and write changes
	update_disk_device_job_progress(job, 0.0);
	primary->SetSize(validatedSize);

	PartitionMapWriter writer(fd, 0, partition->size, partition->block_size);
	status_t error = writer.WriteMBR(NULL, map);
	if (error != B_OK) {
		// putting into previous state
		primary->SetSize(child->size);
		return error;
	}

	child->size = validatedSize;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// pm_move
static
status_t
pm_move(int fd, partition_id partitionID, off_t offset, disk_job_id job)
{
	TRACE(("intel: pm_move\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get out partition
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new start
	if (!pm_validate_move(partition, &offset))
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}

// allocate_buffer (auxiliary function)
static
uint8*
allocate_buffer(uint32 block_size, int32 try_alloc, int32 *allocated)
// tries to allocate buffer with the size: block_size * try_alloc
// if it's not possible, tries smaller buffer (until the size: block_size * 1)
// returns pointer to the buffer (it's size is: block_size * allocated)
// or returns NULL - B_NO_MEMORY
{
	uint8* buffer = NULL;
	for (int32 i = try_alloc; i > 1; i /= 2) {
		buffer = new(nothrow) uint8[i * block_size];
		if (buffer) {
			*allocated = i;
			return buffer;
		}
	}
	*allocated = 0;
	return NULL;
}

// move_block (auxiliary function)
static
status_t
move_block(int fd, off_t from_offset, off_t to_offset, uint8 *buffer, int32 size)
{
	status_t error = B_OK;
	// read block to buffer
	if (read_pos(fd, from_offset, buffer, size) != size) {
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
		TRACE(("intel: move_block(): reading failed: %lx\n", error));
		return error;
	}

	// write block from buffer
	if (write_pos(fd, to_offset, buffer, size) != size) {
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
		TRACE(("intel: move_block(): writing failed: %lx\n", error));
	}

	return error;
}

// move_partition (auxiliary function)
static
status_t
move_partition(int fd, off_t from_offset, off_t to_offset, off_t size,
			   uint8 *buffer, int32 buffer_size, disk_job_id job)
{
	status_t error = B_OK;
	off_t cycle_count = size / buffer_size;
	int32 rest_size = size - cycle_count * buffer_size;
	update_disk_device_job_progress(job, 0.0);
	for (off_t i = 0; i < cycle_count; i++) {
		error = move_block(fd, from_offset, to_offset, buffer, buffer_size);
		if (error != B_OK)
			return error;
		from_offset += buffer_size;
		to_offset += buffer_size;
		update_disk_device_job_progress(job, (float)i / cycle_count);
	}
	if (rest_size)
		error = move_block(fd, from_offset, to_offset, buffer, rest_size);
	update_disk_device_job_progress(job, 1.0);
	return error;
}

// pm_move_child
static
status_t
pm_move_child(int fd, partition_id partitionID, partition_id childID, off_t offset,
			  disk_job_id job)
// move child to the position offset
{
	TRACE(("intel: pm_move_child\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get partition, child and partition map structure
	partition_data *partition = get_partition(partitionID);
	partition_data *child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition *primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	off_t validatedOffset = offset;
	if (!pm_validate_move_child(partition, child, &validatedOffset))
		return B_BAD_VALUE;

	// if the old offset is the same, there is nothing to do
	if (child->offset == validatedOffset)
		return B_OK;

	// buffer allocation
	int32 allocated;
	uint8 *buffer = allocate_buffer(partition->block_size, MAX_MOVE_BUFFER, &allocated);
	if (!buffer)
		return B_NO_MEMORY;

	// partition moving
	update_disk_device_job_progress(job, 0.0);
	status_t error = B_OK;
	error = move_partition(fd, child->offset, validatedOffset, child->size,
						   buffer, allocated * partition->block_size, job);
	delete[] buffer;
	if (error != B_OK)
		return error;

	// partition moved
	// updating data structure
	child->offset = validatedOffset;
	primary->SetOffset(validatedOffset);

	PartitionMapWriter writer(fd, 0, partition->size, partition->block_size);
	error = writer.WriteMBR(NULL, map);
	if (error != B_OK)
		// something went wrong - this is fatal (partition has been moved)
		// but MBR is not updated
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(childID);
	return B_OK;
}

// pm_set_type
static
status_t
pm_set_type(int fd, partition_id partitionID, const char *type, disk_job_id job)
{
	TRACE(("intel: pm_set_type\n"));
	
	if (fd < 0 || !type)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif	//TESTING_INTEL_MODULE

	// get parent partition, child and partition map structure
	partition_data *partition = get_parent_partition(partitionID);
	partition_data *child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition *primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	if (!pm_validate_set_type(child, type))
		return B_BAD_VALUE;

	// if the old type is the same, there is nothing to do
	if (child->type && !strcmp(type, child->type))
		return B_OK;

	PartitionType ptype;
	ptype.SetType(type);
	// this is impossible
	if (!ptype.IsValid() || ptype.IsEmpty())
		return false;

	// setting type to the partition
	update_disk_device_job_progress(job, 0.0);
	uint8 old_type = primary->Type();
	primary->SetType(ptype.Type());

	PartitionMapWriter writer(fd, 0, partition->size, partition->block_size);
	status_t error = writer.WriteMBR(NULL, map);
	if (error != B_OK) {
		// something went wrong - putting into previous state
		primary->SetType(old_type);
		return error;
	}

	free(child->type);
	child->type = strdup(type);
	if (!child->type)
		return B_NO_MEMORY;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


#ifdef TESTING_INTEL_MODULE
	// for testing purpose only
static void perform_test(int fd, partition_id partitionID);
#endif	// TESTING_INTEL_MODULE



// pm_initialize
static
status_t
pm_initialize(int fd, partition_id partitionID, const char *name,
			  const char *parameters, disk_job_id job)
{
	TRACE(("intel: pm_initialize\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition and partition map structure
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// name is ignored - we cannot set it to the intel partitioning map
	if (!pm_validate_initialize(partition, NULL, parameters))
		return B_BAD_VALUE;

	// partition init
	update_disk_device_job_progress(job, 0.0);
	// allocate a PartitionMap
	PartitionMapCookie *map = new(nothrow) PartitionMapCookie;
	if (!map)
		return B_NO_MEMORY;
	map->ref_count = 1;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)
	partition->content_cookie = map;

	for (int32 i = 0; i < 4; i++) {
		PrimaryPartition *primary = map->PrimaryPartitionAt(i);
		primary->Unset();
	}
	
	// we delete code area in MBR, if there is any
	partition_table_sector pts;
	pts.clear_code_area();

	PartitionMapWriter writer(fd, 0, partition->size, partition->block_size);
	status_t error = writer.WriteMBR((uint8*)&pts, map);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
#ifdef TESTING_INTEL_MODULE
	// for testing purpose only
	perform_test(fd, partitionID);
#endif	// TESTING_INTEL_MODULE
	return B_OK;
}

// pm_create_child
static
status_t
pm_create_child(int fd, partition_id partitionID, off_t offset, off_t size,
				const char *type, const char *parameters, disk_job_id job,
				partition_id *childID)
	// childID is used for the return value, but is also an optional input
	// parameter -- -1 to be ignored
{
	TRACE(("intel: pm_create_child\n"));
	
	if (fd < 0 || !childID)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition and partition map structure
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;
	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	if (!map)
		return B_BAD_VALUE;

	// validate the offset, size and get index of the new partition
	off_t validatedOffset = offset;
	off_t validatedSize = size;
	int32 index = 0;
	
	if (!pm_validate_create_child(partition, &validatedOffset, &validatedSize,
								  type, parameters, &index))
		return B_BAD_VALUE;

	// finding out free primary partition in the map (index from pm_validate_create_child)
	PrimaryPartition *primary = map->PrimaryPartitionAt(index);
	if (!primary->IsEmpty())
		return B_BAD_DATA;

	// creating partition
	update_disk_device_job_progress(job, 0.0);
	partition_data *child = create_child_partition(partition->id, index, *childID);
	if (!child)
		return B_ERROR;

	PartitionType ptype;
	ptype.SetType(type);

	primary->SetPTSOffset(0);
	primary->SetOffset(validatedOffset);
	primary->SetSize(validatedSize);
	primary->SetType(ptype.Type());
	// TODO: correctly fill active parameter
	primary->SetActive(false);
	
	// write changes to disk
	PartitionMapWriter writer(fd, 0, partition->size, partition->block_size);
	status_t error = writer.WriteMBR(NULL, map);
	if (error != B_OK) {
		// putting into previous state
		primary->Unset();
		delete_partition(child->id);
		return error;
	}

	*childID = child->id;

	child->offset = partition->offset + primary->Offset();
	child->size = primary->Size();
	child->block_size = partition->block_size;
	// (no name)
	child->type = strdup(type);
	// parameters
	child->parameters = strdup(parameters);
	child->cookie = primary;
	// check for allocation problems
	if (!child->type || !child->parameters)
		return B_NO_MEMORY;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// pm_delete_child
static
status_t
pm_delete_child(int fd, partition_id partitionID, partition_id childID, disk_job_id job)
{
	TRACE(("intel: pm_delete_child\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *partition = get_partition(partitionID);
	partition_data *child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;
	
	if (!pm_supports_deleting_child(partition, child))
		return B_ERROR;

	PartitionMap *map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition *primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	// deleting child
	update_disk_device_job_progress(job, 0.0);
	if (!delete_partition(childID))
		return B_ERROR;
	primary->Unset();

	// write changes to disk
	PartitionMapWriter writer(fd, 0, partition->size, partition->block_size);
	status_t error = writer.WriteMBR(NULL, map);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

#endif	// _BOOT_MODE










///////////////////////////////////////////////
//      Intel Extended Partition Module      //
///////////////////////////////////////////////









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
	return 0.95;
}

// ep_scan_partition
static
status_t
ep_scan_partition(int fd, partition_data *partition, void *cookie)
{
	// check parameters
	if (fd < 0 || !partition || !partition->cookie)
		return B_ERROR;

	TRACE(("intel: ep_scan_partition(%d, %lld, %lld, %ld)\n", fd,
		   partition->offset, partition->size, partition->block_size));

	partition_data *parent = get_parent_partition(partition->id);
	if (!parent)
		return B_ERROR;
	PrimaryPartition *primary = (PrimaryPartition*)partition->cookie;
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
		LogicalPartition *logical = primary->LogicalPartitionAt(i);
		partition_data *child = create_child_partition(partition->id,
													   index, -1);
		index++;
		if (!child) {
			// something went wrong
			TRACE(("intel: ep_scan_partition(): failed to create child "
				"partition\n"));
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


#ifndef _BOOT_MODE



//////////////////////////////////////////////////
// Intel Extended Partition - support functions //
//////////////////////////////////////////////////



// ep_supports_resizing
static
bool
ep_supports_resizing(partition_data *partition)
{
	TRACE(("intel: ep_supports_resizing(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntelExtended));
}

// ep_supports_resizing_child
static
bool
ep_supports_resizing_child(partition_data *partition, partition_data *child)
{
	TRACE(("intel: ep_supports_resizing_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntelExtended));
}

// ep_supports_moving
static
bool
ep_supports_moving(partition_data *partition, bool *isNoOp)
{
	TRACE(("intel: ep_supports_moving(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	*isNoOp = true;
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntelExtended));
}

// ep_supports_moving_child
static
bool
ep_supports_moving_child(partition_data *partition, partition_data *child)
{
	TRACE(("intel: ep_supports_moving_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntelExtended));
}

// ep_supports_setting_name
static
bool
ep_supports_setting_name(partition_data *partition)
{
	return false;
}

// ep_supports_setting_content_name
static
bool
ep_supports_setting_content_name(partition_data *partition)
{
	return false;
}

// ep_supports_setting_type
static
bool
ep_supports_setting_type(partition_data *partition)
{
	TRACE(("intel: ep_supports_setting_type(%ld: %lld, %lld, %ld)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	
	
	// partition should be child of "Intel Extended Partition"
	partition_data *parent;
	return (partition
			&& (parent = get_parent_partition(partition->id))
			&& parent->content_type
			&& !strcmp(parent->content_type, kPartitionTypeIntelExtended)
		   );
}

// ep_supports_initializing
static
bool
ep_supports_initializing(partition_data *partition)
{
	TRACE(("intel: ep_supports_initializing(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	return (partition
			&& !partition->content_type
			&& partition->child_count == 0
			&& partition->content_cookie == NULL
		   );
}

// ep_supports_creating_child
static
bool
ep_supports_creating_child(partition_data *partition)
{
	TRACE(("intel: ep_supports_creating_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	int32 count_spaces = 0;
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntelExtended)
			// free space check
			&& ep_get_partitionable_spaces(partition, NULL, 0,
										   &count_spaces) == B_OK
			&& count_spaces
		   );
}

// ep_supports_deleting_child
static
bool
ep_supports_deleting_child(partition_data *partition, partition_data *child)
{
	TRACE(("intel: ep_supports_deleting_child(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	return (partition && child && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntelExtended));
}

// ep_is_sub_system_for
static
bool
ep_is_sub_system_for(partition_data *partition)
{
	TRACE(("intel: ep_is_sub_system_for(%ld: %lld, %lld, %ld, %s)\n",
		   partition->id, partition->offset, partition->size,
		   partition->block_size, partition->content_type));
	
	
	// Intel Extended Partition can live in child partition of Intel Partition Map
	return (partition && partition->content_type
			&& !strcmp(partition->content_type, kPartitionTypeIntel));
}




///////////////////////////////////////////////////
// Intel Extended Partition - validate functions //
///////////////////////////////////////////////////




// ep_validate_resize
static
bool
ep_validate_resize(partition_data *partition, off_t *size)
{
	TRACE(("intel: ep_validate_resize\n"));
	
	if (!partition || !ep_supports_resizing(partition) || !size)
		return false;

	return validate_resize(partition, size);
}

// ep_validate_resize_child
static
bool
ep_validate_resize_child(partition_data *partition, partition_data *child,
						 off_t *size)
{
	TRACE(("intel: ep_validate_resize_child\n"));
	
	if (!partition || !child || !size
		|| !ep_supports_resizing_child(partition, child))
		return false;

	// validate position
	off_t diff_offset = child->offset - get_offset_ep(child);
	off_t size_ = *size + diff_offset;
	if (!validate_resize_child(partition, child, get_offset_ep(child),
		 get_size_ep(child), &size_, get_sibling_partitions_ep))
		return false;
	*size = size_ - diff_offset;
	return true;
}

// ep_validate_move
static
bool
ep_validate_move(partition_data *partition, off_t *start)
{
	TRACE(("intel: ep_validate_move\n"));
	
	bool isNoOp;
	if (!partition || !start || !ep_supports_moving(partition, &isNoOp))
		return false;
	// nothing to do here
	return true;
}

// ep_validate_move_child
static
bool
ep_validate_move_child(partition_data *partition, partition_data *child,
					   off_t *start)
{
	TRACE(("intel: ep_validate_move_child\n"));
	
	if (!partition || !child || !start
		|| !ep_supports_moving_child(partition, child))
		return false;
	if (*start == child->offset)
		return true;

	// validate position
	off_t diff_offset = child->offset - get_offset_ep(child);
	off_t start_ = *start - diff_offset;
	if (!validate_move_child(partition, child, get_offset_ep(child),
		 get_size_ep(child), &start_, get_sibling_partitions_ep))
		return false;
	*start = start_ + diff_offset;
	return true;
}

// is_type_valid_ep (auxiliary function)
static inline
bool
is_type_valid_ep(const char *type)
{
	// validity check of the type - it has to be known
	PartitionType ptype;
	ptype.SetType(type);
	return (ptype.IsValid() && !ptype.IsEmpty() && !ptype.IsExtended());
}

// ep_validate_set_type
static
bool
ep_validate_set_type(partition_data *partition, const char *type)
{
	TRACE(("intel: ep_validate_set_type\n"));
	
	if (!partition || !ep_supports_setting_type(partition) || !type)
		return false;
	
	// validity check of the type
	return is_type_valid_ep(type);
}

// ep_validate_initialize
static
bool
ep_validate_initialize(partition_data *partition, char *name, const char *parameters)
{
	TRACE(("intel: ep_validate_initialize\n"));
	
	if (!partition || !ep_supports_initializing(partition))
		return false;
	// name is ignored - we cannot set it to the Intel Extended Partition
	// TODO: check parameters - don't know whether any parameters could be set
	//		 to the Intel Extended Partition
	return true;
}

// ep_validate_create_child
static
bool
ep_validate_create_child(partition_data *partition, off_t *start, off_t *size,
						 const char *type, const char *parameters, int32 *index)
	// index - returns position of the new partition (the last one)
{
	TRACE(("intel: ep_validate_create_child\n"));
	
	if (!partition || !ep_supports_creating_child(partition)
			|| !start || !size || !type || !index) {
		return false;
	}

	// TODO: check parameters
	// type check
	if (!is_type_valid_ep(type))
		return false;

	// finding out index of the new partition (it will be the last child)
	*index = partition->child_count;

	// validate position
	off_t diff_offset = PTS_OFFSET * partition->block_size;
	off_t start_ = *start - diff_offset;
	off_t size_ = *size + diff_offset;
	if (start_ < partition->offset + PTS_OFFSET * partition->block_size) {
		start_ = partition->offset + PTS_OFFSET * partition->block_size;
		start_ = block_align_up(start_, partition->block_size);
	}
	if (!validate_create_child_part(partition, &start_, &size_,
									get_sibling_partitions_ep))
		return false;
	*start = start_ + diff_offset;
	*size = size_ - diff_offset;
	if (*size == 0)
		return false;
	return true;
}

// ep_get_partitionable_spaces
static
status_t
ep_get_partitionable_spaces(partition_data *partition,
							partitionable_space_data *buffer, int32 count,
							int32 *actualCount)
{
	TRACE(("intel: ep_get_partitionable_spaces\n"));
	
	if (!partition || !partition->content_type
			|| strcmp(partition->content_type, kPartitionTypeIntelExtended)
			|| !actualCount)
		return B_BAD_VALUE;
	if (count > 0 && !buffer)
		return B_BAD_VALUE;

	return get_partitionable_spaces(partition, buffer, count, actualCount,
									fill_partitionable_spaces_buffer_ep,
									partition->offset + PTS_OFFSET * partition->block_size,
									PTS_OFFSET * partition->block_size,
									PTS_OFFSET * partition->block_size);
}

// ep_get_next_supported_type
static
status_t
ep_get_next_supported_type(partition_data *partition, int32 *cookie, char *type)
{
	TRACE(("intel: ep_get_next_supported_type\n"));
	
	if (!partition || !partition->content_type
			|| strcmp(partition->content_type, kPartitionTypeIntelExtended)
			|| !cookie || !type)
		return B_BAD_VALUE;

	if (*cookie < 1)
		*cookie = 1;
	else if (*cookie > 255)
		return B_BAD_INDEX;

	// finding out supported type
	PartitionType ptype;
	ptype.SetType(*cookie);
	if (!ptype.IsValid())
		ptype.FindNext();
	while (ptype.IsValid() && ptype.IsExtended())
		ptype.FindNext();
	if (!ptype.IsValid())
		return B_BAD_INDEX;

	ptype.GetTypeString(type);
	*cookie = ptype.Type();
	return B_OK;
}

// ep_shadow_changed
static
status_t
ep_shadow_changed(partition_data *partition, uint32 operation)
{
	TRACE(("intel: ep_shadow_changed\n"));
	
	if (!partition)
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}





////////////////////////////////////////////////
// Intel Extended Partition - write functions //
////////////////////////////////////////////////





// ep_resize
static
status_t
ep_resize(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: ep_resize\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif
	
	// get out partition
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new size
	off_t validatedSize = size;
	if (!ep_validate_resize(partition, &validatedSize))
		return B_BAD_VALUE;

	// update data stuctures
	update_disk_device_job_progress(job, 0.0);
	
	partition->size = validatedSize;
	partition->content_size = validatedSize;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// ep_resize_child
static
status_t
ep_resize_child(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: ep_resize_child\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get out partition, child and LogicalPartition structure
	partition_data *partition = get_parent_partition(partitionID);
	partition_data *child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	LogicalPartition *logical = (LogicalPartition*)child->cookie;
	if (!logical)
		return B_BAD_VALUE;

	// validate the new size
	off_t validatedSize = size;
	if (!ep_validate_resize_child(partition, child, &validatedSize))
		return B_BAD_VALUE;
	if (child->size == validatedSize)
		return B_OK;

	// update data stuctures and write changes
	update_disk_device_job_progress(job, 0.0);
	logical->SetSize(validatedSize);

	PartitionMapWriter writer(fd, partition->offset, partition->size, partition->block_size);
	status_t error = writer.WriteLogical(NULL, logical);
	if (error != B_OK) {
		// putting into previous state
		logical->SetSize(child->size);
		return error;
	}
	LogicalPartition *prev = logical->Previous();
	error = prev ? writer.WriteLogical(NULL, prev)
				 : writer.WriteExtendedHead(NULL, logical);
	if (error != B_OK)
		// this should be not so fatal
		return error;

	child->size = validatedSize;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// ep_move
static
status_t
ep_move(int fd, partition_id partitionID, off_t offset, disk_job_id job)
{
	TRACE(("intel: ep_move\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get out partition
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new start
	if (!ep_validate_move(partition, &offset))
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}

// ep_move_child
static
status_t
ep_move_child(int fd, partition_id partitionID, partition_id childID, off_t offset,
			  disk_job_id job)
// move child to the position offset
{
	TRACE(("intel: ep_move_child\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get partition, child and LogicalPartition structure
	partition_data *partition = get_partition(partitionID);
	partition_data *child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;
	LogicalPartition *logical = (LogicalPartition*)child->cookie;
	if (!logical)
		return B_BAD_VALUE;

	off_t validatedOffset = offset;
	if (!ep_validate_move_child(partition, child, &validatedOffset))
		return B_BAD_VALUE;

	// if the old offset is the same, there is nothing to do
	if (child->offset == validatedOffset)
		return B_OK;

	off_t diff_offset = validatedOffset - child->offset;

	// buffer allocation
	int32 allocated;
	uint8 *buffer = allocate_buffer(partition->block_size, MAX_MOVE_BUFFER, &allocated);
	if (!buffer)
		return B_NO_MEMORY;

	// partition moving
	update_disk_device_job_progress(job, 0.0);
	status_t error = B_OK;
	// move partition with its header (PTS table)
	off_t pts_offset = logical->Offset() - logical->PTSOffset();
	error = move_partition(fd, child->offset - pts_offset, validatedOffset - pts_offset,
						   child->size + pts_offset, buffer,
						   allocated * partition->block_size, job);
	delete[] buffer;
	if (error != B_OK)
		return error;

	// partition moved
	// updating data structure
	child->offset = validatedOffset;
	logical->SetOffset(logical->Offset() + diff_offset);
	logical->SetPTSOffset(logical->PTSOffset() + diff_offset);

	PartitionMapWriter writer(fd, partition->offset, partition->size, partition->block_size);
	error = writer.WriteLogical(NULL, logical);
	if (error != B_OK)
		// something went wrong - this is fatal (partition has been moved)
		// but EBR is not updated
		return error;
	LogicalPartition *prev = logical->Previous();
	error = prev ? writer.WriteLogical(NULL, prev)
				 : writer.WriteExtendedHead(NULL, logical);
	if (error != B_OK)
		// this is fatal - linked list is not updated
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(childID);
	return B_OK;
}

// ep_set_type
static
status_t
ep_set_type(int fd, partition_id partitionID, const char *type, disk_job_id job)
{
	TRACE(("intel: ep_set_type\n"));
	
	if (fd < 0 || !type)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get partition, child and LogicalPartition structure
	partition_data *partition = get_parent_partition(partitionID);
	partition_data *child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	LogicalPartition *logical = (LogicalPartition*)child->cookie;
	if (!logical)
		return B_BAD_VALUE;

	if (!ep_validate_set_type(child, type))
		return B_BAD_VALUE;

	// if the old type is the same, there is nothing to do
	if (child->type && !strcmp(type, child->type))
		return B_OK;

	PartitionType ptype;
	ptype.SetType(type);
	// this is impossible
	if (!ptype.IsValid() || ptype.IsEmpty() || ptype.IsExtended())
		return false;

	// setting type to the partition
	update_disk_device_job_progress(job, 0.0);
	uint8 old_type = logical->Type();
	logical->SetType(ptype.Type());

	PartitionMapWriter writer(fd, partition->offset, partition->size, partition->block_size);
	status_t error = writer.WriteLogical(NULL, logical);
	if (error != B_OK) {
		// something went wrong - putting into previous state
		logical->SetType(old_type);
		return error;
	}

	free(child->type);
	child->type = strdup(type);
	if (!child->type)
		return B_NO_MEMORY;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// ep_initialize
static
status_t
ep_initialize(int fd, partition_id partitionID, const char *name,
			  const char *parameters, disk_job_id job)
{
	TRACE(("intel: ep_initialize\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get partition
	partition_data *partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// name is ignored - we cannot set it to the Intel Extended Partition
	if (!ep_validate_initialize(partition, NULL, parameters))
		return B_BAD_VALUE;

	// partition init (we have no child partition)
	update_disk_device_job_progress(job, 0.0);
	PrimaryPartition *primary = (PrimaryPartition*)partition->cookie;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)
	partition->content_cookie = primary;
	
	// we delete code area in EBR - nothing should be there
	partition_table_sector pts;
	pts.clear_code_area();

	PartitionMapWriter writer(fd, partition->offset, partition->size, partition->block_size);
	status_t error = writer.WriteExtendedHead((uint8*)&pts, NULL);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// ep_create_child
static
status_t
ep_create_child(int fd, partition_id partitionID, off_t offset, off_t size,
				const char *type, const char *parameters, disk_job_id job,
				partition_id *childID)
	// childID is used for the return value, but is also an optional input
	// parameter -- -1 to be ignored
{
	TRACE(("intel: ep_create_child\n"));
	
	if (fd < 0 || !childID)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	// get parent, partition and PrimaryPartition structure
	partition_data *parent = get_parent_partition(partitionID);
	partition_data *partition = get_partition(partitionID);
	if (!parent || !partition)
		return B_BAD_VALUE;
	PrimaryPartition *primary = (PrimaryPartition*)partition->cookie;
	if (!primary)
		return B_BAD_VALUE;

	// validate the offset, size and get index of the new partition
	off_t validatedOffset = offset;
	off_t validatedSize = size;
	int32 index = 0;
	
	if (!ep_validate_create_child(partition, &validatedOffset, &validatedSize,
								  type, parameters, &index))
		return B_BAD_VALUE;

	LogicalPartition *logical = new(nothrow) LogicalPartition;
	if (!logical)
		return B_NO_MEMORY;

	// creating partition
	update_disk_device_job_progress(job, 0.0);
	partition_data *child = create_child_partition(partition->id, index, *childID);
	if (!child)
		return B_ERROR;

	PartitionType ptype;
	ptype.SetType(type);

	logical->SetPTSOffset(validatedOffset - PTS_OFFSET * partition->block_size - partition->offset);
	logical->SetOffset(validatedOffset - partition->offset);
	logical->SetSize(validatedSize);
	logical->SetType(ptype.Type());
	// TODO: correctly fill active parameter
	logical->SetActive(false);

	// we delete code area in EBR - nothing should be there
	partition_table_sector pts;
	pts.clear_code_area();

	// write changes to disk
	PartitionMapWriter writer(fd, partition->offset, partition->size, partition->block_size);
	status_t error = writer.WriteLogical((uint8*)&pts, logical);
	if (error != B_OK) {
		// putting into previous state
		delete_partition(child->id);
		delete logical;
		return error;
	}
	// update linked list
	primary->AddLogicalPartition(logical);
	LogicalPartition *prev = logical->Previous();
	error = prev ? writer.WriteLogical(NULL, prev)
				 : writer.WriteExtendedHead(NULL, logical);
	if (error != B_OK) {
		// putting into previous state
		delete_partition(child->id);
		primary->RemoveLogicalPartition(logical);
		delete logical;
		return error;
	}

	*childID = child->id;

	child->offset = partition->offset + logical->Offset();
	child->size = logical->Size();
	child->block_size = partition->block_size;
	// (no name)
	child->type = strdup(type);
	// parameters
	child->parameters = strdup(parameters);
	child->cookie = logical;
	// check for allocation problems
	if (!child->type || !child->parameters)
		error = B_NO_MEMORY;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

// ep_delete_child
static
status_t
ep_delete_child(int fd, partition_id partitionID, partition_id childID, disk_job_id job)
{
	TRACE(("intel: ep_delete_child\n"));
	
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
#ifndef TESTING_INTEL_MODULE
	if (!locker.IsLocked())
		return B_ERROR;
#endif

	partition_data *partition = get_partition(partitionID);
	partition_data *child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;
	
	if (!ep_supports_deleting_child(partition, child))
		return B_ERROR;

	PrimaryPartition *primary = (PrimaryPartition*)partition->cookie;
	LogicalPartition *logical = (LogicalPartition*)child->cookie;
	if (!primary || !logical)
		return B_BAD_VALUE;

	// deleting child
	update_disk_device_job_progress(job, 0.0);
	if (!delete_partition(childID))
		return B_ERROR;

	LogicalPartition *next_logical = logical->Next();
	LogicalPartition *prev_logical = logical->Previous();
	primary->RemoveLogicalPartition(logical);
	delete logical;

	// write changes to disk
	PartitionMapWriter writer(fd, partition->offset, partition->size, partition->block_size);
	status_t error = prev_logical ? writer.WriteLogical(NULL, prev_logical)
								  : writer.WriteExtendedHead(NULL, next_logical);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}




#ifdef TESTING_INTEL_MODULE
//////////////////////////////////////////////////
// for testing purpose only
//////////////////////////////////////////////////

static
void
perform_test(int fd, partition_id partitionID) {
	TRACE(("\nintel: function testing\n\n"));

	bool sup_init = false, val_init = false, b_sup = false, b_val = false;
	status_t result = B_OK;

	partition_data *partition = get_partition(partitionID);
	
	TRACE(("content_type: %s\n", partition->content_type));
	partition->content_type = kPartitionTypeIntel;

	TRACE(("deleting children\n"));
	for (int32 i = 5; i >= 0; i--) {
		partition_data *child = get_child_partition(partition->id, i);
		TRACE(("get_child_partition (index: %d): %d\n", i, child != NULL ));
		if (child) {
			b_sup = pm_supports_deleting_child(partition, child);
			TRACE(("pm_supports_deleting_child: %d\n", b_sup));
			result = pm_delete_child(fd, partition->id, child->id, 1);
			TRACE(("pm_delete_child: %d\n", result == B_OK));
		}
	}

	partitionable_space_data child_buf[] = {
		{ 1024 * 1024 * 12, 1024 * 1024 * 3 },
		{ 0, 1024 * 1024 * 10 },
		{ 1024 * 1024 * 15, 1024 * 1024 * 10 },
	};

	for (int32 j = 0; j < 3; j++ ) {
		TRACE(("\nchild: %i;\toffset: %lld;\tsize: %lld\n", j, child_buf[j].offset, child_buf[j].size));

		partitionable_space_data buffer[10];
		int32 actualCount = 0;
		result = pm_get_partitionable_spaces(partition, buffer, 10, &actualCount);
		TRACE(("pm_get_partitionable_spaces: %d\n\toffset\t\tsize\n", result == B_OK));
		for (int32 i = 0; i < actualCount; i++) {
			TRACE(("\t%lld\t\t%lld\n", buffer[i].offset, buffer[i].size));
		}
		sup_init = pm_supports_creating_child(partition);
		TRACE(("pm_supports_creating_child: %d\n", sup_init));
		off_t start = child_buf[j].offset, size = child_buf[j].size;
		int32 index = 0;
		char *type = "FAT 32-bit, LBA-mapped";
		char *parameters = "active = 0";
		val_init = pm_validate_create_child(partition, &start, &size,
											type, parameters,
											&index);
		TRACE(("pm_validate_create_child: %d\tstart: %lld;\tsize: %lld;\tindex: %i\n",
			   val_init, start, size, index));
		partition_id childID = -1;
		result = pm_create_child(fd, partition->id, start, size, type, parameters, 1,
								 &childID);
		TRACE(("pm_create_child: %d;\tchildID: %i\n", result == B_OK, childID));

	}

	partition_data *child = get_child_partition(partition->id, 1);
	b_sup = pm_supports_setting_type(child);
	TRACE(("pm_supports_setting_type: %d\n", b_sup));
	char *type_e = "Intel Extended Partition";
	b_val = pm_validate_set_type(child, type_e);
	TRACE(("pm_validate_set_type (%s): %d\n", type_e, b_val));
	result = pm_set_type(fd, child->id, type_e, 1);
	TRACE(("pm_set_type: %d\n", result == B_OK));

	sup_init = ep_supports_initializing(child);
	TRACE(("ep_supports_initializing: %d\n", sup_init));
	val_init = ep_validate_initialize(child, "nazev", "active = 0");
	TRACE(("ep_validate_initialize: %d\n", val_init));
	result = ep_initialize(fd, child->id, "nazev", "active = 0", 1);
	TRACE(("ep_initialize: %d\n", result == B_OK));
	TRACE(("content_type: %s\n", child->content_type));
	child->content_type = kPartitionTypeIntelExtended;

	partitionable_space_data ex_child_buf[] = {
		{ 1024 * 1024 * 0, 1024 * 1024 * 3 },
		{ 1024 * 1024 * 5, 1024 * 1024 * 2 },
		{ 1024 * 1024 * 7, 1024 * 1024 * 3 },
	};

	for (int32 j = 0; j < 3; j++ ) {
		TRACE(("\nchild: %i;\toffset: %lld;\tsize: %lld\n", j, ex_child_buf[j].offset, ex_child_buf[j].size));

		partitionable_space_data buffer[10];
		int32 actualCount = 0;
		result = ep_get_partitionable_spaces(child, buffer, 10, &actualCount);
		TRACE(("ep_get_partitionable_spaces: %d\n\toffset\t\tsize\n", result == B_OK));
		for (int32 i = 0; i < actualCount; i++) {
			TRACE(("\t%lld\t\t%lld\n", buffer[i].offset, buffer[i].size));
		}
		sup_init = ep_supports_creating_child(child);
		TRACE(("ep_supports_creating_child: %d\n", sup_init));
		off_t start = ex_child_buf[j].offset, size = ex_child_buf[j].size;
		int32 index = 0;
		char *type = "FAT 32-bit, LBA-mapped";
		char *parameters = "active = 0";
		val_init = ep_validate_create_child(child, &start, &size,
											type, parameters,
											&index);
		TRACE(("ep_validate_create_child: %d\tstart: %lld;\tsize: %lld;\tindex: %i\n",
			   val_init, start, size, index));
		partition_id childID = -1;
		result = ep_create_child(fd, child->id, start, size, type, parameters, 1,
								 &childID);
		TRACE(("ep_create_child: %d;\tchildID: %i\n", result == B_OK, childID));

	}

	
	child = get_child_partition(partition->id, 2);
	b_sup = pm_supports_resizing_child(partition, child);
	TRACE(("pm_supports_resizing_child: %d\n", b_sup));
	off_t val_size = 20 * 1024 * 1024;
	b_val = pm_validate_resize_child(partition, child, &val_size);
	TRACE(("pm_validate_resize_child: %lld\n", val_size));
	result = pm_resize_child(fd, child->id, val_size, 1);
	TRACE(("pm_resize_child: %d\n", result == B_OK));

	child = get_child_partition(partition->id, 1);
	partition_data *ex_child = get_child_partition(child->id, 0);
	b_sup = ep_supports_resizing_child(child, ex_child);
	TRACE(("ep_supports_resizing_child: %d\n", b_sup));
	val_size = 20 * 1024 * 1024;
	b_val = ep_validate_resize_child(child, ex_child, &val_size);
	TRACE(("ep_validate_resize_child: %lld\n", val_size));
	result = ep_resize_child(fd, ex_child->id, val_size, 1);
	TRACE(("ep_resize_child: %d\n", result == B_OK));

	child = get_child_partition(partition->id, 0);
	b_sup = pm_supports_resizing_child(partition, child);
	TRACE(("pm_supports_resizing_child: %d\n", b_sup));
	val_size = 1 * 1024 * 1024;
	b_val = pm_validate_resize_child(partition, child, &val_size);
	TRACE(("pm_validate_resize_child: %lld\n", val_size));
	result = pm_resize_child(fd, child->id, val_size, 0);
	TRACE(("pm_resize_child: %d\n", result == B_OK));

	child = get_child_partition(partition->id, 0);
	b_sup = pm_supports_moving_child(partition, child);
	TRACE(("pm_supports_moving_child: %d\n", b_sup));
	off_t val_start = 13000000;
	b_val = pm_validate_move_child(partition, child, &val_start);
	TRACE(("pm_validate_move_child: %lld\n", val_start));
	result = pm_move_child(fd, partition->id, child->id, val_start, 1);
	TRACE(("pm_move_child: %d\n", result == B_OK));

	child = get_child_partition(partition->id, 1);
	ex_child = get_child_partition(child->id, 0);
	b_sup = ep_supports_resizing_child(child, ex_child);
	TRACE(("ep_supports_resizing_child: %d\n", b_sup));
	val_size = 1 * 1024 * 1024;
	b_val = ep_validate_resize_child(child, ex_child, &val_size);
	TRACE(("ep_validate_resize_child: %lld\n", val_size));
	result = ep_resize_child(fd, ex_child->id, val_size, 1);
	TRACE(("ep_resize_child: %d\n", result == B_OK));

	b_sup = ep_supports_moving_child(child, ex_child);
	TRACE(("ep_supports_moving_child: %d\n", b_sup));
	val_start = 4 * 1024 * 1024;
	b_val = ep_validate_move_child(child, ex_child, &val_start);
	TRACE(("ep_validate_move_child: %lld\n", val_start));
	result = ep_move_child(fd, child->id, ex_child->id, val_start, 1);
	TRACE(("ep_move_child: %d\n", result == B_OK));

/*	child = get_child_partition(partition->id, 1);
	partition_data *ex_child = get_child_partition(child->id, 0);
	b_sup = ep_supports_deleting_child(child, ex_child);
	TRACE(("ep_supports_deleting_child: %d\n", b_sup));
	result = ep_delete_child(fd, child->id, ex_child->id, 1);
	TRACE(("ep_delete_child: %d\n", result == B_OK));
	ex_child = get_child_partition(child->id, 1);
	b_sup = ep_supports_deleting_child(child, ex_child);
	TRACE(("ep_supports_deleting_child: %d\n", b_sup));
	result = ep_delete_child(fd, child->id, ex_child->id, 1);
	TRACE(("ep_delete_child: %d\n", result == B_OK));*/


	
	TRACE(("\nintel: end of function testing\n\n"));
}
#endif // TESTING_INTEL_MODULE

#endif	// _BOOT_MODE
