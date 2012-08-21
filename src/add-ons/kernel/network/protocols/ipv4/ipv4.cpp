/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ipv4.h"
#include "ipv4_address.h"
#include "multicast.h"

#include <net_datalink.h>
#include <net_datalink_protocol.h>
#include <net_device.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>
#include <ProtocolUtilities.h>

#include <KernelExport.h>
#include <util/AutoLock.h>
#include <util/list.h>
#include <util/DoublyLinkedList.h>
#include <util/MultiHashTable.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <utility>


//#define TRACE_IPV4
#ifdef TRACE_IPV4
#	define TRACE(format, args...) \
		dprintf("IPv4 [%llu] " format "\n", system_time() , ##args)
#	define TRACE_SK(protocol, format, args...) \
		dprintf("IPv4 [%llu] %p " format "\n", system_time(), \
			protocol , ##args)
#	define TRACE_ONLY(x) x
#else
#	define TRACE(args...) ;
#	define TRACE_SK(args...) ;
#	define TRACE_ONLY(x)
#endif


#define MAX_HASH_FRAGMENTS 		64
	// slots in the fragment packet's hash
#define FRAGMENT_TIMEOUT		60000000LL
	// discard fragment after 60 seconds


typedef DoublyLinkedList<struct net_buffer,
	DoublyLinkedListCLink<struct net_buffer> > FragmentList;

typedef NetBufferField<uint16, offsetof(ipv4_header, checksum)> IPChecksumField;

struct ipv4_packet_key {
	in_addr_t	source;
	in_addr_t	destination;
	uint16		id;
	uint8		protocol;
};


class FragmentPacket {
public:
								FragmentPacket(const ipv4_packet_key& key);
								~FragmentPacket();

			status_t			AddFragment(uint16 start, uint16 end,
									net_buffer* buffer, bool lastFragment);
			status_t			Reassemble(net_buffer* to);

			bool				IsComplete() const
									{ return fReceivedLastFragment
										&& fBytesLeft == 0; }

			const ipv4_packet_key& Key() const { return fKey; }
			FragmentPacket*&	HashTableLink() { return fNext; }

	static	void				StaleTimer(struct net_timer* timer, void* data);

private:
			FragmentPacket*		fNext;
			struct ipv4_packet_key fKey;
			uint32				fIndex;
			bool				fReceivedLastFragment;
			int32				fBytesLeft;
			FragmentList		fFragments;
			net_timer			fTimer;
};


struct FragmentHashDefinition {
	typedef ipv4_packet_key KeyType;
	typedef FragmentPacket ValueType;

	size_t HashKey(const KeyType& key) const
	{
		return (key.source ^ key.destination ^ key.protocol ^ key.id);
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(value->Key());
	}

	bool Compare(const KeyType& key, ValueType* value) const
	{
		const ipv4_packet_key& packetKey = value->Key();

		return packetKey.id == key.id
			&& packetKey.source == key.source
			&& packetKey.destination == key.destination
			&& packetKey.protocol == key.protocol;
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->HashTableLink();
	}
};

typedef BOpenHashTable<FragmentHashDefinition, false, true> FragmentTable;


class RawSocket
	: public DoublyLinkedListLinkImpl<RawSocket>, public DatagramSocket<> {
public:
								RawSocket(net_socket* socket);
};

typedef DoublyLinkedList<RawSocket> RawSocketList;

typedef MulticastGroupInterface<IPv4Multicast> IPv4GroupInterface;
typedef MulticastFilter<IPv4Multicast> IPv4MulticastFilter;

struct MulticastStateHash {
	typedef std::pair<const in_addr* , uint32> KeyType;
	typedef IPv4GroupInterface ValueType;

	size_t HashKey(const KeyType &key) const
		{ return key.first->s_addr ^ key.second; }
	size_t Hash(ValueType* value) const
		{ return HashKey(std::make_pair(&value->Address(),
			value->Interface()->index)); }
	bool Compare(const KeyType &key, ValueType* value) const
		{ return value->Interface()->index == key.second
			&& value->Address().s_addr == key.first->s_addr; }
	bool CompareValues(ValueType* value1, ValueType* value2) const
		{ return value1->Interface()->index == value2->Interface()->index
			&& value1->Address().s_addr == value2->Address().s_addr; }
	ValueType*& GetLink(ValueType* value) const { return value->HashLink(); }
};


struct ipv4_protocol : net_protocol {
	ipv4_protocol()
		:
		raw(NULL),
		multicast_filter(this)
	{
	}

	~ipv4_protocol()
	{
		delete raw;
	}

	RawSocket*			raw;
	uint8				service_type;
	uint8				time_to_live;
	uint8				multicast_time_to_live;
	uint32				flags;
	struct sockaddr*	multicast_address; // for IP_MULTICAST_IF

	IPv4MulticastFilter	multicast_filter;
};

// protocol flags
#define IP_FLAG_HEADER_INCLUDED		0x01
#define IP_FLAG_RECEIVE_DEST_ADDR	0x02


static const int kDefaultTTL = 254;
static const int kDefaultMulticastTTL = 1;


extern net_protocol_module_info gIPv4Module;
	// we need this in ipv4_std_ops() for registering the AF_INET domain

net_stack_module_info* gStackModule;
net_buffer_module_info* gBufferModule;

static struct net_domain* sDomain;
static net_datalink_module_info* sDatalinkModule;
static net_socket_module_info* sSocketModule;
static int32 sPacketID;
static RawSocketList sRawSockets;
static mutex sRawSocketsLock;
static mutex sFragmentLock;
static FragmentTable sFragmentHash;
static mutex sMulticastGroupsLock;

typedef MultiHashTable<MulticastStateHash> MulticastState;
static MulticastState* sMulticastState;

static net_protocol_module_info* sReceivingProtocol[256];
static mutex sReceivingProtocolLock;


static const char*
print_address(const in_addr* address, char* buf, size_t bufLen)
{
	unsigned int addr = ntohl(address->s_addr);

	snprintf(buf, bufLen, "%u.%u.%u.%u", (addr >> 24) & 0xff,
		(addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);

	return buf;
}


RawSocket::RawSocket(net_socket* socket)
	:
	DatagramSocket<>("ipv4 raw socket", socket)
{
}


//	#pragma mark -


FragmentPacket::FragmentPacket(const ipv4_packet_key& key)
	:
	fKey(key),
	fIndex(0),
	fReceivedLastFragment(false),
	fBytesLeft(IP_MAXPACKET)
{
	gStackModule->init_timer(&fTimer, FragmentPacket::StaleTimer, this);
}


FragmentPacket::~FragmentPacket()
{
	// cancel the kill timer
	gStackModule->set_timer(&fTimer, -1);

	// delete all fragments
	net_buffer* buffer;
	while ((buffer = fFragments.RemoveHead()) != NULL) {
		gBufferModule->free(buffer);
	}
}


status_t
FragmentPacket::AddFragment(uint16 start, uint16 end, net_buffer* buffer,
	bool lastFragment)
{
	// restart the timer
	gStackModule->set_timer(&fTimer, FRAGMENT_TIMEOUT);

	if (start >= end) {
		// invalid fragment
		return B_BAD_DATA;
	}

	// Search for a position in the list to insert the fragment

	FragmentList::ReverseIterator iterator = fFragments.GetReverseIterator();
	net_buffer* previous = NULL;
	net_buffer* next = NULL;
	while ((previous = iterator.Next()) != NULL) {
		if (previous->fragment.start <= start) {
			// The new fragment can be inserted after this one
			break;
		}

		next = previous;
	}

	// See if we already have the fragment's data

	if (previous != NULL && previous->fragment.start <= start
		&& previous->fragment.end >= end) {
		// we do, so we can just drop this fragment
		gBufferModule->free(buffer);
		return B_OK;
	}

	fIndex = buffer->index;
		// adopt the buffer's device index

	TRACE("    previous: %p, next: %p", previous, next);

	// If we have parts of the data already, truncate as needed

	if (previous != NULL && previous->fragment.end > start) {
		TRACE("    remove header %d bytes", previous->fragment.end - start);
		gBufferModule->remove_header(buffer, previous->fragment.end - start);
		start = previous->fragment.end;
	}
	if (next != NULL && next->fragment.start < end) {
		TRACE("    remove trailer %d bytes", next->fragment.start - end);
		gBufferModule->remove_trailer(buffer, next->fragment.start - end);
		end = next->fragment.start;
	}

	// Now try if we can already merge the fragments together

	// We will always keep the last buffer received, so that we can still
	// report an error (in which case we're not responsible for freeing it)

	if (previous != NULL && previous->fragment.end == start) {
		fFragments.Remove(previous);

		buffer->fragment.start = previous->fragment.start;
		buffer->fragment.end = end;

		status_t status = gBufferModule->merge(buffer, previous, false);
		TRACE("    merge previous: %s", strerror(status));
		if (status != B_OK) {
			fFragments.Insert(next, previous);
			return status;
		}

		fFragments.Insert(next, buffer);

		// cut down existing hole
		fBytesLeft -= end - start;

		if (lastFragment && !fReceivedLastFragment) {
			fReceivedLastFragment = true;
			fBytesLeft -= IP_MAXPACKET - end;
		}

		TRACE("    hole length: %d", (int)fBytesLeft);

		return B_OK;
	} else if (next != NULL && next->fragment.start == end) {
		net_buffer* afterNext = (net_buffer*)next->link.next;
		fFragments.Remove(next);

		buffer->fragment.start = start;
		buffer->fragment.end = next->fragment.end;

		status_t status = gBufferModule->merge(buffer, next, true);
		TRACE("    merge next: %s", strerror(status));
		if (status != B_OK) {
			// Insert "next" at its previous position
			fFragments.Insert(afterNext, next);
			return status;
		}

		fFragments.Insert(afterNext, buffer);

		// cut down existing hole
		fBytesLeft -= end - start;

		if (lastFragment && !fReceivedLastFragment) {
			fReceivedLastFragment = true;
			fBytesLeft -= IP_MAXPACKET - end;
		}

		TRACE("    hole length: %d", (int)fBytesLeft);

		return B_OK;
	}

	// We couldn't merge the fragments, so we need to add it as is

	TRACE("    new fragment: %p, bytes %d-%d", buffer, start, end);

	buffer->fragment.start = start;
	buffer->fragment.end = end;
	fFragments.Insert(next, buffer);

	// update length of the hole, if any
	fBytesLeft -= end - start;

	if (lastFragment && !fReceivedLastFragment) {
		fReceivedLastFragment = true;
		fBytesLeft -= IP_MAXPACKET - end;
	}

	TRACE("    hole length: %d", (int)fBytesLeft);

	return B_OK;
}


/*!	Reassembles the fragments to the specified buffer \a to.
	This buffer must have been added via AddFragment() before.
*/
status_t
FragmentPacket::Reassemble(net_buffer* to)
{
	if (!IsComplete())
		return B_ERROR;

	net_buffer* buffer = NULL;

	net_buffer* fragment;
	while ((fragment = fFragments.RemoveHead()) != NULL) {
		if (buffer != NULL) {
			status_t status;
			if (to == fragment) {
				status = gBufferModule->merge(fragment, buffer, false);
				buffer = fragment;
			} else
				status = gBufferModule->merge(buffer, fragment, true);
			if (status != B_OK)
				return status;
		} else
			buffer = fragment;
	}

	if (buffer != to)
		panic("ipv4 packet reassembly did not work correctly.");

	to->index = fIndex;
		// reset the buffer's device index

	return B_OK;
}


/*static*/ void
FragmentPacket::StaleTimer(struct net_timer* timer, void* data)
{
	FragmentPacket* packet = (FragmentPacket*)data;
	TRACE("Assembling FragmentPacket %p timed out!", packet);

	MutexLocker locker(&sFragmentLock);
	sFragmentHash.Remove(packet);
	locker.Unlock();

	if (!packet->fFragments.IsEmpty()) {
		// Send error: fragment reassembly time exceeded
		sDomain->module->error_reply(NULL, packet->fFragments.First(),
			B_NET_ERROR_REASSEMBLY_TIME_EXCEEDED, NULL);
	}

	delete packet;
}


//	#pragma mark -


#ifdef TRACE_IPV4
static void
dump_ipv4_header(ipv4_header &header)
{
	struct pretty_ipv4 {
	#if B_HOST_IS_LENDIAN == 1
		uint8 a;
		uint8 b;
		uint8 c;
		uint8 d;
	#else
		uint8 d;
		uint8 c;
		uint8 b;
		uint8 a;
	#endif
	};
	struct pretty_ipv4* src = (struct pretty_ipv4*)&header.source;
	struct pretty_ipv4* dst = (struct pretty_ipv4*)&header.destination;
	dprintf("  version: %d\n", header.version);
	dprintf("  header_length: 4 * %d\n", header.header_length);
	dprintf("  service_type: %d\n", header.service_type);
	dprintf("  total_length: %d\n", header.TotalLength());
	dprintf("  id: %d\n", ntohs(header.id));
	dprintf("  fragment_offset: %d (flags: %c%c%c)\n",
		header.FragmentOffset() & IP_FRAGMENT_OFFSET_MASK,
		(header.FragmentOffset() & IP_RESERVED_FLAG) ? 'r' : '-',
		(header.FragmentOffset() & IP_DONT_FRAGMENT) ? 'd' : '-',
		(header.FragmentOffset() & IP_MORE_FRAGMENTS) ? 'm' : '-');
	dprintf("  time_to_live: %d\n", header.time_to_live);
	dprintf("  protocol: %d\n", header.protocol);
	dprintf("  checksum: %d\n", ntohs(header.checksum));
	dprintf("  source: %d.%d.%d.%d\n", src->a, src->b, src->c, src->d);
	dprintf("  destination: %d.%d.%d.%d\n", dst->a, dst->b, dst->c, dst->d);
}
#endif	// TRACE_IPV4


static int
dump_ipv4_multicast(int argc, char** argv)
{
	MulticastState::Iterator it = sMulticastState->GetIterator();

	while (it.HasNext()) {
		IPv4GroupInterface* state = it.Next();

		char addressBuffer[64];

		kprintf("%p: group <%s, %s, %s {", state, state->Interface()->name,
			print_address(&state->Address(), addressBuffer,
			sizeof(addressBuffer)),
			state->Mode() == IPv4GroupInterface::kExclude
				? "Exclude" : "Include");

		int count = 0;
		IPv4GroupInterface::AddressSet::Iterator it
			= state->Sources().GetIterator();
		while (it.HasNext()) {
			kprintf("%s%s", count > 0 ? ", " : "", print_address(&it.Next(),
				addressBuffer, sizeof(addressBuffer)));
			count++;
		}

		kprintf("}> sock %p\n", state->Parent()->Socket());
	}

	return 0;
}


/*!	Attempts to re-assemble fragmented packets.
	\return B_OK if everything went well; if it could reassemble the packet, \a _buffer
		will point to its buffer, otherwise, it will be \c NULL.
	\return various error codes if something went wrong (mostly B_NO_MEMORY)
*/
static status_t
reassemble_fragments(const ipv4_header &header, net_buffer** _buffer)
{
	net_buffer* buffer = *_buffer;
	status_t status;

	struct ipv4_packet_key key;
	key.source = (in_addr_t)header.source;
	key.destination = (in_addr_t)header.destination;
	key.id = header.id;
	key.protocol = header.protocol;

	// TODO: Make locking finer grained.
	MutexLocker locker(&sFragmentLock);

	FragmentPacket* packet = sFragmentHash.Lookup(key);
	if (packet == NULL) {
		// New fragment packet
		packet = new (std::nothrow) FragmentPacket(key);
		if (packet == NULL)
			return B_NO_MEMORY;

		// add packet to hash
		status = sFragmentHash.Insert(packet);
		if (status != B_OK) {
			delete packet;
			return status;
		}
	}

	uint16 fragmentOffset = header.FragmentOffset();
	uint16 start = (fragmentOffset & IP_FRAGMENT_OFFSET_MASK) << 3;
	uint16 end = start + header.TotalLength() - header.HeaderLength();
	bool lastFragment = (fragmentOffset & IP_MORE_FRAGMENTS) == 0;

	TRACE("   Received IPv4 %sfragment of size %d, offset %d.",
		lastFragment ? "last ": "", end - start, start);

	// Remove header unless this is the first fragment
	if (start != 0)
		gBufferModule->remove_header(buffer, header.HeaderLength());

	status = packet->AddFragment(start, end, buffer, lastFragment);
	if (status != B_OK)
		return status;

	if (packet->IsComplete()) {
		sFragmentHash.Remove(packet);
			// no matter if reassembling succeeds, we won't need this packet
			// anymore

		status = packet->Reassemble(buffer);
		delete packet;

		// _buffer does not change
		return status;
	}

	// This indicates that the packet is not yet complete
	*_buffer = NULL;
	return B_OK;
}


/*!	Fragments the incoming buffer and send all fragments via the specified
	\a route.
*/
static status_t
send_fragments(ipv4_protocol* protocol, struct net_route* route,
	net_buffer* buffer, uint32 mtu)
{
	TRACE_SK(protocol, "SendFragments(%lu bytes, mtu %lu)", buffer->size, mtu);

	NetBufferHeaderReader<ipv4_header> originalHeader(buffer);
	if (originalHeader.Status() != B_OK)
		return originalHeader.Status();

	uint16 headerLength = originalHeader->HeaderLength();
	uint32 bytesLeft = buffer->size - headerLength;
	uint32 fragmentOffset = 0;
	status_t status = B_OK;

	net_buffer* headerBuffer = gBufferModule->split(buffer, headerLength);
	if (headerBuffer == NULL)
		return B_NO_MEMORY;

	// TODO: we need to make sure ipv4_header is contiguous or
	// use another construct.
	NetBufferHeaderReader<ipv4_header> bufferHeader(headerBuffer);
	ipv4_header* header = &bufferHeader.Data();

	// Adapt MTU to be a multiple of 8 (fragment offsets can only be specified
	// this way)
	mtu -= headerLength;
	mtu &= ~7;
	TRACE("  adjusted MTU to %ld, bytesLeft %ld", mtu, bytesLeft);

	while (bytesLeft > 0) {
		uint32 fragmentLength = min_c(bytesLeft, mtu);
		bytesLeft -= fragmentLength;
		bool lastFragment = bytesLeft == 0;

		header->total_length = htons(fragmentLength + headerLength);
		header->fragment_offset = htons((lastFragment ? 0 : IP_MORE_FRAGMENTS)
			| (fragmentOffset >> 3));
		header->checksum = 0;
		header->checksum = gStackModule->checksum((uint8*)header,
			headerLength);
			// TODO: compute the checksum only for those parts that changed?

		TRACE("  send fragment of %ld bytes (%ld bytes left)", fragmentLength,
			bytesLeft);

		net_buffer* fragmentBuffer;
		if (!lastFragment) {
			fragmentBuffer = gBufferModule->split(buffer, fragmentLength);
			fragmentOffset += fragmentLength;
		} else
			fragmentBuffer = buffer;

		if (fragmentBuffer == NULL) {
			status = B_NO_MEMORY;
			break;
		}

		// copy header to fragment
		status = gBufferModule->prepend(fragmentBuffer, header, headerLength);

		// send fragment
		if (status == B_OK)
			status = sDatalinkModule->send_routed_data(route, fragmentBuffer);

		if (lastFragment) {
			// we don't own the last buffer, so we don't have to free it
			break;
		}

		if (status != B_OK) {
			gBufferModule->free(fragmentBuffer);
			break;
		}
	}

	gBufferModule->free(headerBuffer);
	return status;
}


/*!	Delivers the provided \a buffer to all listeners of this multicast group.
	Does not take over ownership of the buffer.
*/
static bool
deliver_multicast(net_protocol_module_info* module, net_buffer* buffer,
	bool deliverToRaw)
{
	if (module->deliver_data == NULL)
		return false;

	// TODO: fix multicast!
	return false;
	MutexLocker _(sMulticastGroupsLock);

	sockaddr_in* multicastAddr = (sockaddr_in*)buffer->destination;

	MulticastState::ValueIterator it = sMulticastState->Lookup(std::make_pair(
		&multicastAddr->sin_addr, buffer->interface_address->interface->index));

	size_t count = 0;

	while (it.HasNext()) {
		IPv4GroupInterface* state = it.Next();

		ipv4_protocol* ipProtocol = state->Parent()->Socket();
		if (deliverToRaw && (ipProtocol->raw == NULL
				|| ipProtocol->socket->protocol != buffer->protocol))
			continue;

		if (state->FilterAccepts(buffer)) {
			net_protocol* protocol = ipProtocol;
			if (protocol->module != module) {
				// as multicast filters are installed with an IPv4 protocol
				// reference, we need to go and find the appropriate instance
				// related to the 'receiving protocol' with module 'module'.
				net_protocol* protocol = ipProtocol->socket->first_protocol;

				while (protocol != NULL && protocol->module != module)
					protocol = protocol->next;
			}

			if (protocol != NULL) {
				module->deliver_data(protocol, buffer);
				count++;
			}
		}
	}

	return count > 0;
}


/*!	Delivers the buffer to all listening raw sockets without taking ownership of
	the provided \a buffer.
	Returns \c true if there was any receiver, \c false if not.
*/
static bool
raw_receive_data(net_buffer* buffer)
{
	MutexLocker locker(sRawSocketsLock);

	if (sRawSockets.IsEmpty())
		return false;

	TRACE("RawReceiveData(%i)", buffer->protocol);

	if ((buffer->flags & MSG_MCAST) != 0) {
		// we need to call deliver_multicast here separately as
		// buffer still has the IP header, and it won't in the
		// next call. This isn't very optimized but works for now.
		// A better solution would be to hold separate hash tables
		// and lists for RAW and non-RAW sockets.
		return deliver_multicast(&gIPv4Module, buffer, true);
	}

	RawSocketList::Iterator iterator = sRawSockets.GetIterator();
	size_t count = 0;

	while (iterator.HasNext()) {
		RawSocket* raw = iterator.Next();

		if (raw->Socket()->protocol == buffer->protocol) {
			raw->EnqueueClone(buffer);
			count++;
		}
	}

	return count > 0;
}


static inline sockaddr*
fill_sockaddr_in(sockaddr_in* target, in_addr_t address)
{
	target->sin_family = AF_INET;
	target->sin_len = sizeof(sockaddr_in);
	target->sin_port = 0;
	target->sin_addr.s_addr = address;
	return (sockaddr*)target;
}


static status_t
get_int_option(void* target, size_t length, int value)
{
	if (length != sizeof(int))
		return B_BAD_VALUE;

	return user_memcpy(target, &value, sizeof(int));
}


template<typename Type> static status_t
set_int_option(Type &target, const void* _value, size_t length)
{
	int value;

	if (length != sizeof(int))
		return B_BAD_VALUE;

	if (user_memcpy(&value, _value, sizeof(int)) != B_OK)
		return B_BAD_ADDRESS;

	target = value;
	return B_OK;
}


static net_protocol_module_info*
receiving_protocol(uint8 protocol)
{
	net_protocol_module_info* module = sReceivingProtocol[protocol];
	if (module != NULL)
		return module;

	MutexLocker locker(sReceivingProtocolLock);

	module = sReceivingProtocol[protocol];
	if (module != NULL)
		return module;

	if (gStackModule->get_domain_receiving_protocol(sDomain, protocol,
			&module) == B_OK)
		sReceivingProtocol[protocol] = module;

	return module;
}


// #pragma mark - multicast


status_t
IPv4Multicast::JoinGroup(IPv4GroupInterface* state)
{
	MutexLocker _(sMulticastGroupsLock);

	sockaddr_in groupAddr;
	status_t status = sDatalinkModule->join_multicast(state->Interface(),
		sDomain, fill_sockaddr_in(&groupAddr, state->Address().s_addr));
	if (status != B_OK)
		return status;

	sMulticastState->Insert(state);
	return B_OK;
}


status_t
IPv4Multicast::LeaveGroup(IPv4GroupInterface* state)
{
	MutexLocker _(sMulticastGroupsLock);

	sMulticastState->Remove(state);

	sockaddr_in groupAddr;
	return sDatalinkModule->leave_multicast(state->Interface(), sDomain,
		fill_sockaddr_in(&groupAddr, state->Address().s_addr));
}


static status_t
ipv4_delta_group(IPv4GroupInterface* group, int option,
	net_interface* interface, const in_addr* sourceAddr)
{
	switch (option) {
		case IP_ADD_MEMBERSHIP:
			return group->Add();
		case IP_DROP_MEMBERSHIP:
			return group->Drop();
		case IP_BLOCK_SOURCE:
			return group->BlockSource(*sourceAddr);
		case IP_UNBLOCK_SOURCE:
			return group->UnblockSource(*sourceAddr);
		case IP_ADD_SOURCE_MEMBERSHIP:
			return group->AddSSM(*sourceAddr);
		case IP_DROP_SOURCE_MEMBERSHIP:
			return group->DropSSM(*sourceAddr);
	}

	return B_ERROR;
}


static status_t
ipv4_delta_membership(ipv4_protocol* protocol, int option,
	net_interface* interface, const in_addr* groupAddr,
	const in_addr* sourceAddr)
{
	IPv4MulticastFilter& filter = protocol->multicast_filter;
	IPv4GroupInterface* state = NULL;
	status_t status = B_OK;

	switch (option) {
		case IP_ADD_MEMBERSHIP:
		case IP_ADD_SOURCE_MEMBERSHIP:
			status = filter.GetState(*groupAddr, interface, state, true);
			break;

		case IP_DROP_MEMBERSHIP:
		case IP_BLOCK_SOURCE:
		case IP_UNBLOCK_SOURCE:
		case IP_DROP_SOURCE_MEMBERSHIP:
			filter.GetState(*groupAddr, interface, state, false);
			if (state == NULL) {
				if (option == IP_DROP_MEMBERSHIP
					|| option == IP_DROP_SOURCE_MEMBERSHIP)
					return EADDRNOTAVAIL;

				return B_BAD_VALUE;
			}
			break;
	}

	if (status != B_OK)
		return status;

	status = ipv4_delta_group(state, option, interface, sourceAddr);
	filter.ReturnState(state);
	return status;
}


static int
generic_to_ipv4(int option)
{
	switch (option) {
		case MCAST_JOIN_GROUP:
			return IP_ADD_MEMBERSHIP;
		case MCAST_JOIN_SOURCE_GROUP:
			return IP_ADD_SOURCE_MEMBERSHIP;
		case MCAST_LEAVE_GROUP:
			return IP_DROP_MEMBERSHIP;
		case MCAST_BLOCK_SOURCE:
			return IP_BLOCK_SOURCE;
		case MCAST_UNBLOCK_SOURCE:
			return IP_UNBLOCK_SOURCE;
		case MCAST_LEAVE_SOURCE_GROUP:
			return IP_DROP_SOURCE_MEMBERSHIP;
	}

	return -1;
}


static net_interface*
get_multicast_interface(ipv4_protocol* protocol, const in_addr* address)
{
	// TODO: this is broken and leaks references
	sockaddr_in groupAddr;
	net_route* route = sDatalinkModule->get_route(sDomain,
		fill_sockaddr_in(&groupAddr, address ? address->s_addr : INADDR_ANY));
	if (route == NULL)
		return NULL;

	return route->interface_address->interface;
}


static status_t
ipv4_delta_membership(ipv4_protocol* protocol, int option,
	in_addr* interfaceAddr, in_addr* groupAddr, in_addr* sourceAddr)
{
	net_interface* interface = NULL;

	if (interfaceAddr->s_addr == INADDR_ANY) {
		interface = get_multicast_interface(protocol, groupAddr);
	} else {
		sockaddr_in address;
		interface = sDatalinkModule->get_interface_with_address(
			fill_sockaddr_in(&address, interfaceAddr->s_addr));
	}

	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	return ipv4_delta_membership(protocol, option, interface,
		groupAddr, sourceAddr);
}


static status_t
ipv4_generic_delta_membership(ipv4_protocol* protocol, int option,
	uint32 index, const sockaddr_storage* _groupAddr,
	const sockaddr_storage* _sourceAddr)
{
	if (_groupAddr->ss_family != AF_INET
		|| (_sourceAddr != NULL && _sourceAddr->ss_family != AF_INET))
		return B_BAD_VALUE;

	const in_addr* groupAddr = &((const sockaddr_in*)_groupAddr)->sin_addr;

	// TODO: this is broken and leaks references
	net_interface* interface;
	if (index == 0)
		interface = get_multicast_interface(protocol, groupAddr);
	else
		interface = sDatalinkModule->get_interface(sDomain, index);

	if (interface == NULL)
		return B_DEVICE_NOT_FOUND;

	const in_addr* sourceAddr = NULL;
	if (_sourceAddr != NULL)
		sourceAddr = &((const sockaddr_in*)_sourceAddr)->sin_addr;

	return ipv4_delta_membership(protocol, generic_to_ipv4(option), interface,
		groupAddr, sourceAddr);
}


//	#pragma mark - module interface


net_protocol*
ipv4_init_protocol(net_socket* socket)
{
	ipv4_protocol* protocol = new (std::nothrow) ipv4_protocol();
	if (protocol == NULL)
		return NULL;

	protocol->raw = NULL;
	protocol->service_type = 0;
	protocol->time_to_live = kDefaultTTL;
	protocol->multicast_time_to_live = kDefaultMulticastTTL;
	protocol->flags = 0;
	protocol->multicast_address = NULL;
	return protocol;
}


status_t
ipv4_uninit_protocol(net_protocol* _protocol)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;

	delete protocol;

	return B_OK;
}


/*!	Since open() is only called on the top level protocol, when we get here
	it means we are on a SOCK_RAW socket.
*/
status_t
ipv4_open(net_protocol* _protocol)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;

	// Only root may open raw sockets
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	RawSocket* raw = new (std::nothrow) RawSocket(protocol->socket);
	if (raw == NULL)
		return B_NO_MEMORY;

	status_t status = raw->InitCheck();
	if (status != B_OK) {
		delete raw;
		return status;
	}

	TRACE_SK(protocol, "Open()");

	protocol->raw = raw;

	MutexLocker locker(sRawSocketsLock);
	sRawSockets.Add(raw);
	return B_OK;
}


status_t
ipv4_close(net_protocol* _protocol)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;
	RawSocket* raw = protocol->raw;
	if (raw == NULL)
		return B_ERROR;

	TRACE_SK(protocol, "Close()");

	MutexLocker locker(sRawSocketsLock);
	sRawSockets.Remove(raw);
	delete raw;
	protocol->raw = NULL;

	return B_OK;
}


