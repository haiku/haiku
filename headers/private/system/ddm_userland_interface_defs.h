/*
 * Copyright 2003-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */
#ifndef _SYSTEM_DDM_USERLAND_INTERFACE_DEFS_H
#define _SYSTEM_DDM_USERLAND_INTERFACE_DEFS_H


#include <OS.h>

#include <DiskDeviceDefs.h>


// userland partition representation
typedef struct user_partition_data user_partition_data;
struct user_partition_data {
	partition_id			id;
	off_t					offset;
	off_t					size;
	off_t					content_size;
	uint32					block_size;
	uint32					status;
	uint32					flags;
	dev_t					volume;
	int32					index;
	int32					change_counter;	// TODO: needed?
	disk_system_id			disk_system;
	char*					name;
	char*					content_name;
	char*					type;
	char*					content_type;
	char*					parameters;
	char*					content_parameters;
	void*					user_data;
	int32					child_count;
	user_partition_data*	children[1];
};

// userland disk device representation
typedef struct user_disk_device_data {
	uint32					device_flags;
	char*					path;
	user_partition_data		device_partition_data;
} user_disk_device_data;

// userland disk system representation
typedef struct user_disk_system_info {
	disk_system_id	id;
	char			name[B_FILE_NAME_LENGTH];
		// TODO: better B_PATH_NAME_LENGTH?
	char			short_name[B_OS_NAME_LENGTH];
	char			pretty_name[B_OS_NAME_LENGTH];
	uint32			flags;
} user_disk_system_info;

// userland disk device job representation
typedef struct user_disk_device_job_info {
	disk_job_id		id;
	uint32			type;
	partition_id	partition;
	char			description[256];
} user_disk_device_job_info;


#endif	// _SYSTEM_DDM_USERLAND_INTERFACE_DEFS_H
