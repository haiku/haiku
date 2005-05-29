/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	Device Node Registration.
*/


#include "device_manager_private.h"
#include "dl_list.h"

#include <KernelExport.h>

#include <string.h>
#include <stdlib.h>


//#define TRACE_REGISTRATION
#ifdef TRACE_REGISTRATION
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/** mark node being registered, so it can be accessed via load_driver() */

static status_t
mark_node_registered(device_node_info *node)
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

#if 0
/**	Checks whether the device is redetected and removes any other
 *	device that is on same connection and has the same device identifier.
 *	\a redetected is set true if device is already registered;
 *	the node must not yet be linked into parent node's children list
 */

static status_t
is_device_redetected(device_node_info *node, device_node_info *parent, bool *redetected)
{
	device_node_info *sibling;
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
	// no one else (un)-registers a conflicting note during the tests
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
		for (sibling = parent->children; sibling != NULL; sibling = sibling->siblings_next) {
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
		// when gNodeLock has been released
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
#endif


static status_t
push_node_on_stack(struct list *list, device_node_info *node)
{
	struct node_entry *entry = (struct node_entry *)malloc(sizeof(struct node_entry));
	if (entry == NULL)
		return B_NO_MEMORY;

	dm_get_node_nolock(node);
	entry->node = node;
	list_add_item(list, entry);
	return B_OK;
}


static device_node_info *
pop_node_from_stack(struct list *list)
{
	device_node_info *node;

	struct node_entry *entry = (struct node_entry *)list_remove_head_item(list);
	if (entry == NULL)
		return NULL;

	node = entry->node;
	free(entry);

	return node;
}


//	#pragma mark -
//	Public functions part of the module API


/** Register device node.
 *	In terms of I/O resources: if registration fails, they are freed; reason is
 *	that they may have been transferred to node before error and back-transferring 
 *	them would be complicated.
 */

status_t
dm_register_node(device_node_handle parent, const device_attr *attrs,
	const io_resource_handle *ioResources, device_node_handle *_node)
{
	device_node_info *newNode;
	char *driverName = NULL;
	status_t status = B_OK;
	struct list stack;

	status = dm_allocate_node(attrs, ioResources, &newNode);
	if (status != B_OK) {
		// always "consume" I/O resources
		dm_release_io_resources(ioResources);
		return status;
	}

	if (pnp_get_attr_string(newNode, B_DRIVER_MODULE, &driverName, false) != B_OK)
		status = B_BAD_VALUE;

	TRACE(("dm_register_node(driver = %s)\n", driverName));

	free(driverName);

	if (status != B_OK) {
		dprintf("device_manager: Missing driver module name.\n");
		goto err;
	}

	// transfer resources to device, unregistering all colliding devices;
	// this cannot fail - we've already allocated the resource handle array
	dm_assign_io_resources(newNode, ioResources);

	// make it public
	dm_add_child_node(parent, newNode);

	// The following is done to reduce the stack usage of deeply nested
	// child device nodes.
	// There is no other need to delay the complete registration process
	// the way done here. This approach is also slightly different as
	// the registration might fail later than it used in case of errors.

	if (parent == NULL || parent->registered) {
		// register all device nodes not yet registered - build a list
		// that contain all of them, and then empty it one by one (during
		// iteration, there might be new nodes added)

		device_node_info *node = newNode;

		list_init(&stack);
		push_node_on_stack(&stack, newNode);

		while ((node = pop_node_from_stack(&stack)) != NULL) {
			device_node_info *child = NULL;

			// register all fixed child device nodes as well
			status = dm_register_fixed_child_devices(node);
			if (status != B_OK)
				goto err2;

			node->registered = true;

			// and now let it register all child devices, if it's a bus
			status = dm_register_child_devices(node);
			if (status != B_OK)
				goto err2;

			// push all new nodes on the stack

			benaphore_lock(&gNodeLock);

			while ((child = (device_node_info *)list_get_next_item(&node->children, child)) != NULL) {
				if (!child->registered)
					push_node_on_stack(&stack, child);
			}
			benaphore_unlock(&gNodeLock);

			dm_put_node(node);
		}
	}

	if (_node)
		*_node = newNode;

	return B_OK;

err2:
	{
		// nodes popped from the stack also need their reference count released
		device_node_info *node;
		while ((node = pop_node_from_stack(&stack)) != NULL) {
			dm_put_node(node);
		}
	}
err:
	// this also releases the I/O resources
	dm_put_node(newNode);
	return status;
}


/** Unregister device node.
 *	This also makes sure that all children of this node are unregistered.
 */

status_t
dm_unregister_node(device_node_info *node)
{
	device_node_info *child, *nextChild;

	TRACE(("pnp_unregister_device(%p)\n", node));

	if (node == NULL)
		return B_OK;

	// unregistered node and all children
	benaphore_lock(&gNodeLock);
	if (node->parent != NULL)
		list_remove_item(&node->parent->children, node);
	benaphore_unlock(&gNodeLock);

	// tell driver about their unregistration
	dm_notify_unregistration(node);

	// unregister children recursively
	for (child = list_get_first_item(&node->children); child; child = nextChild) {
		nextChild = (device_node_info *)child->siblings.next;
		dm_unregister_node(child);
	}

	dm_put_node(node);

	return B_OK;
}

