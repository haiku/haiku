/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Atis Elsts, the.kfx@gmail.com
 */


#include <net_datalink.h>
#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_stack.h>
#include <net_protocol.h>
#include <NetBufferUtilities.h>

#include <generic_syscall.h>
#include <util/atomic.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/khash.h>
#include <KernelExport.h>

#include <netinet/in.h>
#include <netinet6/in6.h>
#include <netinet/icmp6.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <sys/sockio.h>
#include <new>

#include <ipv6/jenkins.h>
#include <ipv6/ipv6_address.h>
#include "ndp.h"


#define TRACE_NDP
#ifdef TRACE_NDP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct ipv6_datalink_protocol : net_datalink_protocol {
	sockaddr_dl	hardware_address;
	in6_addr	local_address;
};


static void ndp_timer(struct net_timer* timer, void* data);


net_buffer_module_info* gBufferModule;
static net_stack_module_info* sStackModule;
static net_datalink_module_info* sDatalinkModule;
static net_protocol_module_info* sIPv6Module;
static net_protocol* sIPv6Protocol;
static hash_table* sCache;
static mutex sCacheLock;
static const net_buffer* kDeletedBuffer = (net_buffer*)~0;

// needed for IN6_IS_ADDR_UNSPECIFIED() macro
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;


//	#pragma mark -


struct neighbor_discovery_header {
	uint8		icmp6_type;
	uint8		icmp6_code;
	uint16		icmp6_checksum;
	uint32		flags;
	in6_addr	target_address;

	// This part is specific for Ethernet;
	// also, theoretically there could be more than one option.
	uint8		option_type;
	uint8		option_length;
	uint8		link_address[ETHER_ADDRESS_LENGTH];
} _PACKED;

struct router_advertisement_header {
	uint8		icmp6_type;
	uint8		icmp6_code;
	uint16		icmp6_checksum;
	uint8		hop_limit;
	uint8		flags;
	uint16		router_lifetime;
	uint32		reachable_time;
	uint32		retransmit_timer;
	uint8		options[];
} _PACKED;


struct ndp_entry {
	ndp_entry*	next;
	in6_addr	protocol_address;
	sockaddr_dl	hardware_address;
	uint32		flags;
	net_buffer*	request_buffer;
	net_timer	timer;
	uint32		timer_state;
	bigtime_t	timestamp;
	net_datalink_protocol* protocol;

	typedef DoublyLinkedListCLink<net_buffer> NetBufferLink;
	typedef DoublyLinkedList<net_buffer, NetBufferLink> BufferList;

	BufferList  queue;

	static int Compare(void* _entry, const void* _key);
	static uint32 Hash(void* _entry, const void* _key, uint32 range);
	static ndp_entry* Lookup(const in6_addr& protocolAddress);
	static ndp_entry* Add(const in6_addr& protocolAddress,
		sockaddr_dl* hardwareAddress, uint32 flags);

	~ndp_entry();

	void ClearQueue();
	void MarkFailed();
	void MarkValid();
	void ScheduleRemoval();
};

#define NDP_FLAG_LOCAL		0x01
#define NDP_FLAG_REJECT		0x02
#define NDP_FLAG_PERMANENT	0x04
#define NDP_FLAG_PUBLISH	0x08
#define NDP_FLAG_VALID		0x10

#define NDP_FLAG_REMOVED			0x00010000
#define NDP_PUBLIC_FLAG_MASK		0x0000ffff

#define NDP_NO_STATE				0
#define NDP_STATE_REQUEST			1
#define NDP_STATE_LAST_REQUEST		5
#define NDP_STATE_REQUEST_FAILED	6
#define NDP_STATE_REMOVE_FAILED		7
#define NDP_STATE_STALE				8

#define NDP_STALE_TIMEOUT	30 * 60000000LL		// 30 minutes
#define NDP_REJECT_TIMEOUT	20000000LL			// 20 seconds
#define NDP_REQUEST_TIMEOUT	1000000LL			// 1 second


//	#pragma mark -


static void
ipv6_to_ether_multicast(sockaddr_dl* destination, const sockaddr_in6* source)
{
	// To send an IPv6 multicast packet over Ethernet,
	// take the last 32 bits of the destination IPv6 address,
	// prepend 33-33- and use that as the destination Ethernet address.

	destination->sdl_len = sizeof(sockaddr_dl);
	destination->sdl_family = AF_LINK;
	destination->sdl_index = 0;
	destination->sdl_type = IFT_ETHER;
	destination->sdl_e_type = htons(ETHER_TYPE_IPV6);
	destination->sdl_nlen = destination->sdl_slen = 0;
	destination->sdl_alen = ETHER_ADDRESS_LENGTH;

	destination->sdl_data[0] = 0x33;
	destination->sdl_data[1] = 0x33;
	memcpy(&destination->sdl_data[2], &source->sin6_addr.s6_addr[12], 4);
}


