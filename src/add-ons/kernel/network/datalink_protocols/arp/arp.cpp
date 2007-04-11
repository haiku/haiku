/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

//! Ethernet Address Resolution Protocol, see RFC 826.


#include <arp_control.h>
#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_datalink.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <generic_syscall.h>
#include <util/AutoLock.h>
#include <util/khash.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include <sys/sockio.h>


//#define TRACE_ARP
#ifdef TRACE_ARP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct arp_header {
	uint16		hardware_type;
	uint16		protocol_type;
	uint8		hardware_length;
	uint8		protocol_length;
	uint16		opcode;

	// TODO: this should be a variable length header, but for our current
	//	usage (Ethernet/IPv4), this should work fine.
	uint8		hardware_sender[6];
	in_addr_t	protocol_sender;
	uint8		hardware_target[6];
	in_addr_t	protocol_target;
} _PACKED;

#define ARP_OPCODE_REQUEST	1
#define ARP_OPCODE_REPLY	2

#define ARP_HARDWARE_TYPE_ETHER	1

struct arp_entry {
	arp_entry	*next;
	in_addr_t	protocol_address;
	sockaddr_dl	hardware_address;
	uint32		flags;
	sem_id		resolved_sem;
	net_buffer	*request_buffer;
	net_timer	timer;
	uint32		timer_state;
	bigtime_t	timestamp;
	net_datalink_protocol *protocol;

	static int Compare(void *_entry, const void *_key);
	static uint32 Hash(void *_entry, const void *_key, uint32 range);
	static arp_entry *Lookup(in_addr_t protocolAddress);
	static arp_entry *Add(in_addr_t protocolAddress, sockaddr_dl *hardwareAddress,
		uint32 flags);
};

// see arp_control.h for flags

#define ARP_NO_STATE				0
#define ARP_STATE_REQUEST			1
#define ARP_STATE_LAST_REQUEST		5
#define ARP_STATE_REQUEST_FAILED	6
#define ARP_STATE_REMOVE_FAILED		7
#define ARP_STATE_STALE				8

#define ARP_STALE_TIMEOUT	30 * 60000000LL		// 30 minutes
#define ARP_REJECT_TIMEOUT	20000000LL			// 20 seconds
#define ARP_REQUEST_TIMEOUT	1000000LL			// 1 second

struct arp_protocol : net_datalink_protocol {
};


static void arp_timer(struct net_timer *timer, void *data);

struct net_buffer_module_info *gBufferModule;
static net_stack_module_info *sStackModule;
static hash_table *sCache;
static benaphore sCacheLock;
static bool sIgnoreReplies;


/*static*/ int
arp_entry::Compare(void *_entry, const void *_key)
{
	arp_entry *entry = (arp_entry *)_entry;
	in_addr_t key = (in_addr_t)_key;

	if (entry->protocol_address == key)
		return 0;

	return 1;
}


/*static*/ uint32
arp_entry::Hash(void *_entry, const void *_key, uint32 range)
{
	arp_entry *entry = (arp_entry *)_entry;
	in_addr_t key = (in_addr_t)_key;

// TODO: check if this makes a good hash...
#define HASH(o) (((o >> 24) ^ (o >> 16) ^ (o >> 8) ^ o) % range)

#ifdef TRACE_ARP
	in_addr_t a = entry ? entry->protocol_address : key; 
	dprintf("%ld.%ld.%ld.%ld: Hash: %lu\n", a >> 24, (a >> 16) & 0xff,
		(a >> 8) & 0xff, a & 0xff, HASH(a));
#endif

	if (entry != NULL)
		return HASH(entry->protocol_address);

	return HASH(key);
#undef HASH
}


/*static*/ arp_entry *
arp_entry::Lookup(in_addr_t address)
{
	return (arp_entry *)hash_lookup(sCache, (void *)address);
}


