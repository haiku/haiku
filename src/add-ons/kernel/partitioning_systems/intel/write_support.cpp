/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Tomas Kucera, kucerat@centrum.cz
 */


#include "write_support.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <DiskDeviceTypes.h>
#include <KernelExport.h>

#include <AutoDeleter.h>

#include "intel.h"
#include "PartitionLocker.h"
#include "PartitionMap.h"
#include "PartitionMapParser.h"
#include "PartitionMapWriter.h"


//#define TRACE(x) ;
#define TRACE(x) dprintf x

// Maximal size of move buffer (in sectors).
static const int32 MAX_MOVE_BUFFER = 2 * 1024 * 4;

// for logical partitions in Intel Extended Partition
// Count of free sectors after Partition Table Sector (at logical partition).
static const uint32 FREE_SECTORS_AFTER_PTS = 63;
// Count of free sectors after Master Boot Record.
static const uint32 FREE_SECTORS_AFTER_MBR = 63;
// size of logical partition header in blocks
static const uint32 PTS_OFFSET = FREE_SECTORS_AFTER_PTS + 1;
static const uint32 MBR_OFFSET = FREE_SECTORS_AFTER_MBR + 1;


typedef partitionable_space_data PartitionPosition;

typedef void (*fc_get_sibling_partitions)(partition_data* partition,
		partition_data* child, off_t childOffset, partition_data** prec,
		partition_data** follow, off_t* prec_offset, off_t* prec_size,
		off_t* follow_offset, off_t* follow_size);

typedef int32 (*fc_fill_partitionable_spaces_buffer)(partition_data* partition,
		PartitionPosition* positions);


status_t pm_get_partitionable_spaces(partition_data* partition,
	partitionable_space_data* buffer, int32 count, int32* actualCount);
status_t ep_get_partitionable_spaces(partition_data* partition,
	partitionable_space_data* buffer, int32 count, int32* actualCount);


// #pragma mark - Intel Partition Map - support functions


// pm_get_supported_operations
uint32
pm_get_supported_operations(partition_data* partition, uint32 mask)
{
	uint32 flags = B_DISK_SYSTEM_SUPPORTS_RESIZING
		| B_DISK_SYSTEM_SUPPORTS_MOVING
		| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
		| B_DISK_SYSTEM_SUPPORTS_INITIALIZING;

	// creating child
	int32 countSpaces = 0;
	if (partition->child_count < 4
		// free space check
		&& pm_get_partitionable_spaces(partition, NULL, 0, &countSpaces)
			== B_BUFFER_OVERFLOW
		&& countSpaces > 0) {
		flags |= B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
	}

	return flags;
}


// pm_get_supported_child_operations
uint32
pm_get_supported_child_operations(partition_data* partition,
	partition_data* child, uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
		| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


// pm_is_sub_system_for
bool
pm_is_sub_system_for(partition_data* partition)
{
	// primary partition map doesn't naturally live in any other child partition
	return false;
}

bool
get_partition_from_offset_ep(partition_data* partition, off_t offset,
	partition_data** nextPartition)
{
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* sibling = get_child_partition(partition->id, i);
		if (sibling != NULL && sibling->offset == offset) {
			*nextPartition = sibling;
			return true;
		}
	}

	return false;
}


// #pragma mark - Intel Partition Map - validate functions


// sector_align (auxiliary function)
static inline off_t
sector_align(off_t offset, int32 blockSize)
{
	return offset / blockSize * blockSize;
}


// sector_align_up (auxiliary function)
static inline off_t
sector_align_up(off_t offset, int32 blockSize)
{
	return (offset + blockSize - 1) / blockSize * blockSize;
}


// validate_resize (auxiliary function)
static bool
validate_resize(partition_data* partition, off_t* size)
{
	off_t newSize = *size;
	// size remains the same?
	if (newSize == partition->size)
		return true;

	if (newSize < 0)
		newSize = 0;
	else
		newSize = sector_align(newSize, partition->block_size);

	// grow partition?
	if (newSize > partition->size) {
		*size = newSize;
		return true;
	}

	// shrink partition
	// no child has to be over the new size of the parent partition
	// TODO: shouldn't be just: off_t currentEnd = newSize; ??? probably not
	// If child->offset is relative to parent, then yes!
	off_t currentEnd = partition->offset + newSize;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* child = get_child_partition(partition->id, i);
		if (child && child->offset + child->size > currentEnd)
			currentEnd = child->offset + child->size;
	}
	newSize = currentEnd - partition->offset;
	// make the size a multiple of the block size (greater one)
	newSize = sector_align_up(newSize, partition->block_size);
	*size = newSize;
	return true;
}


// pm_validate_resize
bool
pm_validate_resize(partition_data* partition, off_t* size)
{
	TRACE(("intel: pm_validate_resize\n"));

	if (!partition || !size)
		return false;

	return validate_resize(partition, size);
}


// get_sibling_partitions_pm (auxiliary function)
/*!
	according to childOffset returns previous and next sibling or NULL
	precious, next output parameters
	partition - Intel Partition Map
*/
static void
get_sibling_partitions_pm(partition_data* partition,
	partition_data* child, off_t childOffset, partition_data** previous,
	partition_data** next, off_t* previousOffset, off_t* previousSize,
	off_t* nextOffset, off_t* nextSize)
{
	// finding out sibling partitions
	partition_data* previousSibling = NULL;
	partition_data* nextSibling = NULL;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* sibling = get_child_partition(partition->id, i);
		if (sibling && sibling != child) {
			if (sibling->offset <= childOffset) {
				if (!previousSibling || previousSibling->offset < sibling->offset)
					previousSibling = sibling;
			} else {
				// sibling->offset > childOffset
				if (!nextSibling || nextSibling->offset > sibling->offset)
					nextSibling = sibling;
			}
		}
	}
	*previous = previousSibling;
	*next = nextSibling;
	if (previousSibling) {
		*previousOffset = previousSibling->offset;
		*previousSize = previousSibling->size;
	}
	if (nextSibling) {
		*nextOffset = nextSibling->offset;
		*nextSize = nextSibling->size;
	}
}


// get_sibling_partitions_ep (auxiliary function)
/*!
	according to childOffset returns previous and next sibling or NULL
	previous, next output parameters
	partition - Intel Extended Partition
*/
static void
get_sibling_partitions_ep(partition_data* partition,
	partition_data* child, off_t childOffset, partition_data** previous,
	partition_data** next, off_t* previousOffset, off_t* previousSize,
	off_t* nextOffset, off_t* nextSize)
{
	// finding out sibling partitions
	partition_data* previousSibling = NULL;
	partition_data* nextSibling = NULL;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* sibling = get_child_partition(partition->id, i);
		if (sibling && sibling != child) {
			if (sibling->offset <= childOffset) {
				if (!previousSibling || previousSibling->offset < sibling->offset)
					previousSibling = sibling;
			} else {
				// get_offset_ep(sibling) > childOffset
				if (!nextSibling || nextSibling->offset > sibling->offset)
					nextSibling = sibling;
			}
		}
	}
	*previous = previousSibling;
	*next = nextSibling;
	if (previousSibling) {
		*previousOffset = previousSibling->offset;
		*previousSize = previousSibling->size;
	}
	if (nextSibling) {
		*nextOffset = nextSibling->offset;
		*nextSize = nextSibling->size;
	}
}


