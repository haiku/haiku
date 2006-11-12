/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	device node handling.

	To make sure that nodes persist as long as somebody is using them, we
	maintain a reference count (ref_count) which is increased
	 - when the node is officially registered
	 - if someone loads the corresponding driver
	 - for each child

	node_lock must be hold when updating of reference count, children 
	dependency list and during (un)-registration.
*/


#include "device_manager_private.h"
#include "dl_list.h"

#include <kernel.h>

#include <KernelExport.h>
#include <TypeConstants.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_NODES
#ifdef TRACE_NODES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


benaphore gNodeLock;

uint32 sNodeID;	// nodes counter


static void
put_level(int32 level)
{
	while (level-- > 0)
		dprintf("   ");
}


static void
dump_attribute(device_attr_info *attr, int32 level)
{
	if (attr == NULL)
		return;

	put_level(level + 2);
	dprintf("\"%s\" : ", attr->attr.name);
	switch (attr->attr.type) {
		case B_STRING_TYPE:
			dprintf("string : \"%s\"", attr->attr.value.string);
			break;
		case B_UINT8_TYPE:
			dprintf("uint8 : %u (%#x)", attr->attr.value.ui8, attr->attr.value.ui8);
			break;
		case B_UINT16_TYPE:
			dprintf("uint16 : %u (%#x)", attr->attr.value.ui16, attr->attr.value.ui16);
			break;
		case B_UINT32_TYPE:
			dprintf("uint32 : %lu (%#lx)", attr->attr.value.ui32, attr->attr.value.ui32);
			break;
		case B_UINT64_TYPE:
			dprintf("uint64 : %Lu (%#Lx)", attr->attr.value.ui64, attr->attr.value.ui64);
			break;
		default:
			dprintf("raw data");
	}
	dprintf("\n");

	dump_attribute(attr->next, level);	
}


/**	free attributes of node
 *	if an attribute is still in use, it's deletion is postponed
 */

static void
free_node_attrs(device_node_info *node)
{
	device_attr_info *attr, *next_attr;

	for (attr = node->attributes; attr != NULL; attr = next_attr) {
		next_attr = attr->next;

		pnp_remove_attr_int(node, attr);
	}

	node->attributes = NULL;
}


// free node's I/O resource list, setting it to NULL
// (resources must have been released already)

static void
free_node_resources(device_node_info *node)
{
	free(node->io_resources);
	node->io_resources = NULL;
}


// free node
// (node lock must be hold)

static void
free_node(device_node_info *node)
{
	const char *generator_name, *driver_name;
	uint32 auto_id;

	// ToDo: we lose memory here if the attributes are available!
	if (pnp_get_attr_string_nolock(node, B_DRIVER_MODULE, &driver_name, false) != B_OK) 
		driver_name = "?";

	TRACE(("free_node(node: %p, driver: %s)\n", node, driver_name));

	// free associated auto ID, if requested
	if (pnp_get_attr_string_nolock(node, PNP_MANAGER_ID_GENERATOR,
			&generator_name, false) == B_OK) {
		if (pnp_get_attr_uint32(node, PNP_MANAGER_AUTO_ID, &auto_id, false) != B_OK) {
			TRACE(("Cannot find corresponding auto_id of generator %s", generator_name));
		} else
			dm_free_id(generator_name, auto_id);
	}

	dm_release_node_resources(node);

	delete_sem(node->hook_sem);
	delete_sem(node->load_block_sem);

	free_node_attrs(node);
	free_node_resources(node);
	free(node);
}


// copy node attributes into node's attribute list

static status_t
copy_node_attrs(device_node_info *node, const device_attr *src)
{
	status_t res;
	const device_attr *src_attr;
	device_attr_info *last_attr = NULL;

	node->attributes = NULL;

	// take care to keep attributes in same order;
	// this is (currently) not necessary but a naive implementation would
	// reverse order, which looks a bit odd (at least for the driver writer)
	for (src_attr = src; src_attr->name != NULL; ++src_attr) {
		device_attr_info *new_attr;

		res = pnp_duplicate_node_attr( src_attr, &new_attr );
		if (res != B_OK)
			goto err;

		//list_add_link_to_head(&node->attributes, new_attr);
		ADD_DL_LIST_HEAD( new_attr, node->attributes, );

		last_attr = new_attr;
	}

	return B_OK;

err:
	free_node_attrs(node);
	return res;
}


