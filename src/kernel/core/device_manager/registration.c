/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
	Part of PnP Manager

	PnP Node Registration.
*/


#include "device_manager_private.h"
#include "dl_list.h"

#include <KernelExport.h>

#include <string.h>
#include <stdlib.h>


#define TRACE_REGISTRATION
#ifdef TRACE_REGISTRATION
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// decrease ref_count of unregistered nodes, specified by <node_list>

void
pnp_unref_unregistered_nodes(pnp_node_info *node_list)
{
	pnp_node_info *node, *next_node;

	for (node = node_list; node; node = next_node) {
		next_node = node->notify_next;

		pnp_remove_node_ref(node);
	}
}


// mark node being registered, so it can be accessed via load_driver()

static status_t
mark_node_registered(pnp_node_info *node)
{
	status_t res;

	TRACE(("mark_node_registered(%p)\n", node));
	
	benaphore_lock(&gNodeLock);

	if (node->parent && !node->parent->registered) {
		TRACE(("Cannot mark registered: parent is already unregistered\n"));
		res = B_NOT_ALLOWED;
	} else {
		res = B_OK;
		node->registered = true;
		++node->ref_count;
	}

	benaphore_unlock(&gNodeLock);

	return res;
}


// check whether device is redetected and remove 
// any device that is on same connection.
// *redetected is set true if device is already registered;
// the node must not yet be linked into parent node's children list

static status_t
is_device_redetected(pnp_node_info *node, pnp_node_info *parent, bool *redetected)
{
	pnp_node_info *sibling;
	status_t res;
	bool found = false;
	bool same_device = false;
	bool unregister_sibling;

	TRACE(("Checking %p, parent %p\n", node, parent));

	*redetected = false;

	if (parent == NULL) {
		TRACE(("done\n"));
		return false;
	}

	// we keep the lock very long, but this is the only way to be sure that
	// noone else (un)-registers a conflicting note during the tests
	benaphore_lock(&gNodeLock);

	{
		char *connection, *device_identifier;

		// get our connection and device id...
		if (pnp_expand_pattern_attr(node, PNP_DRIVER_CONNECTION, &connection) != B_OK)
			connection = strdup("");

		if (pnp_expand_pattern_attr(node, PNP_DRIVER_DEVICE_IDENTIFIER, 
				&device_identifier) != B_OK)
			device_identifier = strdup("");

		TRACE(("connection: %s, device_identifier: %s\n", connection, device_identifier));

		// ...and compare it with our siblings
		for (sibling = parent->children; sibling != NULL; sibling = sibling->children_next) {
			char *sibling_connection, *sibling_device_identifier;
			bool same_connection;

			// ignore dead siblings
			if (!sibling->registered)
				break;

			TRACE(("%p\n", sibling));

			if (pnp_expand_pattern_attr(sibling, PNP_DRIVER_CONNECTION,
					&sibling_connection) != B_OK)
				sibling_connection = strdup("");

			same_connection = !strcmp(connection, sibling_connection);
			free(sibling_connection);

			if (!same_connection)
				continue;

			// found a device on same connection - check whether it's the same device
			found = true;

			if (pnp_expand_pattern_attr(sibling, PNP_DRIVER_DEVICE_IDENTIFIER,
					&sibling_device_identifier) != B_OK)
				sibling_device_identifier = strdup("");

			TRACE(("device %s is on same connection\n", sibling_device_identifier));
				
			same_device = !strcmp(device_identifier, sibling_device_identifier);
			free(sibling_device_identifier);
			break;
		}

		free(connection);
		free(device_identifier);
	}

	if (found) {
		TRACE(("found device on same connection\n"));
		if (!same_device) {
			// there is a different device on the same connection -
			// kick out the old device
			TRACE(("different device was detected\n"));

			*redetected = false;
			unregister_sibling = true;
		} else {
			const char *driver_name, *old_driver_name;
			const char *driver_type, *old_driver_type;

			TRACE(("it's the same device\n"));

			// found same device on same connection - make sure it's still the same driver
			if (pnp_get_attr_string_nolock(node, PNP_DRIVER_DRIVER, &driver_name, false) != B_OK
				|| pnp_get_attr_string_nolock(sibling, PNP_DRIVER_DRIVER, &old_driver_name, false) != B_OK
				|| pnp_get_attr_string_nolock(node, PNP_DRIVER_TYPE, &driver_type, false) != B_OK
				|| pnp_get_attr_string_nolock(sibling, PNP_DRIVER_TYPE, &old_driver_type, false) != B_OK) {
				// these attributes _must_ be there, so this cannot happen
				// (but we are prepared)
				res = B_ERROR;
				goto err;
			}

			if (strcmp(driver_name, old_driver_name) != 0
				|| strcmp(driver_type, old_driver_type) != 0) {
				// driver has changed - replace device node
				TRACE(("driver got replaced\n"));

				*redetected = false;
				unregister_sibling = true;
			} else {
				// it's the same driver for the same device
				TRACE(("redetected device\n"));

				*redetected = true;
				unregister_sibling = false;
			}
		}
	} else {
		// no collision on the device's connection
		TRACE(("no collision\n"));

		*redetected = false;
		unregister_sibling = false;
	}

	if (*redetected && sibling->verifying)
		sibling->redetected = true;

	if (unregister_sibling) {
		// increase ref_count to make sure node still exists 
		// when node_lock has been released
		++sibling->ref_count;
		benaphore_unlock(&gNodeLock);
	
		pnp_unregister_device(sibling);
		pnp_remove_node_ref_nolock(sibling);
	} else
		benaphore_unlock(&gNodeLock);

	return B_OK;

err:	
	benaphore_unlock(&gNodeLock);

	return res;
}