// validate_resize_child (auxiliary function)
static bool
validate_resize_child(partition_data* partition, partition_data* child,
	off_t childOffset, off_t childSize, off_t* size,
	fc_get_sibling_partitions getSiblingPartitions)
{
	// size remains the same?
	if (*size == childSize)
		return true;
	// shrink partition?
	if (*size < childSize) {
		if (*size < 0)
			*size = 0;
		// make the size a multiple of the block size
		*size = sector_align(*size, partition->block_size);
		return true;
	}
	// grow partition
	// child must completely lie within the parent partition
	if (childOffset + *size > partition->offset + partition->size)
		*size = partition->offset + partition->size - childOffset;

	// child must not intersect with sibling partitions
	// finding out sibling partitions
	partition_data* previousSibling = NULL;
	partition_data* nextSibling = NULL;
	off_t previousOffset = 0, previousSize = 0, nextOffset = 0, nextSize = 0;

	getSiblingPartitions(partition, child, childOffset, &previousSibling,
		&nextSibling, &previousOffset, &previousSize, &nextOffset, &nextSize);

	if (nextSibling && (nextOffset < childOffset + *size))
		*size = nextOffset - childOffset;
	*size = sector_align(*size, partition->block_size);
	return true;
}


// pm_validate_resize_child
bool
pm_validate_resize_child(partition_data* partition, partition_data* child,
	off_t* size)
{
	TRACE(("intel: pm_validate_resize_child\n"));

	if (!partition || !child || !size)
		return false;

	return validate_resize_child(partition, child, child->offset,
		child->size, size, get_sibling_partitions_pm);
}


// pm_validate_move
bool
pm_validate_move(partition_data* partition, off_t* start)
{
	TRACE(("intel: pm_validate_move\n"));

	if (!partition || !start)
		return false;
	// nothing to do here
	return true;
}


// validate_move_child (auxiliary function)
static bool
validate_move_child(partition_data* partition, partition_data* child,
	off_t childOffset, off_t childSize, off_t* _start,
	fc_get_sibling_partitions getSiblingPartitions)
{
	off_t start = *_start;

	if (start < 0)
		start = 0;
	else if (start + childSize > partition->size)
		start = partition->size - childSize;

	start = sector_align(start, partition->block_size);

	// finding out sibling partitions
	partition_data* previousSibling = NULL;
	partition_data* nextSibling = NULL;
	off_t previousOffset = 0, previousSize = 0, nextOffset = 0, nextSize = 0;

	getSiblingPartitions(partition, child, childOffset, &previousSibling,
		&nextSibling, &previousOffset, &previousSize, &nextOffset, &nextSize);

	// we cannot move child over sibling partition
	if (start < childOffset) {
		// moving left
		if (previousSibling && previousOffset + previousSize > start) {
			start = previousOffset + previousSize;
			start = sector_align_up(start, partition->block_size);
		}
	} else {
		// moving right
		if (nextSibling && nextOffset < start + childSize) {
			start = nextOffset - childSize;
			start = sector_align(start, partition->block_size);
		}
	}
	*_start = start;
	return true;
}


// pm_validate_move_child
bool
pm_validate_move_child(partition_data* partition, partition_data* child,
	off_t* start)
{
	TRACE(("intel: pm_validate_move_child\n"));

	if (!partition || !child || !start)
		return false;
	if (*start == child->offset)
		return true;

	return validate_move_child(partition, child, child->offset,
		child->size, start, get_sibling_partitions_pm);
}

// is_type_valid_pm (auxiliary function)
/*!
	type has to be known, only one extended partition is allowed
	partition - intel partition map
	child can be NULL
*/
static bool
is_type_valid_pm(const char* type, partition_data* partition,
	PrimaryPartition* child = NULL)
{
	// validity check of the type
	PartitionType ptype;
	ptype.SetType(type);
	if (!ptype.IsValid() || ptype.IsEmpty())
		return false;

	// only one extended partition is allowed
	if (ptype.IsExtended()) {
		PartitionMap* map = (PartitionMap*)partition->content_cookie;
		if (!map)
			return false;
		for (int32 i = 0; i < partition->child_count; i++) {
			PrimaryPartition* primary = map->PrimaryPartitionAt(i);
			if (primary && primary->IsExtended() && primary != child)
				return false;
		}
	}
	return true;
}


// pm_validate_set_type
bool
pm_validate_set_type(partition_data* partition, const char* type)
{
	TRACE(("intel: pm_validate_set_type\n"));

	if (!partition || !type)
		return false;

	partition_data* parent = get_parent_partition(partition->id);
	if (!parent)
		return false;
	PrimaryPartition* child = (PrimaryPartition*)partition->cookie;
	if (!child)
		return false;

	// validity check of the type
	return is_type_valid_pm(type, parent, child);
}

// pm_validate_initialize
bool
pm_validate_initialize(partition_data* partition, char* name,
	const char* parameters)
{
	TRACE(("intel: pm_validate_initialize\n"));

	if (!partition || !(pm_get_supported_operations(partition)
			& B_DISK_SYSTEM_SUPPORTS_INITIALIZING)) {
		return false;
	}

	// name is ignored
	if (name)
		name[0] = '\0';

	// parameters are ignored, too

	return true;
}


// validate_create_child_partition (auxiliary function)
static bool
validate_create_child_partition(partition_data* partition, off_t* start,
	off_t* size, fc_get_sibling_partitions getSiblingPartitions)
{
	// make the start and size a multiple of the block size
	*start = sector_align(*start, partition->block_size);
	if (*size < 0)
		*size = 0;
	else
		*size = sector_align(*size, partition->block_size);

	// child must completely lie within the parent partition
	if (*start >= partition->offset + partition->size)
		return false;
	if (*start + *size > partition->offset + partition->size)
		*size = partition->offset + partition->size - *start;

	// new child must not intersect with sibling partitions
	// finding out sibling partitions
	partition_data* previousSibling = NULL;
	partition_data* nextSibling = NULL;
	off_t previousOffset = 0, previousSize = 0, nextOffset = 0, nextSize = 0;

	getSiblingPartitions(partition, NULL, *start, &previousSibling,
		&nextSibling, &previousOffset, &previousSize, &nextOffset, &nextSize);

	// position check of the new partition
	if (previousSibling && (previousOffset + previousSize > *start)) {
		*start = previousOffset + previousSize;
		*start = sector_align_up(*start, partition->block_size);
	}

	if (nextSibling && (nextOffset < *start + *size))
		*size = nextOffset - *start;
	*size = sector_align(*size, partition->block_size);
	if (*size == 0)
		return false;

	return true;
}