static inline sockaddr*
ipv6_to_sockaddr(sockaddr_in6* target, const in6_addr& address)
{
	target->sin6_family = AF_INET6;
	target->sin6_len = sizeof(sockaddr_in6);
	target->sin6_port = 0;
	target->sin6_flowinfo = 0;
	target->sin6_scope_id = 0;
	memcpy(target->sin6_addr.s6_addr, address.s6_addr, sizeof(in6_addr));
	return (sockaddr*)target;
}


static inline sockaddr*
ipv6_to_solicited_multicast(sockaddr_in6* target, const in6_addr& address)
{
	// The solicited-node multicast address for a given unicast address
	// is constructed by taking the last three octets of the unicast address
	// and prepending FF02::1:FF00:0000/104.

	target->sin6_family = AF_INET6;
	target->sin6_len = sizeof(sockaddr_in6);
	target->sin6_port = 0;
	target->sin6_flowinfo = 0;
	target->sin6_scope_id = 0;

	uint8* targetIPv6 = target->sin6_addr.s6_addr;
	memset(targetIPv6, 0, sizeof(in6_addr));
	targetIPv6[0] = 0xff;
	targetIPv6[1] = 0x02;
	targetIPv6[11] = 0x01;
	targetIPv6[12] = 0xff;
 	memcpy(&targetIPv6[13], &address.s6_addr[13], 3);

	return (sockaddr*)target;
}


static inline uint32
hash_ipv6_address(const in6_addr& address, uint32 range)
{
	return jenkins_hashword((const uint32*)&address,
		sizeof(in6_addr) / sizeof(uint32), 0) % range;
}


//	#pragma mark -


static net_buffer*
get_request_buffer(ndp_entry* entry)
{
	net_buffer* buffer = entry->request_buffer;
	if (buffer == NULL || buffer == kDeletedBuffer)
		return NULL;

	buffer = atomic_pointer_test_and_set(&entry->request_buffer,
		(net_buffer*)NULL, buffer);
	if (buffer == kDeletedBuffer)
		return NULL;

	return buffer;
}


static void
put_request_buffer(ndp_entry* entry, net_buffer* buffer)
{
	net_buffer* requestBuffer = atomic_pointer_test_and_set(
		&entry->request_buffer, buffer, (net_buffer*)NULL);
	if (requestBuffer != NULL) {
		// someone else took over ownership of the request buffer
		gBufferModule->free(buffer);
	}
}


static void
delete_request_buffer(ndp_entry* entry)
{
	net_buffer* buffer = atomic_pointer_set(&entry->request_buffer,
		kDeletedBuffer);
	if (buffer != NULL && buffer != kDeletedBuffer)
		gBufferModule->free(buffer);
}


int
ndp_entry::Compare(void* _entry, const void* _key)
{
	ndp_entry* entry = (ndp_entry*)_entry;
	in6_addr* key = (in6_addr*)_key;

	if (entry->protocol_address == *key)
		return 0;

	return 1;
}


uint32
ndp_entry::Hash(void* _entry, const void* _key, uint32 range)
{
	ndp_entry* entry = (ndp_entry*)_entry;
	const in6_addr* key = (const in6_addr*)_key;

	if (entry != NULL)
		return hash_ipv6_address(entry->protocol_address, range);

	return hash_ipv6_address(*key, range);
}


ndp_entry*
ndp_entry::Lookup(const in6_addr& address)
{
	return (ndp_entry*)hash_lookup(sCache, &address);
}


ndp_entry*
ndp_entry::Add(const in6_addr& protocolAddress, sockaddr_dl* hardwareAddress,
	uint32 flags)
{
	ASSERT_LOCKED_MUTEX(&sCacheLock);

	ndp_entry* entry = new (std::nothrow) ndp_entry;
	if (entry == NULL)
		return NULL;

	entry->protocol_address = protocolAddress;
	entry->flags = flags;
	entry->timestamp = system_time();
	entry->protocol = NULL;
	entry->request_buffer = NULL;
	entry->timer_state = NDP_NO_STATE;
	sStackModule->init_timer(&entry->timer, ndp_timer, entry);

	if (hardwareAddress != NULL) {
		// this entry is already resolved
		entry->hardware_address = *hardwareAddress;
		entry->hardware_address.sdl_e_type = htons(ETHER_TYPE_IPV6);
	} else {
		// this entry still needs to be resolved
		entry->hardware_address.sdl_alen = 0;
	}
	if (entry->hardware_address.sdl_len != sizeof(sockaddr_dl)) {
		// explicitly set correct length in case our caller hasn't...
		entry->hardware_address.sdl_len = sizeof(sockaddr_dl);
	}

	if (hash_insert(sCache, entry) != B_OK) {
		// We can delete the entry here with the sCacheLock held, since it's
		// guaranteed there are no timers pending.
		delete entry;
		return NULL;
	}

	return entry;
}


