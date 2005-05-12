/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	Execution and synchronization of driver hook calls.
*/


#include "device_manager_private.h"

#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>


#define TRACE_NOTIFICATIONS
#ifdef TRACE_NOTIFICATIONS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/** notify driver that it's device got removed */

status_t
dm_notify_unregistration(device_node_info *node)
{
	driver_module_info *driver;
	bool loaded;
	char *module_name;
	void *cookie;
	status_t res;

	res = pnp_get_attr_string(node, B_DRIVER_MODULE, &module_name, false);
	if (res != B_OK)
		return res;

	TRACE(("notify_device_removed(node: %p, consumer: %s)\n", node, module_name));

	// block concurrent load/unload calls
	pnp_start_hook_call(node);

#if 0
	// don't notify driver if it doesn't know that the device was
	// published
	if (!node->init_finished)
		goto skip;
#endif

	// don't take node->loading into account - we want to know
	// whether driver is loaded, not whether it is about to get loaded	
	loaded = node->load_count > 0;

	if (loaded) {
		TRACE(("Already loaded\n"));
		driver = node->driver;
		cookie = node->cookie;
	} else {
		TRACE(("Not loaded yet\n"));

		// driver is not loaded - load it temporarily without
		// calling init_device and pass <cookie>=NULL to tell
		// the driver about
		res = get_module(module_name, (module_info **)&driver);
		if (res < B_OK)
			goto err;

		cookie = NULL;
	}

	// this hook is optional, so ignore it silently if NULL
	if (driver->device_removed != NULL)
		driver->device_removed(node, cookie);
	else
		dprintf("Driver %s has no device_removed hook\n", module_name);

	// if driver wasn't loaded, call its cleanup method	
	if (!loaded) {
		// again: this hook is optional
		if (driver->device_cleanup != NULL)
			driver->device_cleanup(node);
	}

	if (!loaded)
		put_module(module_name);

skip:
	pnp_finish_hook_call(node);
	free(module_name);
	return B_OK;

err:
	free(module_name);
	return res;
}


/** start driver hook call; must be called before a
 *	load/unload/notification hook is called
 */

void
pnp_start_hook_call(device_node_info *node)
{
	bool blocked;

	benaphore_lock(&gNodeLock);

	blocked = ++node->num_waiting_hooks > 1;

	benaphore_unlock(&gNodeLock);

	if (blocked) {
		TRACE(("Hook call of %p is blocked\n", node));

		acquire_sem(node->hook_sem);
	}
}


/** start driver hook call; same pnp_start_hook_call(), but 
 *	node_lock must be hold
 */

void
pnp_start_hook_call_nolock(device_node_info *node)
{
	if (++node->num_waiting_hooks > 1) {
		benaphore_unlock(&gNodeLock);

		TRACE(("Hook call of %p is blocked\n", node));
		acquire_sem(node->hook_sem);

		benaphore_lock(&gNodeLock);
	}
}


/** finish driver hook call; must be called after a load/unload/notification
 *	hook is called
 */

void
pnp_finish_hook_call(device_node_info *node)
{
	benaphore_lock(&gNodeLock);

	if (--node->num_waiting_hooks > 1) {
		TRACE(("Releasing other hook calls of %p\n", node));

		release_sem(node->hook_sem);
	}

	benaphore_unlock(&gNodeLock);
}


/**	finish driver hook call; same pnp_finish_hook_call(), but 
 *	node_lock must be hold
 */

void
pnp_finish_hook_call_nolock(device_node_info *node)
{
	if (--node->num_waiting_hooks > 1) {
		TRACE(("Releasing other hook calls of %p\n", node));

		release_sem(node->hook_sem);
	}
}