// allocate array of node I/O resources;
// all resource handles are set to zero, so if something goes wrong
// later on, resources aren't transferred to node 

static status_t
allocate_node_resource_array(device_node_info *node, const io_resource_handle *resources)
{
	const io_resource_handle *resource;
	int num_resources = 0;

	if (resources != NULL) {
		for (resource = resources; *resource != NULL; ++resource)
			++num_resources;
	}

	node->num_io_resources = num_resources;

	node->io_resources = malloc((num_resources + 1) * sizeof(io_resource_handle));
	if (node->io_resources == NULL)
		return B_NO_MEMORY;

	memset(node->io_resources, 0, (num_resources + 1) * sizeof(io_resource_handle));
	return B_OK;
}


//	#pragma mark -
//	Device Manager private functions


/**	allocate device node info structure;
 *	initially, ref_count is one to make sure node won't get destroyed by mistake
 */

status_t
dm_allocate_node(const device_attr *attrs, const io_resource_handle *resources,
	device_node_info **_node)
{
	device_node_info *node;
	status_t res;

	node = calloc(1, sizeof(*node));
	if (node == NULL)
		return B_NO_MEMORY;

	res = copy_node_attrs(node, attrs);
	if (res < 0)
		goto err;

	res = allocate_node_resource_array(node, resources);
	if (res < 0)
		goto err2;

	res = node->hook_sem = create_sem(0, "pnp_hook");
	if (res < 0)
		goto err3;

	res = node->load_block_sem = create_sem(0, "pnp_load_block");
	if (res < 0)
		goto err4;

	list_init(&node->children);

	node->parent = NULL;
	node->rescan_depth = 0;
	node->registered = false;
#if 0
	node->verifying = false;
	node->redetected = false;
	node->init_finished = false;
#endif
	node->ref_count = 1;
	node->load_count = 0;
	node->blocked_by_rescan = false;
	node->automatically_loaded = false;

	node->num_waiting_hooks = 0;
	node->num_blocked_loads = 0;
	node->load_block_count = 0;
	node->loading = 0;
	node->internal_id = sNodeID++;

	TRACE(("dm_allocate_node(): new node %p\n", node));

	*_node = node;

	return B_OK;

err4:
	delete_sem(node->hook_sem);
err3:
	free_node_resources(node);
err2:
	free_node_attrs(node);	
err:
	free(node);
	return res;
}


void
dm_add_child_node(device_node_info *parent, device_node_info *node)
{
	TRACE(("dm_add_child_node(parent = %p, child = %p)\n", parent, node));

	if (parent == NULL)
		return;

	benaphore_lock(&gNodeLock);

	// parent must not be destroyed	as long as it has children
	parent->ref_count++;
	node->parent = parent;

	// tell parent about us, so we get unregistered automatically if parent
	// gets unregistered
	list_add_item(&parent->children, node);

	benaphore_unlock(&gNodeLock);
}


void
dm_get_node_nolock(device_node_info *node)
{
	node->ref_count++;
}


// increase ref_count of node

void
dm_get_node(device_node_info *node)
{
	TRACE(("dm_get_device_node(%p)\n", node));

	if (node == NULL)
		return;

	benaphore_lock(&gNodeLock);
	node->ref_count++;
	benaphore_unlock(&gNodeLock);
}


// remove node reference and clean it up if necessary
// (gNodeLock must be hold)

void
dm_put_node_nolock(device_node_info *node)
{
	do {		
		device_node_info *parent;

		TRACE(("pnp_remove_node_ref_internal(ref_count of %p: %ld)\n", node, node->ref_count - 1));

		// unregistered devices lose their I/O resources as soon as they
		// are unloaded
		if (!node->registered && node->loading == 0 && node->load_count == 0)
			dm_release_node_resources(node);

		if (--node->ref_count > 0)
			return;

		TRACE(("cleaning up %p (parent: %p)\n", node, node->parent));

		// time to clean up
		parent = node->parent;

		if (parent != NULL)
			list_remove_item(&parent->children, node);

		free_node(node);

		// unrolled recursive call: decrease ref_count of parent as well		
		node = parent;
	} while (node != NULL);
}