ndp_entry::~ndp_entry()
{
	// make sure there is no active timer left for us
	sStackModule->cancel_timer(&timer);
	sStackModule->wait_for_timer(&timer);

	ClearQueue();
}


void
ndp_entry::ClearQueue()
{
	BufferList::Iterator iterator = queue.GetIterator();
	while (iterator.HasNext()) {
		net_buffer* buffer = iterator.Next();
		iterator.Remove();
		gBufferModule->free(buffer);
	}
}


void
ndp_entry::MarkFailed()
{
	TRACE(("NDP entry %p Marked as FAILED\n", this));

	flags = (flags & ~NDP_FLAG_VALID) | NDP_FLAG_REJECT;
	ClearQueue();
}


void
ndp_entry::MarkValid()
{
	TRACE(("NDP entry %p Marked as VALID\n", this));

	flags = (flags & ~NDP_FLAG_REJECT) | NDP_FLAG_VALID;

	BufferList::Iterator iterator = queue.GetIterator();
	while (iterator.HasNext()) {
		net_buffer* buffer = iterator.Next();
		iterator.Remove();

		TRACE(("  NDP Dequeing packet %p...\n", buffer));

		memcpy(buffer->destination, &hardware_address,
			hardware_address.sdl_len);
		protocol->next->module->send_data(protocol->next, buffer);
	}
}


void
ndp_entry::ScheduleRemoval()
{
	// schedule a timer to remove this entry
	timer_state = NDP_STATE_REMOVE_FAILED;
	sStackModule->set_timer(&timer, 0);
}


//	#pragma mark -


static status_t
ndp_init()
{
	sIPv6Protocol = sIPv6Module->init_protocol(NULL);
	if (sIPv6Protocol == NULL)
		return B_NO_MEMORY;
	sIPv6Protocol->module = sIPv6Module;
	sIPv6Protocol->socket = NULL;
	sIPv6Protocol->next = NULL;

	int value = 255;
	sIPv6Module->setsockopt(sIPv6Protocol, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		&value, sizeof(value));

	mutex_init(&sCacheLock, "ndp cache");

	sCache = hash_init(64, offsetof(struct ndp_entry, next),
		&ndp_entry::Compare, &ndp_entry::Hash);
	if (sCache == NULL) {
		mutex_destroy(&sCacheLock);
		return B_NO_MEMORY;
	}

	return B_OK;
}


static status_t
ndp_uninit()
{
	if (sIPv6Protocol)
		sIPv6Module->uninit_protocol(sIPv6Protocol);

	return B_OK;
}


//	#pragma mark -


/*!	Updates the entry determined by \a protocolAddress with the specified
	\a hardwareAddress.
	If such an entry does not exist yet, a new entry is added. If you try
	to update a local existing entry but didn't ask for it (by setting
	\a flags to NDP_FLAG_LOCAL), an error is returned.

	This function does not lock the cache - you have to do it yourself
	before calling it.
*/
status_t
ndp_update_entry(const in6_addr& protocolAddress, sockaddr_dl* hardwareAddress,
	uint32 flags, ndp_entry** _entry = NULL)
{
	ASSERT_LOCKED_MUTEX(&sCacheLock);

	ndp_entry* entry = ndp_entry::Lookup(protocolAddress);
	if (entry != NULL) {
		// We disallow updating of entries that had been resolved before,
		// but to a different address (only for those that belong to a
		// specific address - redefining INADDR_ANY is always allowed).
		// Right now, you have to manually purge the NDP entries (or wait some
		// time) to let us switch to the new address.
		if (!IN6_IS_ADDR_UNSPECIFIED(&protocolAddress)
			&& entry->hardware_address.sdl_alen != 0
			&& memcmp(LLADDR(&entry->hardware_address),
				LLADDR(hardwareAddress), ETHER_ADDRESS_LENGTH)) {
			// TODO: also printf the address
			dprintf("NDP host updated with different hardware address "
				"%02x:%02x:%02x:%02x:%02x:%02x.\n",
				hardwareAddress->sdl_data[0], hardwareAddress->sdl_data[1],
				hardwareAddress->sdl_data[2], hardwareAddress->sdl_data[3],
				hardwareAddress->sdl_data[4], hardwareAddress->sdl_data[5]);
			return B_ERROR;
		}

		entry->hardware_address = *hardwareAddress;
		entry->timestamp = system_time();
	} else {
		entry = ndp_entry::Add(protocolAddress, hardwareAddress, flags);
		if (entry == NULL)
			return B_NO_MEMORY;
	}

	delete_request_buffer(entry);

	if ((entry->flags & NDP_FLAG_PERMANENT) == 0) {
		// (re)start the stale timer
		entry->timer_state = NDP_STATE_STALE;
		sStackModule->set_timer(&entry->timer, NDP_STALE_TIMEOUT);
	}

	if ((entry->flags & NDP_FLAG_REJECT) != 0)
		entry->MarkFailed();
	else
		entry->MarkValid();

	if (_entry)
		*_entry = entry;

	return B_OK;
}