// postpone searching for consumers if necessary
// return: true, if postponed

static bool
pnp_postpone_probing(pnp_node_info *node)
{
	benaphore_lock(&gNodeLock);

	// ask parent(!) if probing is to be postponed
	if (node->parent == NULL || !node->parent->defer_probing) {
		benaphore_unlock(&gNodeLock);
		return false;
	}

	// yes: this happens if the new node is a child of a bus node whose
	// rescan has not been finished	
	TRACE(("postpone probing of node %p\n", node));

	ADD_DL_LIST_HEAD(node, node->parent->unprobed_children, unprobed_ );
	benaphore_unlock(&gNodeLock);

	return true;
}


// public: register device node.
// in terms of I/O resources: if registration fails, they are freed; reason is
// that they may have been transferred to node before error and back-transferring 
// them would be complicated

status_t
pnp_register_device(pnp_node_handle parent, const pnp_node_attr *attrs,
	const io_resource_handle *io_resources, pnp_node_handle *node)
{
	pnp_node_info *node_inf;
	bool redetected;
	status_t res = B_OK;

	res = pnp_alloc_node(attrs, io_resources, &node_inf);
	if (res != B_OK)
		goto err;

	{
		char *driver_name, *type;

		if (pnp_get_attr_string(node_inf, PNP_DRIVER_DRIVER, &driver_name, false) != B_OK) {
			dprintf("Missing driver filename in node\n");
			res = B_BAD_VALUE;
			goto err1;
		}

		if (pnp_get_attr_string(node_inf, PNP_DRIVER_TYPE, &type, false) != B_OK) {
			dprintf("Missing type in node registered by %s\n", driver_name);

			free(driver_name);
			res = B_BAD_VALUE;
			goto err1;
		}

		TRACE(("driver: %s, type: %s\n", driver_name, type));

		free(driver_name);
		free(type);
	}

	// check whether this device already existed and thus is redetected	
	res = is_device_redetected(node_inf, parent, &redetected);
	if (res != B_OK)
		goto err1;

	if (redetected) {
		// upon redetect, resources are released instead of transferred and
		// no node is returned
		*node = NULL;
		res = B_OK;
		goto err1;
	}

	// transfer resources to device, unregistering all colliding devices;
	// this cannot fail - we've already allocated the resource handle array
	pnp_assign_io_resources(node_inf, io_resources);

	pnp_create_node_links(node_inf, parent);

	// make it public
	res = mark_node_registered(node_inf);
	if (res != B_OK)
		goto err2;

	// from now on, node won't get freed as ref_count has been increased by registration

	// check whether searching for consumers should be deferred
	if (pnp_postpone_probing(node_inf)) {
		// return without decrementing ref_count - else node may get
		// lost before deferred probe
		*node = node_inf;
		return B_OK;
	}

	res = pnp_initial_scan(node_inf);
	if (res != B_OK)
		goto err2;

	pnp_load_driver_automatically(node_inf, false);

	*node = node_inf;

	TRACE(("done: node=%p\n", *node));

	// alloc_node has set ref_count to one for safety, correct this now
	pnp_remove_node_ref(node_inf);
	return res;

err2:
	// use this exit after i/o resources have been transferred to node
	pnp_remove_node_ref(node_inf);
	return res;

err1:
	// alloc_node has set ref_count to one for safety, correct this now
	pnp_remove_node_ref(node_inf);
err:
	// always "consume" i/o resources
	pnp_release_io_resources(io_resources);
	return res;
}