/*static*/ arp_entry *
arp_entry::Add(in_addr_t protocolAddress, sockaddr_dl *hardwareAddress,
	uint32 flags)
{
	arp_entry *entry = new (std::nothrow) arp_entry;
	if (entry == NULL)
		return NULL;

	entry->protocol_address = protocolAddress;
	entry->flags = flags;
	entry->timestamp = system_time();
	entry->protocol = NULL;
	entry->request_buffer = NULL;
	entry->timer_state = ARP_NO_STATE;
	sStackModule->init_timer(&entry->timer, arp_timer, entry);

	if (hardwareAddress != NULL) {
		// this entry is already resolved
		entry->hardware_address = *hardwareAddress;
		entry->hardware_address.sdl_e_type = ETHER_TYPE_IP;
		entry->resolved_sem = -1;
	} else {
		// this entry still needs to be resolved
		entry->hardware_address.sdl_alen = 0;

		char name[32];
		snprintf(name, sizeof(name), "arp %08lx", protocolAddress);
		entry->resolved_sem = create_sem(0, name);
		if (entry->resolved_sem < B_OK) {
			delete entry;
			return NULL;
		}
	}
	if (entry->hardware_address.sdl_len != sizeof(sockaddr_dl)) {
		// explicitly set correct length in case our caller hasn't...
		entry->hardware_address.sdl_len = sizeof(sockaddr_dl);
	}

	if (hash_insert(sCache, entry) != B_OK) {
		delete entry;
		return NULL;
	}

	return entry;
}


//	#pragma mark -


/*!
	Updates the entry determined by \a protocolAddress with the specified
	\a hardwareAddress.
	If such an entry does not exist yet, a new entry is added. If you try
	to update a local existing entry but didn't ask for it (by setting
	\a flags to ARP_FLAG_LOCAL), an error is returned.

	This function does not lock the cache - you have to do it yourself
	before calling it.
*/
status_t
arp_update_entry(in_addr_t protocolAddress, sockaddr_dl *hardwareAddress,
	uint32 flags, arp_entry **_entry = NULL)
{
	arp_entry *entry = arp_entry::Lookup(protocolAddress);
	if (entry != NULL) {
		// We disallow updating of entries that had been resolved before, 
		// but to a different address.
		// Right now, you have to manually purge the ARP entries (or wait some
		// time) to let us switch to the new address.
		if (entry->hardware_address.sdl_alen != 0 
			&& memcmp(LLADDR(&entry->hardware_address),
				LLADDR(hardwareAddress), ETHER_ADDRESS_LENGTH)) {
			dprintf("ARP host %08lx updated with different hardware address %02x:%02x:%02x:%02x:%02x:%02x.\n",
				protocolAddress,
				hardwareAddress->sdl_data[0], hardwareAddress->sdl_data[1],
				hardwareAddress->sdl_data[2], hardwareAddress->sdl_data[3],
				hardwareAddress->sdl_data[4], hardwareAddress->sdl_data[5]);
			return B_ERROR;
		}

		entry->hardware_address = *hardwareAddress;
		entry->timestamp = system_time();
	} else {
		entry = arp_entry::Add(protocolAddress, hardwareAddress, flags);
		if (entry == NULL)
			return B_NO_MEMORY;
	}

	// if someone was waiting for this ARP request to be resolved
	if (entry->resolved_sem >= B_OK) {
		delete_sem(entry->resolved_sem);
		entry->resolved_sem = -1;
	}

	if (entry->request_buffer != NULL) {
		gBufferModule->free(entry->request_buffer);
		entry->request_buffer = NULL;
	}

	if ((entry->flags & ARP_FLAG_PERMANENT) == 0) {
		// (re)start the stale timer
		entry->timer_state = ARP_STATE_STALE;
		sStackModule->set_timer(&entry->timer, ARP_STALE_TIMEOUT);
	}

	if (_entry)
		*_entry = entry;

	return B_OK;
}


static status_t
arp_update_local(net_datalink_protocol *protocol)
{
	net_interface *interface = protocol->interface;
	in_addr_t inetAddress;

	if (interface->address == NULL) {
		// interface has not yet been set
		inetAddress = INADDR_ANY;
	} else
		inetAddress = ((sockaddr_in *)interface->address)->sin_addr.s_addr;

	sockaddr_dl address;
	address.sdl_len = sizeof(sockaddr_dl);
	address.sdl_family = AF_DLI;
	address.sdl_type = IFT_ETHER;
	address.sdl_e_type = ETHER_TYPE_IP;
	address.sdl_nlen = 0;
	address.sdl_slen = 0;
	address.sdl_alen = interface->device->address.length;
	memcpy(LLADDR(&address), interface->device->address.data, address.sdl_alen);

	arp_entry *entry;
	status_t status = arp_update_entry(inetAddress, &address,
		ARP_FLAG_LOCAL | ARP_FLAG_PERMANENT, &entry);
	if (status == B_OK)
		entry->protocol = protocol;

	return status;
}