static void
ndp_remove_local_entry(ipv6_datalink_protocol* protocol, const sockaddr* local,
	bool updateLocalAddress)
{
	in6_addr inetAddress;

	if (local == NULL) {
		// interface has not yet been set
		memset(&inetAddress, 0, sizeof(in6_addr));
	} else {
		memcpy(&inetAddress, &((sockaddr_in6*)local)->sin6_addr,
			sizeof(in6_addr));

		// leave the NS multicast address
		sockaddr_in6 multicast;
		ipv6_to_solicited_multicast(&multicast, inetAddress);

		struct ipv6_mreq mreq;
		memcpy(&mreq.ipv6mr_multiaddr, &multicast.sin6_addr, sizeof(in6_addr));
		mreq.ipv6mr_interface = protocol->interface->index;

		if (sIPv6Protocol != NULL) {
			sIPv6Module->setsockopt(sIPv6Protocol, IPPROTO_IPV6,
				IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
		}
	}

	// TRACE(("%s(): address %s\n", __FUNCTION__, inet6_to_string(inetAddress)));

	MutexLocker locker(sCacheLock);

	ndp_entry* entry = ndp_entry::Lookup(inetAddress);
	if (entry != NULL) {
		hash_remove(sCache, entry);
		entry->flags |= NDP_FLAG_REMOVED;
	}

	if (updateLocalAddress && protocol->local_address == inetAddress) {
		// find new local sender address
		memset(&protocol->local_address, 0, sizeof(in6_addr));

		net_interface_address* address = NULL;
		while (sDatalinkModule->get_next_interface_address(protocol->interface,
				&address)) {
			if (address->local == NULL || address->local->sa_family != AF_INET6)
				continue;

			memcpy(&protocol->local_address,
				&((sockaddr_in6*)address->local)->sin6_addr, sizeof(in6_addr));
		}
	}

	locker.Unlock();
	delete entry;
}


/*!	Removes all entries belonging to the local interface of the \a procotol
	given.
*/
static void
ndp_remove_local(ipv6_datalink_protocol* protocol)
{
	net_interface_address* address = NULL;
	while (sDatalinkModule->get_next_interface_address(protocol->interface,
			&address)) {
		if (address->local == NULL || address->local->sa_family != AF_INET6)
			continue;

		ndp_remove_local_entry(protocol, address->local, false);
	}
}


static status_t
ndp_set_local_entry(ipv6_datalink_protocol* protocol, const sockaddr* local)
{
	MutexLocker locker(sCacheLock);

	net_interface* interface = protocol->interface;
	in6_addr inetAddress;

	if (local == NULL) {
		// interface has not yet been set
		memset(&inetAddress, 0, sizeof(in6_addr));
	} else {
		memcpy(&inetAddress,
			&((sockaddr_in6*)local)->sin6_addr,
			sizeof(in6_addr));

		// join multicast address for listening to NS packets
		sockaddr_in6 multicast;
		ipv6_to_solicited_multicast(&multicast, inetAddress);

		struct ipv6_mreq mreq;
		memcpy(&mreq.ipv6mr_multiaddr, &multicast.sin6_addr, sizeof(in6_addr));
		mreq.ipv6mr_interface = protocol->interface->index;

		if (sIPv6Protocol != NULL) {
			sIPv6Module->setsockopt(sIPv6Protocol, IPPROTO_IPV6,
				IPV6_JOIN_GROUP, &mreq, sizeof(mreq));
		}
	}

	// TRACE(("%s(): address %s\n", __FUNCTION__, inet6_to_string(inetAddress)));

	if (IN6_IS_ADDR_UNSPECIFIED(&protocol->local_address))
		memcpy(&protocol->local_address, &inetAddress, sizeof(in6_addr));

	sockaddr_dl address;
	address.sdl_len = sizeof(sockaddr_dl);
	address.sdl_family = AF_LINK;
	address.sdl_type = IFT_ETHER;
	address.sdl_e_type = htons(ETHER_TYPE_IPV6);
	address.sdl_nlen = 0;
	address.sdl_slen = 0;
	address.sdl_alen = interface->device->address.length;
	memcpy(LLADDR(&address), interface->device->address.data, address.sdl_alen);

	memcpy(&protocol->hardware_address, &address, sizeof(sockaddr_dl));
		// cache the address in our protocol

	ndp_entry* entry;
	status_t status = ndp_update_entry(inetAddress, &address,
		NDP_FLAG_LOCAL | NDP_FLAG_PERMANENT, &entry);
	if (status == B_OK)
		entry->protocol = protocol;

	return status;
}


/*!	Creates permanent local entries for all addresses of the interface belonging
	to this protocol.
	Returns an error if no entry could be added.
*/
static status_t
ndp_update_local(ipv6_datalink_protocol* protocol)
{
	memset(&protocol->local_address, 0, sizeof(in6_addr));

	ssize_t count = 0;

	net_interface_address* address = NULL;
	while (sDatalinkModule->get_next_interface_address(protocol->interface,
			&address)) {
		if (address->local == NULL || address->local->sa_family != AF_INET6)
			continue;

		if (ndp_set_local_entry(protocol, address->local) == B_OK) {
			count++;
		}
	}

	if (count == 0)
		return ndp_set_local_entry(protocol, NULL);

	return B_OK;
}


static status_t
ndp_receive_solicitation(net_buffer* buffer, bool* reuseBuffer)
{
	*reuseBuffer = false;

	NetBufferHeaderReader<neighbor_discovery_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	neighbor_discovery_header& header = bufferHeader.Data();
	if (header.option_type != ND_OPT_SOURCE_LINKADDR
		|| header.option_length != 1)
		return B_OK;

	{
		MutexLocker locker(sCacheLock);

		// remember the address of the sender as we might need it later
		sockaddr_dl hardwareAddress;
		hardwareAddress.sdl_len = sizeof(sockaddr_dl);
		hardwareAddress.sdl_family = AF_LINK;
		hardwareAddress.sdl_index = 0;
		hardwareAddress.sdl_type = IFT_ETHER;
		hardwareAddress.sdl_e_type = htons(ETHER_TYPE_IPV6);
		hardwareAddress.sdl_nlen = hardwareAddress.sdl_slen = 0;
		hardwareAddress.sdl_alen = ETHER_ADDRESS_LENGTH;
		memcpy(LLADDR(&hardwareAddress), header.link_address,
			ETHER_ADDRESS_LENGTH);

		ndp_update_entry(header.target_address, &hardwareAddress, 0);

		// check if this request is for us

		ndp_entry* entry = ndp_entry::Lookup(header.target_address);
		if (entry == NULL
			|| (entry->flags & (NDP_FLAG_LOCAL | NDP_FLAG_PUBLISH)) == 0) {
			// We're not the one to answer this request
			// TODO: instead of letting the other's request time-out, can we
			//	reply failure somehow?
			TRACE(("  not for us\n"));
			return B_ERROR;
		}

		// send a reply (by reusing the buffer we got)
		gBufferModule->trim(buffer, sizeof(neighbor_discovery_header));

		header.icmp6_type = ND_NEIGHBOR_SOLICIT;
		header.icmp6_code = 0;
		header.icmp6_checksum = 0;
		header.flags = ND_NA_FLAG_SOLICITED;
		header.option_type = ND_OPT_TARGET_LINKADDR;
		memcpy(&header.link_address, LLADDR(&entry->hardware_address),
			ETHER_ADDRESS_LENGTH);
		bufferHeader.Sync();
	}

	// fix source and destination address
	sockaddr_in6* source = (sockaddr_in6*)buffer->source;
	sockaddr_in6* destination = (sockaddr_in6*)buffer->destination;
	memcpy(&destination->sin6_addr, &source->sin6_addr, sizeof(in6_addr));
	memcpy(&source->sin6_addr, &header.target_address, sizeof(in6_addr));

	buffer->flags = 0;
		// make sure this won't be a broadcast message

	if (sIPv6Protocol == NULL)
		return B_ERROR;

	*reuseBuffer = true;

	// send the ICMPv6 packet out
	TRACE(("Sending Neighbor Advertisement\n"));
	return sIPv6Module->send_data(sIPv6Protocol, buffer);
}


static void
ndp_receive_advertisement(net_buffer* buffer)
{
	// TODO: also process unsolicited advertisments?
	if ((buffer->flags & MSG_MCAST) != 0)
		return;

	NetBufferHeaderReader<neighbor_discovery_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return;

	neighbor_discovery_header& header = bufferHeader.Data();
	if (header.option_type != ND_OPT_TARGET_LINKADDR
		|| header.option_length != 1) {
		return;
	}

	sockaddr_dl hardwareAddress;
	hardwareAddress.sdl_len = sizeof(sockaddr_dl);
	hardwareAddress.sdl_family = AF_LINK;
	hardwareAddress.sdl_index = 0;
	hardwareAddress.sdl_type = IFT_ETHER;
	hardwareAddress.sdl_e_type = htons(ETHER_TYPE_IPV6);
	hardwareAddress.sdl_nlen = hardwareAddress.sdl_slen = 0;
	hardwareAddress.sdl_alen = ETHER_ADDRESS_LENGTH;
	memcpy(LLADDR(&hardwareAddress), header.link_address, ETHER_ADDRESS_LENGTH);

	MutexLocker locker(sCacheLock);
	// TODO: take in account ND_NA_FLAGs
	ndp_update_entry(header.target_address, &hardwareAddress, 0);
}


static void
ndp_receive_router_advertisement(net_buffer* buffer)
{
	NetBufferHeaderReader<router_advertisement_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return;

	// TODO: check for validity

	// TODO: parse the options
}


static status_t
ndp_receive_data(net_buffer* buffer)
{
	dprintf("ndp_receive_data\n");

	NetBufferHeaderReader<icmp6_hdr> icmp6Header(buffer);
	if (icmp6Header.Status() < B_OK)
		return icmp6Header.Status();

	bool reuseBuffer = false;

	switch (icmp6Header->icmp6_type) {
		case ND_NEIGHBOR_SOLICIT:
			TRACE(("  received Neighbor Solicitation\n"));
			ndp_receive_solicitation(buffer, &reuseBuffer);
			break;

		case ND_NEIGHBOR_ADVERT:
			TRACE(("  received Neighbor Advertisement\n"));
			ndp_receive_advertisement(buffer);
			break;

		case ND_ROUTER_ADVERT:
			TRACE(("  received Router Advertisement\n"));
			ndp_receive_router_advertisement(buffer);
			break;
	}

	if (reuseBuffer == false)
		gBufferModule->free(buffer);
	return B_OK;
}


static void
ndp_timer(struct net_timer* timer, void* data)
{
	ndp_entry* entry = (ndp_entry*)data;
	TRACE(("NDP timer %" B_PRId32 ", entry %p!\n", entry->timer_state, entry));

	switch (entry->timer_state) {
		case NDP_NO_STATE:
			// who are you kidding?
			break;

		case NDP_STATE_REQUEST_FAILED:
			// Requesting the NDP entry failed, we keep it around for a while,
			// though, so that we won't try to request the same address again
			// too soon.
			TRACE(("  requesting NDP entry %p failed!\n", entry));
			entry->timer_state = NDP_STATE_REMOVE_FAILED;
			entry->MarkFailed();
			sStackModule->set_timer(&entry->timer, NDP_REJECT_TIMEOUT);
			break;

		case NDP_STATE_REMOVE_FAILED:
		case NDP_STATE_STALE:
			// the entry has aged so much that we're going to remove it
			TRACE(("  remove NDP entry %p!\n", entry));

			mutex_lock(&sCacheLock);
			if ((entry->flags & NDP_FLAG_REMOVED) != 0) {
				// The entry has already been removed, and is about to be deleted
				mutex_unlock(&sCacheLock);
				break;
			}

			hash_remove(sCache, entry);
			mutex_unlock(&sCacheLock);

			delete entry;
			break;

		default:
		{
			if (entry->timer_state > NDP_STATE_LAST_REQUEST)
				break;

			TRACE(("  send request for NDP entry %p!\n", entry));

			net_buffer* request = get_request_buffer(entry);
			if (request == NULL)
				break;

			if (entry->timer_state < NDP_STATE_LAST_REQUEST) {
				// we'll still need our buffer, so in order to prevent it being
				// freed by a successful send, we need to clone it
				net_buffer* clone = gBufferModule->clone(request, true);
				if (clone == NULL) {
					// cloning failed - that means we won't be able to send as
					// many requests as originally planned
					entry->timer_state = NDP_STATE_LAST_REQUEST;
				} else {
					put_request_buffer(entry, request);
					request = clone;
				}
			}

			if (sIPv6Protocol == NULL)
				break;

			// we're trying to resolve the address, so keep sending requests
			status_t status = sIPv6Module->send_data(sIPv6Protocol, request);
			if (status < B_OK)
				gBufferModule->free(request);

			entry->timer_state++;
			sStackModule->set_timer(&entry->timer, NDP_REQUEST_TIMEOUT);
			break;
		}
	}
}


static status_t
ndp_start_resolve(ipv6_datalink_protocol* protocol, const in6_addr& address,
	ndp_entry** _entry)
{
	ASSERT_LOCKED_MUTEX(&sCacheLock);

	// create an unresolved entry as a placeholder
	ndp_entry* entry = ndp_entry::Add(address, NULL, 0);
	if (entry == NULL)
		return B_NO_MEMORY;

	// prepare NDP request

	net_buffer* buffer = entry->request_buffer = gBufferModule->create(256);
	if (entry->request_buffer == NULL) {
		entry->ScheduleRemoval();
		return B_NO_MEMORY;
	}

	NetBufferPrepend<neighbor_discovery_header> header(buffer);
	status_t status = header.Status();
	if (status < B_OK) {
		entry->ScheduleRemoval();
		return status;
	}

	net_interface* interface = protocol->interface;
	net_device* device = interface->device;

	// prepare source and target addresses

	sockaddr_in6* source = (sockaddr_in6*)buffer->source;
	ipv6_to_sockaddr(source, protocol->local_address);
	// protocol->local_address

	sockaddr_in6* destination = (sockaddr_in6*)buffer->destination;
	ipv6_to_solicited_multicast(destination, address);

	buffer->protocol = IPPROTO_ICMPV6;

	// prepare Neighbor Solicitation header

	header->icmp6_type = ND_NEIGHBOR_SOLICIT;
	header->icmp6_code = 0;
	header->icmp6_checksum = 0;
	header->flags = 0;
	memcpy(&header->target_address, &address, sizeof(in6_addr));
	header->option_type = ND_OPT_SOURCE_LINKADDR;
	header->option_length = (sizeof(nd_opt_hdr) + ETHER_ADDRESS_LENGTH) >> 3;
	memcpy(&header->link_address, device->address.data, ETHER_ADDRESS_LENGTH);
	header.Sync();

	if (sIPv6Protocol == NULL) {
		entry->ScheduleRemoval();
		return B_NO_MEMORY;
	}

	// this does not work, because multicast for now is only looped back!
#if FIXME
	// hack: set to use the correct interface by setting socket option
	sIPv6Module->setsockopt(sIPv6Protocol, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		&source->sin6_addr, sizeof(in6_addr));
#endif

	net_buffer* clone = gBufferModule->clone(buffer, true);
	if (clone == NULL) {
		entry->ScheduleRemoval();
		return B_NO_MEMORY;
	}

	// send the ICMPv6 packet out
	TRACE(("Sending Neighbor Solicitation\n"));
	status = sIPv6Module->send_data(sIPv6Protocol, clone);
	if (status < B_OK) {
		entry->ScheduleRemoval();
		return status;
	}

	entry->protocol = protocol;
	entry->timer_state = NDP_STATE_REQUEST;
	sStackModule->set_timer(&entry->timer, 0);
		// start request timer

	*_entry = entry;
	return B_OK;
}


//	#pragma mark -


static status_t
ipv6_datalink_init(net_interface* interface, net_domain* domain,
	net_datalink_protocol** _protocol)
{
	if (domain->family != AF_INET6)
		return B_BAD_TYPE;

	status_t status = sStackModule->register_domain_device_handler(
		interface->device, B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_IPV6), domain);
	if (status != B_OK)
		return status;

	ipv6_datalink_protocol* protocol = new(std::nothrow) ipv6_datalink_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	memset(&protocol->hardware_address, 0, sizeof(sockaddr_dl));
	memset(&protocol->local_address, 0, sizeof(in6_addr));
	*_protocol = protocol;
	return B_OK;
}


