/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Les De Ridder, les@lesderid.net
 *
 * Distributed under the terms of the MIT License.
 */

#ifndef _BTRFS_DISK_SYSTEM_H
#define _BTRFS_DISK_SYSTEM_H

#include "system_dependencies.h"


struct initialize_parameters {
	uint32	blockSize;
	uint32	sectorSize;
	bool	verbose;
};

status_t parse_initialize_parameters(const char* parameterString,
	initialize_parameters& parameters);
status_t check_volume_name(const char* name);


#endif	// _BTRFS_DISK_SYSTEM_H