status_t
ipv4_free(net_protocol* protocol)
{
	return B_OK;
}


status_t
ipv4_connect(net_protocol* protocol, const struct sockaddr* address)
{
	return B_ERROR;
}


status_t
ipv4_accept(net_protocol* protocol, struct net_socket** _acceptedSocket)
{
	return B_NOT_SUPPORTED;
}


status_t
ipv4_control(net_protocol* _protocol, int level, int option, void* value,
	size_t* _length)
{
	if ((level & LEVEL_MASK) != IPPROTO_IP)
		return sDatalinkModule->control(sDomain, option, value, _length);

	return B_BAD_VALUE;
}


status_t
ipv4_getsockopt(net_protocol* _protocol, int level, int option, void* value,
	int* _length)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;

	if (level == IPPROTO_IP) {
		if (option == IP_HDRINCL) {
			return get_int_option(value, *_length,
				(protocol->flags & IP_FLAG_HEADER_INCLUDED) != 0);
		}
		if (option == IP_RECVDSTADDR) {
			return get_int_option(value, *_length,
				(protocol->flags & IP_FLAG_RECEIVE_DEST_ADDR) != 0);
		}
		if (option == IP_TTL)
			return get_int_option(value, *_length, protocol->time_to_live);
		if (option == IP_TOS)
			return get_int_option(value, *_length, protocol->service_type);
		if (option == IP_MULTICAST_TTL) {
			return get_int_option(value, *_length,
				protocol->multicast_time_to_live);
		}
		if (option == IP_ADD_MEMBERSHIP
			|| option == IP_DROP_MEMBERSHIP
			|| option == IP_BLOCK_SOURCE
			|| option == IP_UNBLOCK_SOURCE
			|| option == IP_ADD_SOURCE_MEMBERSHIP
			|| option == IP_DROP_SOURCE_MEMBERSHIP
			|| option == MCAST_JOIN_GROUP
			|| option == MCAST_LEAVE_GROUP
			|| option == MCAST_BLOCK_SOURCE
			|| option == MCAST_UNBLOCK_SOURCE
			|| option == MCAST_JOIN_SOURCE_GROUP
			|| option == MCAST_LEAVE_SOURCE_GROUP) {
			// RFC 3678, Section 4.1:
			// ``An error of EOPNOTSUPP is returned if these options are
			// used with getsockopt().''
			return B_NOT_SUPPORTED;
		}

		dprintf("IPv4::getsockopt(): get unknown option: %d\n", option);
		return ENOPROTOOPT;
	}

	return sSocketModule->get_option(protocol->socket, level, option, value,
		_length);
}


