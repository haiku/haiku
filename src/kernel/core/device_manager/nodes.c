/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP Manager

	PnP node handling.

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

#include <KernelExport.h>
#include <TypeConstants.h>

#include <stdlib.h>
#include <string.h>


#define TRACE_NODES
#ifdef TRACE_NODES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// increase ref_count of node

void
pnp_add_node_ref(pnp_node_info *node)
{
	TRACE(("add_node_ref(%p)\n", node));

	if (node == NULL)
		return;

	benaphore_lock(&gNodeLock);
	node->ref_count++;
	benaphore_unlock(&gNodeLock);
}


// free attributes of node
// if an attribute is still in use, it's deletion is postponed

static void
free_node_attrs(pnp_node_info *node)
{
	pnp_node_attr_info *attr, *next_attr;

	for (attr = node->attributes; attr != NULL; attr = next_attr) {
		next_attr = attr->next;

		pnp_remove_attr_int(node, attr);
	}

	node->attributes = NULL;
}


// free node's I/O resource list, setting it to NULL
// (resources must have been released already)

static void
free_node_resources(pnp_node_info *node)
{
	free(node->io_resources);
	node->io_resources = NULL;
}


// free node
// (node lock must be hold)

static void
free_node(pnp_node_info *node)
{
	const char *generator_name, *driver_name, *type;
	uint32 auto_id;

	if (pnp_get_attr_string_nolock(node, PNP_DRIVER_DRIVER, &driver_name, false) != B_OK) 
		driver_name = "?";

	if (pnp_get_attr_string_nolock(node, PNP_DRIVER_TYPE, &type, false) != B_OK)
		type = "?";

	TRACE(("free_node(node: %p, driver: %s, type: %s)\n", node, driver_name, type));

	// free associated auto ID, if requested
	if (pnp_get_attr_string_nolock(node, PNP_MANAGER_ID_GENERATOR,
			&generator_name, false) == B_OK) {
		if (pnp_get_attr_uint32(node, PNP_MANAGER_AUTO_ID, &auto_id, false) != B_OK) {
			TRACE(("Cannot find corresponding auto_id of generator %s", generator_name));
		} else
			pnp_free_id(generator_name, auto_id);
	}

	pnp_release_node_resources(node);

	//list_remove_link(node);
	REMOVE_DL_LIST( node, node_list, );

	delete_sem(node->hook_sem);
	delete_sem(node->load_block_sem);

	free_node_attrs(node);
	free_node_resources(node);
	free(node);
}


// remove node reference and clean it up if necessary
// (node_lock must be hold)

void
pnp_remove_node_ref_nolock(pnp_node_info *node)
{
	do {		
		pnp_node_info *parent;

		TRACE(("pnp_remove_node_ref_internal(ref_count of %p: %d)\n", node, node->ref_count - 1));

		// unregistered devices loose their I/O resources as soon as they
		// are unloaded
		if (!node->registered && node->loading == 0 && node->load_count == 0)
			pnp_release_node_resources(node);

		if (--node->ref_count > 0)
			return;

		TRACE(("cleaning up %p (parent: %p)\n", node, node->parent));

		// time to clean up
		parent = node->parent;

		if (parent != NULL)
//			list_remove_item(&parent->children, node);
			REMOVE_DL_LIST(node, parent->children, children_ );

		free_node(node);

		// unrolled recursive call: decrease ref_count of parent as well		
		node = parent;
	} while (node != NULL);
}


// remove node reference and clean it up if necessary

void
pnp_remove_node_ref(pnp_node_info *node)
{
	TRACE(("pnp_remove_node_ref(%p)\n", node));

	benaphore_lock(&gNodeLock);
	pnp_remove_node_ref_nolock(node);
	benaphore_unlock(&gNodeLock);
}


// copy node attributes into node's attribute list

