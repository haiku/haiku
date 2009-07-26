/*
 * Copyright 2003-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */
#ifndef _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H
#define _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H


#include <OS.h>

#include <DiskDeviceDefs.h>

#include <ddm_userland_interface_defs.h>
#include <disk_device_manager.h>


#ifdef __cplusplus
extern "C" {
#endif

/*! Syscalls entries */

// iterating, retrieving device/partition data
partition_id _user_get_next_disk_device_id(int32 *cookie, size_t *neededSize);
partition_id _user_find_disk_device(const char *filename, size_t *neededSize);
partition_id _user_find_partition(const char *filename, size_t *neededSize);
partition_id _user_find_file_disk_device(const char *filename,
				size_t *neededSize);
status_t _user_get_disk_device_data(partition_id deviceID, bool deviceOnly,
				user_disk_device_data *buffer, size_t bufferSize,
				size_t *neededSize);

partition_id _user_register_file_device(const char *filename);
status_t _user_unregister_file_device(partition_id deviceID,
				const char *filename);
	// Only a valid deviceID or filename need to be passed. The other one
	// is -1/NULL. If both is given only filename is ignored.
status_t _user_get_file_disk_device_path(partition_id id, char* buffer,
				size_t bufferSize);

// disk systems
status_t _user_get_disk_system_info(disk_system_id id,
				user_disk_system_info *info);
status_t _user_get_next_disk_system_info(int32 *cookie,
				user_disk_system_info *info);
status_t _user_find_disk_system(const char *name, user_disk_system_info *info);

// disk device modification
status_t _user_defragment_partition(partition_id partitionID,
				int32* changeCounter);
status_t _user_repair_partition(partition_id partitionID, int32* changeCounter,
				bool checkOnly);
status_t _user_resize_partition(partition_id partitionID, int32* changeCounter,
				partition_id childID, int32* childChangeCounter, off_t size,
				off_t contentSize);
status_t _user_move_partition(partition_id partitionID, int32* changeCounter,
				partition_id childID, int32* childChangeCounter,
				off_t newOffset, partition_id* descendantIDs,
				int32* descendantChangeCounters, int32 descendantCount);
status_t _user_set_partition_name(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32* childChangeCounter, const char* name);
status_t _user_set_partition_content_name(partition_id partitionID,
				int32* changeCounter, const char* name);
status_t _user_set_partition_type(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32* childChangeCounter, const char* type);
status_t _user_set_partition_parameters(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32* childChangeCounter, const char* parameters);
status_t _user_set_partition_content_parameters(partition_id partitionID,
				int32* changeCounter, const char* parameters);
status_t _user_initialize_partition(partition_id partitionID,
				int32* changeCounter, const char* diskSystemName,
				const char* name, const char* parameters);
status_t _user_uninitialize_partition(partition_id partitionID,
				int32* changeCounter);

status_t _user_create_child_partition(partition_id partitionID,
				int32* changeCounter, off_t offset, off_t size,
				const char* type, const char* name, const char* parameters,
				partition_id* childID, int32* childChangeCounter);
status_t _user_delete_child_partition(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32 childChangeCounter);

// change notification
status_t _user_start_watching_disks(uint32 eventMask, port_id port,
				int32 token);
status_t _user_stop_watching_disks(port_id port, int32 token);

#ifdef __cplusplus
}
#endif

#endif	// _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H
