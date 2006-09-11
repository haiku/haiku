/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	I/O resource manager.

	I/O resources are allocated in two flavours:
	  1. explicit allocation for hardware detection ("temporary allocation")
	  2. implicit allocation for registered devices

	If hardware detection fails, the resources are freed explicitely,
	if it succeeds, ownership is transfered to the device node.

	Resources allocation collision is handled in the following way:
	 - during ownership transfer to a device node, all other device nodes
	   that collide in terms of resources are removed
	 - if two temporary allocations collide, one must wait
	 - if a temporary allocation collides with a device node whose driver is
	   loaded, the temporary allocation fails (waiting for the driver may
	   be hopeless as the driver may stay in memory forever)
	 - if a temporary allocation collides with a device node whose driver is
	   not loaded, loading of the driver is blocked

	To avoid deadlocks between temporary allocations, they are done atomically:
	either the entire allocation succeeds or fails. If it fails because of a
	collision with another temporary allocation, a complete retry is done as
	soon as someone released some resource.
*/

#include "device_manager_private.h"
#include "dl_list.h"

#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_IO_RESOURCES
#ifdef TRACE_IO_RESOURCES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// forward declarations to avoid inlining
static void unblock_temporary_allocation(void);
static void release_range(io_resource_info **list, io_resource_info *resource, 
	io_resource_info *up_to);
static void unregister_colliding_node_range(io_resource_info *list, 
	io_resource_info *resource);


/** validate I/O resource data */

static status_t
validate_io_resource(io_resource *src)
{
	TRACE(("validate_io_resource()\n"));

	switch (src->type) {
		case IO_MEM:
			if (src->base + src->length < src->base)
				return B_BAD_VALUE;
			break;
		case IO_PORT:
			if ((uint16)src->base != src->base
				|| (uint16)src->length != src->length
				|| (uint16)(src->base + src->length) < src->base)
				return B_BAD_VALUE;
			break;
		case ISA_DMA_CHANNEL:
			if (src->base > 8 || src->length != 1)
				return B_BAD_VALUE;
			break;

		default:
			return B_BAD_VALUE;
	}

	TRACE(("  success!\n"));
	return B_OK;
}


/** Allocate info structure for I/O resources */

static status_t
alloc_io_resource_info(io_resource *src, io_resource_info **dest_out)
{
	io_resource_info *dest;

	TRACE(("alloc_io_resource_info()\n"));	

	dest = calloc(1, sizeof(*dest));
	if (dest == NULL)
		return B_NO_MEMORY;

	dest->owner = NULL;

	dest->resource.type = src->type;
	dest->resource.base = src->base;
	dest->resource.length = src->length;

	*dest_out = dest;
	return B_OK;
}


/**	Acquire I/O range; returns B_WOULD_BLOCK on collision with 
 *	temporary allocation.
 *	gNodeLock must be held
 */

static status_t
acquire_range(io_resource_info **list, io_resource_info *resource)
{
	io_resource_info *cur;
	uint32 base = resource->resource.base;
	uint32 length = resource->resource.length;
	status_t res = B_OK;

	TRACE(("acquire_range(base %lx, length %lx)\n", base, length));

	for (cur = *list; cur != NULL; cur = cur->next) {
		// we need the "base + length - 1" trick to avoid wrap around at 4 GB
		if (cur->resource.base >= base
			&& cur->resource.base + length - 1 <= base + length - 1) {
			device_node_info *owner = cur->owner;

			TRACE(("  collision\n"));

			if (owner == NULL) {
				// collision with temporary allocation - block ourself
				TRACE(("  collision with temporary allocation - retry later\n"));

				res = B_WOULD_BLOCK;
				break;
			} else {
				// collision with device
				if (owner->loading + owner->load_count > 0) {
					// driver is already loaded - give up
					// (driver may get loaded forever)
					TRACE(("  collision with loaded driver - giving up\n"));

					res = B_BUSY;
					break;
				} else {
					// driver is not loaded - block loading
					TRACE(("  collision with unloaded driver - block driver %p\n", owner));

					++owner->load_block_count;
				}
			}
		}
	}

	if (cur != NULL) {
		// collided with someone
		TRACE(("  Resource collision\n"));

		release_range(list, resource, cur);
		return res;
	}

	ADD_DL_LIST_HEAD(resource, *list, );

	return B_OK;
}


/** Free I/O resource info */

static void
free_io_resource_info(io_resource_info *resource)
{
	TRACE(("free_io_resource_info()\n"));

	free(resource);
}


/**	Acquire I/O resource.
 *	An I/O resource info structure is allocated and added to 
 *	appropriate resource list (i.e. mem/port/channel) plus
 *	all colliding resources (including ours) are marked blocked;
 *	returns B_WOULD_BLOCK on collision with temporary allocation;
 *	gNodeLock must be held.
 */

