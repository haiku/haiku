/*
 * Copyright 2007-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Les De Ridder, les@lesderid.net
 *
 * Distributed under the terms of the MIT License.
 */

#include "btrfs_disk_system.h"

#include "btrfs.h"
#include "Volume.h"


status_t
parse_initialize_parameters(const char* parameterString,
	initialize_parameters& parameters)
{
	parameters.verbose = false;

	void *handle = parse_driver_settings_string(parameterString);
	if (handle == NULL)
		return B_ERROR;

	if (get_driver_boolean_parameter(handle, "verbose", false, true))
		parameters.verbose = true;

	const char *ss_string = get_driver_parameter(handle, "sector_size",
		NULL, NULL);
	uint32 sectorSize = B_PAGE_SIZE;
	if (ss_string != NULL)
		sectorSize = strtoul(ss_string, NULL, 0);

	const char *bs_string = get_driver_parameter(handle, "block_size",
		NULL, NULL);
	uint32 blockSize = max_c(16384, B_PAGE_SIZE);
	if (bs_string != NULL)
		blockSize = strtoul(bs_string, NULL, 0);

	// TODO(lesderid): accept more settings (allocation profiles, uuid, etc.)

	delete_driver_settings(handle);

	if ((blockSize != 1024 && blockSize != 2048 && blockSize != 4096
		&& blockSize != 8192 && blockSize != 16384)
		|| blockSize % sectorSize != 0) {
		return B_BAD_VALUE;
	}

	parameters.sectorSize = sectorSize;
	parameters.blockSize = blockSize;

	return B_OK;
}


status_t
check_volume_name(const char* name)
{
	if (name == NULL || strlen(name) >= BTRFS_LABEL_SIZE
		|| strchr(name, '/') != NULL
		|| strchr(name, '\\') != NULL) {
		return B_BAD_VALUE;
	}

	return B_OK;
}