static status_t
handle_arp_request(net_buffer *buffer, arp_header &header)
{
	BenaphoreLocker locker(sCacheLock);

	if (!sIgnoreReplies) {
		arp_update_entry(header.protocol_sender, (sockaddr_dl *)&buffer->source, 0);
			// remember the address of the sender as we might need it later
	}

	// check if this request is for us

	arp_entry *entry = arp_entry::Lookup(header.protocol_target);
	if (entry == NULL || (entry->flags & (ARP_FLAG_LOCAL | ARP_FLAG_PUBLISH)) == 0) {
		// We're not the one to answer this request
		// TODO: instead of letting the other's request time-out, can we reply
		//	failure somehow?
		TRACE(("  not for us\n"));
		return B_ERROR;
	}

	// send a reply (by reusing the buffer we got)

	TRACE(("  send reply!\n"));
	header.opcode = htons(ARP_OPCODE_REPLY);

	memcpy(header.hardware_target, header.hardware_sender, ETHER_ADDRESS_LENGTH);
	header.protocol_target = header.protocol_sender;
	memcpy(header.hardware_sender, LLADDR(&entry->hardware_address), ETHER_ADDRESS_LENGTH);
	header.protocol_sender = entry->protocol_address;

	// exchange source and destination address
	memcpy(LLADDR((sockaddr_dl *)&buffer->source), header.hardware_sender,
		ETHER_ADDRESS_LENGTH);
	memcpy(LLADDR((sockaddr_dl *)&buffer->destination), header.hardware_target,
		ETHER_ADDRESS_LENGTH);

	buffer->flags = 0;
		// make sure this won't be a broadcast message

	return entry->protocol->next->module->send_data(entry->protocol->next, buffer);
}


static void
handle_arp_reply(net_buffer *buffer, arp_header &header)
{
	if (sIgnoreReplies)
		return;

	BenaphoreLocker locker(sCacheLock);
	arp_update_entry(header.protocol_sender, (sockaddr_dl *)&buffer->source, 0);
}


static status_t
arp_receive(void *cookie, net_buffer *buffer)
{
	TRACE(("ARP receive\n"));

	NetBufferHeaderReader<arp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	arp_header &header = bufferHeader.Data();
	uint16 opcode = ntohs(header.opcode);

#ifdef TRACE_ARP
	dprintf("  hw sender: %02x:%02x:%02x:%02x:%02x:%02x\n",
		header.hardware_sender[0], header.hardware_sender[1], header.hardware_sender[2],
		header.hardware_sender[3], header.hardware_sender[4], header.hardware_sender[5]);
	dprintf("  proto sender: %ld.%ld.%ld.%ld\n", header.protocol_sender >> 24, (header.protocol_sender >> 16) & 0xff,
		(header.protocol_sender >> 8) & 0xff, header.protocol_sender & 0xff);
	dprintf("  hw target: %02x:%02x:%02x:%02x:%02x:%02x\n",
		header.hardware_target[0], header.hardware_target[1], header.hardware_target[2],
		header.hardware_target[3], header.hardware_target[4], header.hardware_target[5]);
	dprintf("  proto target: %ld.%ld.%ld.%ld\n", header.protocol_target >> 24, (header.protocol_target >> 16) & 0xff,
		(header.protocol_target >> 8) & 0xff, header.protocol_target & 0xff);
#endif

	if (ntohs(header.protocol_type) != ETHER_TYPE_IP
		|| ntohs(header.hardware_type) != ARP_HARDWARE_TYPE_ETHER)
		return B_BAD_TYPE;

	// check if the packet is okay

	if (header.hardware_length != ETHER_ADDRESS_LENGTH
		|| header.protocol_length != sizeof(in_addr_t))
		return B_BAD_DATA;

	// handle packet

	switch (opcode) {
		case ARP_OPCODE_REQUEST:
			TRACE(("  got ARP request\n"));
			if (handle_arp_request(buffer, header) == B_OK) {
				// the function will take care of the buffer if everything went well
				return B_OK;
			}
			break;
		case ARP_OPCODE_REPLY:
			TRACE(("  got ARP reply\n"));
			handle_arp_reply(buffer, header);
			break;

		default:
			dprintf("unknown ARP opcode %d\n", opcode);
			return B_ERROR;
	}

	gBufferModule->free(buffer);
	return B_OK;
}


