// fs.h

#ifndef _PARTSCAN_FS_H
#define _PARTSCAN_FS_H

#include <module.h>

struct extended_partition_info;

typedef bool (*fs_identify_hook)(int deviceFD,
	struct extended_partition_info *partitionInfo);

typedef struct fs_module_info {
	module_info			module;

	fs_identify_hook	identify;
} fs_module_info;

/*
	identify():
	----------

	Expects partitionInfo to be partially initialized and, if
	the module is able to recognize the FS on the partition, fills in
	the fields:
	* file_system_short_name
	* file_system_long_name
	* volume_name

	The minimally required fields are:
	* offset
	* size
	* logical_block_size

	params:
	deviceFD: a device FD
	partitionInfo: the partition info

	Returns true, if successful (i.e. the FS was recognized), false otherwise.
*/

#endif	// _PARTSCAN_FS_H