status_t
ipv4_setsockopt(net_protocol* _protocol, int level, int option,
	const void* value, int length)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;

	if (level == IPPROTO_IP) {
		if (option == IP_HDRINCL) {
			int headerIncluded;
			if (length != sizeof(int))
				return B_BAD_VALUE;
			if (user_memcpy(&headerIncluded, value, sizeof(headerIncluded))
					!= B_OK)
				return B_BAD_ADDRESS;

			if (headerIncluded)
				protocol->flags |= IP_FLAG_HEADER_INCLUDED;
			else
				protocol->flags &= ~IP_FLAG_HEADER_INCLUDED;

			return B_OK;
		}
		if (option == IP_RECVDSTADDR) {
			int getAddress;
			if (length != sizeof(int))
				return B_BAD_VALUE;
			if (user_memcpy(&getAddress, value, sizeof(int)) != B_OK)
				return B_BAD_ADDRESS;

			if (getAddress && (protocol->socket->type == SOCK_DGRAM
					|| protocol->socket->type == SOCK_RAW))
				protocol->flags |= IP_FLAG_RECEIVE_DEST_ADDR;
			else
				protocol->flags &= ~IP_FLAG_RECEIVE_DEST_ADDR;

			return B_OK;
		}
		if (option == IP_TTL)
			return set_int_option(protocol->time_to_live, value, length);
		if (option == IP_TOS)
			return set_int_option(protocol->service_type, value, length);
		if (option == IP_MULTICAST_IF) {
			if (length != sizeof(struct in_addr))
				return B_BAD_VALUE;

			struct sockaddr_in* address = new (std::nothrow) sockaddr_in;
			if (address == NULL)
				return B_NO_MEMORY;

			if (user_memcpy(&address->sin_addr, value, sizeof(struct in_addr))
					!= B_OK) {
				delete address;
				return B_BAD_ADDRESS;
			}

			// Using INADDR_ANY to remove the previous setting.
			if (address->sin_addr.s_addr == htonl(INADDR_ANY)) {
				delete address;
				delete protocol->multicast_address;
				protocol->multicast_address = NULL;
				return B_OK;
			}

			struct net_interface* interface
				= sDatalinkModule->get_interface_with_address(
					(sockaddr*)address);
			if (interface == NULL) {
				delete address;
				return EADDRNOTAVAIL;
			}

			delete protocol->multicast_address;
			protocol->multicast_address = (struct sockaddr*)address;

			sDatalinkModule->put_interface(interface);
			return B_OK;
		}
		if (option == IP_MULTICAST_TTL) {
			return set_int_option(protocol->multicast_time_to_live, value,
				length);
		}
		if (option == IP_ADD_MEMBERSHIP || option == IP_DROP_MEMBERSHIP) {
			ip_mreq mreq;
			if (length != sizeof(ip_mreq))
				return B_BAD_VALUE;
			if (user_memcpy(&mreq, value, sizeof(ip_mreq)) != B_OK)
				return B_BAD_ADDRESS;

			return ipv4_delta_membership(protocol, option, &mreq.imr_interface,
				&mreq.imr_multiaddr, NULL);
		}
		if (option == IP_BLOCK_SOURCE
			|| option == IP_UNBLOCK_SOURCE
			|| option == IP_ADD_SOURCE_MEMBERSHIP
			|| option == IP_DROP_SOURCE_MEMBERSHIP) {
			ip_mreq_source mreq;
			if (length != sizeof(ip_mreq_source))
				return B_BAD_VALUE;
			if (user_memcpy(&mreq, value, sizeof(ip_mreq_source)) != B_OK)
				return B_BAD_ADDRESS;

			return ipv4_delta_membership(protocol, option, &mreq.imr_interface,
				&mreq.imr_multiaddr, &mreq.imr_sourceaddr);
		}
		if (option == MCAST_LEAVE_GROUP || option == MCAST_JOIN_GROUP) {
			group_req greq;
			if (length != sizeof(group_req))
				return B_BAD_VALUE;
			if (user_memcpy(&greq, value, sizeof(group_req)) != B_OK)
				return B_BAD_ADDRESS;

			return ipv4_generic_delta_membership(protocol, option,
				greq.gr_interface, &greq.gr_group, NULL);
		}
		if (option == MCAST_BLOCK_SOURCE
			|| option == MCAST_UNBLOCK_SOURCE
			|| option == MCAST_JOIN_SOURCE_GROUP
			|| option == MCAST_LEAVE_SOURCE_GROUP) {
			group_source_req greq;
			if (length != sizeof(group_source_req))
				return B_BAD_VALUE;
			if (user_memcpy(&greq, value, sizeof(group_source_req)) != B_OK)
				return B_BAD_ADDRESS;

			return ipv4_generic_delta_membership(protocol, option,
				greq.gsr_interface, &greq.gsr_group, &greq.gsr_source);
		}

		dprintf("IPv4::setsockopt(): set unknown option: %d\n", option);
		return ENOPROTOOPT;
	}

	return sSocketModule->set_option(protocol->socket, level, option,
		value, length);
}