static void
arp_timer(struct net_timer *timer, void *data)
{
	arp_entry *entry = (arp_entry *)data;
	TRACE(("ARP timer %ld, entry %p!\n", entry->timer_state, entry));

	switch (entry->timer_state) {
		case ARP_NO_STATE:
			// who are you kidding?
			break;

		case ARP_STATE_REQUEST_FAILED:
			// requesting the ARP entry failed, we keep it around for a while, though,
			// so that we won't try to request the same address again too soon.
			TRACE(("  requesting ARP entry %p failed!\n", entry));
			entry->timer_state = ARP_STATE_REMOVE_FAILED;
			entry->flags |= ARP_FLAG_REJECT;
			sStackModule->set_timer(&entry->timer, ARP_REJECT_TIMEOUT);
			break;

		case ARP_STATE_REMOVE_FAILED:
		case ARP_STATE_STALE:
			// the entry has aged so much that we're going to remove it
			TRACE(("  remove ARP entry %p!\n", entry));

			benaphore_lock(&sCacheLock);
			hash_remove(sCache, entry);
			benaphore_unlock(&sCacheLock);

			delete entry;

			break;
		default:
			if (entry->timer_state > ARP_STATE_LAST_REQUEST)
				break;

			TRACE(("  send request for ARP entry %p!\n", entry));
			net_buffer *request = entry->request_buffer;
			if (entry->timer_state < ARP_STATE_LAST_REQUEST) {
				// we'll still need our buffer, so in order to prevent it being
				// freed by a successful send, we need to clone it
				request = gBufferModule->clone(request, true);
				if (request == NULL) {
					// cloning failed - that means we won't be able to send as
					// many requests as originally planned
					request = entry->request_buffer;
					entry->timer_state = ARP_STATE_LAST_REQUEST;
				}
			}

			// we're trying to resolve the address, so keep sending requests
			status_t status = entry->protocol->next->module->send_data(
				entry->protocol->next, request);
			if (status < B_OK)
				gBufferModule->free(request);

			if (entry->timer_state == ARP_STATE_LAST_REQUEST) {
				// buffer has been freed on send
				entry->request_buffer = NULL;
			}

			entry->timer_state++;
			sStackModule->set_timer(&entry->timer, ARP_REQUEST_TIMEOUT);
	}
}


/*!
	Checks if the ARP \a entry has already been resolved. If it wasn't yet,
	and MSG_DONTWAIT is not set in \a flags, it will wait for the entry to
	become resolved.
	You need to have the sCacheLock held when calling this function - but
	note that the lock may be interrupted (in which case entry is updated).
*/
static status_t
arp_check_resolved(arp_entry **_entry, uint32 flags)
{
	arp_entry *entry = *_entry;

	if ((entry->flags & ARP_FLAG_REJECT) != 0)
		return EHOSTUNREACH;

	if (entry->resolved_sem < B_OK)
		return B_OK;

	// we need to wait for this entry to become resolved

	if ((flags & MSG_DONTWAIT) != 0)
		return B_ERROR;

	// store information we cannot access anymore after having unlocked the cache
	sem_id waitSem = entry->resolved_sem;
	in_addr_t address = entry->protocol_address;

	benaphore_unlock(&sCacheLock);
	
	status_t status = acquire_sem_etc(waitSem, 1, B_RELATIVE_TIMEOUT, 5 * 1000000);

	benaphore_lock(&sCacheLock);

	if (status == B_TIMED_OUT)
		return EHOSTUNREACH;

	// retrieve the entry again, as we reacquired the cache lock
	entry = arp_entry::Lookup(address);
	if (entry == NULL)
		return B_ERROR;

	*_entry = entry;	
	return B_OK;
}