// pm_validate_create_child
/*!
	index - returns position of the new partition (first free record in MBR)
*/
bool
pm_validate_create_child(partition_data* partition, off_t* start, off_t* size,
	const char* type, const char* name, const char* parameters, int32* index)
{
	TRACE(("intel: pm_validate_create_child\n"));

	if (!partition || !(pm_get_supported_operations(partition)
			& B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD)
		|| !start || !size || !type || !index) {
		return false;
	}

	// TODO: check name
	// TODO: check parameters
	// type check
	if (!is_type_valid_pm(type, partition))
		return false;

	// finding out index of the new partition (first free record in MBR)
	// at least one record has to be free
	PartitionMap* map = (PartitionMap*)partition->content_cookie;
	if (!map)
		return false;
	int32 newIndex = -1;
	for (int32 i = 0; i < 4; i++) {
		PrimaryPartition* primary = map->PrimaryPartitionAt(i);
		if (primary->IsEmpty()) {
			newIndex = i;
			break;
		}
	}
	// this cannot happen
	if (newIndex < 0)
		return false;
	*index = newIndex;

	if (*start < partition->offset + MBR_OFFSET * partition->block_size) {
		*start = partition->offset + MBR_OFFSET * partition->block_size;
		*start = sector_align_up(*start, partition->block_size);
	}

	return validate_create_child_partition(partition, start, size,
		get_sibling_partitions_pm);
}


// cmp_partition_position
static int
cmp_partition_position(const void* o1, const void* o2)
{
	off_t offset1 = ((PartitionPosition*)o1)->offset;
	off_t offset2 = ((PartitionPosition*)o2)->offset;

	if (offset1 < offset2)
		return -1;
	if (offset1 > offset2)
		return 1;

	return 0;
}


// fill_partitionable_spaces_buffer_pm
/*!
	positions - output buffer with sufficient size
	returns partition count
*/
static int32
fill_partitionable_spaces_buffer_pm(partition_data* partition,
	PartitionPosition* positions)
{
	int32 partition_count = 0;
	for (int32 i = 0; i < partition->child_count; i++) {
		const partition_data* child = get_child_partition(partition->id, i);
		if (child) {
			positions[partition_count].offset = child->offset;
			positions[partition_count].size = child->size;
			partition_count++;
		}
	}
	return partition_count;
}


// fill_partitionable_spaces_buffer_ep
/*!
	positions - output buffer with sufficient size
	returns partition count
*/
static int32
fill_partitionable_spaces_buffer_ep(partition_data* partition,
	PartitionPosition* positions)
{
	int32 partition_count = 0;
	for (int32 i = 0; i < partition->child_count; i++) {
		const partition_data* child = get_child_partition(partition->id, i);
		if (child) {
			positions[partition_count].offset = child->offset;
			positions[partition_count].size = child->size;
			partition_count++;
		}
	}
	return partition_count;
}


// get_partitionable_spaces (auxiliary function)
static status_t
get_partitionable_spaces(partition_data* partition,
	partitionable_space_data* buffer, int32 count, int32* _actualCount,
	fc_fill_partitionable_spaces_buffer fillBuffer, off_t startOffset,
	off_t limitSize = 0, off_t headerSize = 0)
{
	PartitionPosition* positions
		= new(nothrow) PartitionPosition[partition->child_count];
	if (!positions)
		return B_NO_MEMORY;
	// fill the array
	int32 partition_count = fillBuffer(partition, positions);
	// sort the array
	qsort(positions, partition_count, sizeof(PartitionPosition),
		cmp_partition_position);

	// first sektor is MBR or EBR
	off_t offset = startOffset + headerSize;
	off_t size = 0;
	int32 actualCount = 0;

	// offset alignment (to upper bound)
	offset = sector_align_up(offset, partition->block_size);
	// finding out all partitionable spaces
	for (int32 i = 0; i < partition_count; i++) {
		size = positions[i].offset - offset;
		size = sector_align(size, partition->block_size);
		if (size >= limitSize) {
			if (actualCount < count) {
				buffer[actualCount].offset = offset;
				buffer[actualCount].size = size;
			}
			actualCount++;
		}
		offset = positions[i].offset + positions[i].size + headerSize;
		offset = sector_align_up(offset, partition->block_size);
	}
	// space in the end of partition
	size = partition->offset + partition->size - offset;
	size = sector_align(size, partition->block_size);
	if (size > 0) {
		if (actualCount < count) {
			buffer[actualCount].offset = offset;
			buffer[actualCount].size = size;
		}
		actualCount++;
	}

	// cleanup
	if (positions)
		delete[] positions;

	TRACE(("intel: get_partitionable_spaces - found: %ld\n", actualCount));

	*_actualCount = actualCount;

	if (count < actualCount)
		return B_BUFFER_OVERFLOW;
	return B_OK;
}


// pm_get_partitionable_spaces
status_t
pm_get_partitionable_spaces(partition_data* partition,
	partitionable_space_data* buffer, int32 count, int32* actualCount)
{
	TRACE(("intel: pm_get_partitionable_spaces\n"));

	if (!partition || !partition->content_type
		|| strcmp(partition->content_type, kPartitionTypeIntel)
		|| !actualCount) {
		return B_BAD_VALUE;
	}
	if (count > 0 && !buffer)
		return B_BAD_VALUE;

	return get_partitionable_spaces(partition, buffer, count, actualCount,
		fill_partitionable_spaces_buffer_pm, MBR_OFFSET * partition->block_size,
		0, 0);
}


// pm_get_next_supported_type
status_t
pm_get_next_supported_type(partition_data* partition, int32* cookie,
	char* _type)
{
	TRACE(("intel: pm_get_next_supported_type\n"));

	if (!partition || !partition->content_type
		|| strcmp(partition->content_type, kPartitionTypeIntel)
		|| !cookie || !_type) {
		return B_BAD_VALUE;
	}

	if (*cookie > 255)
		return B_ENTRY_NOT_FOUND;
	if (*cookie < 1)
		*cookie = 1;

	uint8 type = *cookie;

	// get type
	PartitionType ptype;
	ptype.SetType(type);
	if (!ptype.IsValid())
		return B_ENTRY_NOT_FOUND;

	ptype.GetTypeString(_type);

	// find next type
	if (ptype.FindNext())
		*cookie = ptype.Type();
	else
		*cookie = 256;

	return B_OK;
}

