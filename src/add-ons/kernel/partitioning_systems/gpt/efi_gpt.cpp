/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include "efi_gpt.h"

#include <KernelExport.h>
#include <disk_device_manager/ddm_modules.h>
#include <disk_device_types.h>
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
#	include "PartitionLocker.h"
#endif
#include <util/kernel_cpp.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#ifndef _BOOT_MODE
#include "uuid.h"
#endif

#include "Header.h"
#include "utility.h"


#define TRACE_EFI_GPT
#ifdef TRACE_EFI_GPT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define EFI_PARTITION_MODULE_NAME "partitioning_systems/efi_gpt/v1"


#ifndef _BOOT_MODE
static off_t
block_align(partition_data* partition, off_t offset, bool upwards)
{
	// Take HDs into account that hide the fact they are using a
	// block size of 4096 bytes, and round to that.
	uint32 blockSize = max_c(partition->block_size, 4096);
	if (upwards)
		return ((offset + blockSize - 1) / blockSize) * blockSize;

	return (offset / blockSize) * blockSize;
}
#endif // !_BOOT_MODE


//	#pragma mark - public module interface


static status_t
efi_gpt_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


static float
efi_gpt_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	EFI::Header* header = new (std::nothrow) EFI::Header(fd,
		(partition->size - 1) / partition->block_size, partition->block_size);
	status_t status = header->InitCheck();
	if (status != B_OK) {
		delete header;
		return -1;
	}

	*_cookie = header;
	if (header->IsDirty()) {
		// Either the main or the backup table is missing, it looks like someone
		// tried to erase the GPT with something else. Let's lower the priority,
		// so that other partitioning systems which use either only the start or
		// only the end of the drive, have a chance to run instead.
		return 0.75;
	}
	return 0.96;
		// This must be higher as Intel partitioning, as EFI can contain this
		// partitioning for compatibility
}


static status_t
efi_gpt_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	TRACE(("efi_gpt_scan_partition(cookie = %p)\n", _cookie));
	EFI::Header* header = (EFI::Header*)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	partition->content_cookie = header;

	// scan all children

	uint32 index = 0;

	for (uint32 i = 0; i < header->EntryCount(); i++) {
		const efi_partition_entry& entry = header->EntryAt(i);

		if (entry.partition_type == kEmptyGUID)
			continue;

		if (entry.EndBlock() * partition->block_size
				> (uint64)partition->size) {
			TRACE(("efi_gpt: child partition exceeds existing space (ends at "
				"block %" B_PRIu64 ")\n", entry.EndBlock()));
			continue;
		}

		if (entry.StartBlock() * partition->block_size == 0) {
			TRACE(("efi_gpt: child partition starts at 0 (recursive entry)\n"));
			continue;
		}

		partition_data* child = create_child_partition(partition->id, index++,
			partition->offset + entry.StartBlock() * partition->block_size,
			entry.BlockCount() * partition->block_size, -1);
		if (child == NULL) {
			TRACE(("efi_gpt: Creating child at index %" B_PRIu32 " failed\n",
				index - 1));
			return B_ERROR;
		}

		char name[B_OS_NAME_LENGTH];
		to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, name, sizeof(name));
		child->name = strdup(name);
		child->type = strdup(get_partition_type(entry.partition_type));
		child->block_size = partition->block_size;
		child->cookie = (void*)(addr_t)i;
		child->content_cookie = header;
	}

	return B_OK;
}


static void
efi_gpt_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	// Cookie is freed in efi_gpt_free_partition_content_cookie().
}


static void
efi_gpt_free_partition_content_cookie(partition_data* partition)
{
	delete (EFI::Header*)partition->content_cookie;
}


#ifndef _BOOT_MODE
static uint32
efi_gpt_get_supported_operations(partition_data* partition, uint32 mask)
{
	uint32 flags = B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_MOVING
		| B_DISK_SYSTEM_SUPPORTS_RESIZING
		| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
		// TODO: check for available entries and partitionable space and only
		// add creating child support if both is valid

	return flags;
}