static status_t
acquire_io_resource(io_resource *src, io_resource_handle *dest_out)
{
	io_resource_info *dest;
	status_t res;

	TRACE(("acquire_io_resource(type = %ld, base = %lx, length = %lx)\n",
		src->type, src->base, src->length));

	res = validate_io_resource(src);
	if (res != B_OK)
		return res;

	res = alloc_io_resource_info(src, &dest);
	if (res != B_OK)
		return res;

	switch (src->type) {
		case IO_MEM:
			res = acquire_range(&io_mem_list, dest);
			break;
		case IO_PORT:
			res = acquire_range(&io_port_list, dest);
			break;
		case ISA_DMA_CHANNEL:
			res = acquire_range(&isa_dma_list, dest);
			break;
	}

	if (res != B_OK)
		// collided with someone
		goto err;

	*dest_out = dest;

	TRACE(("  done\n"));
	return B_OK;

err:
	free_io_resource_info(dest);

	TRACE(("  failed: %s\n", strerror(res)));
	return res;
}


/**	Release I/O resource.
 *	gNodeLock must be held
 */

static void
release_io_resource(io_resource_info *resource)
{
	TRACE(("release_io_resource()\n"));

	switch (resource->resource.type) {
		case IO_MEM:
			release_range(&io_mem_list, resource, NULL);
			break;
		case IO_PORT:
			release_range(&io_port_list, resource, NULL);
			break;
		case ISA_DMA_CHANNEL:
			release_range(&isa_dma_list, resource, NULL);
			break;
	}

	free_io_resource_info(resource);
}


/**	Release I/O range, which can be I/O memory, ports and ISA DMA channels
 *	up_to - up to but not including colliding range to be released
 *	        (NULL for normal release, not-NULL if used during failed allocation)
 *	blocked users get notified;
 *	gNodeLock must be held
 */

static void
release_range(io_resource_info **list, io_resource_info *resource, io_resource_info *up_to)
{
	io_resource_info *cur;
	uint32 base = resource->resource.base;
	uint32 length = resource->resource.length;

	TRACE(("release_range()\n"));

	for (cur = *list; cur != NULL && cur != up_to; cur = cur->next) {
		if (cur == resource)
			// ignore ourselves
			continue;

		// we need the "base + length - 1" trick to avoid wrap around at 4 GB
		if (cur->resource.base >= base && cur->resource.base + length - 1 <= base + length - 1) {
			device_node_info *owner = cur->owner;

			TRACE(("  unblock\n"));

			if (owner != NULL) {
				// collision with driver - unblock it
				TRACE(("  collision with driver - unblock driver %p\n", owner));

				pnp_unblock_load(owner);
			}
		}
	}

	if (up_to == NULL) {
		// normal release - remove from list and 
		// notify blocked temporary allocations
		REMOVE_DL_LIST(resource, *list, );

		unblock_temporary_allocation();
	}
}


/**	Wait until someone freed some resource or 
 *	allocated some resource permanently.
 *	gNodeLock must be held.
 */

static void
wait_for_resources(void)
{
	TRACE(("wait_for_resources()\n"));

	++pnp_resource_wait_count;

	// we have to release while waiting
	benaphore_unlock(&gNodeLock);

	acquire_sem(pnp_resource_wait_sem);

	benaphore_lock(&gNodeLock);
}


/**	Try to acquire list of resources.
 *	returns B_WOULD_BLOCK on collision with temporary allocation.
 *	gNodeLock must be held.
 */

static status_t
try_acquire_io_resources(io_resource *resources, io_resource_handle *handles)
{
	io_resource *resource;
	io_resource_handle *handle, *handle2;
	status_t res = B_OK;

	for (resource = resources, handle = handles;
			resource->type != 0; ++resource, ++handle) {
		res = acquire_io_resource(resource, handle);
		if (res != B_OK)
			break;
	}

	if (res != B_OK)
		goto err;

	*handle++ = NULL;

	return B_OK;

err:	
	// collision detected: free all previously acquired resources
	for (handle2 = handle; handle2 != handle; ++handle2)
		release_io_resource(*handle2);

	return res;
}


/**	Acquire I/O resources.
 *	gNodeLock must be held.
 */

static status_t 
acquire_io_resources(io_resource *resources, io_resource_handle *handles)
{
	// try allocation until we either got all resources or
	// detected a collision with a loaded device node
	while (true) {
		status_t res;

		// allocate all resources
		res = try_acquire_io_resources( resources, handles );
		if (res == B_OK)
			return B_OK;
		
		if (res == B_WOULD_BLOCK) {
			// collision with temporary allocation:
			// wait until some resource is freed or blocked forever
			// by a loaded driver
			wait_for_resources();
		} else {
			// collided with loaded node - no point waiting for the node
			TRACE(("acquire_io_resouces(): Collision with loaded device node\n"));
			return res;
		}
	}
}