#if 0
// public: find node with some node attributes given

device_node_info *
dm_find_device(device_node_info *parent, const device_attr *attrs)
{
	device_node_info *node, *found_node;

	found_node = NULL;

	benaphore_lock(&gNodeLock);

	for (node = gNodeList; node; node = node->next) {
		const device_attr *attr;

		// list contains removed devices too, so skip them
		if (!node->registered)
			continue;

		if (parent != (device_node_info *)-1 && parent != node->parent)
			continue;

		for (attr = attrs; attr && attr->name; ++attr) {
			device_attr_info *other = pnp_find_attr_nolock(node, attr->name, false, attr->type);
			if (other == NULL || pnp_compare_attrs(attr, &other->attr))
				break;
		}

		if (attr != NULL && attr->name != NULL)
			continue;

		// we found a node
		if (found_node != NULL)
			// but it wasn't the only one
			return NULL;

		// even though we found a node, keep on search to make sure no other
		// node is matched too
		found_node = node;
	}

err:
	benaphore_unlock(&gNodeLock);

	return found_node;
}
#endif


void
dm_dump_node(device_node_info *node, int32 level)
{
	device_node_info *child = NULL;

	if (node == NULL)
		return;

	put_level(level);
	dprintf("(%ld) @%p \"%s\"\n", level, node, node->driver ? node->driver->info.name : "---");
	dump_attribute(node->attributes, level);

	while ((child = (device_node_info *)list_get_next_item(&node->children, child)) != NULL) {
		dm_dump_node(child, level + 1);
	}
}


status_t
dm_init_nodes(void)
{
	sNodeID = 1;
	return benaphore_init(&gNodeLock, "device nodes");
}


//	#pragma mark -
//	Functions part of the module API


/** remove node reference and clean it up if necessary */

void
dm_put_node(device_node_info *node)
{
	TRACE(("dm_put_node(%p)\n", node));

	benaphore_lock(&gNodeLock);
	dm_put_node_nolock(node);
	benaphore_unlock(&gNodeLock);
}


device_node_info *
dm_get_root(void)
{
	dm_get_node(gRootNode);
	return gRootNode;
}


device_node_info *
dm_get_parent(device_node_info *node)
{
	dm_get_node(node->parent);
	return node->parent;
}


status_t
dm_get_next_child_node(device_node_info *parent, device_node_info **_node,
	const device_attr *attrs)
{
	device_node_info *node = *_node;
	device_node_info *nodeToPut = node;
	
	benaphore_lock(&gNodeLock);

	while ((node = (device_node_info *)list_get_next_item(&parent->children, node)) != NULL) {
		const device_attr *attr;

		// list contains removed devices too, so skip them
		if (!node->registered)
			continue;

		for (attr = attrs; attr && attr->name; ++attr) {
			device_attr_info *other = pnp_find_attr_nolock(node, attr->name, false, attr->type);
			if (other == NULL || pnp_compare_attrs(attr, &other->attr))
				break;
		}

		if (attr != NULL && attr->name != NULL)
			continue;

		// we found a node
		dm_get_node_nolock(node);
		*_node = node;

		if (nodeToPut != NULL)
			dm_put_node_nolock(nodeToPut);

		benaphore_unlock(&gNodeLock);
		
		return B_OK;
	}

	if (nodeToPut != NULL)
		dm_put_node_nolock(nodeToPut);

	benaphore_unlock(&gNodeLock);
	return B_ENTRY_NOT_FOUND;
}


// hold gNodeLock
static device_node_info *
device_manager_find_device(device_node_info *parent, uint32 id)
{
	device_node_info *node = NULL;

	if (parent->internal_id == id)
		return parent;

	while ((node = (device_node_info *)list_get_next_item(&parent->children, node)) != NULL) {
		device_node_info *child = device_manager_find_device(node, id);
		if (child != NULL)
			return child;
	}	
	return NULL;
}


