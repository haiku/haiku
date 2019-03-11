/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>
#include <string.h>
#include <driver_settings.h>
#include <KernelExport.h>
#include "settings.h"

#define DEFAULT_GOOGLE_SERVER "74.125.136.105"
#define DEFAULT_MAX_VNODES 5000
char google_server[20] = DEFAULT_GOOGLE_SERVER;
int google_server_port = 80;
uint32 max_vnodes = DEFAULT_MAX_VNODES;
uint32 max_results = 50;
bool sync_unlink_queries = false;

status_t load_settings(void)
{
	void *handle;
	const char *val;
	handle = load_driver_settings("googlefs");
	if (!handle)
		return ENOENT;

	dprintf("googlefs: loaded settings\n");

	val = get_driver_parameter(handle, "server", \
			DEFAULT_GOOGLE_SERVER, DEFAULT_GOOGLE_SERVER);
	strncpy(google_server, val, 20);
	google_server[20-1] = '\0';

	val = get_driver_parameter(handle, "port", "80", "80");
	google_server_port = strtoul(val, NULL, 10);

	val = get_driver_parameter(handle, "max_nodes", "5000", "5000");
	max_vnodes = strtoul(val, NULL, 10);
	max_vnodes = MIN(max_vnodes, 1000000);
	max_vnodes = MAX(max_vnodes, 10);

	val = get_driver_parameter(handle, "max_results", "50", "50");
	max_results = strtoul(val, NULL, 10);
	max_results = MIN(max_results, 1000);
	max_results = MAX(max_results, 5);

	sync_unlink_queries = get_driver_boolean_parameter(handle, "sync_unlink", false, true);

	dprintf("googlefs: settings: server = %s\n", google_server);
	dprintf("googlefs: settings: max_nodes = %lu\n", max_vnodes);
	dprintf("googlefs: settings: max_results = %lu\n", max_results);
	dprintf("googlefs: settings: sync_unlink = %c\n", sync_unlink_queries?'t':'f');
	unload_driver_settings(handle);
	return B_OK;
}

