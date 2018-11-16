/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 */


//! Ethernet Address Resolution Protocol, see RFC 826.


#include <arp_control.h>
#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_datalink.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <generic_syscall.h>
#include <util/atomic.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

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
	net_buffer	*request_buffer;
	net_timer	timer;
	uint32		timer_state;
	bigtime_t	timestamp;
	net_datalink_protocol *protocol;

	typedef DoublyLinkedListCLink<net_buffer> NetBufferLink;
	typedef DoublyLinkedList<net_buffer, NetBufferLink> BufferList;

	BufferList  queue;

	static arp_entry *Lookup(in_addr_t protocolAddress);
	static arp_entry *Add(in_addr_t protocolAddress,
		sockaddr_dl *hardwareAddress, uint32 flags);

	~arp_entry();

	void ClearQueue();
	void MarkFailed();
	void MarkValid();
	void ScheduleRemoval();
};

// see arp_control.h for more flags
#define ARP_FLAG_REMOVED			0x00010000
#define ARP_PUBLIC_FLAG_MASK		0x0000ffff

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
	sockaddr_dl	hardware_address;
	in_addr_t	local_address;
};


static const net_buffer* kDeletedBuffer = (net_buffer*)~0;

static void arp_timer(struct net_timer *timer, void *data);

net_buffer_module_info* gBufferModule;
static net_stack_module_info* sStackModule;
static net_datalink_module_info* sDatalinkModule;
static mutex sCacheLock;
static bool sIgnoreReplies;


struct arpHash {
	typedef in_addr_t KeyType;
	typedef arp_entry ValueType;

	size_t HashKey(KeyType key) const
	{
		return key;
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(value->protocol_address);
	}

	bool Compare(KeyType key, ValueType* value) const
	{
		return value->protocol_address == key;
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->next;
	}
};


typedef BOpenHashTable<arpHash> AddressCache;
static AddressCache* sCache;


#ifdef TRACE_ARP


const char*
mac_to_string(uint8* address)
{
	static char buffer[20];
	snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
		address[0], address[1], address[2], address[3], address[4], address[5]);
	return buffer;
}


const char*
inet_to_string(in_addr_t address)
{
	static char buffer[20];

	unsigned int hostAddress = ntohl(address);
	snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d",
		hostAddress >> 24, (hostAddress >> 16) & 0xff,
		(hostAddress >> 8) & 0xff, hostAddress & 0xff);
	return buffer;
}


#endif	// TRACE_ARP


static net_buffer*
get_request_buffer(arp_entry* entry)
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
put_request_buffer(arp_entry* entry, net_buffer* buffer)
{
	net_buffer* requestBuffer = atomic_pointer_test_and_set(
		&entry->request_buffer, buffer, (net_buffer*)NULL);
	if (requestBuffer != NULL) {
		// someone else took over ownership of the request buffer
		gBufferModule->free(buffer);
	}
}


static void
delete_request_buffer(arp_entry* entry)
{
	net_buffer* buffer = atomic_pointer_get_and_set(&entry->request_buffer,
		kDeletedBuffer);
	if (buffer != NULL && buffer != kDeletedBuffer)
		gBufferModule->free(buffer);
}


static void
ipv4_to_ether_multicast(sockaddr_dl *destination, const sockaddr_in *source)
{
	// RFC 1112 - Host extensions for IP multicasting
	//
	//   ``An IP host group address is mapped to an Ethernet multicast
	//   address by placing the low-order 23-bits of the IP address into
	//   the low-order 23 bits of the Ethernet multicast address
	//   01-00-5E-00-00-00 (hex).''

	destination->sdl_len = sizeof(sockaddr_dl);
	destination->sdl_family = AF_LINK;
	destination->sdl_index = 0;
	destination->sdl_type = IFT_ETHER;
	destination->sdl_e_type = htons(ETHER_TYPE_IP);
	destination->sdl_nlen = destination->sdl_slen = 0;
	destination->sdl_alen = ETHER_ADDRESS_LENGTH;

	memcpy(LLADDR(destination) + 2, &source->sin_addr, sizeof(in_addr));
	uint32 *data = (uint32 *)LLADDR(destination);
	data[0] = (data[0] & htonl(0x7f)) | htonl(0x01005e00);
}


// #pragma mark -


/*static*/ arp_entry *
arp_entry::Lookup(in_addr_t address)
{
	return sCache->Lookup(address);
}


