/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INTEL_WRITE_SUPPORT_H
#define INTEL_WRITE_SUPPORT_H


#include <disk_device_manager/ddm_modules.h>


uint32		pm_get_supported_operations(partition_data* partition,
				uint32 mask = ~0);
uint32		pm_get_supported_child_operations(partition_data* partition,
				partition_data* child, uint32 mask = ~0);
bool		pm_is_sub_system_for(partition_data* partition);

bool		pm_validate_resize(partition_data* partition, off_t* size);
bool		pm_validate_resize_child(partition_data* partition,
				partition_data* child, off_t* size);
bool		pm_validate_move(partition_data* partition, off_t* start);
bool		pm_validate_move_child(partition_data* partition,
				partition_data* child, off_t* start);
bool		pm_validate_set_type(partition_data* partition, const char* type);
bool		pm_validate_initialize(partition_data* partition, char* name,
				const char* parameters);
bool		pm_validate_create_child(partition_data* partition, off_t* start,
				off_t* size, const char* type, const char* name,
				const char* parameters, int32* index);

status_t	pm_get_partitionable_spaces(partition_data* partition,
				partitionable_space_data* buffer, int32 count,
				int32* actualCount);
status_t	pm_get_next_supported_type(partition_data* partition,
				int32* cookie, char* _type);
status_t	pm_shadow_changed(partition_data* partition, partition_data* child,
				uint32 operation);

status_t	pm_resize(int fd, partition_id partitionID, off_t size,
				disk_job_id job);
status_t	pm_resize_child(int fd, partition_id partitionID, off_t size,
				disk_job_id job);
status_t	pm_move(int fd, partition_id partitionID, off_t offset,
				disk_job_id job);
status_t	pm_move_child(int fd, partition_id partitionID,
				partition_id childID, off_t offset, disk_job_id job);
status_t	pm_set_type(int fd, partition_id partitionID, const char* type,
				disk_job_id job);
status_t	pm_initialize(int fd, partition_id partitionID, const char* name,
				const char* parameters, off_t partitionSize, disk_job_id job);
status_t	pm_uninitialize(int fd, partition_id partitionID,
				off_t partitionSize, disk_job_id job);
status_t	pm_create_child(int fd, partition_id partitionID, off_t offset,
				off_t size, const char* type, const char* name,
				const char* parameters, disk_job_id job,
				partition_id* childID);
status_t	pm_delete_child(int fd, partition_id partitionID,
				partition_id childID, disk_job_id job);


uint32		ep_get_supported_operations(partition_data* partition,
				uint32 mask = ~0);
uint32		ep_get_supported_child_operations(partition_data* partition,
				partition_data* child, uint32 mask = ~0);
bool		ep_is_sub_system_for(partition_data* partition);

bool		ep_validate_resize(partition_data* partition, off_t* size);
bool		ep_validate_resize_child(partition_data* partition,
				partition_data* child, off_t* _size);
bool		ep_validate_move(partition_data* partition, off_t* start);
bool		ep_validate_move_child(partition_data* partition,
				partition_data* child, off_t* _start);
bool		ep_validate_set_type(partition_data* partition, const char* type);
bool		ep_validate_initialize(partition_data* partition, char* name,
				const char* parameters);
bool		ep_validate_create_child(partition_data* partition, off_t* _start,
				off_t* _size, const char* type, const char* name,
				const char* parameters, int32* index);
status_t	ep_get_partitionable_spaces(partition_data* partition,
				partitionable_space_data* buffer, int32 count,
				int32* actualCount);
status_t	ep_get_next_supported_type(partition_data* partition,
				int32* cookie, char* _type);
status_t	ep_shadow_changed(partition_data* partition, partition_data* child,
				uint32 operation);

status_t	ep_resize(int fd, partition_id partitionID, off_t size,
				disk_job_id job);
status_t	ep_resize_child(int fd, partition_id partitionID, off_t size,
				disk_job_id job);
status_t	ep_move(int fd, partition_id partitionID, off_t offset,
				disk_job_id job);
status_t	ep_move_child(int fd, partition_id partitionID,
				partition_id childID, off_t offset, disk_job_id job);
status_t	ep_set_type(int fd, partition_id partitionID, const char* type,
				disk_job_id job);
status_t	ep_initialize(int fd, partition_id partitionID, const char* name,
				const char* parameters, off_t partitionSize, disk_job_id job);
status_t	ep_create_child(int fd, partition_id partitionID, off_t offset,
				off_t size, const char* type, const char* name,
				const char* parameters, disk_job_id job,
				partition_id* childID);
status_t	ep_delete_child(int fd, partition_id partitionID,
				partition_id childID, disk_job_id job);


#endif	// INTEL_WRITE_SUPPORT_H
