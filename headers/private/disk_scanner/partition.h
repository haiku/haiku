// partition.h

#ifndef _PARTSCAN_PARTITION_H
#define _PARTSCAN_PARTITION_H

#include <module.h>

struct extended_partition_info;

typedef bool (*partition_identify_hook)(int deviceFD, off_t sessionOffset,
	off_t sessionSize, const uchar *block, int32 blockSize);
typedef status_t (*partition_get_nth_info_hook)(int deviceFD,
	off_t sessionOffset, off_t sessionSize, const uchar *block,
	int32 blockSize, int32 index,
	struct extended_partition_info *partitionInfo);

typedef struct partition_module_info {
	module_info					module;

	partition_identify_hook		identify;
	partition_get_nth_info_hook	get_nth_info;
} partition_module_info;

/*
	identify():
	----------

	Checks whether the partition map of the given session can be recognized
	by the module. Returns true, if it can, false otherwise.

	params:
	deviceFD: a device FD
	sessionOffset: start of the session in bytes from the beginning of the
				   device
	sessionSize: size of the session in bytes
	block: the first block of the session
	blockSize: the logical block size


	get_nth_info():
	--------------

	Fills in the following fields of partitionInfo with information about
	the indexth partition on the specified session:
	* offset
	* size
	* flags
	* partition_name
	* partition_type
	* partition_code

	params:
	deviceFD: a device FD
	sessionOffset: start of the session in bytes from the beginning of the
				   device
	sessionSize: size of the session in bytes
	block: the first block of the session
	blockSize: the logical block size
	index: the partition index
	partitionInfo: the partition info

	The functions is only called, when a call to identify() returned
	true.

	Returns B_OK, if successful, B_ENTRY_NOT_FOUND, if the index is out of
	range.
*/

#endif _PARTSCAN_PARTITION_H