status_t
ipv4_bind(net_protocol* protocol, const struct sockaddr* address)
{
	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	// only INADDR_ANY and addresses of local interfaces are accepted:
	if (((sockaddr_in*)address)->sin_addr.s_addr == INADDR_ANY
		|| IN_MULTICAST(ntohl(((sockaddr_in*)address)->sin_addr.s_addr))
		|| sDatalinkModule->is_local_address(sDomain, address, NULL, NULL)) {
		memcpy(&protocol->socket->address, address, sizeof(struct sockaddr_in));
		protocol->socket->address.ss_len = sizeof(struct sockaddr_in);
			// explicitly set length, as our callers can't be trusted to
			// always provide the correct length!
		return B_OK;
	}

	return B_ERROR;
		// address is unknown on this host
}


status_t
ipv4_unbind(net_protocol* protocol, struct sockaddr* address)
{
	// nothing to do here
	return B_OK;
}


status_t
ipv4_listen(net_protocol* protocol, int count)
{
	return B_NOT_SUPPORTED;
}


status_t
ipv4_shutdown(net_protocol* protocol, int direction)
{
	return B_NOT_SUPPORTED;
}


status_t
ipv4_send_routed_data(net_protocol* _protocol, struct net_route* route,
	net_buffer* buffer)
{
	if (route == NULL)
		return B_BAD_VALUE;

	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;
	net_interface_address* interfaceAddress = route->interface_address;
	net_interface* interface = interfaceAddress->interface;

	TRACE_SK(protocol, "SendRoutedData(%p, %p [%ld bytes])", route, buffer,
		buffer->size);

	sockaddr_in& source = *(sockaddr_in*)buffer->source;
	sockaddr_in& destination = *(sockaddr_in*)buffer->destination;
	sockaddr_in* broadcastAddress = (sockaddr_in*)interfaceAddress->destination;

	bool checksumNeeded = true;
	bool headerIncluded = false;
	if (protocol != NULL)
		headerIncluded = (protocol->flags & IP_FLAG_HEADER_INCLUDED) != 0;

	buffer->flags &= ~(MSG_BCAST | MSG_MCAST);

	if (destination.sin_addr.s_addr == INADDR_ANY)
		return EDESTADDRREQ;

	if ((interface->device->flags & IFF_BROADCAST) != 0
		&& (destination.sin_addr.s_addr == INADDR_BROADCAST
			|| (broadcastAddress != NULL && destination.sin_addr.s_addr
					== broadcastAddress->sin_addr.s_addr))) {
		if (protocol && !(protocol->socket->options & SO_BROADCAST))
			return B_BAD_VALUE;
		buffer->flags |= MSG_BCAST;
	} else if (IN_MULTICAST(ntohl(destination.sin_addr.s_addr)))
		buffer->flags |= MSG_MCAST;

	// Add IP header (if needed)

	if (!headerIncluded) {
		NetBufferPrepend<ipv4_header> header(buffer);
		if (header.Status() != B_OK)
			return header.Status();

		header->version = IPV4_VERSION;
		header->header_length = sizeof(ipv4_header) / 4;
		header->service_type = protocol ? protocol->service_type : 0;
		header->total_length = htons(buffer->size);
		header->id = htons(atomic_add(&sPacketID, 1));
		header->fragment_offset = 0;
		if (protocol) {
			header->time_to_live = (buffer->flags & MSG_MCAST) != 0
				? protocol->multicast_time_to_live : protocol->time_to_live;
		} else {
			header->time_to_live = (buffer->flags & MSG_MCAST) != 0
				? kDefaultMulticastTTL : kDefaultTTL;
		}
		header->protocol = protocol
			? protocol->socket->protocol : buffer->protocol;
		header->checksum = 0;

		header->source = source.sin_addr.s_addr;
		header->destination = destination.sin_addr.s_addr;

		TRACE_ONLY(dump_ipv4_header(*header));
	} else {
		// if IP_HDRINCL, check if the source address is set
		NetBufferHeaderReader<ipv4_header> header(buffer);
		if (header.Status() != B_OK)
			return header.Status();

		if (header->source == 0) {
			header->source = source.sin_addr.s_addr;
			header->checksum = 0;
			header.Sync();
		} else
			checksumNeeded = false;

		TRACE("  Header was already supplied:");
		TRACE_ONLY(dump_ipv4_header(*header));
	}

	if (buffer->size > 0xffff)
		return EMSGSIZE;

	if (checksumNeeded) {
		*IPChecksumField(buffer) = gBufferModule->checksum(buffer, 0,
			sizeof(ipv4_header), true);
	}

	TRACE_SK(protocol, "  SendRoutedData(): header chksum: %ld, buffer "
		"checksum: %ld",
		gBufferModule->checksum(buffer, 0, sizeof(ipv4_header), true),
		gBufferModule->checksum(buffer, 0, buffer->size, true));

	TRACE_SK(protocol, "  SendRoutedData(): destination: %08x",
		ntohl(destination.sin_addr.s_addr));

	uint32 mtu = route->mtu ? route->mtu : interface->mtu;
	if (buffer->size > mtu) {
		// we need to fragment the packet
		return send_fragments(protocol, route, buffer, mtu);
	}

	return sDatalinkModule->send_routed_data(route, buffer);
}


