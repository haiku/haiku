//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		partition.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	hook function declarations for disk_scanner partition modules
//------------------------------------------------------------------------------

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
	sessionInfo: a complete info about the session the partition resides on
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
	* partition_code

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
*/

#endif _PARTSCAN_PARTITION_H
