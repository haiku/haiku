/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP Manager

	Scanning for consumers.

	Logic to execute (re)scanning of nodes.

	During initial registration, problems with fixed consumers are
	fatal. During rescan, this is ignored 
	(TODO: should this be done more consistently?)
*/


#include "device_manager_private.h"
#include "pnp_bus.h"

#include <KernelExport.h>
#include <string.h>


#define TRACE_SCAN
#ifdef TRACE_SCAN
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/** public function: rescan PnP node */

status_t
pnp_rescan(pnp_node_info *node, uint depth)
{
	return pnp_rescan_int(node, depth, false);
}


/** execute registration of fully constructed node */

status_t
pnp_initial_scan(pnp_node_info *node)
{
	pnp_start_hook_call(node);

	node->init_finished = true;

	pnp_finish_hook_call(node);

	return pnp_rescan_int(node, 1, false);
}	


/** check whether rescanning a node makes sense in terms of
 *	double scans.
 *	node_lock must be hold
 */

static bool
is_rescan_sensible(pnp_node_info *node)
{
	uint depth;
	pnp_node_info *child;

	TRACE(("is_rescan_sensible(node: %p)\n", node));

	// if a child is being scanned, abort our rescan to avoid
	// concurrent scans by the same driver on the same device	
	for (child = node->children; child != NULL; child = child->children_next) {
		if (child->rescan_depth > 0) {
			TRACE(("child %p is being scanned\n", child));
			return false;
		}
	}

	TRACE(("check parents\n"));

	// make sure neither this nor parent node is scanned recursively
	// up to this node as this would lead to unnecessary double-scans
	// (don't need to check whether parent is unregistered as
	// all children would have been unregistered immediately)
	for (depth = 1; node != NULL; node = node->parent, ++depth) {
		if (node->rescan_depth >= depth) {
			TRACE(("parent %p is being scanned", node));
			return false;
		}
	}

	return true;
}


/**	mark children of node as being scanned.
 *	children that have their drivers loaded are not 
 *	scanned unless they allow that explicitely.
 *	node_lock must be hold
 */

static void
mark_children_scanned(pnp_node_info *node)
{
	pnp_node_info *child, *next_child;

	TRACE(("mark_children_scanned()\n"));

	for (child = node->children; child != NULL; child = next_child) {
		uint8 never_rescan, no_live_rescan;

		next_child = child->children_next;

		// ignore dead children
		if (!child->registered)
			continue;

		// check whether device can be rescanned at all
		if (pnp_get_attr_uint8_nolock(child, PNP_DRIVER_NEVER_RESCAN, 
				&never_rescan, false) == B_OK && never_rescan) {
			TRACE(("no rescan allowed on device %p\n", child));
			continue;
		}

		if (pnp_get_attr_uint8_nolock(child, PNP_DRIVER_NO_LIVE_RESCAN, 
				&no_live_rescan, false) != B_OK)
			no_live_rescan = false;

		if (no_live_rescan) {
			TRACE(("no life rescan allowed on device %p\n", child));

			if (child->load_count + child->loading > 0) {
				TRACE(("device %p loaded - skipping it\n", child));
				continue;
			}

			child->blocked_by_rescan = true;
			++child->load_block_count;			
		}

		child->verifying = true;
		child->redetected = false;

		// unload driver during rescan if needed
		// (need to lock next_child temporarily as we release node_lock)
		if (next_child)
			++next_child->ref_count;

		benaphore_unlock(&gNodeLock);

		pnp_unload_driver_automatically(child, true);

		benaphore_lock(&gNodeLock);

		if (next_child)
			pnp_remove_node_ref_nolock(next_child);
	}
}


/** stop children verification, resetting associated flag and unblocking loads. */

static void
reset_children_verification(pnp_node_info *node)
{
	pnp_node_info *child, *next_child;

	TRACE(("reset_children_verification()\n"));

	benaphore_lock(&gNodeLock);

	for (child = node->children; child != NULL; child = next_child) {
		next_child = child->children_next;

		if (!child->verifying)
			continue;

		child->verifying = false;

		if (child->blocked_by_rescan) {
			child->blocked_by_rescan = false;
			pnp_unblock_load(child);
		}

		if (next_child)
			++next_child->ref_count;

		benaphore_unlock(&gNodeLock);

		pnp_load_driver_automatically(node, true);

		benaphore_lock(&gNodeLock);

		if (next_child)
			pnp_remove_node_ref_nolock(next_child);
	}

	benaphore_unlock(&gNodeLock);
}


/** unregister child nodes that weren't detected anymore */

static void
unregister_lost_children(pnp_node_info *node)
{
	pnp_node_info *child, *dependency_list;

	TRACE(("unregister_lost_children()\n"));

	benaphore_lock(&gNodeLock);

	for (child = node->children, dependency_list = NULL; child != NULL;
			child = child->children_next) {
		if (child->verifying && !child->redetected) {
			TRACE(("removing lost device %p\n", child));
			// child wasn't detected anymore - put it onto remove list
			pnp_unregister_node_rec(child, &dependency_list);
		}
	}

	benaphore_unlock(&gNodeLock);

	// finish verification of children 
	// (must be done now to unblock them; 
	//  else we risk deadlocks during notification)
	reset_children_verification(node);

	// inform all drivers of unregistered nodes
	pnp_notify_unregistration(dependency_list);

	// now, we can safely decrease ref_count of unregistered nodes
	pnp_unref_unregistered_nodes(dependency_list);
}