/*static*/ arp_entry *
arp_entry::Add(in_addr_t protocolAddress, sockaddr_dl *hardwareAddress,
	uint32 flags)
{
	ASSERT_LOCKED_MUTEX(&sCacheLock);

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
		entry->hardware_address.sdl_e_type = htons(ETHER_TYPE_IP);
	} else {
		// this entry still needs to be resolved
		entry->hardware_address.sdl_alen = 0;
	}
	if (entry->hardware_address.sdl_len != sizeof(sockaddr_dl)) {
		// explicitly set correct length in case our caller hasn't...
		entry->hardware_address.sdl_len = sizeof(sockaddr_dl);
	}

	if (sCache->Insert(entry) != B_OK) {
		// We can delete the entry here with the sCacheLock held, since it's
		// guaranteed there are no timers pending.
		delete entry;
		return NULL;
	}

	return entry;
}


arp_entry::~arp_entry()
{
	// make sure there is no active timer left for us
	sStackModule->cancel_timer(&timer);
	sStackModule->wait_for_timer(&timer);

	ClearQueue();
}


void
arp_entry::ClearQueue()
{
	BufferList::Iterator iterator = queue.GetIterator();
	while (iterator.HasNext()) {
		net_buffer *buffer = iterator.Next();
		iterator.Remove();
		gBufferModule->free(buffer);
	}
}


void
arp_entry::MarkFailed()
{
	TRACE(("ARP entry %p Marked as FAILED\n", this));

	flags = (flags & ~ARP_FLAG_VALID) | ARP_FLAG_REJECT;
	ClearQueue();
}


void
arp_entry::MarkValid()
{
	TRACE(("ARP entry %p Marked as VALID, have %li packets queued.\n", this,
		queue.Count()));

	flags = (flags & ~ARP_FLAG_REJECT) | ARP_FLAG_VALID;

	BufferList::Iterator iterator = queue.GetIterator();
	while (iterator.HasNext()) {
		net_buffer *buffer = iterator.Next();
		iterator.Remove();

		TRACE(("  ARP Dequeing packet %p...\n", buffer));

		memcpy(buffer->destination, &hardware_address,
			hardware_address.sdl_len);
		protocol->next->module->send_data(protocol->next, buffer);
	}
}


void
arp_entry::ScheduleRemoval()
{
	// schedule a timer to remove this entry
	timer_state = ARP_STATE_REMOVE_FAILED;
	sStackModule->set_timer(&timer, 0);
}


//	#pragma mark -


/*!	Updates the entry determined by \a protocolAddress with the specified
	\a hardwareAddress.
	If such an entry does not exist yet, a new entry is added. If you try
	to update a local existing entry but didn't ask for it (by setting
	\a flags to ARP_FLAG_LOCAL), an error is returned.

	This function does not lock the cache - you have to do it yourself
	before calling it.
*/
static status_t
arp_update_entry(in_addr_t protocolAddress, sockaddr_dl *hardwareAddress,
	uint32 flags, arp_entry **_entry = NULL)
{
	ASSERT_LOCKED_MUTEX(&sCacheLock);
	TRACE(("%s(%s, %s, flags 0x%" B_PRIx32 ")\n", __FUNCTION__,
		inet_to_string(protocolAddress), mac_to_string(LLADDR(hardwareAddress)),
		flags));

	arp_entry *entry = arp_entry::Lookup(protocolAddress);
	if (entry != NULL) {
		// We disallow updating of entries that had been resolved before,
		// but to a different address (only for those that belong to a
		// specific address - redefining INADDR_ANY is always allowed).
		// Right now, you have to manually purge the ARP entries (or wait some
		// time) to let us switch to the new address.
		if (protocolAddress != INADDR_ANY
			&& entry->hardware_address.sdl_alen != 0
			&& memcmp(LLADDR(&entry->hardware_address),
				LLADDR(hardwareAddress), ETHER_ADDRESS_LENGTH)) {
			uint8* data = LLADDR(hardwareAddress);
			dprintf("ARP host %08x updated with different hardware address "
				"%02x:%02x:%02x:%02x:%02x:%02x.\n", protocolAddress,
				data[0], data[1], data[2], data[3], data[4], data[5]);
			return B_ERROR;
		}

		entry->hardware_address = *hardwareAddress;
		entry->timestamp = system_time();
	} else {
		entry = arp_entry::Add(protocolAddress, hardwareAddress, flags);
		if (entry == NULL)
			return B_NO_MEMORY;
	}

	delete_request_buffer(entry);

	if ((entry->flags & ARP_FLAG_PERMANENT) == 0) {
		// (re)start the stale timer
		entry->timer_state = ARP_STATE_STALE;
		sStackModule->set_timer(&entry->timer, ARP_STALE_TIMEOUT);
	}

	if ((entry->flags & ARP_FLAG_REJECT) != 0)
		entry->MarkFailed();
	else
		entry->MarkValid();

	if (_entry)
		*_entry = entry;

	return B_OK;
}


