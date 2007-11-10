/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BFS_DISK_SYSTEM_H
#define _BFS_DISK_SYSTEM_H

#include "system_dependencies.h"


struct initialize_parameters {
	uint32	blockSize;
	uint32	flags;
	bool	verbose;
};

status_t parse_initialize_parameters(const char* parameterString,
	initialize_parameters& parameters);
status_t check_volume_name(const char* name);


#endif	// _BFS_DISK_SYSTEM_H
