//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file partition.h
	hook function declarations for disk_scanner partition modules
*/


#ifndef _PARTSCAN_PARTITION_H
#define _PARTSCAN_PARTITION_H

#include <module.h>

struct extended_partition_info;

typedef bool (*partition_identify_hook)(int deviceFD,
	const struct session_info *session, const uchar *block);
typedef status_t (*partition_get_nth_info_hook)(int deviceFD,
	const struct session_info *session, const uchar *block, int32 index,
	struct extended_partition_info *partitionInfo);
typedef bool (*partition_identify_module_hook)(const char *identifier);
typedef status_t (*partition_get_partitioning_params_hook)(int deviceFD,
	const struct session_info *sessionInfo, char *buffer, size_t bufferSize,
	size_t *actualSize);
typedef status_t (*partition_partition_hook)(int deviceFD,
	const struct session_info *sessionInfo, const char *parameters);

typedef struct partition_module_info {
	module_info								module;
	const char								*short_name;

	partition_identify_hook					identify;
	partition_get_nth_info_hook				get_nth_info;
	partition_get_partitioning_params_hook	get_partitioning_params;
	partition_partition_hook				partition;
} partition_module_info;

/*
	short_name:
	----------

	Identifies the module. That's the identifier to be passed to
	partition_session().


	identify():
	----------

	Checks whether the partition map of the given session can be recognized
	by the module. Returns true, if it can, false otherwise.

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session the partitions reside on
	block: the first block of the session


	get_nth_info():
	--------------

	Fills in the following fields of partitionInfo with information about
	the indexth partition on the specified session:
	* offset
	* size
	* flags
	* partition_name
	* partition_type

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session the partition resides on
	block: the first block of the session
	index: the partition index
	partitionInfo: the partition info

	The functions is only called, when a call to identify() returned
	true.

	Returns B_OK, if successful, B_ENTRY_NOT_FOUND, if the index is out of
	range.


	get_partitioning_params():
	-------------------------

	Returns parameters for partitioning the supplied session.
	If the session is already partitioned using this module, then the
	parameters describing the current layout will be returned, otherwise
	default values.

	If the supplied buffer is too small for the parameters, the function
	returns B_OK, but doesn't fill in the buffer; the required buffer
	size is returned in actualSize. If the buffer is large enough,
	actualSize is set to the actually used size. The size includes the
	terminating null.

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session to be partitioned
	buffer: pointer to a pre-allocated buffer of size bufferSize
	bufferSize: the size of buffer
	actualSize: pointer to a pre-allocated size_t to be set to the
				actually needed buffer size

	Returns B_OK, if the parameters could be returned successfully or the
	buffer is too small, an error code otherwise.


	partition():
	-----------

	Partitions the specified session of the device according to the supplied
	parameters.

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session to be partitioned
	parameters: the parameters for partitioning

	Returns B_OK, if everything went fine, an error code otherwise.
*/

#endif // _PARTSCAN_PARTITION_H