static status_t
arp_set_local_entry(arp_protocol* protocol, const sockaddr* local)
{
	MutexLocker locker(sCacheLock);

	net_interface* interface = protocol->interface;
	in_addr_t inetAddress;

	if (local == NULL) {
		// interface has not yet been set
		inetAddress = INADDR_ANY;
	} else
		inetAddress = ((sockaddr_in*)local)->sin_addr.s_addr;

	TRACE(("%s(): address %s\n", __FUNCTION__, inet_to_string(inetAddress)));

	if (protocol->local_address == 0)
		protocol->local_address = inetAddress;

	sockaddr_dl address;
	address.sdl_len = sizeof(sockaddr_dl);
	address.sdl_family = AF_LINK;
	address.sdl_type = IFT_ETHER;
	address.sdl_e_type = htons(ETHER_TYPE_IP);
	address.sdl_nlen = 0;
	address.sdl_slen = 0;
	address.sdl_alen = interface->device->address.length;
	memcpy(LLADDR(&address), interface->device->address.data, address.sdl_alen);

	memcpy(&protocol->hardware_address, &address, sizeof(sockaddr_dl));
		// cache the address in our protocol

	arp_entry* entry;
	status_t status = arp_update_entry(inetAddress, &address,
		ARP_FLAG_LOCAL | ARP_FLAG_PERMANENT, &entry);
	if (status == B_OK)
		entry->protocol = protocol;

	return status;
}


static void
arp_remove_local_entry(arp_protocol* protocol, const sockaddr* local,
	net_interface_address* updateLocalAddress = NULL)
{
	in_addr_t inetAddress;

	if (local == NULL) {
		// interface has not yet been set
		inetAddress = INADDR_ANY;
	} else
		inetAddress = ((sockaddr_in*)local)->sin_addr.s_addr;

	TRACE(("%s(): address %s\n", __FUNCTION__, inet_to_string(inetAddress)));

	MutexLocker locker(sCacheLock);

	arp_entry* entry = arp_entry::Lookup(inetAddress);
	if (entry != NULL) {
		sCache->Remove(entry);
		entry->flags |= ARP_FLAG_REMOVED;
	}

	if (updateLocalAddress != NULL && protocol->local_address == inetAddress) {
		// find new local sender address
		protocol->local_address = 0;

		net_interface_address* address = NULL;
		while (sDatalinkModule->get_next_interface_address(protocol->interface,
				&address)) {
			if (address == updateLocalAddress || address->local == NULL
				|| address->local->sa_family != AF_INET)
				continue;

			protocol->local_address
				= ((sockaddr_in*)address->local)->sin_addr.s_addr;
		}
	}

	locker.Unlock();
	delete entry;

	if (protocol->local_address == 0 && updateLocalAddress) {
		// Try to keep the interface operational
		arp_set_local_entry(protocol, NULL);
	}
}


/*!	Removes all entries belonging to the local interface of the \a procotol
	given.
*/
static void
arp_remove_local(arp_protocol* protocol)
{
	net_interface_address* address = NULL;
	while (sDatalinkModule->get_next_interface_address(protocol->interface,
			&address)) {
		if (address->local == NULL || address->local->sa_family != AF_INET)
			continue;

		arp_remove_local_entry(protocol, address->local);
	}
}


