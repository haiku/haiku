/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Scanning for consumers.

	Logic to execute (re)scanning of nodes.

	During initial registration, problems with fixed consumers are
	fatal. During rescan, this is ignored 
	(TODO: should this be done more consistently?)
*/


#include "device_manager_private.h"

#include <KernelExport.h>

#include <string.h>
#include <stdlib.h>


//#define TRACE_SCAN
#ifdef TRACE_SCAN
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static status_t scan(device_node_info *node, bool rescan);


static int32 sRescanning = 0;


#if 0
// execute deferred probing of children
// (node_lock must be hold)

void
pnp_probe_waiting_children_nolock(device_node_info *node)
{
	TRACE(("execute deferred probing of parent %p\n", node));

	while (node->unprobed_children) {
		device_node_info *child = node->unprobed_children;

		REMOVE_DL_LIST(child, node->unprobed_children, unprobed_);

		// child may have been removed meanwhile
		if (child->registered) {
			benaphore_unlock(&gNodeLock);

			if (pnp_initial_scan(child) == B_OK)
				pnp_load_driver_automatically(node, false);

			benaphore_lock(&gNodeLock);
		}

		// reference count was increment to keep node alive in wannabe list;
		// this is not necessary anymore
		pnp_remove_node_ref(node);
	}

	TRACE((".. done.\n"));
}


// execute deferred probing of children

void
pnp_probe_waiting_children(device_node_info *node)
{
	benaphore_lock(&gNodeLock);

	pnp_probe_waiting_children_nolock(node);

	benaphore_unlock(&gNodeLock);
}

#endif


/** ask bus to (re-)scan for connected devices */

static void
scan_bus(device_node_info *node, bool rescan)
{
	// busses can register their children themselves
	bus_module_info *interface;
	status_t status;
	void *cookie;

	// load driver during rescan

	status = pnp_load_driver(node, NULL, (driver_module_info **)&interface, &cookie);
	if (status < B_OK) {
		// we couldn't load a driver that had already registered itself...
		return;
	}

	TRACE(("scan bus\n"));

	// block other notifications (currently, only "removed" could be
	// called as driver is/will stay loaded and scanning the bus 
	// concurrently has been assured to not happen)
	pnp_start_hook_call(node);

	if (rescan) {
		if (interface->rescan_bus != NULL)
			interface->rescan_bus(cookie);
	} else {
		if (interface->register_child_devices != NULL)
			interface->register_child_devices(cookie);
	}

	pnp_finish_hook_call(node);

	pnp_unload_driver(node);

	// time to execute delayed probing
//	pnp_probe_waiting_children(node);
}


/** recursively scan children of node (using depth-scan).
 *	node_lock must be hold
 */

static status_t
recursive_scan(device_node_info *node)
{
	device_node_info *child = NULL;
	status_t status;

	TRACE(("recursive_scan(node = %p)\n", node));

	while ((child = list_get_next_item(&node->children, child)) != NULL) {
		dm_get_node_nolock(child);

		// during rescan, we must release node_lock to not deadlock
		benaphore_unlock(&gNodeLock);

		status = scan(child, true);

		benaphore_lock(&gNodeLock);

		// unlock current child as it's not accessed anymore
		dm_put_node_nolock(child);

		// ignore errors because of touching unregistered nodes	
		if (status != B_OK && status != B_NAME_NOT_FOUND)
			return status;
	}

	// no need unlock last child (don't be loop already)
	TRACE(("recursive_scan(): done\n"));
	return B_OK;
}


/** (re)scan for child nodes
 */

static status_t
scan(device_node_info *node, bool rescan)
{
	status_t status = B_OK;
	uint8 dummy;
	bool isBus;

	TRACE(("scan(node = %p, mode: %s)\n", node, rescan ? "rescan" : "register"));

	isBus = pnp_get_attr_uint8(node, PNP_BUS_IS_BUS, &dummy, false) == B_OK;

	// do the real thing - scan node
	if (!rescan) {
		uint8 loadDriversLater = false;
		char *deviceType = NULL;
		pnp_get_attr_uint8(node, B_DRIVER_FIND_DEVICES_ON_DEMAND, &loadDriversLater, false);
		pnp_get_attr_string(node, B_DRIVER_DEVICE_TYPE, &deviceType, false);

		if (isBus)
			scan_bus(node, false);

		// ask possible children to register their nodes

		if (!loadDriversLater && deviceType == NULL) {		
			status = dm_register_dynamic_child_devices(node);
			if (status != B_OK)
				return status;
		}

		free(deviceType);
	} else {
		if (!isBus)
			status = dm_register_dynamic_child_devices(node);	
	}
	
	// scan children recursively;
	// keep the node_lock to make sure noone removes children meanwhile
	if (rescan && isBus) {
		scan_bus(node, true);
		benaphore_lock(&gNodeLock);
		status = recursive_scan(node);
		benaphore_unlock(&gNodeLock);
	}

	TRACE(("scan(): done (%p) - %s\n", node, strerror(status)));
	return status;
}


//	#pragma mark -


/** Rescan device node (only works if it's a bus) */

status_t
dm_rescan(device_node_info *node)
{
	status_t err;

	// only allow a single rescan at a time
	if (atomic_add(&sRescanning, 1) > 1) {
		dprintf("dm_rescan already scanning\n");
		atomic_add(&sRescanning, -1);
		return B_BUSY;
	}

	err = scan(node, true);
	atomic_add(&sRescanning, -1);
	return err;
}


/** execute registration of fully constructed node */

status_t
dm_register_child_devices(device_node_info *node)
{
	return scan(node, false);
}	