// pm_shadow_changed
status_t
pm_shadow_changed(partition_data* partition, partition_data* child,
	uint32 operation)
{
	TRACE(("intel: pm_shadow_changed(%p, %p, %lu)\n", partition, child,
		operation));

	switch (operation) {
		case B_PARTITION_SHADOW:
		{
			// get the physical partition
			partition_data* physicalPartition = get_partition(
				partition->id);
			if (!physicalPartition) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_SHADOW): no "
					"physical partition with ID %ld\n", partition->id);
				return B_ERROR;
			}

			// clone the map
			if (!physicalPartition->content_cookie) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_SHADOW): no "
					"content cookie, physical partition: %ld\n", partition->id);
				return B_ERROR;
			}

			PartitionMapCookie* map = new(nothrow) PartitionMapCookie;
			if (!map)
				return B_NO_MEMORY;

			status_t error = map->Assign(
				*(PartitionMapCookie*)physicalPartition->content_cookie);
			if (error != B_OK) {
				delete map;
				return error;
			}

			partition->content_cookie = map;

			return B_OK;
		}

		case B_PARTITION_SHADOW_CHILD:
		{
			// get the physical child partition
			partition_data* physical = get_partition(child->id);
			if (!physical) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_SHADOW_CHILD): "
					"no physical partition with ID %ld\n", child->id);
				return B_ERROR;
			}

			if (!physical->cookie) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_SHADOW_CHILD): "
					"no cookie, physical partition: %ld\n", child->id);
				return B_ERROR;
			}

			// primary partition index
			int32 index = ((PrimaryPartition*)physical->cookie)->Index();

			if (!partition->content_cookie) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_SHADOW_CHILD): "
					"no content cookie, physical partition: %ld\n",
					partition->id);
				return B_ERROR;
			}

			// get the primary partition
			PartitionMapCookie* map
				= ((PartitionMapCookie*)partition->content_cookie);
			PrimaryPartition* primary = map->PrimaryPartitionAt(index);

			if (!primary || primary->IsEmpty()) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_SHADOW_CHILD): "
					"partition %ld is empty, primary index: %ld\n", child->id,
					index);
				return B_BAD_VALUE;
			}

			child->cookie = primary;

			return B_OK;
		}

		case B_PARTITION_INITIALIZE:
		{
			// create an empty partition map
			PartitionMapCookie* map = new(nothrow) PartitionMapCookie;
			if (!map)
				return B_NO_MEMORY;

			partition->content_cookie = map;

			return B_OK;
		}

		case B_PARTITION_CREATE_CHILD:
		{
			if (!partition->content_cookie) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_CREATE_CHILD): "
					"no content cookie, partition: %ld\n", partition->id);
				return B_ERROR;
			}

			PartitionMapCookie* map
				= ((PartitionMapCookie*)partition->content_cookie);

			// find an empty primary partition slot
			PrimaryPartition* primary = NULL;
			for (int32 i = 0; i < 4; i++) {
				if (map->PrimaryPartitionAt(i)->IsEmpty()) {
					primary = map->PrimaryPartitionAt(i);
					break;
				}
			}

			if (!primary) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_CREATE_CHILD): "
					"no empty primary slot, partition: %ld\n", partition->id);
				return B_ERROR;
			}

			// apply type
			PartitionType type;
			type.SetType(child->type);
			if (!type.IsValid()) {
				dprintf("intel: pm_shadow_changed(B_PARTITION_CREATE_CHILD): "
					"invalid partition type, partition: %ld\n", partition->id);
				return B_ERROR;
			}

			primary->SetType(type.Type());

			// TODO: Apply parameters!

			child->cookie = primary;

			return B_OK;
		}

		case B_PARTITION_DEFRAGMENT:
		case B_PARTITION_REPAIR:
		case B_PARTITION_RESIZE:
		case B_PARTITION_RESIZE_CHILD:
		case B_PARTITION_MOVE:
		case B_PARTITION_MOVE_CHILD:
		case B_PARTITION_SET_NAME:
		case B_PARTITION_SET_CONTENT_NAME:
		case B_PARTITION_SET_TYPE:
		case B_PARTITION_SET_PARAMETERS:
		case B_PARTITION_SET_CONTENT_PARAMETERS:
		case B_PARTITION_DELETE_CHILD:
			break;
	}

	return B_ERROR;
}


// #pragma mark - Intel Partition Map - writing functions


// pm_resize
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
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new size
// TODO: The parameter has already been checked and must not be altered!
	off_t validatedSize = size;
	if (!pm_validate_resize(partition, &validatedSize))
		return B_BAD_VALUE;

	// update data stuctures
	update_disk_device_job_progress(job, 0.0);

// TODO: partition->size is not supposed to be touched.
	partition->size = validatedSize;
	partition->content_size = validatedSize;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