/*!	Creates permanent local entries for all addresses of the interface belonging
	to this protocol.
	Returns an error if no entry could be added.
*/
static status_t
arp_update_local(arp_protocol* protocol)
{
	protocol->local_address = 0;
		// TODO: test if this actually works - maybe we should use
		// INADDR_BROADCAST instead

	ssize_t count = 0;

	net_interface_address* address = NULL;
	while (sDatalinkModule->get_next_interface_address(protocol->interface,
			&address)) {
		if (address->local == NULL || address->local->sa_family != AF_INET)
			continue;

		if (arp_set_local_entry(protocol, address->local) == B_OK) {
			count++;
		}
	}

	if (count == 0)
		return arp_set_local_entry(protocol, NULL);

	return B_OK;
}


static status_t
handle_arp_request(net_buffer *buffer, arp_header &header)
{
	MutexLocker locker(sCacheLock);

	if (!sIgnoreReplies) {
		arp_update_entry(header.protocol_sender,
			(sockaddr_dl *)buffer->source, 0);
			// remember the address of the sender as we might need it later
	}

	// check if this request is for us

	arp_entry *entry = arp_entry::Lookup(header.protocol_target);
	if (entry == NULL || entry->protocol == NULL
		|| (entry->flags & (ARP_FLAG_LOCAL | ARP_FLAG_PUBLISH)) == 0) {
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
	memcpy(header.hardware_sender, LLADDR(&entry->hardware_address),
		ETHER_ADDRESS_LENGTH);
	header.protocol_sender = entry->protocol_address;

	// exchange source and destination address
	memcpy(LLADDR((sockaddr_dl *)buffer->source), header.hardware_sender,
		ETHER_ADDRESS_LENGTH);
	memcpy(LLADDR((sockaddr_dl *)buffer->destination), header.hardware_target,
		ETHER_ADDRESS_LENGTH);

	buffer->flags = 0;
		// make sure this won't be a broadcast message

	gBufferModule->trim(buffer, sizeof(arp_header));
	return entry->protocol->next->module->send_data(entry->protocol->next,
		buffer);
}


static void
handle_arp_reply(net_buffer *buffer, arp_header &header)
{
	if (sIgnoreReplies)
		return;

	MutexLocker locker(sCacheLock);
	arp_update_entry(header.protocol_sender, (sockaddr_dl *)buffer->source, 0);
}


static status_t
arp_receive(void *cookie, net_device *device, net_buffer *buffer)
{
	TRACE(("ARP receive\n"));

	NetBufferHeaderReader<arp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	arp_header &header = bufferHeader.Data();
	uint16 opcode = ntohs(header.opcode);

#ifdef TRACE_ARP
	dprintf("  hw sender: %s\n", mac_to_string(header.hardware_sender));
	dprintf("  proto sender: %s\n", inet_to_string(header.protocol_sender));
	dprintf("  hw target: %s\n", mac_to_string(header.hardware_target));;
	dprintf("  proto target: %s\n", inet_to_string(header.protocol_target));
#endif	// TRACE_ARP

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
				// the function will take care of the buffer if everything
				// went well
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
			// Requesting the ARP entry failed, we keep it around for a while,
			// though, so that we won't try to request the same address again
			// too soon.
			TRACE(("  requesting ARP entry %p failed!\n", entry));
			entry->timer_state = ARP_STATE_REMOVE_FAILED;
			entry->MarkFailed();
			sStackModule->set_timer(&entry->timer, ARP_REJECT_TIMEOUT);
			break;

		case ARP_STATE_REMOVE_FAILED:
		case ARP_STATE_STALE:
		{
			// the entry has aged so much that we're going to remove it
			TRACE(("  remove ARP entry %p!\n", entry));

			MutexLocker locker(sCacheLock);
			if ((entry->flags & ARP_FLAG_REMOVED) != 0) {
				// The entry has already been removed, and is about to be
				// deleted
				break;
			}

			sCache->Remove(entry);
			locker.Unlock();

			delete entry;
			break;
		}

		default:
		{
			if (entry->timer_state > ARP_STATE_LAST_REQUEST
				|| entry->protocol == NULL)
				break;

			TRACE(("  send request for ARP entry %p!\n", entry));

			net_buffer *request = get_request_buffer(entry);
			if (request == NULL)
				break;

			if (entry->timer_state < ARP_STATE_LAST_REQUEST) {
				// we'll still need our buffer, so in order to prevent it being
				// freed by a successful send, we need to clone it
				net_buffer* clone = gBufferModule->clone(request, true);
				if (clone == NULL) {
					// cloning failed - that means we won't be able to send as
					// many requests as originally planned
					entry->timer_state = ARP_STATE_LAST_REQUEST;
				} else {
					put_request_buffer(entry, request);
					request = clone;
				}
			}

			// we're trying to resolve the address, so keep sending requests
			status_t status = entry->protocol->next->module->send_data(
				entry->protocol->next, request);
			if (status < B_OK)
				gBufferModule->free(request);

			entry->timer_state++;
			sStackModule->set_timer(&entry->timer, ARP_REQUEST_TIMEOUT);
			break;
		}
	}
}


