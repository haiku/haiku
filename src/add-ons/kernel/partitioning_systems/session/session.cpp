//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
/*!
	\file session.cpp
	\brief Disk device manager partition module for CD/DVD sessions.
*/

#include <ddm_modules.h>
#include <DiskDeviceTypes.h>
#include <KernelExport.h>
#include <unistd.h>

#include "Debug.h"
#include "Disc.h"

static const char *kSessionModuleName = "partitioning_systems/session/v1";

static status_t standard_operations(int32 op, ...);
static float identify_partition(int fd, partition_data *partition,
                                void **cookie);
static status_t scan_partition(int fd, partition_data *partition,
                               void *cookie);
static void free_identify_partition_cookie(partition_data *partition,
                                           void *cookie);
static void free_partition_cookie(partition_data *partition);
static void free_partition_content_cookie(partition_data *partition);

static partition_module_info session_module = {
	{
		kSessionModuleName,
		0,
		standard_operations
	},
	kPartitionTypeMultisession,			// pretty_name
	0,									// flags

	// scanning
	identify_partition,					// identify_partition
	scan_partition,						// scan_partition
	free_identify_partition_cookie,		// free_identify_partition_cookie
	free_partition_cookie,				// free_partition_cookie
	free_partition_content_cookie,		// free_partition_content_cookie

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
};

extern "C" partition_module_info *modules[];
_EXPORT partition_module_info *modules[] =
{
	&session_module,
	NULL
};

static
status_t
standard_operations(int32 op, ...)
{
	status_t error = B_ERROR;
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			error = B_OK;
			break;
	}
	return error;
}

static
float
identify_partition(int fd, partition_data *partition, void **cookie)
{
	DEBUG_INIT_ETC(NULL, ("fd: %d, id: %ld, offset: %Ld, "
	       "size: %Ld, block_size: %ld", fd,
	       partition->id, partition->offset, partition->size,
	       partition->block_size));
	device_geometry geometry;
	float result = -1;
	if (partition->block_size == 2048
	    && ioctl(fd, B_GET_GEOMETRY, &geometry) == 0
	    && geometry.device_type == B_CD)
	{
		Disc *disc = new Disc(fd);
		if (disc && disc->InitCheck() == B_OK) {
			*cookie = static_cast<void*>(disc);
			result = 0.5;		
		} 	
	}
	PRINT(("returning %4.2f\n", result));
	return result;
}

static
status_t
scan_partition(int fd, partition_data *partition, void *cookie)
{
	DEBUG_INIT_ETC(NULL, ("fd: %d, id: %ld, offset: %Ld, size: %Ld, "
	               "block_size: %ld, cookie: %p", fd, partition->id, partition->offset,
	               partition->size, partition->block_size, cookie));

	status_t error = fd >= 0 && partition && cookie ? B_OK : B_BAD_VALUE;
	if (!error) {
		Disc *disc = static_cast<Disc*>(cookie);
		partition->status = B_PARTITION_VALID;
		partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
							| B_PARTITION_READ_ONLY;
		partition->content_size = partition->size;
		partition->content_cookie = disc;
		
		for (int i = 1; disc->GetSessionInfo(i) == B_OK; i++) {	
		
		}
	}
	PRINT(("error: 0x%lx, `%s'\n", error, strerror(error)));
	RETURN(B_ERROR);
}

static
void
free_identify_partition_cookie(partition_data */*partition*/, void *cookie)
{
	if (cookie) {
		DEBUG_INIT_ETC(NULL, ("cookie: %p", cookie));
	}
}

static
void
free_partition_cookie(partition_data *partition)
{
	DEBUG_INIT(NULL);
}

static
void
free_partition_content_cookie(partition_data *partition)
{
	DEBUG_INIT(NULL);
}