/*!
	Address resolver function: prepares and sends the ARP request necessary
	to retrieve the hardware address for \a address.
	You need to have the sCacheLock held when calling this function - but
	note that the lock will be interrupted here if everything goes well.
*/
static status_t
arp_resolve(net_datalink_protocol *protocol, in_addr_t address, arp_entry **_entry)
{
	// create an unresolved ARP entry as a placeholder
	arp_entry *entry = arp_entry::Add(address, NULL, 0);
	if (entry == NULL)
		return B_NO_MEMORY;

	// prepare ARP request

	entry->request_buffer = gBufferModule->create(256);
	if (entry->request_buffer == NULL) {
		// TODO: do something with the entry
		return B_NO_MEMORY;
	}

	NetBufferPrepend<arp_header> bufferHeader(entry->request_buffer);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		// TODO: do something with the entry
		return status;
	}

	// prepare ARP header

	net_device *device = protocol->interface->device;
	arp_header &header = bufferHeader.Data();

	header.hardware_type = htons(ARP_HARDWARE_TYPE_ETHER);
	header.protocol_type = htons(ETHER_TYPE_IP);
	header.hardware_length = ETHER_ADDRESS_LENGTH;
	header.protocol_length = sizeof(in_addr_t);
	header.opcode = htons(ARP_OPCODE_REQUEST);

	memcpy(header.hardware_sender, device->address.data, ETHER_ADDRESS_LENGTH);
	if (protocol->interface->address != NULL)
		header.protocol_sender = ((sockaddr_in *)protocol->interface->address)->sin_addr.s_addr;
	else
		header.protocol_sender = 0;
			// TODO: test if this actually works - maybe we should use INADDR_BROADCAST instead
	memset(header.hardware_target, 0, ETHER_ADDRESS_LENGTH);
	header.protocol_target = address;

	// prepare source and target addresses

	struct sockaddr_dl &source = *(struct sockaddr_dl *)&entry->request_buffer->source;
	source.sdl_len = sizeof(sockaddr_dl);
	source.sdl_family = AF_DLI;
	source.sdl_index = device->index;
	source.sdl_type = IFT_ETHER;
	source.sdl_e_type = ETHER_TYPE_ARP;
	source.sdl_nlen = source.sdl_slen = 0;
	source.sdl_alen = ETHER_ADDRESS_LENGTH;
	memcpy(source.sdl_data, device->address.data, ETHER_ADDRESS_LENGTH);

	entry->request_buffer->flags = MSG_BCAST;
		// this is a broadcast packet, we don't need to fill in the destination

	entry->protocol = protocol;
	entry->timer_state = ARP_STATE_REQUEST;
	sStackModule->set_timer(&entry->timer, 0);
		// start request timer

	sem_id waitSem = entry->resolved_sem;
	benaphore_unlock(&sCacheLock);

	status = acquire_sem_etc(waitSem, 1, B_RELATIVE_TIMEOUT, 5 * 1000000);
		// wait for the entry to become resolved

	benaphore_lock(&sCacheLock);

	// retrieve the entry again, as we reacquired the cache lock
	entry = arp_entry::Lookup(address);
	if (entry == NULL)
		return B_ERROR;

	if (status == B_TIMED_OUT) {
		// we didn't get a response, mark ARP entry as non-existant
		entry->flags = ARP_FLAG_REJECT;
		// TODO: remove the ARP entry after some time
		return EHOSTUNREACH;
	}

	*_entry = entry;
	return B_OK;
}