/*!	Address resolver function: prepares and triggers the ARP request necessary
	to retrieve the hardware address for \a address.

	You need to have the sCacheLock held when calling this function.
*/
static status_t
arp_start_resolve(arp_protocol* protocol, in_addr_t address, arp_entry** _entry)
{
	ASSERT_LOCKED_MUTEX(&sCacheLock);

	// create an unresolved ARP entry as a placeholder
	arp_entry *entry = arp_entry::Add(address, NULL, 0);
	if (entry == NULL)
		return B_NO_MEMORY;

	// prepare ARP request

	entry->request_buffer = gBufferModule->create(256);
	if (entry->request_buffer == NULL) {
		entry->ScheduleRemoval();
		return B_NO_MEMORY;
	}

	NetBufferPrepend<arp_header> bufferHeader(entry->request_buffer);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		entry->ScheduleRemoval();
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
	memset(header.hardware_target, 0, ETHER_ADDRESS_LENGTH);
	header.protocol_sender = protocol->local_address;
	header.protocol_target = address;

	// prepare source and target addresses

	struct sockaddr_dl &source = *(struct sockaddr_dl *)
		entry->request_buffer->source;
	source.sdl_len = sizeof(sockaddr_dl);
	source.sdl_family = AF_LINK;
	source.sdl_index = device->index;
	source.sdl_type = IFT_ETHER;
	source.sdl_e_type = htons(ETHER_TYPE_ARP);
	source.sdl_nlen = source.sdl_slen = 0;
	source.sdl_alen = ETHER_ADDRESS_LENGTH;
	memcpy(source.sdl_data, device->address.data, ETHER_ADDRESS_LENGTH);

	entry->request_buffer->flags = MSG_BCAST;
		// this is a broadcast packet, we don't need to fill in the destination

	entry->protocol = protocol;
	entry->timer_state = ARP_STATE_REQUEST;
	sStackModule->set_timer(&entry->timer, 0);
		// start request timer

	*_entry = entry;
	return B_OK;
}


