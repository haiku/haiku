/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ancillary_data.h"
#include "device_interfaces.h"
#include "domains.h"
#include "interfaces.h"
#include "link.h"
#include "stack_private.h"
#include "utility.h"

#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_protocol.h>
#include <net_stack.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/khash.h>

#include <KernelExport.h>

#include <net/if_types.h>
#include <new>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_STACK
#ifdef TRACE_STACK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define MAX_CHAIN_MODULES 5

struct chain;
typedef DoublyLinkedList<chain> ChainList;

struct chain_key {
	int		family;
	int		type;
	int		protocol;
};

struct family {
	family(int type);

	void Acquire();
	void Release();

	static int Compare(void* _family, const void* _key);
	static uint32 Hash(void* _family, const void* _key, uint32 range);
	static struct family* Lookup(int type);
	static struct family* Add(int type);

	struct family*	next;
	int				type;
	int32			ref_count;
	ChainList		chains;
};

struct chain : DoublyLinkedListLinkImpl<chain> {
	chain(int family, int type, int protocol);
	~chain();

	status_t Acquire();
	void Release();
	void Uninitialize();

	static int Compare(void* _chain, const void* _key);
	static uint32 Hash(void* _chain, const void* _key, uint32 range);
	static struct chain* Lookup(hash_table* chains, int family, int type,
		int protocol);
	static struct chain* Add(hash_table* chains, int family, int type,
		int protocol, va_list modules);
	static struct chain* Add(hash_table* chains, int family, int type,
		int protocol, ...);
	static void DeleteChains(hash_table* chains);

	chain*			next;
	struct family*	parent;

	int 			family;
	int				type;
	int				protocol;

	int32			ref_count;
	uint32			flags;
	const char*		modules[MAX_CHAIN_MODULES + 1];
	module_info*	infos[MAX_CHAIN_MODULES + 1];
};

#define CHAIN_MISSING_MODULE	0x02
#define CHAIN_INITIALIZED		0x01

static mutex sChainLock;
static mutex sInitializeChainLock;
static hash_table* sProtocolChains;
static hash_table* sDatalinkProtocolChains;
static hash_table* sReceivingProtocolChains;
static hash_table* sFamilies;
static bool sInitialized;


family::family(int _type)
	:
	type(_type),
	ref_count(0)
{
}


void
family::Acquire()
{
	atomic_add(&ref_count, 1);
}


void
family::Release()
{
	if (atomic_add(&ref_count, -1) > 1)
		return;

	TRACE(("family %d unused, uninit chains\n", type));
	MutexLocker _(sChainLock);

	ChainList::Iterator iterator = chains.GetIterator();
	while (struct chain* chain = iterator.Next()) {
		chain->Uninitialize();
	}
}


/*static*/ int
family::Compare(void* _family, const void* _key)
{
	struct family* family = (struct family*)_family;
	int key = (addr_t)_key;

	if (family->type == key)
		return 0;

	return 1;
}


/*static*/ uint32
family::Hash(void* _family, const void* _key, uint32 range)
{
	struct family* family = (struct family*)_family;
	int key = (addr_t)_key;

	if (family != NULL)
		return family->type % range;

	return key % range;
}


/*static*/ struct family*
family::Lookup(int type)
{
	return (struct family*)hash_lookup(sFamilies, (void*)(addr_t)type);
}


/*static*/ struct family*
family::Add(int type)
{
	struct family* family = new (std::nothrow) ::family(type);
	if (family == NULL)
		return NULL;

	if (hash_insert(sFamilies, family) != B_OK) {
		delete family;
		return NULL;
	}

	return family;
}


//	#pragma mark -


chain::chain(int _family, int _type, int _protocol)
	:
	family(_family),
	type(_type),
	protocol(_protocol),
	ref_count(0),
	flags(0)
{
	parent = ::family::Lookup(family);
	if (parent == NULL)
		parent = ::family::Add(family);

	//parent->chains.Add(this);

	for (int32 i = 0; i < MAX_CHAIN_MODULES; i++) {
		modules[i] = NULL;
		infos[i] = NULL;
	}
}


chain::~chain()
{
	for (int32 i = 0; i < MAX_CHAIN_MODULES; i++) {
		free((char*)modules[i]);
	}

	//parent->chains.Remove(this);
}


