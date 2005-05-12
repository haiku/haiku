/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	device driver loader.
*/

#include "device_manager_private.h"

#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>


#define TRACE_DRIVER_LOADER
#ifdef TRACE_DRIVER_LOADER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/** unblock loading of driver
 *	node_lock must be hold
 */

void
pnp_unblock_load(device_node_info *node)
{
	if (--node->load_block_count == 0 && node->num_blocked_loads > 0) {
		TRACE(("Releasing blocked loads of %p\n", node));

		release_sem_etc(node->load_block_sem, 
			node->num_blocked_loads, B_DO_NOT_RESCHEDULE);

		node->num_blocked_loads = 0;
	}
}


/** if loading of driver is blocked, wait until it is unblocked
 *	node_lock must be hold
 */

static void
pnp_wait_load_block(device_node_info *node)
{
	if (node->load_block_count > 0) {
		TRACE(("Loading of %p is blocked - wait until unblock\n", node));

		// make sure we get informed about unblock
		++node->num_blocked_loads;

		benaphore_unlock(&gNodeLock);

		acquire_sem(node->load_block_sem);

		benaphore_lock(&gNodeLock);

		TRACE(("Continuing loading of %p\n", node));
	}
}


/** load driver for real
 *	(loader_lock must be hold)
 */

static status_t
load_driver_int(device_node_info *node, void *user_cookie)
{
	driver_module_info *driver;
	char *moduleName;
	void *cookie;
	status_t status;

	TRACE(("load_driver_int()\n"));

	status = pnp_get_attr_string(node, B_DRIVER_MODULE, &moduleName, false);
	if (status != B_OK)
		return status;

	TRACE(("%s\n", moduleName));

	status = get_module(moduleName, (module_info **)&driver);
	if (status < B_OK) {
		dprintf("Cannot load module %s\n", moduleName);
		goto err1;
	}

	if (driver->init_driver == NULL) {
		dprintf("Driver %s has no init_driver hook\n", moduleName);
		status = B_ERROR;
		goto err2;
	}

	status = driver->init_driver(node, user_cookie, &cookie);
	if (status < B_OK) {
		dprintf("init driver failed (node %p, %s): %s\n", node, moduleName, strerror(status));
		goto err2;
	}

	node->driver = driver;
	node->cookie = cookie;

	free(moduleName);
	return B_OK;

err2:
	put_module(moduleName);
err1:
	free(moduleName);
	return status;
}


/** public: load driver */

status_t
pnp_load_driver(device_node_handle node, void *user_cookie, 
	driver_module_info **interface, void **cookie)
{
	status_t res;

	TRACE(("pnp_load_driver()\n"));

	if (node == NULL)
		return B_BAD_VALUE;

	benaphore_lock(&gNodeLock);

	if (!node->registered) {
		benaphore_unlock(&gNodeLock);

		TRACE(("Refused loading as driver got unregistered\n"));
		return B_NAME_NOT_FOUND;
	}

	// increase ref_count whenever someone loads the driver
	// to keep node information alive
	++node->ref_count;

	if (node->load_count == 0) {
		// driver is not loaded
		// tell others that someone will load the driver
		// (after this, noone is allowed to block loading)
		++node->loading;

		// wait until loading is not blocked
		pnp_wait_load_block(node);

		// make sure noone else load/unloads/notifies the driver
		pnp_start_hook_call_nolock(node);

		if (!node->registered) {
			// device got lost meanwhile
			TRACE(("Device got lost during wait\n"));
			res = B_NAME_NOT_FOUND;
		} else if (node->load_count == 0) {
			// noone loaded the driver meanwhile, so
			// we have to do that
			benaphore_unlock(&gNodeLock);

			TRACE(("Driver will be loaded\n"));

			res = load_driver_int(node, user_cookie);

			benaphore_lock(&gNodeLock);
		} else {
			// some other thread loaded the driver meanwhile
			TRACE(("Driver got loaded during wait\n"));
			res = B_OK;
		}

		// info: the next statements may have a strange order,
		// but as we own node_lock, order is not important

		// finish loading officially
		// (don't release node_lock before load_count gets increased!)
		--node->loading;
		
		// unblock concurrent load/unload/notification 
		pnp_finish_hook_call_nolock(node);

		if (res == B_OK) {
			// everything went fine - increase load counter
			++node->load_count;
		} else {
			// loading failed or device got removed - restore reference count;
			// but first (i.e. as long as ref_count is increased) get 
			// rid of waiting children
			dm_put_node_nolock(node);
		}
	} else {
		// driver is already loaded, so increase load_count
		++node->load_count;
		res = B_OK;
	}

	benaphore_unlock(&gNodeLock);

	if (res == B_OK) {
		if (interface)
			*interface = node->driver;

		if (cookie)
			*cookie = node->cookie;

		TRACE(("load_driver: Success \"%s\"\n",
			((struct module_info *)node->driver)->name));
	} else {
		TRACE(("load_driver: Failure (%s)\n", strerror(res)));
	}

	return res;	
}