static status_t
ipv6_datalink_uninit(net_datalink_protocol* protocol)
{
	sStackModule->unregister_device_handler(protocol->interface->device,
		B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_IPV6));

	delete protocol;
 	return B_OK;
}


static status_t
ipv6_datalink_send_data(net_datalink_protocol* _protocol, net_buffer* buffer)
{
	ipv6_datalink_protocol* protocol = (ipv6_datalink_protocol*)_protocol;

	memcpy(buffer->source, &protocol->hardware_address,
		protocol->hardware_address.sdl_len);

	if ((buffer->flags & MSG_MCAST) != 0) {
		sockaddr_dl multicastDestination;
		ipv6_to_ether_multicast(&multicastDestination,
			(sockaddr_in6*)buffer->destination);
		memcpy(buffer->destination, &multicastDestination,
			sizeof(sockaddr_dl));
	} else {
		MutexLocker locker(sCacheLock);

		// Lookup destination (we may need to wait for this)
		ndp_entry* entry = ndp_entry::Lookup(
			((struct sockaddr_in6*)buffer->destination)->sin6_addr);
		if (entry == NULL) {
			status_t status = ndp_start_resolve(protocol,
				((struct sockaddr_in6*)buffer->destination)->sin6_addr, &entry);
			if (status < B_OK)
				return status;
		}

		if ((entry->flags & NDP_FLAG_REJECT) != 0)
			return EHOSTUNREACH;
		if (!(entry->flags & NDP_FLAG_VALID)) {
			// entry is still being resolved.
			TRACE(("NDP Queuing packet %p, entry still being resolved.\n",
					buffer));
			entry->queue.Add(buffer);
			return B_OK;
		}

		memcpy(buffer->destination, &entry->hardware_address,
			entry->hardware_address.sdl_len);
	}

	return protocol->next->module->send_data(protocol->next, buffer);
}