status_t
chain::Acquire()
{
	if (atomic_add(&ref_count, 1) > 0) {
		if ((flags & CHAIN_MISSING_MODULE) != 0) {
			atomic_add(&ref_count, -1);
			return EAFNOSUPPORT;
		}

		while ((flags & CHAIN_INITIALIZED) == 0) {
			mutex_lock(&sInitializeChainLock);
			mutex_unlock(&sInitializeChainLock);
		}
		return B_OK;
	}

	parent->Acquire();

	if ((flags & CHAIN_INITIALIZED) != 0)
		return B_OK;

	TRACE(("initializing chain %d.%d.%d\n", family, type, protocol));
	MutexLocker locker(&sInitializeChainLock);

	for (int32 i = 0; modules[i] != NULL; i++) {
		if (get_module(modules[i], &infos[i]) < B_OK) {
			flags |= CHAIN_MISSING_MODULE;

			// put already opened modules
			while (i-- > 0) {
				put_module(modules[i]);
			}
			return EAFNOSUPPORT;
		}
	}

	flags |= CHAIN_INITIALIZED;
	return B_OK;
}


void
chain::Release()
{
	if (atomic_add(&ref_count, -1) > 1)
		return;

	TRACE(("chain %d.%d.%d unused\n", family, type, protocol));
	parent->Release();
}


void
chain::Uninitialize()
{
	if ((flags & CHAIN_INITIALIZED) == 0)
		return;

	TRACE(("uninit chain %d.%d.%d\n", family, type, protocol));
	MutexLocker _(sInitializeChainLock);

	for (int32 i = 0; modules[i] != NULL; i++) {
		put_module(modules[i]);
	}

	flags &= ~CHAIN_INITIALIZED;
}


/*static*/ int
chain::Compare(void* _chain, const void* _key)
{
	const chain_key* key = (const chain_key*)_key;
	struct chain* chain = (struct chain*)_chain;

	if (chain->family == key->family
		&& chain->type == key->type
		&& chain->protocol == key->protocol)
		return 0;

	return 1;
}


/*static*/ uint32
chain::Hash(void* _chain, const void* _key, uint32 range)
{
	const chain_key* key = (const chain_key*)_key;
	struct chain* chain = (struct chain*)_chain;

// TODO: check if this makes a good hash...
#define HASH(o) ((uint32)(((o)->family) ^ ((o)->type) ^ ((o)->protocol)) % range)
#if 0
	TRACE(("%d.%d.%d: Hash: %lu\n", chain ? chain->family : key->family,
		chain ? chain->type : key->type, chain ? chain->protocol : key->protocol,
		chain ? HASH(chain) : HASH(key)));
#endif

	if (chain != NULL)
		return HASH(chain);

	return HASH(key);
#undef HASH
}


/*static*/ struct chain*
chain::Lookup(hash_table* chains, int family, int type, int protocol)
{
	struct chain_key key = { family, type, protocol };
	return (struct chain*)hash_lookup(chains, &key);
}


/*static*/ struct chain*
chain::Add(hash_table* chains, int family, int type, int protocol,
	va_list modules)
{
	struct chain* chain = new (std::nothrow) ::chain(family, type, protocol);
	if (chain == NULL)
		return NULL;

	if (chain->parent == NULL || hash_insert(chains, chain) != B_OK) {
		delete chain;
		return NULL;
	}

	TRACE(("Add chain %d.%d.%d:\n", family, type, protocol));
	const char* module;
	int32 count = 0;

	while (true) {
		module = va_arg(modules, const char*);
		if (module == NULL)
			break;

		TRACE(("  [%ld] %s\n", count, module));
		chain->modules[count] = strdup(module);
		if (chain->modules[count] == NULL
			|| ++count >= MAX_CHAIN_MODULES) {
			hash_remove(chains, chain);
			delete chain;
			return NULL;
		}
	}

	if (chains == sProtocolChains && count == 0) {
		hash_remove(chains, chain);
		delete chain;
		return NULL;
	}

	return chain;
}


/*static*/ struct chain*
chain::Add(hash_table* chains, int family, int type, int protocol, ...)
{
	va_list modules;
	va_start(modules, protocol);

	struct chain* chain = Add(chains, family, type, 0, modules);

	va_end(modules);
	return chain;
}


/*static*/ void
chain::DeleteChains(hash_table* chains)
{
	uint32 cookie = 0;
	while (true) {
		struct chain* chain = (struct chain*)hash_remove_first(chains, &cookie);
		if (chain == NULL)
			break;

		chain->Uninitialize();
		delete chain;
	}
}


//	#pragma mark -


static void
uninit_domain_protocols(net_socket* socket)
{
	net_protocol* protocol = socket->first_protocol;
	while (protocol != NULL) {
		net_protocol* next = protocol->next;
		protocol->module->uninit_protocol(protocol);

		protocol = next;
	}

	socket->first_protocol = NULL;
	socket->first_info = NULL;
}