status_t
ipv4_send_data(net_protocol* _protocol, net_buffer* buffer)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;

	TRACE_SK(protocol, "SendData(%p [%ld bytes])", buffer, buffer->size);

	if (protocol != NULL && (protocol->flags & IP_FLAG_HEADER_INCLUDED)) {
		if (buffer->size < sizeof(ipv4_header))
			return B_BAD_VALUE;

		sockaddr_in* source = (sockaddr_in*)buffer->source;
		sockaddr_in* destination = (sockaddr_in*)buffer->destination;

		fill_sockaddr_in(source, *NetBufferField<in_addr_t,
			offsetof(ipv4_header, source)>(buffer));
		fill_sockaddr_in(destination, *NetBufferField<in_addr_t,
			offsetof(ipv4_header, destination)>(buffer));
	}

	// handle IP_MULTICAST_IF
	if (IN_MULTICAST(ntohl(
			((sockaddr_in*)buffer->destination)->sin_addr.s_addr))
		&& protocol != NULL && protocol->multicast_address != NULL) {
		net_interface_address* address = sDatalinkModule->get_interface_address(
			protocol->multicast_address);
		if (address == NULL || (address->interface->flags & IFF_UP) == 0) {
			sDatalinkModule->put_interface_address(address);
			return EADDRNOTAVAIL;
		}

		sDatalinkModule->put_interface_address(buffer->interface_address);
		buffer->interface_address = address;
			// the buffer takes over ownership of the address

		net_route* route = sDatalinkModule->get_route(sDomain, address->local);
		if (route == NULL)
			return ENETUNREACH;

		return sDatalinkModule->send_routed_data(route, buffer);
	}

	return sDatalinkModule->send_data(protocol, sDomain, buffer);
}


