/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "domains.h"
#include "interfaces.h"
#include "utility.h"
#include "stack_private.h"

#include <net_device.h>
#include <NetUtilities.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>

#include <net/if_media.h>
#include <new>
#include <string.h>
#include <sys/sockio.h>


#define TRACE_DOMAINS
#ifdef TRACE_DOMAINS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define ENABLE_DEBUGGER_COMMANDS	1


typedef DoublyLinkedList<net_domain_private> DomainList;

static mutex sDomainLock;
static DomainList sDomains;


/*!	Scans the domain list for the specified family.
	You need to hold the sDomainLock when calling this function.
*/
static net_domain_private*
lookup_domain(int family)
{
	ASSERT_LOCKED_MUTEX(&sDomainLock);

	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		if (domain->family == family)
			return domain;
	}

	return NULL;
}


#if	ENABLE_DEBUGGER_COMMANDS


static int
dump_domains(int argc, char** argv)
{
	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		kprintf("domain: %p, %s, %d\n", domain, domain->name, domain->family);
		kprintf("  module:         %p\n", domain->module);
		kprintf("  address_module: %p\n", domain->address_module);

		if (domain->rnh_head != NULL && domain->rnh_head->rnh_treetop != NULL) {
			kprintf("  routes (radix tree):\n");
			// Define a walktree callback function
			struct DumpContext {
				net_domain_private* domain;
				int count;
			};
			walktree_f_t dump_route_callback_func = [](struct radix_node* node, void* context) -> int {
				DumpContext* ctx = (DumpContext*)context;
				net_domain_private* currentDomain = ctx->domain;
				net_route_private* route = net_route_private::FromRadixNode(node);

				// Iterate through the main node and its duped keys
				for (net_route_private* currentRoute = route; currentRoute != NULL;
					 currentRoute = (currentRoute == route) ? net_route_private::FromRadixNode(node->rn_dupedkey)
					                                        : net_route_private::FromRadixNode(currentRoute->rn_nodes[0].rn_dupedkey)) {
					// Check if currentRoute is valid (FromRadixNode can return NULL)
					if (currentRoute == NULL && node->rn_dupedkey != NULL && currentRoute != route ) {
						// This can happen if FromRadixNode on a dupedkey's node fails.
						// We need a more robust way to walk dupedkeys if rn_nodes[0] is not always the rn_dupedkey link anchor.
						// The rn_dupedkey is on the radix_node itself.
						// So, if currentRoute was derived from 'node', the next is from node->rn_dupedkey.
						// If currentRoute was from a previous dupedkey, its own rn_nodes[0].rn_dupedkey.
						// This loop structure is a bit complex. Let's simplify.
						// rn_walktree gives one leaf node. We then iterate its dupedkey chain.
						break; // Safety break for complex loop logic, will refine.
					}
					if (currentRoute == NULL) break;


					kprintf("    %p: dest %s, mask %s, gw %s, flags %" B_PRIx32 ", "
						"address %p, iface %s, ref %" B_PRId32 "\n",
						currentRoute,
						AddressString(currentDomain, currentRoute->destination).Data(),
						AddressString(currentDomain, currentRoute->mask).Data(),
						AddressString(currentDomain, currentRoute->gateway).Data(),
						currentRoute->flags, currentRoute->interface_address,
						currentRoute->interface_address ? currentRoute->interface_address->interface->name : "none",
						currentRoute->ref_count);
					ctx->count++;
					if (currentRoute == route && node->rn_dupedkey == NULL) break; // No dupes or only one
					if (currentRoute != route && currentRoute->rn_nodes[0].rn_dupedkey == NULL) break; // End of chain
				}
				return 0; // Continue walking
			};

			// Simpler iteration for duped keys directly from the node returned by walktree
			walktree_f_t dump_route_callback_func_simplified = [](struct radix_node* rn, void* context) -> int {
				DumpContext* ctx = (DumpContext*)context;
				net_domain_private* currentDomain = ctx->domain;

				// rn is a leaf node. Iterate it and its dupedkey chain.
				for (struct radix_node* iterNode = rn; iterNode != NULL; iterNode = iterNode->rn_dupedkey) {
					net_route_private* route = net_route_private::FromRadixNode(iterNode);
					if (route == NULL) continue; // Should not happen for valid nodes

					kprintf("    %p: dest %s, mask %s, gw %s, flags %" B_PRIx32 ", "
						"address %p, iface %s, ref %" B_PRId32 "\n",
						route,
						AddressString(currentDomain, route->destination).Data(),
						AddressString(currentDomain, route->mask).Data(),
						AddressString(currentDomain, route->gateway).Data(),
						route->flags, route->interface_address,
						route->interface_address ? route->interface_address->interface->name : "none",
						route->ref_count);
					ctx->count++;
				}
				return 0; // Continue walking
			};

			DumpContext context = {domain, 0};
			if (domain->rnh_head->rnh_walktree != NULL) { // Check if function pointer is valid
				// Acquire domain lock before walking its routes
				RecursiveLocker locker(domain->lock);
				domain->rnh_head->rnh_walktree(domain->rnh_head, dump_route_callback_func_simplified, &context);
				kprintf("    Total routes in radix tree: %d\n", context.count);
			} else {
				kprintf("    rnh_walktree function pointer is NULL.\n");
			}
		} else {
			kprintf("  No routes or radix head not initialized.\n");
		}

		if (!domain->route_infos.IsEmpty())
			kprintf("  route infos:\n");
	
		RouteInfoList::Iterator infoIterator = domain->route_infos.GetIterator();
		while (net_route_info* info = infoIterator.Next()) {
			kprintf("    %p\n", info);
		}
	}

	return 0;
}