static status_t
arp_control(const char *subsystem, uint32 function,
	void *buffer, size_t bufferSize)
{
	struct arp_control control;
	if (bufferSize != sizeof(struct arp_control))
		return B_BAD_VALUE;
	if (user_memcpy(&control, buffer, sizeof(struct arp_control)) < B_OK)
		return B_BAD_ADDRESS;

	BenaphoreLocker locker(sCacheLock);

	switch (function) {
		case ARP_SET_ENTRY:
			sockaddr_dl hardwareAddress;

			hardwareAddress.sdl_len = sizeof(sockaddr_dl);
			hardwareAddress.sdl_family = AF_DLI;
			hardwareAddress.sdl_index = 0;
			hardwareAddress.sdl_type = IFT_ETHER;
			hardwareAddress.sdl_e_type = ETHER_TYPE_IP;
			hardwareAddress.sdl_nlen = hardwareAddress.sdl_slen = 0;
			hardwareAddress.sdl_alen = ETHER_ADDRESS_LENGTH;
			memcpy(hardwareAddress.sdl_data, control.ethernet_address, ETHER_ADDRESS_LENGTH);

			return arp_update_entry(control.address, &hardwareAddress,
				control.flags & (ARP_FLAG_PUBLISH | ARP_FLAG_PERMANENT | ARP_FLAG_REJECT));

		case ARP_GET_ENTRY:
		{
			arp_entry *entry = arp_entry::Lookup(control.address);
			if (entry == NULL || entry->resolved_sem < B_OK)
				return B_ENTRY_NOT_FOUND;

			memcpy(control.ethernet_address, entry->hardware_address.sdl_data,
				ETHER_ADDRESS_LENGTH);
			control.flags = entry->flags;
			return user_memcpy(buffer, &control, sizeof(struct arp_control));
		}

		case ARP_GET_ENTRIES:
		{
			hash_iterator iterator;
			hash_open(sCache, &iterator);

			arp_entry *entry;
			uint32 i = 0;
			while ((entry = (arp_entry *)hash_next(sCache, &iterator)) != NULL
				&& i < control.cookie) {
				i++;
			}
			hash_close(sCache, &iterator, false);

			if (entry == NULL)
				return B_ENTRY_NOT_FOUND;

			control.cookie++;
			control.address = entry->protocol_address;
			memcpy(control.ethernet_address, entry->hardware_address.sdl_data,
				ETHER_ADDRESS_LENGTH);
			control.flags = entry->flags;

			return user_memcpy(buffer, &control, sizeof(struct arp_control));
		}

		case ARP_DELETE_ENTRY:
		{
			arp_entry *entry = arp_entry::Lookup(control.address);
			if (entry == NULL || entry->resolved_sem < B_OK)
				return B_ENTRY_NOT_FOUND;
			if ((entry->flags & ARP_FLAG_LOCAL) != 0)
				return B_BAD_VALUE;

			// schedule a timer to remove this entry
			entry->timer_state = ARP_STATE_REMOVE_FAILED;
			sStackModule->set_timer(&entry->timer, 0);
			return B_OK;
		}

		case ARP_FLUSH_ENTRIES:
		{
			hash_iterator iterator;
			hash_open(sCache, &iterator);

			arp_entry *entry;
			while ((entry = (arp_entry *)hash_next(sCache, &iterator)) != NULL) {
				// we never flush local ARP entries
				if ((entry->flags & ARP_FLAG_LOCAL) != 0)
					continue;

				// schedule a timer to remove this entry
				entry->timer_state = ARP_STATE_REMOVE_FAILED;
				sStackModule->set_timer(&entry->timer, 0);
			}
			hash_close(sCache, &iterator, false);
			return B_OK;
		}

		case ARP_IGNORE_REPLIES:
			sIgnoreReplies = control.flags != 0;
			return B_OK;
	}

	return B_BAD_VALUE;
}


static status_t
arp_init()
{
	status_t status = get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);
	if (status < B_OK)
		return status;
	status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK)
		goto err1;

	status = benaphore_init(&sCacheLock, "arp cache");
	if (status < B_OK)
		goto err2;

	sCache = hash_init(64, offsetof(struct arp_entry, next),
		&arp_entry::Compare, &arp_entry::Hash);
	if (sCache == NULL) {
		status = B_NO_MEMORY;
		goto err3;
	}

	register_generic_syscall(ARP_SYSCALLS, arp_control, 1, 0);
	return B_OK;

err3:
	benaphore_destroy(&sCacheLock);
err2:
	put_module(NET_BUFFER_MODULE_NAME);
err1:
	put_module(NET_STACK_MODULE_NAME);
	return status;
}


static status_t
arp_uninit()
{
	unregister_generic_syscall(ARP_SYSCALLS, 1);

	put_module(NET_BUFFER_MODULE_NAME);
	put_module(NET_STACK_MODULE_NAME);
	return B_OK;
}


//	#pragma mark -


status_t
arp_init_protocol(struct net_interface *interface, net_datalink_protocol **_protocol)
{
	// We currently only support a single family and type!
	if (interface->domain->family != AF_INET
		|| interface->device->type != IFT_ETHER)
		return B_BAD_TYPE;

	status_t status = sStackModule->register_device_handler(interface->device,
		ETHER_FRAME_TYPE | ETHER_TYPE_ARP, &arp_receive, NULL);

	if (status < B_OK)
		return status;

	arp_protocol *protocol = new (std::nothrow) arp_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	*_protocol = protocol;
	return B_OK;
}


status_t
arp_uninit_protocol(net_datalink_protocol *protocol)
{
	sStackModule->unregister_device_handler(protocol->interface->device,
		ETHER_FRAME_TYPE | ETHER_TYPE_ARP);
	sStackModule->unregister_device_handler(protocol->interface->device,
		ETHER_FRAME_TYPE | ETHER_TYPE_IP);

	delete protocol;
	return B_OK;
}