// public: unregister device node

status_t
pnp_unregister_device(pnp_node_info *node)
{
	pnp_node_info *dependency_list = NULL;

	TRACE(("pnp_unregister_device(%p)\n", node));

	if (node == NULL)
		return B_OK;

	// unregistered node and all children	
	benaphore_lock(&gNodeLock);
	pnp_unregister_node_rec(node, &dependency_list);
	benaphore_unlock(&gNodeLock);

	pnp_unload_driver_automatically(node, false);

	// tell drivers about their unregistration
	pnp_notify_unregistration(dependency_list);

	// now, we can safely decrease ref_count of unregistered nodes
	pnp_unref_unregistered_nodes(dependency_list);

	return B_OK;
}


// remove <registered> flag of node and all children.
// list of all unregistered nodes is appended to <dependency_list>;
// (node_lock must be hold)

void
pnp_unregister_node_rec(pnp_node_info *node, pnp_node_info **dependency_list)
{
	pnp_node_info *child, *next_child;

	TRACE(("pnp_unregister_node_rec(%p)\n", node));

	{
		const char *driver_name, *type;

		if (pnp_get_attr_string_nolock(node, PNP_DRIVER_DRIVER, &driver_name, false) != B_OK) {
			dprintf("unregister_node: Missing driver filename in node\n");
			goto err1;
		}

		if (pnp_get_attr_string_nolock( node, PNP_DRIVER_TYPE, &type, false) != B_OK) {
			dprintf("unregister_node: Missing type in node registered by %s\n", driver_name);
			goto err1;
		}

		TRACE(("driver: %s, type: %s\n", driver_name, type));
err1:
	}

	// especially when we go through children, it can happen that they
	// got unregistered already, so ignore them silently
	if (!node->registered)
		return;

	TRACE(("Preparing unregistration\n"));

	node->registered = false;
	ADD_DL_LIST_HEAD(node, *dependency_list, notify_);
	
	// unregister children recursively
	for (child = node->children; child; child = next_child) {
		next_child = child->children_next;
		pnp_unregister_node_rec(child, dependency_list);
	}
}


// defer probing of children.
// node_lock must be hold

void
pnp_defer_probing_of_children_nolock(pnp_node_info *node)
{
	++node->defer_probing;
}


// defer probing of children

void
pnp_defer_probing_of_children(pnp_node_info *node)
{
	benaphore_lock(&gNodeLock);

	pnp_defer_probing_of_children_nolock(node);

	benaphore_unlock(&gNodeLock);
}


// execute deferred probing of children
// (node_lock must be hold)

void
pnp_probe_waiting_children_nolock(pnp_node_info *node)
{
	if (--node->defer_probing > 0 && node->unprobed_children != 0)
		return;

	TRACE(("execute deferred probing of parent %p", node));

	while (node->unprobed_children) {
		pnp_node_info *child = node->unprobed_children;

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
}


// execute deferred probing of children

void
pnp_probe_waiting_children(pnp_node_info *node)
{
	benaphore_lock(&gNodeLock);

	pnp_probe_waiting_children_nolock(node);

	benaphore_unlock(&gNodeLock);
}