// pm_resize_child
status_t
pm_resize_child(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: pm_resize_child\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get out partition, child and partition map structure
	partition_data* partition = get_parent_partition(partitionID);
	partition_data* child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap* map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition* primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	// validate the new size
// TODO: The parameter has already been checked and must not be altered!
	off_t validatedSize = size;
	if (!pm_validate_resize_child(partition, child, &validatedSize))
		return B_BAD_VALUE;
	if (child->size == validatedSize)
		return B_OK;

	// update data stuctures and write changes
	update_disk_device_job_progress(job, 0.0);
	primary->SetSize(validatedSize);

// TODO: The partition is not supposed to be locked here!
	PartitionMapWriter writer(fd, primary->BlockSize());
		// TODO: disk size?
	status_t error = writer.WriteMBR(map, false);
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
status_t
pm_move(int fd, partition_id partitionID, off_t offset, disk_job_id job)
{
	TRACE(("intel: pm_move\n"));

	if (fd < 0)
		return B_ERROR;

// TODO: Should be a no-op!

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get out partition
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new start
	if (!pm_validate_move(partition, &offset))
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}


// allocate_buffer (auxiliary function)
/*!
	tries to allocate buffer with the size: blockSize * tryAlloc
	if it's not possible, tries smaller buffer (until the size: blockSize * 1)
	returns pointer to the buffer (it's size is: blockSize * allocated)
	or returns NULL - B_NO_MEMORY
*/
static uint8*
allocate_buffer(uint32 blockSize, int32 tryAlloc, int32* allocated)
{
	uint8* buffer = NULL;
	for (int32 i = tryAlloc; i > 1; i /= 2) {
		buffer = new(nothrow) uint8[i * blockSize];
		if (buffer) {
			*allocated = i;
			return buffer;
		}
	}
	*allocated = 0;
	return NULL;
}


// move_block (auxiliary function)
static status_t
move_block(int fd, off_t fromOffset, off_t toOffset, uint8* buffer, int32 size)
{
	status_t error = B_OK;
	// read block to buffer
	if (read_pos(fd, fromOffset, buffer, size) != size) {
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
		TRACE(("intel: move_block(): reading failed: %lx\n", error));
		return error;
	}

	// write block from buffer
	if (write_pos(fd, toOffset, buffer, size) != size) {
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
		TRACE(("intel: move_block(): writing failed: %lx\n", error));
	}

	return error;
}


// move_partition (auxiliary function)
static status_t
move_partition(int fd, off_t fromOffset, off_t toOffset, off_t size,
	uint8* buffer, int32 buffer_size, disk_job_id job)
{
	// TODO: This should be a service function of the DDM!
	// TODO: This seems to be broken if source and destination overlap.
	status_t error = B_OK;
	off_t cycleCount = size / buffer_size;
	int32 remainingSize = size - cycleCount * buffer_size;
	update_disk_device_job_progress(job, 0.0);
	for (off_t i = 0; i < cycleCount; i++) {
		error = move_block(fd, fromOffset, toOffset, buffer, buffer_size);
		if (error != B_OK)
			return error;
		fromOffset += buffer_size;
		toOffset += buffer_size;
		update_disk_device_job_progress(job, (float)i / cycleCount);
	}
	if (remainingSize)
		error = move_block(fd, fromOffset, toOffset, buffer, remainingSize);
	update_disk_device_job_progress(job, 1.0);
	return error;
}


// pm_move_child
status_t
pm_move_child(int fd, partition_id partitionID, partition_id childID,
	off_t offset, disk_job_id job)
{
	TRACE(("intel: pm_move_child\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition, child and partition map structure
	partition_data* partition = get_partition(partitionID);
	partition_data* child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap* map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition* primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	// TODO: The parameter has already been checked and must not be altered!
	off_t validatedOffset = offset;
	if (!pm_validate_move_child(partition, child, &validatedOffset))
		return B_BAD_VALUE;

	// if the old offset is the same, there is nothing to do
	if (child->offset == validatedOffset)
		return B_OK;

	// buffer allocation
	int32 allocated;
	uint8* buffer = allocate_buffer(partition->block_size, MAX_MOVE_BUFFER,
		&allocated);
	if (!buffer)
		return B_NO_MEMORY;

	// partition moving
	// TODO: The partition is not supposed to be locked at this point!
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

	PartitionMapWriter writer(fd, partition->block_size);
		// TODO: disk size?
	error = writer.WriteMBR(map, false);
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
status_t
pm_set_type(int fd, partition_id partitionID, const char* type, disk_job_id job)
{
	TRACE(("intel: pm_set_type\n"));

	if (fd < 0 || !type)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get parent partition, child and partition map structure
	partition_data* partition = get_parent_partition(partitionID);
	partition_data* child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	PartitionMap* map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition* primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

// TODO: The parameter has already been checked and must not be altered!
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
	// TODO: Incompatible return value!

	// setting type to the partition
	update_disk_device_job_progress(job, 0.0);
	uint8 oldType = primary->Type();
	primary->SetType(ptype.Type());

	// TODO: The partition is not supposed to be locked at this point!
	PartitionMapWriter writer(fd, primary->BlockSize());
		// TODO: disk size?
	status_t error = writer.WriteMBR(map, false);
	if (error != B_OK) {
		// something went wrong - putting into previous state
		primary->SetType(oldType);
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


// pm_initialize
status_t
pm_initialize(int fd, partition_id partitionID, const char* name,
	const char* parameters, off_t partitionSize, disk_job_id job)
{
	TRACE(("intel: pm_initialize\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition and partition map structure
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;
	update_disk_device_job_progress(job, 0.0);

	// we will write an empty partition map
	PartitionMap map;

	// write the sector to disk
	PartitionMapWriter writer(fd, partition->block_size);
		// TODO: disk size or 2 * SECTOR_SIZE?
	status_t error = writer.WriteMBR(&map, true);
	if (error != B_OK)
		return error;

	// rescan partition
	error = scan_partition(partitionID);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);

	return B_OK;
}


status_t
pm_uninitialize(int fd, partition_id partitionID, off_t partitionSize,
	disk_job_id job)
{
	// get partition's block size
	size_t blockSize;
	{
		PartitionReadLocker locker(partitionID);
		if (!locker.IsLocked())
			return B_ERROR;

		partition_data* partition = get_partition(partitionID);
		if (partition == NULL)
			return B_BAD_VALUE;
		update_disk_device_job_progress(job, 0.0);

		blockSize = partition->block_size;
		if (blockSize == 0)
			return B_BAD_VALUE;
	}

	// We overwrite the first block, which contains the partition table.
	// Allocate a buffer, we can clear and write.
	void* block = malloc(blockSize);
	if (block == NULL)
		return B_NO_MEMORY;
	MemoryDeleter blockDeleter(block);

	memset(block, 0, blockSize);

	if (write_pos(fd, 0, block, blockSize) < 0)
		return errno;

	update_disk_device_job_progress(job, 1.0);

	return B_OK;
}


// pm_create_child
/*!	childID is used for the return value, but is also an optional input
	parameter -- -1 to be ignored
*/
status_t
pm_create_child(int fd, partition_id partitionID, off_t offset, off_t size,
	const char* type, const char* name, const char* parameters,
	disk_job_id job, partition_id* childID)
{
	TRACE(("intel: pm_create_child\n"));

	if (fd < 0 || !childID)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition and partition map structure
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;
	PartitionMap* map = (PartitionMap*)partition->content_cookie;
	if (!map)
		return B_BAD_VALUE;

	// validate the offset, size and get index of the new partition
	// TODO: The parameters have already been checked and must not be altered!
	off_t validatedOffset = offset;
	off_t validatedSize = size;
	int32 index = 0;

	if (!pm_validate_create_child(partition, &validatedOffset, &validatedSize,
			type, name, parameters, &index)) {
		return B_BAD_VALUE;
	}

	// finding out free primary partition in the map (index from
	// pm_validate_create_child)
	PrimaryPartition* primary = map->PrimaryPartitionAt(index);
	if (!primary->IsEmpty())
		return B_BAD_DATA;

	// creating partition
	update_disk_device_job_progress(job, 0.0);
	partition_data* child = create_child_partition(partition->id, index,
		validatedOffset, validatedSize, *childID);
	if (!child)
		return B_ERROR;

	PartitionType ptype;
	ptype.SetType(type);

	// check parameters
	void* handle = parse_driver_settings_string(parameters);
	if (handle == NULL)
		return B_ERROR;

	bool active = get_driver_boolean_parameter(handle, "active", false, true);
	delete_driver_settings(handle);

	// set the active flags to false
	if (active) {
		for (int i = 0; i < 4; i++) {
			PrimaryPartition* partition = map->PrimaryPartitionAt(i);
			partition->SetActive(false);
		}
	}

	primary->SetPartitionTableOffset(0);
	primary->SetOffset(validatedOffset);
	primary->SetSize(validatedSize);
	primary->SetType(ptype.Type());
	primary->SetActive(active);

	// write changes to disk
	PartitionMapWriter writer(fd, primary->BlockSize());

	// TODO: The partition is not supposed to be locked at this point!
	status_t error = writer.WriteMBR(map, false);
	if (error != B_OK) {
		// putting into previous state
		primary->Unset();
		delete_partition(child->id);
		return error;
	}

	*childID = child->id;

	child->block_size = primary->BlockSize();
	// (no name)
	child->type = strdup(type);
	// parameters
	child->parameters = strdup(parameters);
	child->cookie = primary;
	// check for allocation problems
	if (!child->type || !child->parameters)
		return B_NO_MEMORY;

	// rescan partition if needed
	if (strcmp(type, INTEL_EXTENDED_PARTITION_NAME) == 0) {
		writer.ClearExtendedHead(primary);
		error = scan_partition(partitionID);
		if (error != B_OK)
			return error;
	}

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


// pm_delete_child
status_t
pm_delete_child(int fd, partition_id partitionID, partition_id childID,
	disk_job_id job)
{
	TRACE(("intel: pm_delete_child\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	partition_data* child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;

	PartitionMap* map = (PartitionMap*)partition->content_cookie;
	PrimaryPartition* primary = (PrimaryPartition*)child->cookie;
	if (!map || !primary)
		return B_BAD_VALUE;

	// deleting child
	update_disk_device_job_progress(job, 0.0);
	if (!delete_partition(childID))
		return B_ERROR;
	primary->Unset();

	// write changes to disk
	PartitionMapWriter writer(fd, primary->BlockSize());
		// TODO: disk size or 2 * SECTOR_SIZE?
	// TODO: The partition is not supposed to be locked at this point!
	status_t error = writer.WriteMBR(map, false);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


// #pragma mark - Intel Extended Partition - support functions


// ep_get_supported_operations
uint32
ep_get_supported_operations(partition_data* partition, uint32 mask)
{
	uint32 flags = B_DISK_SYSTEM_SUPPORTS_RESIZING
		| B_DISK_SYSTEM_SUPPORTS_MOVING
		| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS;

	// initializing
	if (partition_data* parent = get_parent_partition(partition->id)) {
		if (partition->type
			&& strcmp(partition->type, kPartitionTypeIntelExtended) == 0
			&& strcmp(parent->content_type, kPartitionTypeIntel) == 0) {
			flags |= B_DISK_SYSTEM_SUPPORTS_INITIALIZING;
		}
	}

	// creating child
	int32 countSpaces = 0;
	if (ep_get_partitionable_spaces(partition, NULL, 0, &countSpaces)
			== B_BUFFER_OVERFLOW
		&& countSpaces > 0) {
		flags |= B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
	}

	return flags;
}


// ep_get_supported_child_operations
uint32
ep_get_supported_child_operations(partition_data* partition,
	partition_data* child, uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
		| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


// ep_is_sub_system_for
bool
ep_is_sub_system_for(partition_data* partition)
{
	if (partition == NULL)
		return false;

	TRACE(("intel: ep_is_sub_system_for(%ld: %lld, %lld, %ld, %s)\n",
		partition->id, partition->offset, partition->size,
		partition->block_size, partition->content_type));

	// Intel Extended Partition can live in child partition of Intel Partition
	// Map
	return partition->content_type
		&& !strcmp(partition->content_type, kPartitionTypeIntel);
}


// #pragma mark - Intel Extended Partition - validate functions


// ep_validate_resize
bool
ep_validate_resize(partition_data* partition, off_t* size)
{
	TRACE(("intel: ep_validate_resize\n"));

	if (!partition || !size)
		return false;

	return validate_resize(partition, size);
}


// ep_validate_resize_child
bool
ep_validate_resize_child(partition_data* partition, partition_data* child,
	off_t* _size)
{
	TRACE(("intel: ep_validate_resize_child\n"));

	if (!partition || !child || !_size)
		return false;

	// validate position
	off_t size = *_size;
	if (!validate_resize_child(partition, child, child->offset,
		 child->size, &size, get_sibling_partitions_ep))
		return false;
	*_size = size;
	return true;
}


// ep_validate_move
bool
ep_validate_move(partition_data* partition, off_t* start)
{
	TRACE(("intel: ep_validate_move\n"));

	if (!partition || !start)
		return false;
	// nothing to do here
	return true;
}


// ep_validate_move_child
bool
ep_validate_move_child(partition_data* partition, partition_data* child,
	off_t* _start)
{
	TRACE(("intel: ep_validate_move_child\n"));

	if (!partition || !child || !_start)
		return false;
	if (*_start == child->offset)
		return true;

	// validate position
	off_t start = *_start;
	if (!validate_move_child(partition, child, child->offset,
		child->size, &start, get_sibling_partitions_ep))
		return false;
	*_start = start;
	return true;
}


// is_type_valid_ep (auxiliary function)
static inline bool
is_type_valid_ep(const char* type)
{
	// validity check of the type - it has to be known
	PartitionType ptype;
	ptype.SetType(type);
	return (ptype.IsValid() && !ptype.IsEmpty() && !ptype.IsExtended());
}


// ep_validate_set_type
bool
ep_validate_set_type(partition_data* partition, const char* type)
{
	TRACE(("intel: ep_validate_set_type\n"));

	if (!partition || !type)
		return false;

	// validity check of the type
	return is_type_valid_ep(type);
}


// ep_validate_initialize
bool
ep_validate_initialize(partition_data* partition, char* name,
	const char* parameters)
{
	TRACE(("intel: ep_validate_initialize\n"));

	if (!partition || !(ep_get_supported_operations(partition)
			& B_DISK_SYSTEM_SUPPORTS_INITIALIZING)) {
		return false;
	}
	// name is ignored - we cannot set it to the Intel Extended Partition
	// TODO: check parameters - don't know whether any parameters could be set
	//		 to the Intel Extended Partition
	return true;
}


// ep_validate_create_child
bool
ep_validate_create_child(partition_data* partition, off_t* offset, off_t* size,
	const char* type, const char* name, const char* parameters, int32* index)
	// index - returns position of the new partition (the last one)
{
	return false;
}


// ep_get_partitionable_spaces
status_t
ep_get_partitionable_spaces(partition_data* partition,
	partitionable_space_data* buffer, int32 count, int32* actualCount)
{
	TRACE(("intel: ep_get_partitionable_spaces\n"));

	if (!partition || !partition->content_type
		|| strcmp(partition->content_type, kPartitionTypeIntelExtended)
		|| !actualCount) {
		return B_BAD_VALUE;
	}
	if (count > 0 && !buffer)
		return B_BAD_VALUE;

	return get_partitionable_spaces(partition, buffer, count, actualCount,
		fill_partitionable_spaces_buffer_ep,
		partition->offset + PTS_OFFSET * partition->block_size,
		PTS_OFFSET * partition->block_size,
		PTS_OFFSET * partition->block_size);
}


// ep_get_next_supported_type
status_t
ep_get_next_supported_type(partition_data* partition, int32* cookie,
	char* _type)
{
	TRACE(("intel: ep_get_next_supported_type\n"));

	if (!partition || !partition->content_type
		|| strcmp(partition->content_type, kPartitionTypeIntelExtended)
		|| !cookie || !_type) {
		return B_BAD_VALUE;
	}

	if (*cookie > 255)
		return B_ENTRY_NOT_FOUND;
	if (*cookie < 1)
		*cookie = 1;

	uint8 type = *cookie;

	// get type
	PartitionType ptype;
	ptype.SetType(type);
	while (ptype.IsValid() && !ptype.IsExtended())
		ptype.FindNext();

	if (!ptype.IsValid())
		return B_ENTRY_NOT_FOUND;

	ptype.GetTypeString(_type);

	// find next type
	if (ptype.FindNext())
		*cookie = ptype.Type();
	else
		*cookie = 256;

	return B_OK;
}


// ep_shadow_changed
status_t
ep_shadow_changed(partition_data* partition, partition_data* child,
	uint32 operation)
{
	TRACE(("intel: ep_shadow_changed\n"));

	if (!partition)
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}


bool
check_partition_location_ep(partition_data* partition, off_t offset,
	off_t size, off_t ptsOffset)
{
	if (!partition)
		return false;

	// make sure we are sector aligned
	off_t alignedOffset = sector_align(offset, partition->block_size);
	if (alignedOffset != offset)
		return false;

	// partition does not lie within extended partition
	if (offset < partition->offset
		|| (offset > partition->offset + partition->size
		&& offset + size <= partition->offset + partition->size))
		return false;

	// check if the new partition table is within an existing partition
	// or that the new partition does not overwrite an existing partition
	// table.
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* sibling = get_child_partition(partition->id, i);
		LogicalPartition* logical = (LogicalPartition*)sibling->cookie;
		if (logical == NULL)
			return false;
		if (ptsOffset > logical->Offset()
			&& ptsOffset < logical->Offset() + logical->Size())
			return false;
		if ((logical->PartitionTableOffset() >= offset
			&& logical->PartitionTableOffset() < offset + size)
			|| logical->PartitionTableOffset() == ptsOffset)
			return false;
	}

	return true;
}


// #pragma mark - Intel Extended Partition - write functions


// ep_resize
status_t
ep_resize(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: ep_resize\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get out partition
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new size
	// TODO: The parameter has already been checked and must not be altered!
	off_t validatedSize = size;
	if (!ep_validate_resize(partition, &validatedSize))
		return B_BAD_VALUE;

	// update data stuctures
	update_disk_device_job_progress(job, 0.0);

	// TODO: partition->size is not supposed to be touched.
	partition->size = validatedSize;
	partition->content_size = validatedSize;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


// ep_resize_child
status_t
ep_resize_child(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	TRACE(("intel: ep_resize_child\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get out partition, child and LogicalPartition structure
	partition_data* partition = get_parent_partition(partitionID);
	partition_data* child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	LogicalPartition* logical = (LogicalPartition*)child->cookie;
	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	if (!logical || !primary)
		return B_BAD_VALUE;

	// validate the new size
	// TODO: The parameter has already been checked and must not be altered!
	off_t validatedSize = size;
	if (!ep_validate_resize_child(partition, child, &validatedSize))
		return B_BAD_VALUE;
	if (child->size == validatedSize)
		return B_OK;

	// update data stuctures and write changes
	update_disk_device_job_progress(job, 0.0);
	logical->SetSize(validatedSize);

	PartitionMapWriter writer(fd, partition->block_size);
	// TODO: The partition is not supposed to be locked here!
	status_t error = writer.WriteLogical(logical, primary, false);
	if (error != B_OK) {
		// putting into previous state
		logical->SetSize(child->size);
		return error;
	}
	LogicalPartition* prev = logical->Previous();
	error = prev ? writer.WriteLogical(prev, primary, false)
				 : writer.WriteLogical(logical, primary, false);
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
status_t
ep_move(int fd, partition_id partitionID, off_t offset, disk_job_id job)
{
	TRACE(("intel: ep_move\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get out partition
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	// validate the new start
	// TODO: The parameter has already been checked and must not be altered!
	if (!ep_validate_move(partition, &offset))
		return B_BAD_VALUE;

	// nothing to do here
	return B_OK;
}


// ep_move_child
status_t
ep_move_child(int fd, partition_id partitionID, partition_id childID,
	off_t offset, disk_job_id job)
{
	TRACE(("intel: ep_move_child\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition, child and LogicalPartition structure
	partition_data* partition = get_partition(partitionID);
	partition_data* child = get_partition(childID);
	if (!partition || !child)
		return B_BAD_VALUE;
	LogicalPartition* logical = (LogicalPartition*)child->cookie;
	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	if (!logical || !primary)
		return B_BAD_VALUE;

	// TODO: The parameter has already been checked and must not be altered!
	off_t validatedOffset = offset;
	if (!ep_validate_move_child(partition, child, &validatedOffset))
		return B_BAD_VALUE;

	// if the old offset is the same, there is nothing to do
	if (child->offset == validatedOffset)
		return B_OK;

	off_t diffOffset = validatedOffset - child->offset;

	// buffer allocation
	int32 allocated;
	uint8* buffer = allocate_buffer(partition->block_size, MAX_MOVE_BUFFER,
		&allocated);
	if (!buffer)
		return B_NO_MEMORY;

	// partition moving
	update_disk_device_job_progress(job, 0.0);
	status_t error = B_OK;
	// move partition with its header (partition table)
	off_t pts_offset = logical->Offset() - logical->PartitionTableOffset();
	error = move_partition(fd, child->offset - pts_offset,
		validatedOffset - pts_offset, child->size + pts_offset, buffer,
		allocated * partition->block_size, job);
	delete[] buffer;
	if (error != B_OK)
		return error;

	// partition moved
	// updating data structure
	child->offset = validatedOffset;
	logical->SetOffset(logical->Offset() + diffOffset);
	logical->SetPartitionTableOffset(logical->PartitionTableOffset() + diffOffset);

	PartitionMapWriter writer(fd, partition->block_size);
		// TODO: If partition->offset is > prev->offset, then writing
		// the previous logical partition table will fail!
	// TODO: The partition is not supposed to be locked here!
	error = writer.WriteLogical(logical, primary, false);
	if (error != B_OK)
		// something went wrong - this is fatal (partition has been moved)
		// but EBR is not updated
		return error;
	LogicalPartition* prev = logical->Previous();
	error = prev ? writer.WriteLogical(prev, primary, false)
				 : writer.WriteLogical(logical, primary, false);
	if (error != B_OK)
		// this is fatal - linked list is not updated
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(childID);
	return B_OK;
}


// ep_set_type
status_t
ep_set_type(int fd, partition_id partitionID, const char* type, disk_job_id job)
{
	TRACE(("intel: ep_set_type\n"));

	if (fd < 0 || !type)
		return B_BAD_VALUE;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition, child and LogicalPartition structure
	partition_data* partition = get_parent_partition(partitionID);
	partition_data* child = get_partition(partitionID);
	if (!partition || !child)
		return B_BAD_VALUE;
	LogicalPartition* logical = (LogicalPartition*)child->cookie;
	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	if (!logical || !primary)
		return B_BAD_VALUE;

	// TODO: The parameter has already been checked and must not be altered!
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
	uint8 oldType = logical->Type();
	logical->SetType(ptype.Type());

	PartitionMapWriter writer(fd, partition->block_size);
	// TODO: The partition is not supposed to be locked here!
	status_t error = writer.WriteLogical(logical, primary, false);
	if (error != B_OK) {
		// something went wrong - putting into previous state
		logical->SetType(oldType);
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
status_t
ep_initialize(int fd, partition_id partitionID, const char* name,
	const char* parameters, off_t partitionSize, disk_job_id job)
{
	TRACE(("intel: ep_initialize\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition
	partition_data* partition = get_partition(partitionID);
	if (!partition)
		return B_BAD_VALUE;

	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	if (!primary)
		return B_BAD_VALUE;

	// name is ignored - we cannot set it to the Intel Extended Partition
// TODO: The parameter has already been checked and must not be altered!
	if (!ep_validate_initialize(partition, NULL, parameters))
		return B_BAD_VALUE;

	// partition init (we have no child partition)
	update_disk_device_job_progress(job, 0.0);
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)
	partition->content_cookie = primary;

	// we delete code area in EBR - nothing should be there
	partition_table table;
	table.clear_code_area();

	PartitionMapWriter writer(fd, partition->block_size);
	// TODO: The partition is not supposed to be locked here!
	status_t error = writer.ClearExtendedHead(primary);
	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


// ep_create_child
/*!
	childID is used for the return value, but is also an optional input
	parameter -- -1 to be ignored
*/
status_t
ep_create_child(int fd, partition_id partitionID, off_t offset, off_t size,
	const char* type, const char* name, const char* parameters, disk_job_id job,
	partition_id* childID)
{
	TRACE(("intel: ep_create_child\n"));

	if (fd < 0 || !childID)
		return B_BAD_VALUE;

	// aquire lock
	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	// get partition data
	partition_data* partition = get_partition(partitionID);
	partition_data* parent = get_parent_partition(partitionID);
	if (partition == NULL || parent == NULL)
		return B_BAD_VALUE;

	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	if (!primary)
		return B_BAD_VALUE;

	// parse parameters
	void* handle = parse_driver_settings_string(parameters);
	if (handle == NULL)
		return B_ERROR;

	bool active = get_driver_boolean_parameter(handle, "active", false, true);

	off_t ptsOffset = 0;
	const char* buffer = get_driver_parameter(
		handle, "partition_table_offset", NULL, NULL);
	if (buffer != NULL)
		ptsOffset = strtoull(buffer, NULL, 10);
	else {
		delete_driver_settings(handle);
		return B_BAD_VALUE;
	}
	delete_driver_settings(handle);

	// check the partition location
	if (!check_partition_location_ep(partition, offset, size, ptsOffset))
		return B_BAD_VALUE;

	// creating partition
	update_disk_device_job_progress(job, 0.0);
	partition_data* child = create_child_partition(partition->id,
		partition->child_count, offset, size, *childID);
	if (!child)
		return B_ERROR;

	// setup logical partition
	LogicalPartition* logical = new(nothrow) LogicalPartition;
	if (!logical)
		return B_NO_MEMORY;

	PartitionType ptype;
	ptype.SetType(type);
	logical->SetPartitionTableOffset(ptsOffset - parent->offset);
	logical->SetOffset(offset);
	logical->SetSize(size);
	logical->SetType(ptype.Type());
	logical->SetActive(active);
	logical->SetPrimaryPartition(primary);
	logical->SetBlockSize(partition->block_size);
	primary->AddLogicalPartition(logical);

	int parentFD = open_partition(parent->id, O_RDWR);
	if (parentFD < 0) {
		primary->RemoveLogicalPartition(logical);
		delete logical;
		return B_IO_ERROR;
	}

	// write changes to disk
	PartitionMapWriter writer(parentFD, primary->BlockSize());

	// Write the logical partition's EBR first in case of failure.
	// This way we will not add a partition to the previous logical
	// partition. If there is no previous logical partition then write
	// the current partition's EBR to the first sector of the primary partition
	status_t error = writer.WriteLogical(logical, primary, true);
	if (error != B_OK) {
		primary->RemoveLogicalPartition(logical);
		delete logical;
		return error;
	}

	LogicalPartition* previous = logical->Previous();
	if (previous != NULL) {
		error = writer.WriteLogical(previous, primary, true);
		if (error != B_OK) {
			primary->RemoveLogicalPartition(logical);
			delete logical;
			return error;
		}
	}
	*childID = child->id;

	child->block_size = logical->BlockSize();
	child->type = strdup(type);
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
status_t
ep_delete_child(int fd, partition_id partitionID, partition_id childID,
	disk_job_id job)
{
	TRACE(("intel: ep_delete_child\n"));

	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	partition_data* parent = get_parent_partition(partitionID);
	partition_data* child = get_partition(childID);
	if (partition == NULL || parent == NULL || child == NULL)
		return B_BAD_VALUE;

	PrimaryPartition* primary = (PrimaryPartition*)partition->cookie;
	LogicalPartition* logical = (LogicalPartition*)child->cookie;
	if (primary == NULL || logical == NULL)
		return B_BAD_VALUE;

	// deleting child
	update_disk_device_job_progress(job, 0.0);
	if (!delete_partition(childID))
		return B_ERROR;

	LogicalPartition* previous = logical->Previous();
	LogicalPartition* next = logical->Next();

	primary->RemoveLogicalPartition(logical);
	delete logical;

	int parentFD = open_partition(parent->id, O_RDWR);
	if (parentFD < 0)
		return B_IO_ERROR;

	// write changes to disk
	PartitionMapWriter writer(parentFD, primary->BlockSize());

	status_t error;
	if (previous != NULL) {
		error = writer.WriteLogical(previous, primary, true);
	} else {
		error = writer.WriteExtendedHead(next, primary, true);

		if (next != NULL) {
			next->SetPartitionTableOffset(primary->Offset());

			partition_data* nextSibling = NULL;
			if (get_partition_from_offset_ep(partition, next->Offset(),
				&nextSibling)) {
				char buffer[128];
				sprintf(buffer, "active %s ;\npartition_table_offset %lld ;\n",
					next->Active() ? "true" : "false",
					next->PartitionTableOffset());
				nextSibling->parameters = strdup(buffer);
			}
		}
	}

	close(parentFD);

	if (error != B_OK)
		return error;

	// all changes applied
	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}

