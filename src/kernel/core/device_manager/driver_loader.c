/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP Manager

	PnP Driver loader.
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
pnp_unblock_load(pnp_node_info *node)
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
pnp_wait_load_block(pnp_node_info *node)
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


/** load driver automatically after registration/rescan of node. */

void
pnp_load_driver_automatically(pnp_node_info *node, bool after_rescan)
{
	uint8 always_loaded;
	status_t res;
	pnp_driver_info *interface;
	void *cookie;

	benaphore_lock(&gNodeLock);

	// ignore dead nodes	
	if (!node->registered)
		goto err;

	if (node->automatically_loaded)
		goto err;

	if (pnp_get_attr_uint8_nolock(node, PNP_DRIVER_ALWAYS_LOADED, 
			&always_loaded, false ) != B_OK)
		goto err;

	if (always_loaded < 1 || always_loaded > 2)
		goto err;

	// this test is probably useless as the driver would be kept loaded
	// during rescan anyway
	if (always_loaded == 2 && after_rescan)
		goto err;

	// alright - time to get the driver loaded
	benaphore_unlock(&gNodeLock);

	res = pnp_load_driver(node, NULL, &interface, &cookie);

	benaphore_lock(&gNodeLock);

	// if driver cannot be loaded, we don't really care
	if (res == B_OK )
		node->automatically_loaded = true;
	else
		dprintf("driver could not be automatically loaded\n");

err:
	benaphore_unlock(&gNodeLock);
	return;
}


/** unload driver that got loaded automatically. */

void
pnp_unload_driver_automatically(pnp_node_info *node, bool before_rescan)
{
	uint8 always_loaded;
	status_t res;

	benaphore_lock(&gNodeLock);

	if (!node->automatically_loaded)
		goto err;

	if (pnp_get_attr_uint8_nolock(node, PNP_DRIVER_ALWAYS_LOADED, 
			&always_loaded, false) != B_OK)
		goto err;

	// this test is probably useless as the driver wouldn't be
	// loaded anyway
	if (always_loaded < 1 || always_loaded > 2)
		goto err;

	if (always_loaded == 2 && before_rescan)
		goto err;

	// time to unload the driver		
	benaphore_unlock(&gNodeLock);

	res = pnp_unload_driver(node);

	benaphore_lock(&gNodeLock);

	// don't reset flag if unloading failed;
	// if this happens during unregistration, this won't help;
	// but if this happens during rescan, the driver only stays
	// loaded during rescan, which can be handled
	if (res == B_OK)
		node->automatically_loaded = false;
	else
		dprintf("driver could not be unloaded during scan\n");

err:
	benaphore_unlock(&gNodeLock);
	return;
}


/** load driver for real
 *	(loader_lock must be hold)
 */

static status_t
load_driver_int(pnp_node_info *node, void *user_cookie)
{
	char *module_name;
	pnp_driver_info *driver;
	void *cookie;
	status_t res;

	TRACE(("load_driver_int()\n"));

	res = pnp_get_attr_string(node, PNP_DRIVER_DRIVER, &module_name, false);
	if (res != B_OK)
		return res;

	TRACE(("%s\n", module_name));

	res = get_module(module_name, (module_info **)&driver);
	if (res < B_OK) {
		dprintf("Cannot load module %s\n", module_name);
		goto err;
	}

	if (driver->init_device == NULL) {
		dprintf("Driver %s has no init_device hook\n", module_name);
		res = B_ERROR;
		goto err2;
	}

	res = driver->init_device(node, user_cookie, &cookie);
	if (res < B_OK) {
		dprintf("init device failed (node %p, %s): %s\n", node, module_name, strerror(res));
		goto err2;
	}

	node->driver = driver;
	node->cookie = cookie;

	free(module_name);
	return B_OK;

err2:
	put_module(module_name);
	
err:
	free(module_name);
	return res;
}


/** public: load driver */

status_t
pnp_load_driver(pnp_node_handle node, void *user_cookie, 
	pnp_driver_info **interface, void **cookie)
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

		// during load, driver may register children;
		// probing them for consumers may be impossible as they 
		// may try to load driver -> deadlock
		// so we block probing for consumers of children
		pnp_defer_probing_of_children_nolock(node);

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

			// now we can safely probe for consumers of children as the
			// driver is fully loaded
			pnp_probe_waiting_children_nolock(node);
		} else {
			// loading failed or device got removed - restore reference count;
			// but first (i.e. as long as ref_count is increased) get 
			// rid of waiting children
			pnp_probe_waiting_children_nolock(node);
			pnp_remove_node_ref_nolock(node);
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
			((struct module_info *)(*interface))->name));
	} else {
		TRACE(("load_driver: Failure (%s)\n", strerror(res)));
	}

	return res;	
}


/** unload driver for real */

static status_t
unload_driver_int(pnp_node_info *node)
{
	char *module_name;
	status_t res;
	pnp_driver_info *driver = node->driver;

	res = pnp_get_attr_string(node, PNP_DRIVER_DRIVER, &module_name, false);
	if (res != B_OK)
		return res;

	TRACE(("unload_driver_int: %s\n", module_name));

	if (driver->uninit_device == NULL) {
		// it has no uninit - we can't unload it, so it stays in memory forever
		TRACE(("Driver %s has no uninit_device hook\n", module_name));
		res = B_ERROR;
		goto err;
	}

	res = driver->uninit_device(node->cookie);
	if (res != B_OK) {
		TRACE(("Failed to uninit driver %s (%s)\n", module_name, strerror(res)));
		goto err;
	}

	// if it was unregistered a while ago but couldn't get cleaned up
	// because it was in use, do the cleanup now
	if (!node->registered) {
		if (driver->device_cleanup != NULL)
			driver->device_cleanup(node);
	}

	put_module(module_name);

	free(module_name);
	return B_OK;

err:
	free(module_name);
	return res;
}


/** public: unload driver */

status_t
pnp_unload_driver(pnp_node_handle node)
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
			TRACE(("don't unload as current load_count is %d\n", 
				node->load_count));
			res = B_OK;
		}

		// signal unload finished
		--node->loading;

		if (res == B_OK) {
			// everything is fine, so decrease load_count ...
			--node->load_count;
			// ... and reference count
			pnp_remove_node_ref_nolock(node);
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
		pnp_remove_node_ref_nolock(node);

		res = B_OK;
	}

	benaphore_unlock(&gNodeLock);

	return res;
}

