//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

/*! \file DriveSetupAddon.cpp

	\brief UDF DriveSetup add-on for R5.
	
	The interface implemented here is detailed in the Be Newsletter,
	Volume II, Issue 23, "Getting Mounted". Thanks to Ingo Weinhold
	for digging that up. :-)
*/

#include "UdfDebug.h"
#include "Recognition.h"

struct partition_data {
	char partition_name[B_FILE_NAME_LENGTH];
	char partition_type[B_FILE_NAME_LENGTH];
	char file_system_short_name[B_FILE_NAME_LENGTH];
	char file_system_long_name[B_FILE_NAME_LENGTH];
	char volume_name[B_FILE_NAME_LENGTH];
	char mounted_at[B_FILE_NAME_LENGTH];
	uint32 logical_block_size;
	uint64 offset;	// in logical blocks from start of session
	uint64 blocks;
	bool hidden;	//"non-file system" partition
	bool reserved1;
	uint32 reserved2;
};

extern "C" bool ds_fs_id(partition_data*, int32, uint64, int32);

bool
ds_fs_id(partition_data *data, int32 device, uint64 sessionOffset,
		 int32 blockSize)
{
	DEBUG_INIT_ETC(NULL, ("%p, %ld, %Lu, %ld", data,
	               device, sessionOffset, blockSize));

	if (!data || device < 0)
		return false;

	bool result = false;

	char name[256];
		// Udf volume names are at most 63 2-byte unicode chars, so 256 UTF-8
		// chars should cover us.

	status_t error = Udf::udf_recognize(device, (data->offset + sessionOffset), data->blocks, blockSize, name);
	if (!error) {
		strcpy(data->file_system_short_name, "udf");
		strcpy(data->file_system_long_name, "Universal Disk Format");
		strcpy(data->volume_name, name);
		result = true;
	} 

	return result;
}