/** Unregister devices that collide with one of our I/O resources */

static void
unregister_colliding_nodes(const io_resource_handle *resources)
{
	const io_resource_handle *resource;

	for (resource = resources; *resource != NULL; ++resource) {
		switch ((*resource)->resource.type) {
			case IO_MEM:
				unregister_colliding_node_range(io_mem_list, *resource);
				break;
			case IO_PORT:
				unregister_colliding_node_range(io_port_list, *resource);
				break;
			case ISA_DMA_CHANNEL:
				unregister_colliding_node_range(isa_dma_list, *resource);
				break;
		}
	}
}


/** Unregister device nodes that collide with I/O resource */

static void
unregister_colliding_node_range(io_resource_info *list, io_resource_info *resource)
{
	io_resource_info *cur;
	uint32 base = resource->resource.base;
	uint32 length = resource->resource.length;

	do {
		for (cur = list; cur != NULL; cur = cur->next) {
			// ignore ourself
			if (cur == resource)
				continue;

			// we need the "base + length - 1" trick to avoid wrap around at 4 GB
			if (cur->resource.base >= base
				&& cur->resource.base + length - 1 <= base + length - 1) {
				device_node_handle owner = cur->owner;

				if (owner == NULL)
					panic("Transfer of I/O resources from temporary to device node collided with other temporary allocation");

				TRACE(("Deleting colliding device node %p\n", owner));

				// make sure node still exists after we'll have released lock
				++owner->ref_count;

				// we have to release lock during unregistration
				benaphore_unlock(&gNodeLock);

				// unregister device node - we own its resources now
				// (its resources are freed by the next remove_node_ref call)
				dm_unregister_node(owner);

				benaphore_lock(&gNodeLock);

				dm_put_node_nolock(owner);
				// restart loop as resource list may have changed meanwhile
				break;
			}
		}
	} while (cur != NULL);
}


/**	Unblock other temporary allocations so they can retry.
 *	gNodeLock must be held.
 */

static void
unblock_temporary_allocation(void)
{
	if (pnp_resource_wait_count > 0) {
		TRACE(("unblock concurrent temporary allocation\n"));

		release_sem_etc(pnp_resource_wait_sem, pnp_resource_wait_count, 0);
		pnp_resource_wait_count = 0;
	}
}


//	#pragma mark -


/**	Transfer temporary I/O resources to device node;
 *	colliding devices are unregistered.
 */

void
dm_assign_io_resources(device_node_info *node, const io_resource_handle *resources)
{
	io_resource_handle *resource;

	if (resources == NULL)
		return;

	unregister_colliding_nodes(resources);

	memcpy(node->io_resources, resources,
		(node->num_io_resources + 1) * sizeof(io_resource_handle));
	
	benaphore_lock(&gNodeLock);

	for (resource = node->io_resources; *resource != NULL; ++resource) {
		(*resource)->owner = node;
	}

	// as the resources were only temporary previously, other temporary 
	// allocations may be waiting for us; time to wake them, so they can
	// give up
	unblock_temporary_allocation();

	benaphore_unlock(&gNodeLock);
}


/**	Release I/O resources of a device node and set list to NULL;
 *	users previously blocked by our resource alloction are notified;
 *	gNodeLock must be held.
 */

void
dm_release_node_resources(device_node_info *node)
{
	io_resource_handle *resource;

	if (*node->io_resources != NULL)
		TRACE(("freeing I/O resources of node %p\n", node));

	for (resource = node->io_resources; *resource != NULL; ++resource) {
		// set owner to NULL as we don't want to resource anymore
		(*resource)->owner = NULL;
		release_io_resource(*resource);
	}

	// make sure we won't release resources twice
	*node->io_resources = NULL;
}


//	#pragma mark -
//	Part of the module API


/** acquire I/O resources */

status_t
dm_acquire_io_resources(io_resource *resources, io_resource_handle *handles)
{
	status_t status;

	TRACE(("dm_acquire_io_resources()\n"));

	benaphore_lock(&gNodeLock);
	status = acquire_io_resources(resources, handles);
	benaphore_unlock(&gNodeLock);

	TRACE(("  done (%s)\n", strerror(status)));

	return status;
}


/** release I/O resources */

status_t
dm_release_io_resources(const io_resource_handle *handles)
{
	io_resource_info *const *resource;

	if (handles == NULL)
		return B_OK;

	benaphore_lock(&gNodeLock);

	for (resource = handles; *resource != NULL; ++resource)
		release_io_resource(*resource);

	benaphore_unlock(&gNodeLock);

	return B_OK;
}