static uint32
efi_gpt_get_supported_child_operations(partition_data* partition,
	partition_data* child, uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
		| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


static bool
efi_gpt_is_sub_system_for(partition_data* partition)
{
	// a GUID Partition Table doesn't usually live inside another partition
	return false;
}


static bool
efi_gpt_validate_resize(partition_data* partition, off_t* size)
{
	off_t newSize = *size;
	if (newSize == partition->size)
		return true;

	if (newSize < 0)
		newSize = 0;
	else
		newSize = block_align(partition, newSize, false);

	// growing
	if (newSize > partition->size) {
		*size = newSize;
		return true;
	}

	// shrinking, only so that no child would be truncated
	off_t newEnd = partition->offset + newSize;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* child = get_child_partition(partition->id, i);
		if (child == NULL)
			continue;

		if (child->offset + child->size > newEnd)
			newEnd = child->offset + child->size;
	}

	newSize = block_align(partition, newEnd - partition->offset, true);
	*size = newSize;
	return true;
}


static bool
efi_gpt_validate_resize_child(partition_data* partition, partition_data* child,
	off_t* size)
{
	off_t newSize = *size;
	if (newSize == child->size)
		return true;

	// shrinking
	if (newSize < child->size) {
		if (newSize < 0)
			newSize = 0;

		*size = block_align(partition, newSize, false);
		return true;
	}

	// growing, but only so much that the child doesn't get bigger than
	// the parent
	if (child->offset + newSize > partition->offset + partition->size)
		newSize = partition->offset + partition->size - child->offset;

	// make sure that the child doesn't overlap any sibling partitions
	off_t newEnd = child->offset + newSize;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* other = get_child_partition(partition->id, i);
		if (other == NULL || other->id == child->id
			|| other->offset < child->offset)
			continue;

		if (newEnd > other->offset)
			newEnd = other->offset;
	}

	*size = block_align(partition, newEnd - child->offset, false);
	return true;
}


static bool
efi_gpt_validate_move(partition_data* partition, off_t* start)
{
	// nothing to do
	return true;
}


static bool
efi_gpt_validate_move_child(partition_data* partition, partition_data* child,
	off_t* start)
{
	off_t newStart = *start;
	if (newStart < 0)
		newStart = 0;

	if (newStart + child->size > partition->size)
		newStart = partition->size - child->size;

	newStart = block_align(partition, newStart, false);
	if (newStart > child->offset) {
		for (int32 i = 0; i < partition->child_count; i++) {
			partition_data* other = get_child_partition(partition->id, i);
			if (other == NULL || other->id == child->id
				|| other->offset < child->offset)
				continue;

			if (other->offset < newStart + child->size)
				newStart = other->offset - child->size;
		}

		newStart = block_align(partition, newStart, false);
	} else {
		for (int32 i = 0; i < partition->child_count; i++) {
			partition_data* other = get_child_partition(partition->id, i);
			if (other == NULL || other->id == child->id
				|| other->offset > child->offset)
				continue;

			if (other->offset + other->size > newStart)
				newStart = other->offset + other->size;
		}

		newStart = block_align(partition, newStart, true);
	}

	*start = newStart;
	return true;
}


static bool
efi_gpt_validate_set_name(partition_data* partition, char* name)
{
	// TODO: should validate that the utf-8 -> ucs-2 is valid
	// TODO: should count actual utf-8 chars
	if (strlen(name) > EFI_PARTITION_NAME_LENGTH)
		name[EFI_PARTITION_NAME_LENGTH - 1] = 0;
	return true;
}


static bool
efi_gpt_validate_set_type(partition_data* partition, const char* type)
{
	guid_t typeGUID;
	return get_guid_for_partition_type(type, typeGUID);
}