/** unload driver for real */

static status_t
unload_driver_int(device_node_info *node)
{
	driver_module_info *driver = node->driver;
	char *moduleName;
	status_t status;

	status = pnp_get_attr_string(node, B_DRIVER_MODULE, &moduleName, false);
	if (status != B_OK)
		return status;

	TRACE(("unload_driver_int: %s\n", moduleName));

	if (driver->uninit_driver == NULL) {
		// it has no uninit - we can't unload it, so it stays in memory forever
		TRACE(("Driver %s has no uninit_device hook\n", moduleName));
		status = B_ERROR;
		goto out;
	}

	status = driver->uninit_driver(node->cookie);
	if (status != B_OK) {
		TRACE(("Failed to uninit driver %s (%s)\n", moduleName, strerror(status)));
		goto out;
	}

	// if it was unregistered a while ago but couldn't get cleaned up
	// because it was in use, do the cleanup now
	if (!node->registered) {
		if (driver->device_cleanup != NULL)
			driver->device_cleanup(node);
	}

	put_module(moduleName);

out:
	free(moduleName);
	return status;
}


/** public: unload driver */

status_t
pnp_unload_driver(device_node_handle node)
{
	status_t res;

	TRACE(("pnp_unload_driver: %p\n", node));

	if (node == NULL)
		return B_BAD_VALUE;

	benaphore_lock(&gNodeLock);

	if (node->loading > 0 || node->load_count == 1) {
		// we may have to unload the driver because:
		// - load_count will be zero after decrement, or
		// - there is a concurrent load/unload call, so we
		//   don't know what the future load_count will be

		// signal active unload;
		// don't decrease load_count - the driver is still loaded!
		++node->loading;

		// wait for concurrent load/unload/notification
		pnp_start_hook_call_nolock(node);

		if (node->load_count == 1) {
			// node is still to be unloaded
			benaphore_unlock(&gNodeLock);

			TRACE(("unloading %p\n", node));
			res = unload_driver_int(node);

			benaphore_lock(&gNodeLock);
		} else {
			// concurrent load/unload either unloaded the driver or
			// increased load_count so unload is not necessary or forbidden
			TRACE(("don't unload as current load_count is %ld\n",
				node->load_count));
			res = B_OK;
		}

		// signal unload finished
		--node->loading;

		if (res == B_OK) {
			// everything is fine, so decrease load_count ...
			--node->load_count;
			// ... and reference count
			dm_put_node_nolock(node);
		} else {
			// unloading failed: leave loaded in memory forever
			// as load_count is not decreased, the driver will never
			// be unloaded, and as reference count is not decreased,
			// the node will exist forever
			TRACE(("Unload failed - leaving driver of %p in memory\n", node));
		}

		// unblock others	
		pnp_finish_hook_call_nolock(node);
	} else {
		// no concurrent load/unload and load_count won't reach zero
		--node->load_count;
		// each load increased reference count by one - time to undo that
		dm_put_node_nolock(node);

		res = B_OK;
	}

	benaphore_unlock(&gNodeLock);

	return res;
}