status_t
device_manager_control(const char *subsystem, uint32 function, void *buffer, size_t bufferSize)
{
	switch (function) {
		case DM_GET_ROOT: {
			uint32 cookie;
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize < sizeof(uint32))
				return B_BAD_VALUE;
			cookie = gRootNode->internal_id;

			// copy back to user space
			if (user_memcpy(buffer, &cookie, sizeof(uint32)) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}
		case DM_GET_CHILD: {
			uint32 cookie;
			device_node_info *node;
			device_node_info *child;

			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize < sizeof(uint32))
				return B_BAD_VALUE;
			if (user_memcpy(&cookie, buffer, sizeof(uint32)) < B_OK)
                                return B_BAD_ADDRESS;
			
        		benaphore_lock(&gNodeLock);
			node = device_manager_find_device(gRootNode, cookie);
			if (!node) {
				benaphore_unlock(&gNodeLock);
				return B_BAD_VALUE;
			}

			child = (device_node_info *)list_get_next_item(&node->children, NULL);
			if (child)
				cookie = child->internal_id;
			benaphore_unlock(&gNodeLock);
			
			if (!child)
				return B_ENTRY_NOT_FOUND;

			// copy back to user space
			if (user_memcpy(buffer, &cookie, sizeof(uint32)) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}
		case DM_GET_NEXT_CHILD: {
			uint32 cookie;
			device_node_info *node;
			device_node_info *child;

			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize < sizeof(uint32))
				return B_BAD_VALUE;
			if (user_memcpy(&cookie, buffer, sizeof(uint32)) < B_OK)
				return B_BAD_ADDRESS;
			
			benaphore_lock(&gNodeLock);
			node = device_manager_find_device(gRootNode, cookie);
			if (!node) {
				benaphore_unlock(&gNodeLock);
				return B_BAD_VALUE;
			}
			child = (device_node_info *)list_get_next_item(&node->parent->children, node);
			if (child)
				cookie = child->internal_id;
			benaphore_unlock(&gNodeLock);

			cookie++;

			if (!child)
				return B_ENTRY_NOT_FOUND;
			// copy back to user space
			if (user_memcpy(buffer, &cookie, sizeof(uint32)) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}
		case DM_GET_NEXT_ATTRIBUTE: {
			struct dev_attr attr;
			device_node_info *node;
			uint32 i = 0;
			device_attr_info *attr_info;
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize < sizeof(struct dev_attr))
				return B_BAD_VALUE;
			if (user_memcpy(&attr, buffer, sizeof(struct dev_attr)) < B_OK)
				return B_BAD_ADDRESS;

			benaphore_lock(&gNodeLock);
			node = device_manager_find_device(gRootNode, attr.node_cookie);
			if (!node) {
				benaphore_unlock(&gNodeLock);
				return B_BAD_VALUE;
			}
			for (attr_info = node->attributes; attr.cookie > i && attr_info != NULL; attr_info = attr_info->next) {
				i++;
			}

			if (!attr_info) {
				benaphore_unlock(&gNodeLock);
				return B_ENTRY_NOT_FOUND;
			}

			attr.cookie++;

			strlcpy(attr.name, attr_info->attr.name, 254);
			attr.type = attr_info->attr.type;
			switch (attr_info->attr.type) {
				case B_UINT8_TYPE:
					attr.value.ui8 = attr_info->attr.value.ui8; break;
				case B_UINT16_TYPE:
					attr.value.ui16 = attr_info->attr.value.ui16; break;
				case B_UINT32_TYPE:
					attr.value.ui32 = attr_info->attr.value.ui32; break;
				case B_UINT64_TYPE:
					attr.value.ui64 = attr_info->attr.value.ui64; break;
				case B_STRING_TYPE:
					strlcpy(attr.value.string, attr_info->attr.value.string, 254); break;
				case B_RAW_TYPE:
					if (attr.value.raw.length > attr_info->attr.value.raw.length)
						attr.value.raw.length = attr_info->attr.value.raw.length;
					memcpy(attr.value.raw.data, attr_info->attr.value.raw.data,
						attr.value.raw.length);
					break;
			}

			benaphore_unlock(&gNodeLock);

			// copy back to user space
			if (user_memcpy(buffer, &attr, sizeof(struct dev_attr)) < B_OK) 
				return B_BAD_ADDRESS;
			return B_OK;
		}
	};
	return B_BAD_HANDLER;
}