ssize_t
ipv4_send_avail(net_protocol* protocol)
{
	return B_ERROR;
}


status_t
ipv4_read_data(net_protocol* _protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;
	RawSocket* raw = protocol->raw;
	if (raw == NULL)
		return B_ERROR;

	TRACE_SK(protocol, "ReadData(%lu, 0x%lx)", numBytes, flags);

	return raw->Dequeue(flags, _buffer);
}


ssize_t
ipv4_read_avail(net_protocol* _protocol)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;
	RawSocket* raw = protocol->raw;
	if (raw == NULL)
		return B_ERROR;

	return raw->AvailableData();
}


struct net_domain*
ipv4_get_domain(net_protocol* protocol)
{
	return sDomain;
}


size_t
ipv4_get_mtu(net_protocol* protocol, const struct sockaddr* address)
{
	net_route* route = sDatalinkModule->get_route(sDomain, address);
	if (route == NULL)
		return 0;

	size_t mtu;
	if (route->mtu != 0)
		mtu = route->mtu;
	else
		mtu = route->interface_address->interface->mtu;

	sDatalinkModule->put_route(sDomain, route);
	return mtu - sizeof(ipv4_header);
}


status_t
ipv4_receive_data(net_buffer* buffer)
{
	TRACE("ipv4_receive_data(%p [%ld bytes])", buffer, buffer->size);

	NetBufferHeaderReader<ipv4_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	ipv4_header& header = bufferHeader.Data();
	TRACE_ONLY(dump_ipv4_header(header));

	if (header.version != IPV4_VERSION)
		return B_BAD_TYPE;

	uint16 packetLength = header.TotalLength();
	uint16 headerLength = header.HeaderLength();
	if (packetLength > buffer->size
		|| headerLength < sizeof(ipv4_header))
		return B_BAD_DATA;

	// TODO: would be nice to have a direct checksum function somewhere
	if (gBufferModule->checksum(buffer, 0, headerLength, true) != 0)
		return B_BAD_DATA;

	// lower layers notion of broadcast or multicast have no relevance to us
	// other than deciding whether to send an ICMP error
	bool wasMulticast = (buffer->flags & (MSG_BCAST | MSG_MCAST)) != 0;
	bool notForUs = false;
	buffer->flags &= ~(MSG_BCAST | MSG_MCAST);

	sockaddr_in destination;
	fill_sockaddr_in(&destination, header.destination);

	if (header.destination == INADDR_BROADCAST) {
		buffer->flags |= MSG_BCAST;

		// Find first interface with a matching family
		if (!sDatalinkModule->is_local_link_address(sDomain, true,
				buffer->destination, &buffer->interface_address))
			notForUs = !wasMulticast;
	} else if (IN_MULTICAST(ntohl(header.destination))) {
		buffer->flags |= MSG_MCAST;
	} else {
		uint32 matchedAddressType = 0;

		// test if the packet is really for us
		if (!sDatalinkModule->is_local_address(sDomain, (sockaddr*)&destination,
				&buffer->interface_address, &matchedAddressType)
			&& !sDatalinkModule->is_local_link_address(sDomain, true,
				buffer->destination, &buffer->interface_address)) {
			// if the buffer was a link layer multicast, regard it as a
			// broadcast, and let the upper levels decide what to do with it
			if (wasMulticast)
				buffer->flags |= MSG_BCAST;
			else
				notForUs = true;
		} else {
			// copy over special address types (MSG_BCAST or MSG_MCAST):
			buffer->flags |= matchedAddressType;
		}
	}

	// set net_buffer's source/destination address
	fill_sockaddr_in((struct sockaddr_in*)buffer->source, header.source);
	memcpy(buffer->destination, &destination, sizeof(sockaddr_in));

	buffer->protocol = header.protocol;

	if (notForUs) {
		TRACE("  ipv4_receive_data(): packet was not for us %x -> %x",
			ntohl(header.source), ntohl(header.destination));

		if (!wasMulticast) {
			// Send ICMP error: Host unreachable
			sDomain->module->error_reply(NULL, buffer, B_NET_ERROR_UNREACH_HOST,
				NULL);
		}

		return B_ERROR;
	}

	// remove any trailing/padding data
	status_t status = gBufferModule->trim(buffer, packetLength);
	if (status != B_OK)
		return status;

	// check for fragmentation
	uint16 fragmentOffset = header.FragmentOffset();
	if ((fragmentOffset & IP_MORE_FRAGMENTS) != 0
		|| (fragmentOffset & IP_FRAGMENT_OFFSET_MASK) != 0) {
		// this is a fragment
		TRACE("  ipv4_receive_data(): Found a Fragment!");
		status = reassemble_fragments(header, &buffer);
		TRACE("  ipv4_receive_data():  -> %s", strerror(status));
		if (status != B_OK)
			return status;

		if (buffer == NULL) {
			// buffer was put into fragment packet
			TRACE("  ipv4_receive_data(): Not yet assembled.");
			return B_OK;
		}
	}

	// Since the buffer might have been changed (reassembled fragment)
	// we must no longer access bufferHeader or header anymore after
	// this point

	bool rawDelivered = raw_receive_data(buffer);

	// Preserve the ipv4 header for ICMP processing
	gBufferModule->store_header(buffer);

	bufferHeader.Remove(headerLength);
		// the header is of variable size and may include IP options
		// (TODO: that we ignore for now)

	net_protocol_module_info* module = receiving_protocol(buffer->protocol);
	if (module == NULL) {
		// no handler for this packet
		if (!rawDelivered) {
			sDomain->module->error_reply(NULL, buffer,
				B_NET_ERROR_UNREACH_PROTOCOL, NULL);
		}
		return EAFNOSUPPORT;
	}

	if ((buffer->flags & MSG_MCAST) != 0) {
		// Unfortunately historical reasons dictate that the IP multicast
		// model be a little different from the unicast one. We deliver
		// this frame directly to all sockets registered with interest
		// for this multicast group.
		deliver_multicast(module, buffer, false);
		gBufferModule->free(buffer);
		return B_OK;
	}

	return module->receive_data(buffer);
}