/** recursively scan children of node (using depth-scan).
 *	node_lock must be hold
 */

static status_t
recursive_scan(pnp_node_info *node, uint depth)
{
	pnp_node_info *child, *next_child;
	status_t res;

	TRACE(("recursive_scan(node: %p, depth=%d)\n", node, depth));

	child = node->children;

	// the child we want to access must be locked
	if (child != NULL)
		++child->ref_count;

	for (; child != NULL; child = next_child) {
		next_child = child->children_next;

		// lock next child, so its node is still valid after rescan
		if (next_child != NULL)
			++next_child->ref_count;

		// during rescan, we must release node_lock to not deadlock
		benaphore_unlock(&gNodeLock);

		res = pnp_rescan(child, depth - 1);

		benaphore_lock(&gNodeLock);

		// unlock current child as it's not accessed anymore
		pnp_remove_node_ref_nolock(child);

		// ignore errors because of touching unregistered nodes	
		if (res != B_OK && res != B_NAME_NOT_FOUND)
			goto err;
	}

	// no need unlock last child (don't be loop already)
	TRACE(("recursive_scan(): done\n"));
	return B_OK;

err:
	if (next_child != NULL)
		pnp_remove_node_ref_nolock(next_child);

	TRACE(("recursive_scan(): failed\n"));
	return res;
}


/** ask bus to (re-)scan for connected devices */

static void
rescan_bus(pnp_node_info *node)
{
	// busses can register their children themselves
	pnp_bus_info *interface;
	void *cookie;
	bool defer_probe;
	uint8 defer_probe_var;

	// delay children probing if requested
	defer_probe = pnp_get_attr_uint8(node,
			PNP_BUS_DEFER_PROBE, &defer_probe_var, false ) == B_OK
		&& defer_probe_var != 0;

	if (defer_probe) {
		TRACE(("defer probing for consumers\n"));
		pnp_defer_probing_of_children(node);
	}

	// load driver during rescan
	pnp_load_driver(node, NULL, (pnp_driver_info **)&interface, &cookie);

	TRACE(("scan bus\n"));

	// block other notifications (currently, only "removed" could be
	// called as driver is/will stay loaded and scanning the bus 
	// concurrently has been assured to not happen)
	pnp_start_hook_call(node);

	if (interface->rescan != NULL)
		interface->rescan(cookie);

	pnp_finish_hook_call(node);

	pnp_unload_driver(node);

	// time to execute delayed probing
	if (defer_probe)
		pnp_probe_waiting_children(node);
}


/** rescan for consumers
 *	ignore_fixed_consumers - true, to leave fixed consumers alone
 */

status_t
pnp_rescan_int(pnp_node_info *node, uint depth, 
	bool ignore_fixed_consumers)
{
	bool is_bus;
	uint8 dummy;
	status_t res = B_OK;

	TRACE(("pnp_rescan_int(node: %p, depth=%d)\n", node, depth));

	//pnp_load_boot_links();

	// ignore depth 0 silently
	if (depth == 0) {
		res = B_OK;
		goto err3;
	}

	is_bus = pnp_get_attr_uint8(node, PNP_BUS_IS_BUS, &dummy, false) == B_OK;

	benaphore_lock(&gNodeLock);

	// don't scan unregistered nodes
	if (!node->registered) {
		TRACE(("node not registered"));
		res = B_NAME_NOT_FOUND;
		goto err2;
	}

	if (!is_rescan_sensible(node)) {
		// somebody else does the rescan, so no need to report error
		TRACE(("rescan not sensible\n"));
		res = B_OK;
		goto err2;
	}

	// mark old devices
	// exception: during registration, fixed consumers are
	// initialized seperately, and we don't want to mark their node
	// as being old
	if (!ignore_fixed_consumers || is_bus)
		mark_children_scanned(node);

	// tell other scans what we are doing to avoid double scans
	node->rescan_depth = depth;

	// keep node alive
	++node->ref_count;
	
	benaphore_unlock(&gNodeLock);

	// do the real thing - scan node
	if (is_bus)
		rescan_bus(node);

	// ask possible consumers to register their nodes
	if (!ignore_fixed_consumers) {
		TRACE(("scan fixed consumers\n"));

		res = pnp_notify_fixed_consumers(node);
		if (res != B_OK)
			goto err;
	}

	res = pnp_notify_dynamic_consumers(node);
	if (res != B_OK)
		goto err;

	// unregister children that weren't detected anymore
	unregister_lost_children(node);

	benaphore_lock(&gNodeLock);

	// mark scan as being finished to not block recursive scans
	node->rescan_depth = 0;

	// scan children recursively;
	// keep the node_lock to make sure noone removes children meanwhile
	if (depth > 1)
		res = recursive_scan(node, depth);

	pnp_remove_node_ref_nolock(node);
	benaphore_unlock(&gNodeLock);

	//pnp_unload_boot_links();

	TRACE(("pnp_rescan_int(): done (%p) - %s\n", node, strerror(res)));
	return res;

err:
	reset_children_verification(node);

	benaphore_lock(&gNodeLock);

	node->rescan_depth = 0;
	pnp_remove_node_ref_nolock(node);

err2:
	benaphore_unlock(&gNodeLock);

	TRACE(("pnp_rescan_int(): failed (%p, %s)\n", node, strerror(res)));

err3:
	//pnp_unload_boot_links();

	return res;	
}