static status_t
ipv6_datalink_up(net_datalink_protocol* _protocol)
{
	ipv6_datalink_protocol* protocol = (ipv6_datalink_protocol*)_protocol;
	status_t status = protocol->next->module->interface_up(protocol->next);
	if (status != B_OK)
		return status;

	// cache this device's address for later use

	status = ndp_update_local(protocol);
	if (status != B_OK) {
		protocol->next->module->interface_down(protocol->next);
		return status;
	}

	return B_OK;
}


static void
ipv6_datalink_down(net_datalink_protocol *protocol)
{
	// remove local NDP entries from the cache
	ndp_remove_local((ipv6_datalink_protocol*)protocol);

	protocol->next->module->interface_down(protocol->next);
}


status_t
ipv6_datalink_change_address(net_datalink_protocol* _protocol,
	net_interface_address* address, int32 option,
	const struct sockaddr* oldAddress, const struct sockaddr* newAddress)
{
	ipv6_datalink_protocol* protocol = (ipv6_datalink_protocol*)_protocol;
	switch (option) {
		case SIOCSIFADDR:
		case SIOCAIFADDR:
		case SIOCDIFADDR:
			// Those are the options we handle
			if ((protocol->interface->flags & IFF_UP) != 0) {
				// Update NDP entry for the local address

				if (newAddress != NULL && newAddress->sa_family == AF_INET6) {
					status_t status = ndp_set_local_entry(protocol, newAddress);
					if (status != B_OK)
						return status;

					// add IPv6 multicast route (ff00::/8)
					sockaddr_in6 socketAddress;
					memset(&socketAddress, 0, sizeof(sockaddr_in6));
					socketAddress.sin6_family = AF_INET6;
					socketAddress.sin6_len = sizeof(sockaddr_in6);
					socketAddress.sin6_addr.s6_addr[0] = 0xff;

					net_route route;
					memset(&route, 0, sizeof(net_route));
					route.destination = (sockaddr*)&socketAddress;
					route.mask = (sockaddr*)&socketAddress;
					route.flags = 0;
					sDatalinkModule->add_route(address->domain, &route);
				}

				if (oldAddress != NULL && oldAddress->sa_family == AF_INET6) {
					ndp_remove_local_entry(protocol, oldAddress, true);

					// remove IPv6 multicast route (ff00::/8)
					sockaddr_in6 socketAddress;
					memset(&socketAddress, 0, sizeof(sockaddr_in6));
					socketAddress.sin6_family = AF_INET6;
					socketAddress.sin6_len = sizeof(sockaddr_in6);
					socketAddress.sin6_addr.s6_addr[0] = 0xff;

					net_route route;
					memset(&route, 0, sizeof(net_route));
					route.destination = (sockaddr*)&socketAddress;
					route.mask = (sockaddr*)&socketAddress;
					route.flags = 0;
					sDatalinkModule->remove_route(address->domain, &route);
				}
			}
			break;

		default:
			break;
	}

	return protocol->next->module->change_address(protocol->next, address,
		option, oldAddress, newAddress);
}


