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
//	File Name:		filesystem.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	hook function declarations for disk_scanner filesystem modules
//------------------------------------------------------------------------------

/*!
	\file filesystem.h
	hook function declarations for disk_scanner filesystem modules
*/

#ifndef _PARTSCAN_FS_H
#define _PARTSCAN_FS_H

#include <module.h>

struct extended_partition_info;

typedef bool (*fs_identify_hook)(int deviceFD,
	struct extended_partition_info *partitionInfo, float *priority);

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
	priority: Pointer to a float in which the priority of the FS shall be
			  stored. Used in case several FS add-ons recognize the FS;
			  then the module returning the highest priority is used.
			  -1 <= *priority <= 1

	Returns true, if successful (i.e. the FS was recognized), false otherwise.
*/

#endif	// _PARTSCAN_FS_H