static bool
efi_gpt_validate_initialize(partition_data* partition, char* name,
	const char* parameters)
{
	if ((efi_gpt_get_supported_operations(partition, ~0)
		& B_DISK_SYSTEM_SUPPORTS_INITIALIZING) == 0)
		return false;

	// name and parameters are ignored
	if (name != NULL)
		name[0] = 0;

	return true;
}


static bool
efi_gpt_validate_create_child(partition_data* partition, off_t* start,
	off_t* size, const char* type, const char* name, const char* parameters,
	int32* index)
{
	if ((efi_gpt_get_supported_operations(partition, ~0)
			& B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD) == 0)
		return false;

	if (!efi_gpt_validate_set_type(partition, type))
		return false;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	int32 entryIndex = -1;
	for (uint32 i = 0; i < header->EntryCount(); i++) {
		const efi_partition_entry& entry = header->EntryAt(i);
		if (entry.partition_type == kEmptyGUID) {
			entryIndex = i;
			break;
		}
	}

	if (entryIndex < 0)
		return false;

	*index = entryIndex;

	// ensure that child lies between first and last usable block
	off_t firstUsable = header->FirstUsableBlock() * partition->block_size;
	if (*start < firstUsable)
		*start = firstUsable;

	off_t lastUsable = header->LastUsableBlock() * partition->block_size;
	if (*start + *size > lastUsable) {
		if (*start > lastUsable)
			return false;

		*size = lastUsable - *start;
	}

	// ensure that we don't overlap any siblings
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data* other = get_child_partition(partition->id, i);
		if (other == NULL)
			continue;

		if (other->offset < *start && other->offset + other->size > *start)
			*start = other->offset + other->size;

		if (other->offset > *start && other->offset < *start + *size)
			*size = other->offset - *start;
	}

	*start = block_align(partition, *start, true);
	*size = block_align(partition, *size, false);

	// TODO: support parameters
	return true;
}


static status_t
efi_gpt_get_partitionable_spaces(partition_data* partition,
	partitionable_space_data* buffer, int32 count, int32* actualCount)
{
	// TODO: implement
	return B_ERROR;
}


static status_t
efi_gpt_get_next_supported_type(partition_data* partition, int32* cookie,
	char* type)
{
	// TODO: implement
	return B_ERROR;
}


static status_t
efi_gpt_shadow_changed(partition_data* partition, partition_data* child,
	uint32 operation)
{
	// TODO: implement
	return B_ERROR;
}


static status_t
efi_gpt_repair(int fd, partition_id partition, bool checkOnly, disk_job_id job)
{
	// TODO: implement, validate CRCs and restore from backup area if corrupt
	return B_ERROR;
}


static status_t
efi_gpt_resize(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	off_t validatedSize = size;
	if (!efi_gpt_validate_resize(partition, &validatedSize))
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	partition->size = validatedSize;
	partition->content_size = validatedSize;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_resize_child(int fd, partition_id partitionID, off_t size,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* child = get_partition(partitionID);
	if (child == NULL)
		return B_BAD_VALUE;

	partition_data* partition = get_parent_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)(addr_t)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	off_t validatedSize = size;
	if (!efi_gpt_validate_resize_child(partition, child, &validatedSize))
		return B_BAD_VALUE;

	if (child->size == validatedSize)
		return B_OK;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry& entry = header->EntryAt(entryIndex);
	entry.SetBlockCount(validatedSize / partition->block_size);

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK) {
		entry.SetBlockCount(child->size / partition->block_size);
		return result;
	}

	child->size = validatedSize;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_move(int fd, partition_id partition, off_t offset, disk_job_id job)
{
	// nothing to do here
	return B_OK;
}