status_t
get_domain_protocols(net_socket* socket)
{
	struct chain* chain;

	{
		MutexLocker _(sChainLock);

		chain = chain::Lookup(sProtocolChains, socket->family, socket->type,
			socket->type == SOCK_RAW ? 0 : socket->protocol);
			// in SOCK_RAW mode, we ignore the protocol information
		if (chain == NULL) {
			// TODO: if we want to be POSIX compatible, we should also support
			//	the error codes EPROTONOSUPPORT and EPROTOTYPE.
			return EAFNOSUPPORT;
		}
	}

	// create net_protocol objects for the protocols in the chain

	status_t status = chain->Acquire();
	if (status != B_OK)
		return status;

	net_protocol* last = NULL;

	for (int32 i = 0; chain->infos[i] != NULL; i++) {
		net_protocol* protocol =
			((net_protocol_module_info*)chain->infos[i])->init_protocol(socket);
		if (protocol == NULL) {
			// free protocols we already initialized
			uninit_domain_protocols(socket);
			chain->Release();
			return B_NO_MEMORY;
		}

		protocol->module = (net_protocol_module_info*)chain->infos[i];
		protocol->socket = socket;
		protocol->next = NULL;

		if (last == NULL) {
			socket->first_protocol = protocol;
			socket->first_info = protocol->module;
		} else
			last->next = protocol;

		last = protocol;
	}

	return B_OK;
}


status_t
put_domain_protocols(net_socket* socket)
{
	struct chain* chain;

	{
		MutexLocker _(sChainLock);

		chain = chain::Lookup(sProtocolChains, socket->family, socket->type,
			socket->protocol);
		if (chain == NULL)
			return B_ERROR;
	}

	uninit_domain_protocols(socket);
	chain->Release();
	return B_OK;
}


static void
uninit_domain_datalink_protocols(domain_datalink* datalink)
{
	TRACE(("%s(datalink %p)\n", __FUNCTION__, datalink));

	if (datalink == NULL)
		return;

	net_datalink_protocol* protocol = datalink->first_protocol;
	while (protocol != NULL) {
		net_datalink_protocol* next = protocol->next;
		protocol->module->uninit_protocol(protocol);

		protocol = next;
	}

	datalink->first_protocol = NULL;
	datalink->first_info = NULL;
}


status_t
get_domain_datalink_protocols(Interface* interface, net_domain* domain)
{
	TRACE(("%s(interface %p, domain %d)\n", __FUNCTION__, interface,
		domain->family));

	struct chain* chain;

	{
		MutexLocker _(sChainLock);

		chain = chain::Lookup(sDatalinkProtocolChains, domain->family,
			interface->DeviceInterface()->device->type, 0);
		if (chain == NULL)
			return EAFNOSUPPORT;
	}

	domain_datalink* datalink = interface->DomainDatalink(domain->family);
	if (datalink == NULL)
		return B_BAD_VALUE;
	if (datalink->first_protocol != NULL)
		return B_NAME_IN_USE;

	// create net_protocol objects for the protocols in the chain

	status_t status = chain->Acquire();
	if (status != B_OK)
		return status;

	net_datalink_protocol* last = NULL;

	for (int32 i = 0; chain->infos[i] != NULL; i++) {
		net_datalink_protocol* protocol;
		status_t status = ((net_datalink_protocol_module_info*)
			chain->infos[i])->init_protocol(interface, domain, &protocol);
		if (status != B_OK) {
			// free protocols we already initialized
			uninit_domain_datalink_protocols(datalink);
			chain->Release();
			return status;
		}

		protocol->module = (net_datalink_protocol_module_info*)chain->infos[i];
		protocol->interface = interface;
		protocol->domain = domain;
		protocol->next = NULL;

		if (last == NULL) {
			datalink->first_protocol = protocol;
			datalink->first_info = protocol->module;
		} else
			last->next = protocol;

		last = protocol;
	}

	return B_OK;
}


status_t
put_domain_datalink_protocols(Interface* interface, net_domain* domain)
{
	TRACE(("%s(interface %p, domain %d)\n", __FUNCTION__, interface,
		domain->family));

	struct chain* chain;

	{
		MutexLocker _(sChainLock);

		chain = chain::Lookup(sDatalinkProtocolChains, domain->family,
			interface->DeviceInterface()->device->type, 0);
		if (chain == NULL)
			return B_ERROR;
	}

	uninit_domain_datalink_protocols(interface->DomainDatalink(domain->family));
	chain->Release();
	return B_OK;
}