static status_t
arp_control(const char *subsystem, uint32 function, void *buffer,
	size_t bufferSize)
{
	struct arp_control control;
	if (bufferSize != sizeof(struct arp_control))
		return B_BAD_VALUE;
	if (user_memcpy(&control, buffer, sizeof(struct arp_control)) < B_OK)
		return B_BAD_ADDRESS;

	MutexLocker locker(sCacheLock);

	switch (function) {
		case ARP_SET_ENTRY:
		{
			sockaddr_dl hardwareAddress;

			hardwareAddress.sdl_len = sizeof(sockaddr_dl);
			hardwareAddress.sdl_family = AF_LINK;
			hardwareAddress.sdl_index = 0;
			hardwareAddress.sdl_type = IFT_ETHER;
			hardwareAddress.sdl_e_type = htons(ETHER_TYPE_IP);
			hardwareAddress.sdl_nlen = hardwareAddress.sdl_slen = 0;
			hardwareAddress.sdl_alen = ETHER_ADDRESS_LENGTH;
			memcpy(hardwareAddress.sdl_data, control.ethernet_address,
				ETHER_ADDRESS_LENGTH);

			return arp_update_entry(control.address, &hardwareAddress,
				control.flags & (ARP_FLAG_PUBLISH | ARP_FLAG_PERMANENT
					| ARP_FLAG_REJECT));
		}

		case ARP_GET_ENTRY:
		{
			arp_entry *entry = arp_entry::Lookup(control.address);
			if (entry == NULL || !(entry->flags & ARP_FLAG_VALID))
				return B_ENTRY_NOT_FOUND;

			if (entry->hardware_address.sdl_alen == ETHER_ADDRESS_LENGTH) {
				memcpy(control.ethernet_address,
					entry->hardware_address.sdl_data, ETHER_ADDRESS_LENGTH);
			} else
				memset(control.ethernet_address, 0, ETHER_ADDRESS_LENGTH);

			control.flags = entry->flags & ARP_PUBLIC_FLAG_MASK;
			return user_memcpy(buffer, &control, sizeof(struct arp_control));
		}

		case ARP_GET_ENTRIES:
		{
			AddressCache::Iterator iterator(sCache);

			arp_entry *entry = NULL;
			for (uint32 i = 0; i <= control.cookie; i++) {
				if (!iterator.HasNext())
					return B_ENTRY_NOT_FOUND;
				entry = iterator.Next();
			}

			control.cookie++;
			control.address = entry->protocol_address;
			if (entry->hardware_address.sdl_alen == ETHER_ADDRESS_LENGTH) {
				memcpy(control.ethernet_address,
					entry->hardware_address.sdl_data, ETHER_ADDRESS_LENGTH);
			} else
				memset(control.ethernet_address, 0, ETHER_ADDRESS_LENGTH);
			control.flags = entry->flags & ARP_PUBLIC_FLAG_MASK;

			return user_memcpy(buffer, &control, sizeof(struct arp_control));
		}

		case ARP_DELETE_ENTRY:
		{
			arp_entry *entry = arp_entry::Lookup(control.address);
			if (entry == NULL)
				return B_ENTRY_NOT_FOUND;
			if ((entry->flags & ARP_FLAG_LOCAL) != 0)
				return B_BAD_VALUE;

			entry->ScheduleRemoval();
			return B_OK;
		}

		case ARP_FLUSH_ENTRIES:
		{
			AddressCache::Iterator iterator(sCache);

			arp_entry *entry;
			while (iterator.HasNext()) {
				entry = iterator.Next();
				// we never flush local ARP entries
				if ((entry->flags & ARP_FLAG_LOCAL) != 0)
					continue;

				entry->ScheduleRemoval();
			}
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
	mutex_init(&sCacheLock, "arp cache");

	sCache = new(std::nothrow) AddressCache();
	if (sCache == NULL || sCache->Init(64) != B_OK) {
		mutex_destroy(&sCacheLock);
		return B_NO_MEMORY;
	}

	register_generic_syscall(ARP_SYSCALLS, arp_control, 1, 0);
	return B_OK;
}


static status_t
arp_uninit()
{
	unregister_generic_syscall(ARP_SYSCALLS, 1);
	return B_OK;
}


//	#pragma mark - net_datalink_protocol


status_t
arp_init_protocol(net_interface* interface, net_domain* domain,
	net_datalink_protocol** _protocol)
{
	// We currently only support a single family and type!
	if (interface->device->type != IFT_ETHER
		|| domain->family != AF_INET)
		return B_BAD_TYPE;

	status_t status = sStackModule->register_device_handler(interface->device,
		B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_ARP), &arp_receive, NULL);
	if (status != B_OK)
		return status;

	status = sStackModule->register_domain_device_handler(
		interface->device, B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_IP), domain);
	if (status != B_OK)
		return status;

	arp_protocol* protocol = new(std::nothrow) arp_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	memset(&protocol->hardware_address, 0, sizeof(sockaddr_dl));
	protocol->local_address = 0;

	*_protocol = protocol;
	return B_OK;
}


status_t
arp_uninit_protocol(net_datalink_protocol *protocol)
{
	sStackModule->unregister_device_handler(protocol->interface->device,
		B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_ARP));
	sStackModule->unregister_device_handler(protocol->interface->device,
		B_NET_FRAME_TYPE(IFT_ETHER, ETHER_TYPE_IP));

	delete protocol;
	return B_OK;
}