static status_t
efi_gpt_move_child(int fd, partition_id partitionID, partition_id childID,
	off_t offset, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	partition_data* child = get_partition(childID);
	if (child == NULL)
		return B_BAD_VALUE;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)(addr_t)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	off_t validatedOffset = offset;
	if (!efi_gpt_validate_move_child(partition, child, &validatedOffset))
		return B_BAD_VALUE;

	if (child->offset == validatedOffset)
		return B_OK;

	// TODO: implement actual moving, need to move the partition content
	// (the raw data) here and need to take overlap into account
	return B_ERROR;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry& entry = header->EntryAt(entryIndex);
	uint64 blockCount = entry.BlockCount();
	entry.SetStartBlock((validatedOffset - partition->offset)
		/ partition->block_size);
	entry.SetBlockCount(blockCount);

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK) {
		// fatal error: the data has been moved but the partition table could
		// not be updated to reflect that change!
		return result;
	}

	child->offset = validatedOffset;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(childID);
	return B_OK;
}


static status_t
efi_gpt_set_name(int fd, partition_id partitionID, const char* name,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* child = get_partition(partitionID);
	if (child == NULL)
		return B_BAD_VALUE;

	partition_data* partition = get_parent_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)(addr_t)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry& entry = header->EntryAt(entryIndex);
	to_ucs2(name, strlen(name), entry.name, EFI_PARTITION_NAME_LENGTH);

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK)
		return result;

	char newName[B_OS_NAME_LENGTH];
	to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, newName, sizeof(newName));
	child->name = strdup(newName);

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_set_type(int fd, partition_id partitionID, const char* type,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* child = get_partition(partitionID);
	if (child == NULL)
		return B_BAD_VALUE;

	partition_data* partition = get_parent_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)(addr_t)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	guid_t typeGUID;
	if (!get_guid_for_partition_type(type, typeGUID))
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry& entry = header->EntryAt(entryIndex);
	entry.partition_type = typeGUID;

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK)
		return result;

	child->type = strdup(type);

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_initialize(int fd, partition_id partitionID, const char* name,
	const char* parameters, off_t partitionSize, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	EFI::Header header((partitionSize - 1) / partition->block_size,
		partition->block_size);
	status_t result = header.InitCheck();
	if (result != B_OK)
		return result;

	result = header.Write(fd);
	if (result != B_OK)
		return result;

	result = scan_partition(partitionID);
	if (result != B_OK)
		return result;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_uninitialize(int fd, partition_id partitionID, off_t partitionSize,
	uint32 blockSize, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	const int headerSize = partition->block_size * 3;
	// The first block is the protective MBR
	// The second block is the GPT header
	// The third block is the start of the partition list (it can span more
	// blocks, but that doesn't matter as soon as the header is erased).

	uint8 buffer[headerSize];
	memset(buffer, 0xE5, sizeof(buffer));

	// Erase the first blocks
	if (write_pos(fd, 0, &buffer, headerSize) < 0)
		return errno;

	// Erase the last blocks
	// Only 2 blocks, as there is no protective MBR
	if (write_pos(fd, partitionSize - 2 * partition->block_size,
			&buffer, 2 * partition->block_size) < 0) {
		return errno;
	}

	update_disk_device_job_progress(job, 1.0);

	return B_OK;
}


static status_t
efi_gpt_create_child(int fd, partition_id partitionID, off_t offset,
	off_t size, const char* type, const char* name, const char* parameters,
	disk_job_id job, partition_id* childID)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	off_t validatedOffset = offset;
	off_t validatedSize = size;
	uint32 entryIndex = 0;

	if (!efi_gpt_validate_create_child(partition, &validatedOffset,
			&validatedSize, type, name, parameters, (int32*)&entryIndex))
		return B_BAD_VALUE;

	guid_t typeGUID;
	if (!get_guid_for_partition_type(type, typeGUID))
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	partition_data* child = create_child_partition(partition->id, entryIndex,
		validatedOffset, validatedSize, *childID);
	if (child == NULL)
		return B_ERROR;

	efi_partition_entry& entry = header->EntryAt(entryIndex);
	entry.partition_type = typeGUID;
	uuid_t uuid;
	uuid_generate_random(uuid);
	memcpy((uint8*)&entry.unique_guid, uuid, sizeof(guid_t));
	to_ucs2(name, strlen(name), entry.name, EFI_PARTITION_NAME_LENGTH);
	entry.SetStartBlock((validatedOffset - partition->offset)
		/ partition->block_size);
	entry.SetBlockCount(validatedSize / partition->block_size);
	entry.SetAttributes(0); // TODO

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK) {
		delete_partition(child->id);
		return result;
	}

	*childID = child->id;
	child->block_size = partition->block_size;
	child->name = strdup(name);
	child->type = strdup(type);
	child->parameters = strdup(parameters);
	child->cookie = (void*)(addr_t)entryIndex;

	if (child->type == NULL || child->parameters == NULL) {
		delete_partition(child->id);
		return B_NO_MEMORY;
	}

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_delete_child(int fd, partition_id partitionID, partition_id childID,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data* partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	partition_data* child = get_partition(childID);
	if (child == NULL)
		return B_BAD_VALUE;

	EFI::Header* header = (EFI::Header*)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)(addr_t)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	if (!delete_partition(childID))
		return B_ERROR;

	efi_partition_entry& entry = header->EntryAt(entryIndex);
	memset(&entry, 0, sizeof(efi_partition_entry));
	entry.partition_type = kEmptyGUID;

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK)
		return result;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}