static status_t
ipv6_datalink_control(net_datalink_protocol* protocol, int32 op, void* argument,
	size_t length)
{
	return protocol->next->module->control(protocol->next, op, argument,
		length);
}


static status_t
ipv6_datalink_join_multicast(net_datalink_protocol* protocol,
	const sockaddr* address)
{
	if (address->sa_family != AF_INET6)
		return EINVAL;

	sockaddr_dl multicastAddress;
	ipv6_to_ether_multicast(&multicastAddress, (const sockaddr_in6*)address);

	return protocol->next->module->join_multicast(protocol->next,
		(sockaddr*)&multicastAddress);
}


static status_t
ipv6_datalink_leave_multicast(net_datalink_protocol* protocol,
	const sockaddr* address)
{
	if (address->sa_family != AF_INET6)
		return EINVAL;

	sockaddr_dl multicastAddress;
	ipv6_to_ether_multicast(&multicastAddress, (const sockaddr_in6*)address);

	return protocol->next->module->leave_multicast(protocol->next,
		(sockaddr*)&multicastAddress);
}


static status_t
ipv6_datalink_std_ops(int32 op, ...)
{
	switch (op) {
	case B_MODULE_INIT:
		return ndp_init();

	case B_MODULE_UNINIT:
		return ndp_uninit();
	}

	return B_ERROR;
}


net_datalink_protocol_module_info gIPv6DataLinkModule = {
	{
		"network/datalink_protocols/ipv6_datagram/v1",
		0,
		ipv6_datalink_std_ops
	},
	ipv6_datalink_init,
	ipv6_datalink_uninit,
	ipv6_datalink_send_data,
	ipv6_datalink_up,
	ipv6_datalink_down,
	ipv6_datalink_change_address,
	ipv6_datalink_control,
	ipv6_datalink_join_multicast,
	ipv6_datalink_leave_multicast,
};

net_ndp_module_info gIPv6NDPModule = {
	{
		"network/datalink_protocols/ipv6_datagram/ndp/v1",
		0,
		NULL
	},
	ndp_receive_data
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&sStackModule},
	{NET_DATALINK_MODULE_NAME, (module_info**)&sDatalinkModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{"network/protocols/ipv6/v1", (module_info**)&sIPv6Module},
	{}
};

module_info* modules[] = {
	(module_info*)&gIPv6DataLinkModule,
	(module_info*)&gIPv6NDPModule,
	NULL
};