status_t
arp_send_data(net_datalink_protocol *_protocol, net_buffer *buffer)
{
	arp_protocol *protocol = (arp_protocol *)_protocol;
	{
		MutexLocker locker(sCacheLock);

		// Set buffer target and destination address

		memcpy(buffer->source, &protocol->hardware_address,
			protocol->hardware_address.sdl_len);

		if ((buffer->flags & MSG_MCAST) != 0) {
			sockaddr_dl multicastDestination;
			ipv4_to_ether_multicast(&multicastDestination,
				(sockaddr_in *)buffer->destination);
			memcpy(buffer->destination, &multicastDestination,
				sizeof(multicastDestination));
		} else if ((buffer->flags & MSG_BCAST) == 0) {
			// Lookup destination (we may need to wait for this)
			arp_entry *entry = arp_entry::Lookup(
				((struct sockaddr_in *)buffer->destination)->sin_addr.s_addr);
			if (entry == NULL) {
				status_t status = arp_start_resolve(protocol,
					((struct sockaddr_in*)buffer->destination)->sin_addr.s_addr,
					&entry);
				if (status != B_OK)
					return status;
			}

			if ((entry->flags & ARP_FLAG_REJECT) != 0)
				return EHOSTUNREACH;

			if ((entry->flags & ARP_FLAG_VALID) == 0) {
				// entry is still being resolved.
				TRACE(("ARP Queuing packet %p, entry still being resolved.\n",
					buffer));
				entry->queue.Add(buffer);
				return B_OK;
			}

			memcpy(buffer->destination, &entry->hardware_address,
				entry->hardware_address.sdl_len);
		}
		// the broadcast address is set in the ethernet frame module
	}
	TRACE(("%s(%p): from %s\n", __FUNCTION__, buffer,
		mac_to_string(LLADDR((sockaddr_dl*)buffer->source))));
	TRACE(("  to %s\n",
		mac_to_string(LLADDR((sockaddr_dl*)buffer->destination))));

	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
arp_up(net_datalink_protocol* _protocol)
{
	arp_protocol* protocol = (arp_protocol*)_protocol;
	status_t status = protocol->next->module->interface_up(protocol->next);
	if (status != B_OK)
		return status;

	// cache this device's address for later use

	status = arp_update_local(protocol);
	if (status != B_OK) {
		protocol->next->module->interface_down(protocol->next);
		return status;
	}

	return B_OK;
}


void
arp_down(net_datalink_protocol *protocol)
{
	// remove local ARP entries from the cache
	arp_remove_local((arp_protocol*)protocol);

	protocol->next->module->interface_down(protocol->next);
}


status_t
arp_change_address(net_datalink_protocol* _protocol,
	net_interface_address* address, int32 option,
	const struct sockaddr* oldAddress, const struct sockaddr* newAddress)
{
	arp_protocol* protocol = (arp_protocol*)_protocol;
	TRACE(("%s(option %" B_PRId32 ")\n", __FUNCTION__, option));

	switch (option) {
		case SIOCSIFADDR:
		case SIOCAIFADDR:
		case SIOCDIFADDR:
			// Those are the options we handle
			if ((protocol->interface->flags & IFF_UP) != 0) {
				// Update ARP entry for the local address

				if (newAddress != NULL && newAddress->sa_family == AF_INET) {
					status_t status = arp_set_local_entry(protocol, newAddress);
					if (status != B_OK)
						return status;
				}

				if (option != SIOCAIFADDR
					&& (oldAddress == NULL || oldAddress->sa_family == AF_INET))
					arp_remove_local_entry(protocol, oldAddress, address);
			}
			break;

		default:
			break;
	}

	return protocol->next->module->change_address(protocol->next, address,
		option, oldAddress, newAddress);
}


status_t
arp_control(net_datalink_protocol *_protocol, int32 op, void *argument,
	size_t length)
{
	arp_protocol* protocol = (arp_protocol*)_protocol;
	return protocol->next->module->control(protocol->next, op, argument,
		length);
}


static status_t
arp_join_multicast(net_datalink_protocol *protocol, const sockaddr *address)
{
	if (address->sa_family != AF_INET)
		return EINVAL;

	sockaddr_dl multicastAddress;
	ipv4_to_ether_multicast(&multicastAddress, (const sockaddr_in *)address);

	return protocol->next->module->join_multicast(protocol->next,
		(sockaddr *)&multicastAddress);
}


static status_t
arp_leave_multicast(net_datalink_protocol *protocol, const sockaddr *address)
{
	if (address->sa_family != AF_INET)
		return EINVAL;

	sockaddr_dl multicastAddress;
	ipv4_to_ether_multicast(&multicastAddress, (const sockaddr_in *)address);

	return protocol->next->module->leave_multicast(protocol->next,
		(sockaddr *)&multicastAddress);
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
	arp_change_address,
	arp_control,
	arp_join_multicast,
	arp_leave_multicast,
};


module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&sStackModule},
	{NET_DATALINK_MODULE_NAME, (module_info**)&sDatalinkModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{}
};

module_info* modules[] = {
	(module_info*)&sARPModule,
	NULL
};