status_t
get_domain_receiving_protocol(net_domain* _domain, uint32 type,
	net_protocol_module_info** _module)
{
	struct net_domain_private* domain = (net_domain_private*)_domain;
	struct chain* chain;

	TRACE(("get_domain_receiving_protocol(family %d, type %lu)\n",
		domain->family, type));

	{
		MutexLocker _(sChainLock);

		chain = chain::Lookup(sReceivingProtocolChains, domain->family,
			type, 0);
		if (chain == NULL)
			return EAFNOSUPPORT;
	}

	status_t status = chain->Acquire();
	if (status != B_OK)
		return status;

	*_module = (net_protocol_module_info*)chain->infos[0];
	return B_OK;
}


status_t
put_domain_receiving_protocol(net_domain* _domain, uint32 type)
{
	struct net_domain_private* domain = (net_domain_private*)_domain;
	struct chain* chain;

	{
		MutexLocker _(sChainLock);

		chain = chain::Lookup(sReceivingProtocolChains, domain->family,
			type, 0);
		if (chain == NULL)
			return B_ERROR;
	}

	chain->Release();
	return B_OK;
}


status_t
register_domain_protocols(int family, int type, int protocol, ...)
{
	if (type == SOCK_RAW) {
		// in SOCK_RAW mode, we ignore the protocol information
		protocol = 0;
	}

	MutexLocker locker(&sChainLock);

	struct chain* chain = chain::Lookup(sProtocolChains, family, type, protocol);
	if (chain != NULL)
		return B_OK;

	va_list modules;
	va_start(modules, protocol);

	chain = chain::Add(sProtocolChains, family, type, protocol, modules);

	va_end(modules);

	if (chain == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
register_domain_datalink_protocols(int family, int type, ...)
{
	TRACE(("register_domain_datalink_protocol(%d.%d)\n", family, type));
	MutexLocker locker(&sChainLock);

	struct chain* chain
		= chain::Lookup(sDatalinkProtocolChains, family, type, 0);
	if (chain != NULL)
		return B_OK;

	va_list modules;
	va_start(modules, type);

	chain = chain::Add(sDatalinkProtocolChains, family, type, 0, modules);

	va_end(modules);

	if (chain == NULL)
		return B_NO_MEMORY;

	// Add datalink interface protocol as the last protocol in the chain; it's
	// name stays unset, so that it won't be part of the release/acquire process.

	uint32 count = 0;
	while (chain->modules[count] != NULL) {
		count++;
	}

	chain->infos[count] = (module_info*)&gDatalinkInterfaceProtocolModule;
	return B_OK;
}


static status_t
register_domain_receiving_protocol(int family, int type, const char* moduleName)
{
	TRACE(("register_domain_receiving_protocol(%d.%d, %s)\n", family, type,
		moduleName));

	MutexLocker _(sChainLock);

	struct chain* chain
		= chain::Lookup(sReceivingProtocolChains, family, type, 0);
	if (chain != NULL)
		return B_OK;

	chain = chain::Add(sReceivingProtocolChains, family, type, 0, moduleName,
		NULL);
	if (chain == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
scan_modules(const char* path)
{
	void* cookie = open_module_list(path);
	if (cookie == NULL)
		return;

	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t length = sizeof(name);
		if (read_next_module_name(cookie, name, &length) != B_OK)
			break;

		TRACE(("scan %s\n", name));

		module_info* module;
		if (get_module(name, &module) == B_OK) {
			// we don't need the module right now, but we give it a chance
			// to register itself
			put_module(name);
		}
	}
}


status_t
init_stack()
{
	status_t status = init_domains();
	if (status != B_OK)
		return status;

	status = init_interfaces();
	if (status != B_OK)
		goto err1;

	status = init_device_interfaces();
	if (status != B_OK)
		goto err2;

	status = init_timers();
	if (status != B_OK)
		goto err3;

	status = init_notifications();
	if (status < B_OK) {
		// If this fails, it just means there won't be any notifications,
		// it's not a fatal error.
		dprintf("networking stack notifications could not be initialized: %s\n",
			strerror(status));
	}

	module_info* dummy;
	status = get_module(NET_SOCKET_MODULE_NAME, &dummy);
	if (status != B_OK)
		goto err4;

	mutex_init(&sChainLock, "net chains");
	mutex_init(&sInitializeChainLock, "net intialize chains");

	sFamilies = hash_init(10, offsetof(struct family, next),
		&family::Compare, &family::Hash);
	if (sFamilies == NULL) {
		status = B_NO_MEMORY;
		goto err5;
	}

	sProtocolChains = hash_init(10, offsetof(struct chain, next),
		&chain::Compare, &chain::Hash);
	if (sProtocolChains == NULL) {
		status = B_NO_MEMORY;
		goto err6;
	}

	sDatalinkProtocolChains = hash_init(10, offsetof(struct chain, next),
		&chain::Compare, &chain::Hash);
	if (sDatalinkProtocolChains == NULL) {
		status = B_NO_MEMORY;
		goto err7;
	}

	sReceivingProtocolChains = hash_init(10, offsetof(struct chain, next),
		&chain::Compare, &chain::Hash);
	if (sReceivingProtocolChains == NULL) {
		status = B_NO_MEMORY;
		goto err8;
	}

	sInitialized = true;

	link_init();
	scan_modules("network/protocols");
	scan_modules("network/datalink_protocols");

	// TODO: for now!
	register_domain_datalink_protocols(AF_INET, IFT_LOOP,
		"network/datalink_protocols/loopback_frame/v1", NULL);
	register_domain_datalink_protocols(AF_INET6, IFT_LOOP,
		"network/datalink_protocols/loopback_frame/v1", NULL);
	register_domain_datalink_protocols(AF_INET, IFT_ETHER,
		"network/datalink_protocols/arp/v1",
		"network/datalink_protocols/ethernet_frame/v1",
		NULL);
	register_domain_datalink_protocols(AF_INET6, IFT_ETHER,
		"network/datalink_protocols/ipv6_datagram/v1",
		"network/datalink_protocols/ethernet_frame/v1",
		NULL);

	return B_OK;

err8:
	hash_uninit(sDatalinkProtocolChains);
err7:
	hash_uninit(sProtocolChains);
err6:
	hash_uninit(sFamilies);
err5:
	mutex_destroy(&sInitializeChainLock);
	mutex_destroy(&sChainLock);
err4:
	uninit_timers();
err3:
	uninit_device_interfaces();
err2:
	uninit_interfaces();
err1:
	uninit_domains();
	return status;
}


status_t
uninit_stack()
{
	TRACE(("Unloading network stack\n"));

	put_module(NET_SOCKET_MODULE_NAME);
	uninit_timers();
	uninit_device_interfaces();
	uninit_interfaces();
	uninit_domains();
	uninit_notifications();

	mutex_destroy(&sChainLock);
	mutex_destroy(&sInitializeChainLock);

	// remove chains and families

	chain::DeleteChains(sProtocolChains);
	chain::DeleteChains(sDatalinkProtocolChains);
	chain::DeleteChains(sReceivingProtocolChains);

	uint32 cookie = 0;
	while (true) {
		struct family* family = (struct family*)hash_remove_first(sFamilies,
			&cookie);
		if (family == NULL)
			break;

		delete family;
	}

	hash_uninit(sProtocolChains);
	hash_uninit(sDatalinkProtocolChains);
	hash_uninit(sReceivingProtocolChains);
	hash_uninit(sFamilies);

	return B_OK;
}


static status_t
stack_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return sInitialized ? B_OK : B_BUSY;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_stack_module_info gNetStackModule = {
	{
		NET_STACK_MODULE_NAME,
		0,
		stack_std_ops
	},

	register_domain,
	unregister_domain,
	get_domain,

	register_domain_protocols,
	register_domain_datalink_protocols,
	register_domain_receiving_protocol,

	get_domain_receiving_protocol,
	put_domain_receiving_protocol,

	register_device_deframer,
	unregister_device_deframer,
	register_domain_device_handler,
	register_device_handler,
	unregister_device_handler,
	register_device_monitor,
	unregister_device_monitor,
	device_link_changed,
	device_removed,
	device_enqueue_buffer,

	notify_socket,

	checksum,

	init_fifo,
	uninit_fifo,
	fifo_enqueue_buffer,
	fifo_dequeue_buffer,
	clear_fifo,
	fifo_socket_enqueue_buffer,

	init_timer,
	set_timer,
	cancel_timer,
	wait_for_timer,
	is_timer_active,
	is_timer_running,

	is_syscall,
	is_restarted_syscall,
	store_syscall_restart_timeout,
	restore_syscall_restart_timeout,

	create_ancillary_data_container,
	delete_ancillary_data_container,
	add_ancillary_data,
	remove_ancillary_data,
	move_ancillary_data,
	next_ancillary_data
};

module_info* modules[] = {
	(module_info*)&gNetStackModule,
	(module_info*)&gNetBufferModule,
	(module_info*)&gNetSocketModule,
	(module_info*)&gNetDatalinkModule,
	(module_info*)&gLinkModule,
	(module_info*)&gNetStackInterfaceModule,
	NULL
};