static status_t
copy_node_attrs(pnp_node_info *node, const pnp_node_attr *src)
{
	status_t res;
	const pnp_node_attr *src_attr;
	pnp_node_attr_info *last_attr = NULL;

	node->attributes = NULL;

	// take care to keep attributes in same order;
	// this is (currently) not necessary but a naive implementation would
	// reverse order, which looks a bit odd (at least for the driver writer)
	for (src_attr = src; src_attr->name != NULL; ++src_attr) {
		pnp_node_attr_info *new_attr;

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
allocate_node_resource_array(pnp_node_info *node, const io_resource_handle *resources)
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


// allocate device node info structure;
// initially, ref_count is one to make sure node won't get destroyed by mistake

status_t
pnp_alloc_node(const pnp_node_attr *attrs, const io_resource_handle *resources,
	pnp_node_info **new_node)
{
	pnp_node_info *node;
	status_t res;

	TRACE(("pnp_alloc_node()\n"));

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

	node->parent = NULL;
	node->rescan_depth = 0;
	node->verifying = false;
	node->redetected = false;
	node->init_finished = false;
	node->ref_count = 1;
	node->load_count = 0;
	node->blocked_by_rescan = false;
	node->automatically_loaded = false;

	node->num_waiting_hooks = 0;
	node->num_blocked_loads = 0;
	node->load_block_count = 0;
	node->loading = 0;
	node->defer_probing = 0;
	node->unprobed_children = NULL;

	// make public (well, almost: it's not officially registered yet)
	benaphore_lock(&gNodeLock);

	ADD_DL_LIST_HEAD(node, node_list, );

	benaphore_unlock(&gNodeLock);

	*new_node = node;

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


// create links to parent node

void
pnp_create_node_links(pnp_node_info *node, pnp_node_info *parent)
{
	TRACE(("pnp_create_node_links(%p, parent=%p)\n", node, parent));

	if (parent == NULL)
		return;

	benaphore_lock(&gNodeLock);

	TRACE(("Adding parent link"));

	// parent must not be destroyed	
	parent->ref_count++;
	node->parent = parent;

	// tell parent about us, so we get unregistered automatically if parent
	// gets unregistered
	ADD_DL_LIST_HEAD(node, parent->children, children_ );

	benaphore_unlock(&gNodeLock);
}


// public: get parent of node

pnp_node_handle
pnp_get_parent(pnp_node_handle node)
{
	return node->parent;
}


// public: find node with some node attributes given

pnp_node_info *
pnp_find_device(pnp_node_info *parent, const pnp_node_attr *attrs)
{
	pnp_node_info *node, *found_node;

	found_node = NULL;

	benaphore_lock(&gNodeLock);

	for (node = node_list; node; node = node->next) {
		const pnp_node_attr *attr;

		// list contains removed devices too, so skip them
		if (!node->registered)
			continue;

		if (parent != (pnp_node_info *)-1 && parent != node->parent)
			continue;

		for (attr = attrs; attr && attr->name; ++attr) {
			bool equal = true;

			switch (attr->type) {
				case B_UINT8_TYPE: {
					uint8 value;

					equal = pnp_get_attr_uint8_nolock( node, attr->name, &value, false ) == B_OK
						&& value == attr->value.ui8;
					break;
				}
				case B_UINT16_TYPE: {
					uint16 value;

					equal = pnp_get_attr_uint16_nolock( node, attr->name, &value, false ) == B_OK
						&& value == attr->value.ui16;
					break;
				}
				case B_UINT32_TYPE: {
					uint32 value;

					equal = pnp_get_attr_uint32_nolock( node, attr->name, &value, false ) == B_OK
						&& value == attr->value.ui32;
					break;
				}
				case B_UINT64_TYPE: {
					uint64 value;

					equal = pnp_get_attr_uint64_nolock( node, attr->name, &value, false ) == B_OK
						&& value == attr->value.ui64;
					break;
				}
				case B_STRING_TYPE: {
					const char *str;

					equal = pnp_get_attr_string_nolock( node, attr->name, &str, false ) == B_OK
						&& strcmp( str, attr->value.string ) == 0;
					break;
				}
				case B_RAW_TYPE: {
					const void *data;
					size_t len;

					equal = pnp_get_attr_raw_nolock( node, attr->name, &data, &len, false ) == B_OK
						&& len == attr->value.raw.len
						&& !memcmp(data, attr->value.raw.data, len);
					break;
				}
				default:
					goto err;
			}

			if (!equal)
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


status_t
nodes_init(void)
{
	return benaphore_init(&gNodeLock, "device nodes");
}