status_t
ipv4_deliver_data(net_protocol* _protocol, net_buffer* buffer)
{
	ipv4_protocol* protocol = (ipv4_protocol*)_protocol;

	if (protocol->raw == NULL)
		return B_ERROR;

	return protocol->raw->EnqueueClone(buffer);
}


status_t
ipv4_error_received(net_error error, net_buffer* buffer)
{
	TRACE("  ipv4_error_received(error %d, buffer %p [%zu bytes])", (int)error,
		buffer, buffer->size);

	NetBufferHeaderReader<ipv4_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	ipv4_header& header = bufferHeader.Data();
	TRACE_ONLY(dump_ipv4_header(header));

	// We do not check the packet length, as we usually only get a part of it
	uint16 headerLength = header.HeaderLength();
	if (header.version != IPV4_VERSION
		|| headerLength < sizeof(ipv4_header)
		|| gBufferModule->checksum(buffer, 0, headerLength, true) != 0)
		return B_BAD_DATA;

	// Restore addresses of the original buffer

	// lower layers notion of broadcast or multicast have no relevance to us
	// TODO: they actually have when deciding whether to send an ICMP error
	buffer->flags &= ~(MSG_BCAST | MSG_MCAST);

	fill_sockaddr_in((struct sockaddr_in*)buffer->source, header.source);
	fill_sockaddr_in((struct sockaddr_in*)buffer->destination,
		header.destination);

	if (header.destination == INADDR_BROADCAST)
		buffer->flags |= MSG_BCAST;
	else if (IN_MULTICAST(ntohl(header.destination)))
		buffer->flags |= MSG_MCAST;

	// test if the packet is really from us
	if (!sDatalinkModule->is_local_address(sDomain, buffer->source, NULL,
			NULL)) {
		TRACE("  ipv4_error_received(): packet was not for us %x -> %x",
			ntohl(header.source), ntohl(header.destination));
		return B_ERROR;
	}

	buffer->protocol = header.protocol;

	bufferHeader.Remove(headerLength);

	net_protocol_module_info* protocol = receiving_protocol(buffer->protocol);
	if (protocol == NULL)
		return B_ERROR;

	// propagate error
	return protocol->error_received(error, buffer);
}


