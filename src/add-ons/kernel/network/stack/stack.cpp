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

struct ChainHash;

struct chain : DoublyLinkedListLinkImpl<chain> {
	chain(int family, int type, int protocol);
	~chain();

	status_t Acquire();
	void Release();
	void Uninitialize();

	static struct chain* Lookup(BOpenHashTable<ChainHash>* chains,
		int family, int type, int protocol);
	static struct chain* Add(BOpenHashTable<ChainHash>* chains,
		int family, int type, int protocol, va_list modules);
	static struct chain* Add(BOpenHashTable<ChainHash>* chains,
		int family, int type, int protocol, ...);
	static void DeleteChains(BOpenHashTable<ChainHash>* chains);

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

struct ChainHash {
	typedef chain_key	KeyType;
	typedef	chain		ValueType;

// TODO: check if this makes a good hash...
#define HASH(o) ((uint32)(((o)->family) ^ ((o)->type) ^ ((o)->protocol)))

	size_t HashKey(KeyType key) const
	{
		return HASH(&key);
	}

	size_t Hash(ValueType* value) const
	{
		return HASH(value);
	}

#undef HASH

	bool Compare(KeyType key, ValueType* chain) const
	{
		if (chain->family == key.family
			&& chain->type == key.type
			&& chain->protocol == key.protocol)
			return true;

		return false;
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->next;
	}
};

struct FamilyHash {
	typedef int			KeyType;
	typedef	family		ValueType;

	size_t HashKey(KeyType key) const
	{
		return key;
	}

	size_t Hash(ValueType* value) const
	{
		return value->type;
	}

	bool Compare(KeyType key, ValueType* family) const
	{
		return family->type == key;
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->next;
	}
};

typedef BOpenHashTable<ChainHash> ChainTable;
typedef BOpenHashTable<FamilyHash> FamilyTable;

#define CHAIN_MISSING_MODULE	0x02
#define CHAIN_INITIALIZED		0x01

static mutex sChainLock;
static mutex sInitializeChainLock;
static ChainTable* sProtocolChains;
static ChainTable* sDatalinkProtocolChains;
static ChainTable* sReceivingProtocolChains;
static FamilyTable* sFamilies;
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


/*static*/ struct family*
family::Lookup(int type)
{
	return sFamilies->Lookup(type);
}


/*static*/ struct family*
family::Add(int type)
{
	struct family* family = new (std::nothrow) ::family(type);
	if (family == NULL)
		return NULL;

	if (sFamilies->Insert(family) != B_OK) {
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


/*static*/ struct chain*
chain::Lookup(ChainTable* chains, int family, int type, int protocol)
{
	struct chain_key key = { family, type, protocol };
	return chains->Lookup(key);
}


/*static*/ struct chain*
chain::Add(ChainTable* chains, int family, int type, int protocol,
	va_list modules)
{
	struct chain* chain = new (std::nothrow) ::chain(family, type, protocol);
	if (chain == NULL)
		return NULL;

	if (chain->parent == NULL || chains->Insert(chain) != B_OK) {
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
			chains->Remove(chain);
			delete chain;
			return NULL;
		}
	}

	if (chains == sProtocolChains && count == 0) {
		chains->Remove(chain);
		delete chain;
		return NULL;
	}

	return chain;
}


/*static*/ struct chain*
chain::Add(ChainTable* chains, int family, int type, int protocol, ...)
{
	va_list modules;
	va_start(modules, protocol);

	struct chain* chain = Add(chains, family, type, 0, modules);

	va_end(modules);
	return chain;
}


/*static*/ void
chain::DeleteChains(ChainTable* chains)
{
	struct chain* current;
	current = chains->Clear(true);
	while (current) {
		struct chain* next = current->next;

		current->Uninitialize();
		delete current;
		current = next;
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

	close_module_list(cookie);
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

	sFamilies = new(std::nothrow) FamilyTable();
	if (sFamilies == NULL || sFamilies->Init(10) != B_OK) {
		status = B_NO_MEMORY;
		goto err5;
	}

	sProtocolChains = new(std::nothrow) ChainTable();
	if (sProtocolChains == NULL || sProtocolChains->Init(10) != B_OK) {
		status = B_NO_MEMORY;
		goto err6;
	}

	sDatalinkProtocolChains = new(std::nothrow) ChainTable();
	if (sDatalinkProtocolChains == NULL
			|| sDatalinkProtocolChains->Init(10) != B_OK) {
		status = B_NO_MEMORY;
		goto err7;
	}

	sReceivingProtocolChains = new(std::nothrow) ChainTable();
	if (sReceivingProtocolChains == NULL
			|| sReceivingProtocolChains->Init(10) != B_OK) {
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
#if 0 // PPP is not (currently) included in the build
	register_domain_datalink_protocols(AF_INET, IFT_PPP,
		"network/datalink_protocols/ppp_frame/v1", NULL);
#endif
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
	delete sDatalinkProtocolChains;
err7:
	delete sProtocolChains;
err6:
	delete sFamilies;
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
	uninit_notifications();

	// remove chains and families

	chain::DeleteChains(sProtocolChains);
	chain::DeleteChains(sDatalinkProtocolChains);
	chain::DeleteChains(sReceivingProtocolChains);

	mutex_destroy(&sChainLock);
	mutex_destroy(&sInitializeChainLock);

	uninit_timers();
	uninit_device_interfaces();
	uninit_interfaces();
	uninit_domains();

	struct family* current;
	current = sFamilies->Clear(true);
	while (current) {
		struct family* next = current->next;

		delete current;
		current = next;
	}

	delete sProtocolChains;
	delete sDatalinkProtocolChains;
	delete sReceivingProtocolChains;
	delete sFamilies;

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
