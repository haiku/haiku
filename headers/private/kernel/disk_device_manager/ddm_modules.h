/*
 * Copyright 2003-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_DISK_DEVICE_MODULES_H
#define _K_DISK_DEVICE_MODULES_H

//! Interface to be implemented by partitioning modules.

#include <disk_device_manager.h>
#include <module.h>
#include <SupportDefs.h>


typedef struct partition_module_info {
	module_info									module;
	const char*									short_name;
	const char*									pretty_name;
	uint32										flags;

	// scanning
	// (the device is write locked)
	float (*identify_partition)(int fd, partition_data* partition,
				void** cookie);
	status_t (*scan_partition)(int fd, partition_data* partition,
				void* identifyCookie);
	void (*free_identify_partition_cookie)(partition_data* partition,
				void* cookie);
	void (*free_partition_cookie)(partition_data* partition);
	void (*free_partition_content_cookie)(partition_data* partition);


	// querying
	// (the device is read locked)
	uint32 (*get_supported_operations)(partition_data* partition, uint32 mask);
	uint32 (*get_supported_child_operations)(partition_data* partition,
				partition_data* child, uint32 mask);

	bool (*supports_initializing_child)(partition_data* partition,
				const char* system);
	bool (*is_sub_system_for)(partition_data* partition);

	bool (*validate_resize)(partition_data* partition, off_t* size);
	bool (*validate_resize_child)(partition_data* partition,
				partition_data* child, off_t* size);
	bool (*validate_move)(partition_data* partition, off_t* start);
	bool (*validate_move_child)(partition_data* partition,
				partition_data* child, off_t* start);
	bool (*validate_set_name)(partition_data* partition, char* name);
	bool (*validate_set_content_name)(partition_data* partition, char* name);
	bool (*validate_set_type)(partition_data* partition, const char* type);
	bool (*validate_set_parameters)(partition_data* partition,
				const char* parameters);

	bool (*validate_set_content_parameters)(partition_data* partition,
				const char* parameters);
	bool (*validate_initialize)(partition_data* partition, char* name,
				const char* parameters);
	bool (*validate_create_child)(partition_data* partition, off_t* start,
				off_t* size, const char* type, const char* name,
				const char* parameters, int32* index);
	status_t (*get_partitionable_spaces)(partition_data* partition,
				partitionable_space_data* buffer, int32 count,
				int32* actualCount);
		// When not implemented, a standard algorithm is used.

	status_t (*get_next_supported_type)(partition_data* partition,
				int32* cookie, char* type);
	status_t (*get_type_for_content_type)(const char* contentType, char* type);


	// shadow partition modification
	// (device is write locked)
	status_t (*shadow_changed)(partition_data* partition,
				partition_data *child, uint32 operation);


	// writing
	// (device is NOT locked)
	status_t (*repair)(int fd, partition_id partition, bool checkOnly,
				disk_job_id job);
	status_t (*resize)(int fd, partition_id partition, off_t size,
				disk_job_id job);
	status_t (*resize_child)(int fd, partition_id partition, off_t size,
				disk_job_id job);
	status_t (*move)(int fd, partition_id partition, off_t offset,
				disk_job_id job);
	status_t (*move_child)(int fd, partition_id partition, partition_id child,
				off_t offset, disk_job_id job);
	status_t (*set_name)(int fd, partition_id partition, const char* name,
				disk_job_id job);
	status_t (*set_content_name)(int fd, partition_id partition,
				const char* name, disk_job_id job);
	status_t (*set_type)(int fd, partition_id partition, const char* type,
				disk_job_id job);
	status_t (*set_parameters)(int fd, partition_id partition,
				const char* parameters, disk_job_id job);
	status_t (*set_content_parameters)(int fd, partition_id partition,
				const char* parameters, disk_job_id job);
	status_t (*initialize)(int fd, partition_id partition, const char* name,
				const char *parameters, off_t partitionSize, disk_job_id job);
	status_t (*uninitialize)(int fd, partition_id partition,
				off_t partitionSize, disk_job_id job);
	status_t (*create_child)(int fd, partition_id partition, off_t offset,
				off_t size, const char* type, const char* name,
				const char* parameters, disk_job_id job,
				partition_id* childID);
		// childID is used for the return value, but is also an optional input
		// parameter -- -1 to be ignored
	status_t (*delete_child)(int fd, partition_id partition, partition_id child,
				disk_job_id job);
} partition_module_info;

#endif	// _K_DISK_DEVICE_MODULES_H