status_t
ipv4_error_reply(net_protocol* protocol, net_buffer* cause, net_error error,
	net_error_data* errorData)
{
	// Directly obtain the ICMP protocol module
	net_protocol_module_info* icmp = receiving_protocol(IPPROTO_ICMP);
	if (icmp == NULL)
		return B_ERROR;

	return icmp->error_reply(protocol, cause, error, errorData);
}


ssize_t
ipv4_process_ancillary_data_no_container(net_protocol* protocol,
	net_buffer* buffer, void* msgControl, size_t msgControlLen)
{
	ssize_t bytesWritten = 0;

	if ((((ipv4_protocol*)protocol)->flags & IP_FLAG_RECEIVE_DEST_ADDR) != 0) {
		if (msgControlLen < CMSG_SPACE(sizeof(struct in_addr)))
			return B_NO_MEMORY;

		cmsghdr* messageHeader = (cmsghdr*)msgControl;
		messageHeader->cmsg_len = CMSG_LEN(sizeof(struct in_addr));
		messageHeader->cmsg_level = IPPROTO_IP;
		messageHeader->cmsg_type = IP_RECVDSTADDR;

		memcpy(CMSG_DATA(messageHeader),
		 	&((struct sockaddr_in*)buffer->destination)->sin_addr,
		 	sizeof(struct in_addr));

		bytesWritten += CMSG_SPACE(sizeof(struct in_addr));
	}

	return bytesWritten;
}


//	#pragma mark -


status_t
init_ipv4()
{
	sPacketID = (int32)system_time();

	mutex_init(&sRawSocketsLock, "raw sockets");
	mutex_init(&sFragmentLock, "IPv4 Fragments");
	mutex_init(&sMulticastGroupsLock, "IPv4 multicast groups");
	mutex_init(&sReceivingProtocolLock, "IPv4 receiving protocols");

	status_t status;

	sMulticastState = new MulticastState();
	if (sMulticastState == NULL) {
		status = B_NO_MEMORY;
		goto err4;
	}

	status = sMulticastState->Init();
	if (status != B_OK)
		goto err5;

	new (&sFragmentHash) FragmentTable();
	status = sFragmentHash.Init(256);
	if (status != B_OK)
		goto err5;

	new (&sRawSockets) RawSocketList;
		// static initializers do not work in the kernel,
		// so we have to do it here, manually
		// TODO: for modules, this shouldn't be required

	status = gStackModule->register_domain_protocols(AF_INET, SOCK_RAW, 0,
		"network/protocols/ipv4/v1", NULL);
	if (status != B_OK)
		goto err6;

	status = gStackModule->register_domain(AF_INET, "internet", &gIPv4Module,
		&gIPv4AddressModule, &sDomain);
	if (status != B_OK)
		goto err6;

	add_debugger_command("ipv4_multicast", dump_ipv4_multicast,
		"list all current IPv4 multicast states");

	return B_OK;

err6:
	sFragmentHash.~FragmentTable();
err5:
	delete sMulticastState;
err4:
	mutex_destroy(&sReceivingProtocolLock);
	mutex_destroy(&sMulticastGroupsLock);
	mutex_destroy(&sFragmentLock);
	mutex_destroy(&sRawSocketsLock);
	return status;
}


status_t
uninit_ipv4()
{
	mutex_lock(&sReceivingProtocolLock);

	remove_debugger_command("ipv4_multicast", dump_ipv4_multicast);

	// put all the domain receiving protocols we gathered so far
	for (uint32 i = 0; i < 256; i++) {
		if (sReceivingProtocol[i] != NULL)
			gStackModule->put_domain_receiving_protocol(sDomain, i);
	}

	gStackModule->unregister_domain(sDomain);
	mutex_unlock(&sReceivingProtocolLock);

	delete sMulticastState;
	sFragmentHash.~FragmentTable();

	mutex_destroy(&sMulticastGroupsLock);
	mutex_destroy(&sFragmentLock);
	mutex_destroy(&sRawSocketsLock);
	mutex_destroy(&sReceivingProtocolLock);

	return B_OK;
}


static status_t
ipv4_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_ipv4();
		case B_MODULE_UNINIT:
			return uninit_ipv4();

		default:
			return B_ERROR;
	}
}


net_protocol_module_info gIPv4Module = {
	{
		"network/protocols/ipv4/v1",
		0,
		ipv4_std_ops
	},
	NET_PROTOCOL_ATOMIC_MESSAGES,

	ipv4_init_protocol,
	ipv4_uninit_protocol,
	ipv4_open,
	ipv4_close,
	ipv4_free,
	ipv4_connect,
	ipv4_accept,
	ipv4_control,
	ipv4_getsockopt,
	ipv4_setsockopt,
	ipv4_bind,
	ipv4_unbind,
	ipv4_listen,
	ipv4_shutdown,
	ipv4_send_data,
	ipv4_send_routed_data,
	ipv4_send_avail,
	ipv4_read_data,
	ipv4_read_avail,
	ipv4_get_domain,
	ipv4_get_mtu,
	ipv4_receive_data,
	ipv4_deliver_data,
	ipv4_error_received,
	ipv4_error_reply,
	NULL,		// add_ancillary_data()
	NULL,		// process_ancillary_data()
	ipv4_process_ancillary_data_no_container,
	NULL,		// send_data_no_buffer()
	NULL		// read_data_no_buffer()
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{NET_DATALINK_MODULE_NAME, (module_info**)&sDatalinkModule},
	{NET_SOCKET_MODULE_NAME, (module_info**)&sSocketModule},
	{}
};

module_info* modules[] = {
	(module_info*)&gIPv4Module,
	NULL
};