#endif	// ENABLE_DEBUGGER_COMMANDS


//	#pragma mark -


/*!	Gets the domain of the specified family.
*/
net_domain*
get_domain(int family)
{
	MutexLocker locker(sDomainLock);
	return lookup_domain(family);
}


status_t
register_domain(int family, const char* name,
	struct net_protocol_module_info* module,
	struct net_address_module_info* addressModule,
	net_domain** _domain)
{
	TRACE(("register_domain(%d, %s)\n", family, name));
	MutexLocker locker(sDomainLock);

	struct net_domain_private* domain = lookup_domain(family);
	if (domain != NULL)
		return B_NAME_IN_USE;

	domain = new(std::nothrow) net_domain_private;
	if (domain == NULL)
		return B_NO_MEMORY;

	recursive_lock_init(&domain->lock, name);

	domain->family = family;
	domain->name = name;
	domain->module = module;
	domain->address_module = addressModule;
	domain->rnh_head = NULL; // Initialize to NULL

	status_t status = init_routing_domain_radix(domain);
	if (status != B_OK) {
		recursive_lock_destroy(&domain->lock);
		delete domain;
		return status;
	}

	sDomains.Add(domain);

	*_domain = domain;
	return B_OK;
}


status_t
unregister_domain(net_domain* _domain)
{
	TRACE(("unregister_domain(%p, %d, %s)\n", _domain, _domain->family,
		_domain->name));

	net_domain_private* domain = (net_domain_private*)_domain;
	MutexLocker locker(sDomainLock);

	sDomains.Remove(domain);

	// TODO: Before deinitializing the radix head and deleting the domain,
	// all routes associated with this domain must be removed from the radix tree
	// and their net_route_private objects must be deleted. This will involve
	// using rn_walktree() to iterate through all routes, calling rn_delete()
	// for each, and then freeing the net_route_private object.
	// This cleanup will be fully implemented as part of later steps (route deletion
	// and adapting iteration logic).
	deinit_routing_domain_radix(domain);

	recursive_lock_destroy(&domain->lock);
	delete domain;
	return B_OK;
}


status_t
init_domains()
{
	mutex_init(&sDomainLock, "net domains");

	new (&sDomains) DomainList;
		// static C++ objects are not initialized in the module startup

#if ENABLE_DEBUGGER_COMMANDS
	add_debugger_command("net_domains", &dump_domains,
		"Dump network domains");
#endif
	return B_OK;
}


status_t
uninit_domains()
{
#if ENABLE_DEBUGGER_COMMANDS
	remove_debugger_command("net_domains", &dump_domains);
#endif

	mutex_destroy(&sDomainLock);
	return B_OK;
}