#endif // !_BOOT_MODE


#ifndef _BOOT_MODE
static partition_module_info sEFIPartitionModule = {
#else
partition_module_info gEFIPartitionModule = {
#endif
	{
		EFI_PARTITION_MODULE_NAME,
		0,
		efi_gpt_std_ops
	},
	"gpt",									// short_name
	EFI_PARTITION_NAME,						// pretty_name
	0										// flags
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_MOVING
	| B_DISK_SYSTEM_SUPPORTS_RESIZING
	| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
	| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_SETTING_NAME
	| B_DISK_SYSTEM_SUPPORTS_NAME
	,

	// scanning
	efi_gpt_identify_partition,
	efi_gpt_scan_partition,
	efi_gpt_free_identify_partition_cookie,
	NULL, // free_partition_cookie
	efi_gpt_free_partition_content_cookie,

#ifndef _BOOT_MODE
	// querying
	efi_gpt_get_supported_operations,
	efi_gpt_get_supported_child_operations,
	NULL, // supports_initializing_child
	efi_gpt_is_sub_system_for,

	efi_gpt_validate_resize,
	efi_gpt_validate_resize_child,
	efi_gpt_validate_move,
	efi_gpt_validate_move_child,
	efi_gpt_validate_set_name,
	NULL, // validate_set_content_name
	efi_gpt_validate_set_type,
	NULL, // validate_set_parameters
	NULL, // validate_set_content_parameters
	efi_gpt_validate_initialize,
	efi_gpt_validate_create_child,
	efi_gpt_get_partitionable_spaces,
	efi_gpt_get_next_supported_type,
	NULL, // get_type_for_content_type

	// shadow partition modification
	efi_gpt_shadow_changed,

	// writing
	efi_gpt_repair,
	efi_gpt_resize,
	efi_gpt_resize_child,
	efi_gpt_move,
	efi_gpt_move_child,
	efi_gpt_set_name,
	NULL, // set_content_name
	efi_gpt_set_type,
	NULL, // set_parameters
	NULL, // set_content_parameters
	efi_gpt_initialize,
	efi_gpt_uninitialize,
	efi_gpt_create_child,
	efi_gpt_delete_child
#else
	NULL
#endif // _BOOT_MODE
};

#ifndef _BOOT_MODE
partition_module_info* modules[] = {
	&sEFIPartitionModule,
	NULL
};
#endif
