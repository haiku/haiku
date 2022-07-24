/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>
#include <string.h>
#include <driver_settings.h>
#include <KernelExport.h>
#include "settings.h"

#include <stdio.h>

#define DEFAULT_MAX_VNODES 5000
uint32 max_vnodes = DEFAULT_MAX_VNODES;
uint32 max_results = 50;
bool sync_unlink_queries = false;

status_t load_settings(void)
{
	void *handle;
	const char *val;
	handle = load_driver_settings("websearchfs");
	if (!handle)
		return ENOENT;

	fprintf(stderr, "websearchfs: loaded settings\n");

	val = get_driver_parameter(handle, "max_nodes", "5000", "5000");
	max_vnodes = strtoul(val, NULL, 10);
	max_vnodes = MIN(max_vnodes, 1000000);
	max_vnodes = MAX(max_vnodes, 10);

	val = get_driver_parameter(handle, "max_results", "50", "50");
	max_results = strtoul(val, NULL, 10);
	max_results = MIN(max_results, 1000);
	max_results = MAX(max_results, 5);

	sync_unlink_queries = get_driver_boolean_parameter(handle, "sync_unlink", false, true);

	fprintf(stderr, "websearchfs: settings: max_nodes = %" B_PRIu32 "\n", max_vnodes);
	fprintf(stderr, "websearchfs: settings: max_results = %" B_PRIu32 " \n", max_results);
	fprintf(stderr, "websearchfs: settings: sync_unlink = %c\n", sync_unlink_queries?'t':'f');
	unload_driver_settings(handle);
	return B_OK;
}

