/*
 * Copyright 2007-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "bfs_disk_system.h"

#include "bfs.h"
#include "Volume.h"


status_t
parse_initialize_parameters(const char* parameterString,
	initialize_parameters& parameters)
{
	parameters.flags = 0;
	parameters.verbose = false;

	void *handle = parse_driver_settings_string(parameterString);
	if (handle == NULL)
		return B_ERROR;

	if (get_driver_boolean_parameter(handle, "noindex", false, true))
		parameters.flags |= VOLUME_NO_INDICES;
	if (get_driver_boolean_parameter(handle, "verbose", false, true))
		parameters.verbose = true;

	const char *string = get_driver_parameter(handle, "block_size",
		NULL, NULL);
	uint32 blockSize = 2048;
	if (string != NULL)
		blockSize = strtoul(string, NULL, 0);

	delete_driver_settings(handle);

	if (blockSize != 1024 && blockSize != 2048 && blockSize != 4096
		&& blockSize != 8192) {
		return B_BAD_VALUE;
	}

	parameters.blockSize = blockSize;

	return B_OK;
}


status_t
check_volume_name(const char* name)
{
	if (name == NULL || strlen(name) >= BFS_DISK_NAME_LENGTH
		|| strchr(name, '/') != NULL) {
		return B_BAD_VALUE;
	}

	return B_OK;
}