status_t
arp_send_data(net_datalink_protocol *protocol,
	net_buffer *buffer)
{
	{
		BenaphoreLocker locker(sCacheLock);

		// Lookup source (us)
		// TODO: this could be cached - the lookup isn't really needed at all

		arp_entry *entry = arp_entry::Lookup(
			((struct sockaddr_in *)&buffer->source)->sin_addr.s_addr);
		if (entry == NULL)
			return B_ERROR;

		memcpy(&buffer->source, &entry->hardware_address,
			entry->hardware_address.sdl_len);

		if ((buffer->flags & MSG_BCAST) == 0) {
			// Lookup destination (we may need to wait for this)
			entry = arp_entry::Lookup(
				((struct sockaddr_in *)&buffer->destination)->sin_addr.s_addr);
			if (entry == NULL) {
				// The ARP entry does not yet exist, if we're allowed to wait,
				// we'll send an ARP request and try to change that.
				if ((buffer->flags & MSG_DONTWAIT) != 0) {
					// TODO: implement delaying packet send after ARP response!
					return B_ERROR;
				}

				status_t status = arp_resolve(protocol,
					((struct sockaddr_in *)&buffer->destination)->sin_addr.s_addr, &entry);
				if (status < B_OK)
					return status;
			} else {
				// The entry exists, but we have to check if it has already been
				// resolved and is valid.
				status_t status = arp_check_resolved(&entry, buffer->flags);
				if (status < B_OK)
					return status;
			}

			memcpy(&buffer->destination, &entry->hardware_address,
				entry->hardware_address.sdl_len);
		}
	}

	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
arp_up(net_datalink_protocol *_protocol)
{
	arp_protocol *protocol = (arp_protocol *)_protocol;
	status_t status = protocol->next->module->interface_up(protocol->next);
	if (status < B_OK)
		return status;

	// cache this device's address for later use

	status = arp_update_local(protocol);
	if (status < B_OK) {
		protocol->next->module->interface_down(protocol->next);
		return status;
	}

	return B_OK;
}


void
arp_down(net_datalink_protocol *protocol)
{
	// remove local ARP entry from the cache

	if (protocol->interface->address != NULL) {
		BenaphoreLocker locker(sCacheLock);

		arp_entry *entry = arp_entry::Lookup(
			((sockaddr_in *)protocol->interface->address)->sin_addr.s_addr);
		if (entry != NULL) {
			hash_remove(sCache, entry);
			delete entry;
		}
	}

	protocol->next->module->interface_down(protocol->next);	
}


status_t
arp_control(net_datalink_protocol *protocol,
	int32 op, void *argument, size_t length)
{
	if (op == SIOCSIFADDR && (protocol->interface->flags & IFF_UP) != 0) {
		// The interface may get a new address, so we need to update our
		// local entries.
		bool hasOldAddress = false;
		in_addr_t oldAddress = 0;
		if (protocol->interface->address != NULL) {
			oldAddress = ((sockaddr_in *)protocol->interface->address)->sin_addr.s_addr;
			hasOldAddress = true;
		}

		status_t status = protocol->next->module->control(protocol->next,
			SIOCSIFADDR, argument, length);
		if (status < B_OK)
			return status;

		arp_update_local(protocol);

		if (oldAddress == ((sockaddr_in *)protocol->interface->address)->sin_addr.s_addr
			|| !hasOldAddress)
			return B_OK;

		// remove previous address from cache
		// TODO: we should be able to do this (add/remove) in one atomic operation!

		BenaphoreLocker locker(sCacheLock);

		arp_entry *entry = arp_entry::Lookup(oldAddress);
		if (entry != NULL) {
			hash_remove(sCache, entry);
			delete entry;
		}

		return B_OK;
	}

	return protocol->next->module->control(protocol->next,
		op, argument, length);
}


static status_t
arp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return arp_init();
		case B_MODULE_UNINIT:
			return arp_uninit();

		default:
			return B_ERROR;
	}
}


static net_datalink_protocol_module_info sARPModule = {
	{
		"network/datalink_protocols/arp/v1",
		0,
		arp_std_ops
	},
	arp_init_protocol,
	arp_uninit_protocol,
	arp_send_data,
	arp_up,
	arp_down,
	arp_control,
};

module_info *modules[] = {
	(module_info *)&sARPModule,
	NULL
};
