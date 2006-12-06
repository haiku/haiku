/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ipv4_address.h"

#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <ByteOrder.h>
#include <KernelExport.h>
#include <util/AutoLock.h>
#include <util/list.h>
#include <util/khash.h>
#include <util/DoublyLinkedList.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <new>
#include <stdlib.h>
#include <string.h>


#define TRACE_IPV4
#ifdef TRACE_IPV4
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

struct ipv4_header {
#if B_HOST_IS_LENDIAN == 1
	uint8		header_length : 4;	// header length in 32-bit words
	uint8		version : 4;
#else
	uint8		version : 4;
	uint8		header_length : 4;
#endif
	uint8		service_type;
	uint16		total_length;
	uint16		id;
	uint16		fragment_offset;
	uint8		time_to_live;
	uint8		protocol;
	uint16		checksum;
	in_addr_t	source;
	in_addr_t	destination;

	uint16 HeaderLength() const { return header_length << 2; }
	uint16 TotalLength() const { return ntohs(total_length); }
	uint16 FragmentOffset() const { return ntohs(fragment_offset); }
} _PACKED;

#define IP_VERSION				4

// fragment flags
#define IP_RESERVED_FLAG		0x8000
#define IP_DONT_FRAGMENT		0x4000
#define IP_MORE_FRAGMENTS		0x2000
#define IP_FRAGMENT_OFFSET_MASK	0x1fff

#define MAX_HASH_FRAGMENTS 		64
	// slots in the fragment packet's hash
#define FRAGMENT_TIMEOUT		60000000LL
	// discard fragment after 60 seconds

typedef DoublyLinkedList<struct net_buffer, DoublyLinkedListCLink<struct net_buffer> > FragmentList;

struct ipv4_packet_key {
	in_addr_t	source;
	in_addr_t	destination;
	uint16		id;
	uint8		protocol;
};

class FragmentPacket {
	public:
		FragmentPacket(const ipv4_packet_key &key);
		~FragmentPacket();

		status_t AddFragment(uint16 start, uint16 end, net_buffer *buffer,
					bool lastFragment);
		status_t Reassemble(net_buffer *to);

		bool IsComplete() const { return fReceivedLastFragment && fBytesLeft == 0; }

		static uint32 Hash(void *_packet, const void *_key, uint32 range);
		static int Compare(void *_packet, const void *_key);
		static int32 NextOffset() { return offsetof(FragmentPacket, fNext); }
		static void StaleTimer(struct net_timer *timer, void *data);

	private:
		FragmentPacket	*fNext;
		struct ipv4_packet_key fKey;
		bool			fReceivedLastFragment;
		int32			fBytesLeft;
		FragmentList	fFragments;
		net_timer		fTimer;
};

typedef DoublyLinkedList<class RawSocket> RawSocketList;

class RawSocket : public DoublyLinkedListLinkImpl<RawSocket> {
	public:
		RawSocket(net_socket *socket);
		~RawSocket();

		status_t InitCheck();

		status_t Read(size_t numBytes, uint32 flags, bigtime_t timeout,
					net_buffer **_buffer);
		ssize_t BytesAvailable();

		status_t Write(net_buffer *buffer);

	private:
		net_socket	*fSocket;
		net_fifo	fFifo;
};

struct ipv4_protocol : net_protocol {
	RawSocket	*raw;
	uint32		flags;
};

// protocol flags
#define IP_FLAG_HEADER_INCLUDED	0x01


extern net_protocol_module_info gIPv4Module;
	// we need this in ipv4_std_ops() for registering the AF_INET domain

static struct net_domain *sDomain;
static net_datalink_module_info *sDatalinkModule;
static net_stack_module_info *sStackModule;
struct net_buffer_module_info *gBufferModule;
static int32 sPacketID;
static RawSocketList sRawSockets;
static benaphore sRawSocketsLock;
static benaphore sFragmentLock;
static hash_table *sFragmentHash;


RawSocket::RawSocket(net_socket *socket)
	:
	fSocket(socket)
{
	status_t status = sStackModule->init_fifo(&fFifo, "ipv4 raw socket", 65536);
	if (status < B_OK)
		fFifo.notify = status;
}


RawSocket::~RawSocket()
{
	if (fFifo.notify >= B_OK)
		sStackModule->uninit_fifo(&fFifo);
}


status_t
RawSocket::InitCheck()
{
	return fFifo.notify >= B_OK ? B_OK : fFifo.notify;
}


status_t
RawSocket::Read(size_t numBytes, uint32 flags, bigtime_t timeout,
	net_buffer **_buffer)
{
	net_buffer *buffer;
	status_t status = sStackModule->fifo_dequeue_buffer(&fFifo,
		flags, timeout, &buffer);
	if (status < B_OK)
		return status;

	if (numBytes < buffer->size) {
		// discard any data behind the amount requested
		gBufferModule->trim(buffer, numBytes);
	}

	*_buffer = buffer;
	return B_OK;
}


ssize_t
RawSocket::BytesAvailable()
{
	return fFifo.current_bytes;
}


status_t
RawSocket::Write(net_buffer *source)
{
	// we need to make a clone for that buffer and pass it to the socket
	net_buffer *buffer = gBufferModule->clone(source, false);
	TRACE(("ipv4::RawSocket::Write(): cloned buffer %p\n", buffer));
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = sStackModule->fifo_enqueue_buffer(&fFifo, buffer);
	if (status >= B_OK)
		sStackModule->notify_socket(fSocket, B_SELECT_READ, BytesAvailable());
	else
		gBufferModule->free(buffer);

	return status;
}


//	#pragma mark -


FragmentPacket::FragmentPacket(const ipv4_packet_key &key)
	:
	fKey(key),
	fReceivedLastFragment(false),
	fBytesLeft(IP_MAXPACKET)
{
	sStackModule->init_timer(&fTimer, StaleTimer, this);
}


FragmentPacket::~FragmentPacket()
{
	// cancel the kill timer
	sStackModule->set_timer(&fTimer, -1);

	// delete all fragments
	net_buffer *buffer;
	while ((buffer = fFragments.RemoveHead()) != NULL) {
		gBufferModule->free(buffer);
	}
}


status_t
FragmentPacket::AddFragment(uint16 start, uint16 end, net_buffer *buffer,
	bool lastFragment)
{
	// restart the timer
	sStackModule->set_timer(&fTimer, FRAGMENT_TIMEOUT);

	if (start >= end) {
		// invalid fragment
		return B_BAD_DATA;
	}

	// Search for a position in the list to insert the fragment

	FragmentList::ReverseIterator iterator = fFragments.GetReverseIterator();
	net_buffer *previous = NULL;
	net_buffer *next = NULL;
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

	TRACE(("    previous: %p, next: %p\n", previous, next));

	// If we have parts of the data already, truncate as needed

	if (previous != NULL && previous->fragment.end > start) {
		TRACE(("    remove header %d bytes\n", previous->fragment.end - start));
		gBufferModule->remove_header(buffer, previous->fragment.end - start);
		start = previous->fragment.end;
	}
	if (next != NULL && next->fragment.start < end) {
		TRACE(("    remove trailer %d bytes\n", next->fragment.start - end));
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
		TRACE(("    merge previous: %s\n", strerror(status)));
		if (status < B_OK) {
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

		TRACE(("    hole length: %d\n", (int)fBytesLeft));

		return B_OK;
	} else if (next != NULL && next->fragment.start == end) {
		fFragments.Remove(next);

		buffer->fragment.start = start;
		buffer->fragment.end = next->fragment.end;

		status_t status = gBufferModule->merge(buffer, next, true);
		TRACE(("    merge next: %s\n", strerror(status)));
		if (status < B_OK) {
			fFragments.Insert((net_buffer *)previous->link.next, next);
			return status;
		}

		fFragments.Insert((net_buffer *)previous->link.next, buffer);

		// cut down existing hole
		fBytesLeft -= end - start;

		if (lastFragment && !fReceivedLastFragment) {
			fReceivedLastFragment = true;
			fBytesLeft -= IP_MAXPACKET - end;
		}

		TRACE(("    hole length: %d\n", (int)fBytesLeft));

		return B_OK;
	}

	// We couldn't merge the fragments, so we need to add it as is

	TRACE(("    new fragment: %p, bytes %d-%d\n", buffer, start, end));

	buffer->fragment.start = start;
	buffer->fragment.end = end;
	fFragments.Insert(next, buffer);

	// update length of the hole, if any
	fBytesLeft -= end - start;

	if (lastFragment && !fReceivedLastFragment) {
		fReceivedLastFragment = true;
		fBytesLeft -= IP_MAXPACKET - end;
	}

	TRACE(("    hole length: %d\n", (int)fBytesLeft));

	return B_OK;
}


/*!
	Reassembles the fragments to the specified buffer \a to.
	This buffer must have been added via AddFragment() before.
*/
status_t
FragmentPacket::Reassemble(net_buffer *to)
{
	if (!IsComplete())
		return NULL;

	net_buffer *buffer = NULL;

	net_buffer *fragment;
	while ((fragment = fFragments.RemoveHead()) != NULL) {
		if (buffer != NULL) {
			status_t status;
			if (to == fragment) {
				status = gBufferModule->merge(fragment, buffer, false);
				buffer = fragment;
			} else
				status = gBufferModule->merge(buffer, fragment, true);
			if (status < B_OK)
				return status;
		} else
			buffer = fragment;
	}

	if (buffer != to)
		panic("ipv4 packet reassembly did not work correctly.\n");

	return B_OK;
}


int
FragmentPacket::Compare(void *_packet, const void *_key)
{
	const ipv4_packet_key *key = (ipv4_packet_key *)_key;
	ipv4_packet_key *packetKey = &((FragmentPacket *)_packet)->fKey;

	if (packetKey->id == key->id
		&& packetKey->source == key->source
		&& packetKey->destination == key->destination
		&& packetKey->protocol == key->protocol)
		return 0;

	return 1;
}


uint32
FragmentPacket::Hash(void *_packet, const void *_key, uint32 range)
{
	const struct ipv4_packet_key *key = (struct ipv4_packet_key *)_key;
	FragmentPacket *packet = (FragmentPacket *)_packet;
	if (packet != NULL)
		key = &packet->fKey;

	return (key->source ^ key->destination ^ key->protocol ^ key->id) % range;
}


/*static*/ void
FragmentPacket::StaleTimer(struct net_timer *timer, void *data)
{
	FragmentPacket *packet = (FragmentPacket *)data;
	TRACE(("Assembling FragmentPacket %p timed out!\n", packet));

	BenaphoreLocker locker(&sFragmentLock);

	hash_remove(sFragmentHash, packet);
	delete packet;
}


//	#pragma mark -


#if 0
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
	struct pretty_ipv4 *src = (struct pretty_ipv4 *)&header.source;
	struct pretty_ipv4 *dst = (struct pretty_ipv4 *)&header.destination;
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
#endif


/*!
	Attempts to re-assemble fragmented packets.
	\return B_OK if everything went well; if it could reassemble the packet, \a _buffer
		will point to its buffer, otherwise, it will be \c NULL.
	\return various error codes if something went wrong (mostly B_NO_MEMORY)
*/
static status_t
reassemble_fragments(const ipv4_header &header, net_buffer **_buffer)
{
	net_buffer *buffer = *_buffer;
	status_t status;

	struct ipv4_packet_key key;
	key.source = (in_addr_t)header.source;
	key.destination = (in_addr_t)header.destination;
	key.id = header.id;
	key.protocol = header.protocol;

	// TODO: Make locking finer grained.
	BenaphoreLocker locker(&sFragmentLock);

	FragmentPacket *packet = (FragmentPacket *)hash_lookup(sFragmentHash, &key);
	if (packet == NULL) {
		// New fragment packet
		packet = new (std::nothrow) FragmentPacket(key);
		if (packet == NULL)
			return B_NO_MEMORY;

		// add packet to hash
		status = hash_insert(sFragmentHash, packet);
		if (status != B_OK) {
			delete packet;
			return status;
		}
	}

	uint16 fragmentOffset = header.FragmentOffset();
	uint16 start = (fragmentOffset & IP_FRAGMENT_OFFSET_MASK) << 3;
	uint16 end = start + header.TotalLength() - header.HeaderLength();
	bool lastFragment = (fragmentOffset & IP_MORE_FRAGMENTS) == 0;

	TRACE(("   Received IPv4 %sfragment of size %d, offset %d.\n",
		lastFragment ? "last ": "", end - start, start));

	// Remove header unless this is the first fragment
	if (start != 0)
		gBufferModule->remove_header(buffer, header.HeaderLength());

	status = packet->AddFragment(start, end, buffer, lastFragment);
	if (status != B_OK)
		return status;

	if (packet->IsComplete()) {
		hash_remove(sFragmentHash, packet);
			// no matter if reassembling succeeds, we won't need this packet anymore

		status = packet->Reassemble(buffer);
		delete packet;

		// _buffer does not change
		return status;
	}

	// This indicates that the packet is not yet complete
	*_buffer = NULL;
	return B_OK;
}


/*!
	Fragments the incoming buffer and send all fragments via the specified
	\a route.
*/
static status_t
send_fragments(ipv4_protocol *protocol, struct net_route *route,
	net_buffer *buffer, uint32 mtu)
{
	TRACE(("ipv4 needs to fragment (size %lu, MTU %lu)...\n",
		buffer->size, mtu));

	NetBufferHeader<ipv4_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	ipv4_header *header = &bufferHeader.Data();
	bufferHeader.Detach();

	uint16 headerLength = header->HeaderLength();
	uint32 bytesLeft = buffer->size - headerLength;
	uint32 fragmentOffset = 0;
	status_t status = B_OK;

	net_buffer *headerBuffer = gBufferModule->split(buffer, headerLength);
	if (headerBuffer == NULL)
		return B_NO_MEMORY;

	bufferHeader.SetTo(headerBuffer);
	header = &bufferHeader.Data();
	bufferHeader.Detach();

	// adapt MTU to be a multiple of 8 (fragment offsets can only be specified this way)
	mtu -= headerLength;
	mtu &= ~7;
	dprintf("  adjusted MTU to %ld\n", mtu);

	dprintf("  bytesLeft = %ld\n", bytesLeft);
	while (bytesLeft > 0) {
		uint32 fragmentLength = min_c(bytesLeft, mtu);
		bytesLeft -= fragmentLength;
		bool lastFragment = bytesLeft == 0;

		header->total_length = htons(fragmentLength + headerLength);
		header->fragment_offset = htons((lastFragment ? 0 : IP_MORE_FRAGMENTS)
			| (fragmentOffset >> 3));
		header->checksum = 0;
		header->checksum = sStackModule->checksum((uint8 *)header, headerLength);
			// TODO: compute the checksum only for those parts that changed?

		dprintf("  send fragment of %ld bytes (%ld bytes left)\n", fragmentLength, bytesLeft);

		net_buffer *fragmentBuffer;
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
			status = sDatalinkModule->send_data(route, fragmentBuffer);

		if (lastFragment) {
			// we don't own the last buffer, so we don't have to free it
			break;
		}

		if (status < B_OK) {
			gBufferModule->free(fragmentBuffer);
			break;
		}
	}

	gBufferModule->free(headerBuffer);
	return status;
}


static void
raw_receive_data(net_buffer *buffer)
{
	BenaphoreLocker locker(sRawSocketsLock);
	RawSocketList::Iterator iterator = sRawSockets.GetIterator();

	while (iterator.HasNext()) {
		RawSocket *raw = iterator.Next();
		raw->Write(buffer);
	}
}


//	#pragma mark -


net_protocol *
ipv4_init_protocol(net_socket *socket)
{
	ipv4_protocol *protocol = new (std::nothrow) ipv4_protocol;
	if (protocol == NULL)
		return NULL;

	protocol->raw = NULL;
	protocol->flags = 0;
	return protocol;
}


status_t
ipv4_uninit_protocol(net_protocol *_protocol)
{
	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;

	delete protocol->raw;
	delete protocol;
	return B_OK;
}


/*!
	Since open() is only called on the top level protocol, when we get here
	it means we are on a SOCK_RAW socket.
*/
status_t
ipv4_open(net_protocol *_protocol)
{
	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;

	RawSocket *raw = new (std::nothrow) RawSocket(protocol->socket);
	if (raw == NULL)
		return B_NO_MEMORY;

	status_t status = raw->InitCheck();
	if (status < B_OK) {
		delete raw;
		return status;
	}

	protocol->raw = raw;

	BenaphoreLocker locker(sRawSocketsLock);
	sRawSockets.Add(raw);
	return B_OK;
}


status_t
ipv4_close(net_protocol *_protocol)
{
	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;
	RawSocket *raw = protocol->raw;
	if (raw == NULL)
		return B_ERROR;

	BenaphoreLocker locker(sRawSocketsLock);
	sRawSockets.Remove(raw);
	delete raw;
	protocol->raw = NULL;

	return B_OK;
}


status_t
ipv4_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
ipv4_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
ipv4_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
ipv4_control(net_protocol *_protocol, int level, int option, void *value,
	size_t *_length)
{
	if ((level & LEVEL_MASK) != IPPROTO_IP)
		return sDatalinkModule->control(sDomain, option, value, _length);

	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;

	if (level & LEVEL_GET_OPTION) {
		// get options

		switch (option) {
			case IP_HDRINCL:
			{
				if (*_length != sizeof(int))
					return B_BAD_VALUE;

				int headerIncluded = (protocol->flags & IP_FLAG_HEADER_INCLUDED) != 0;
				return user_memcpy(value, &headerIncluded, sizeof(headerIncluded));
			}

			default:
				return ENOPROTOOPT;
		}
	} else {
		// set options

		switch (option) {
			case IP_HDRINCL:
			{
				int headerIncluded;
				if (*_length != sizeof(int))
					return B_BAD_VALUE;
				if (user_memcpy(&headerIncluded, value, sizeof(headerIncluded)) < B_OK)
					return B_BAD_ADDRESS;

				if (headerIncluded)
					protocol->flags |= IP_FLAG_HEADER_INCLUDED;
				else
					protocol->flags &= ~IP_FLAG_HEADER_INCLUDED;
				break;
			}

			default:
				return ENOPROTOOPT;
		}
	}

	return B_BAD_VALUE;
}


status_t
ipv4_bind(net_protocol *protocol, struct sockaddr *address)
{
	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	// only INADDR_ANY and addresses of local interfaces are accepted:
	if (((sockaddr_in *)address)->sin_addr.s_addr == INADDR_ANY
		|| sDatalinkModule->is_local_address(sDomain, address, NULL, NULL)) {
		protocol->socket->address.ss_len = sizeof(struct sockaddr_in);
			// explicitly set length, as our callers can't be trusted to
			// always provide the correct length!
		return B_OK;
	}

	return B_ERROR;
		// address is unknown on this host
}


status_t
ipv4_unbind(net_protocol *protocol, struct sockaddr *address)
{
	// nothing to do here
	return B_OK;
}


status_t
ipv4_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
ipv4_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
ipv4_send_routed_data(net_protocol *_protocol, struct net_route *route,
	net_buffer *buffer)
{
	if (route == NULL)
		return B_BAD_VALUE;

	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;
	net_interface *interface = route->interface;

	TRACE(("someone tries to send some actual routed data!\n"));

	sockaddr_in &source = *(sockaddr_in *)&buffer->source;
	if (source.sin_addr.s_addr == INADDR_ANY && route->interface->address != NULL) {
		// replace an unbound source address with the address of the interface
		// TODO: couldn't we replace all addresses here?
		source.sin_addr.s_addr = ((sockaddr_in *)route->interface->address)->sin_addr.s_addr;
	}

	// Add IP header (if needed)

	if (protocol == NULL || (protocol->flags & IP_FLAG_HEADER_INCLUDED) == 0) {
		NetBufferPrepend<ipv4_header> bufferHeader(buffer);
		if (bufferHeader.Status() < B_OK)
			return bufferHeader.Status();

		ipv4_header &header = bufferHeader.Data();

		header.version = IP_VERSION;
		header.header_length = sizeof(ipv4_header) >> 2;
		header.service_type = 0;
		header.total_length = htons(buffer->size);
		header.id = htons(atomic_add(&sPacketID, 1));
		header.fragment_offset = 0;
		header.time_to_live = 254;
		header.protocol = protocol ? protocol->socket->protocol : buffer->protocol;
		header.checksum = 0;
		if (route->interface->address != NULL) {
			header.source = ((sockaddr_in *)route->interface->address)->sin_addr.s_addr;
				// always use the actual used source address
		} else
			header.source = 0;

		header.destination = ((sockaddr_in *)&buffer->destination)->sin_addr.s_addr;

		header.checksum = gBufferModule->checksum(buffer, 0, sizeof(ipv4_header), true);
		//dump_ipv4_header(header);

		bufferHeader.Detach();
			// make sure the IP-header is already written to the buffer at this point
	}

	TRACE(("header chksum: %ld, buffer checksum: %ld\n",
		gBufferModule->checksum(buffer, 0, sizeof(ipv4_header), true),
		gBufferModule->checksum(buffer, 0, buffer->size, true)));

	TRACE(("destination-IP: buffer=%p addr=%p %08lx\n", buffer, &buffer->destination,
		ntohl(((sockaddr_in *)&buffer->destination)->sin_addr.s_addr)));

	uint32 mtu = route->mtu ? route->mtu : interface->mtu;
	if (buffer->size > mtu) {
		// we need to fragment the packet
		return send_fragments(protocol, route, buffer, mtu);
	}

	return sDatalinkModule->send_data(route, buffer);
}


status_t
ipv4_send_data(net_protocol *protocol, net_buffer *buffer)
{
	TRACE(("someone tries to send some actual data!\n"));

	// find route
	struct net_route *route = sDatalinkModule->get_route(sDomain,
		(sockaddr *)&buffer->destination);
	if (route == NULL)
		return ENETUNREACH;

	status_t status = ipv4_send_routed_data(protocol, route, buffer);
	sDatalinkModule->put_route(sDomain, route);

	return status;
}


ssize_t
ipv4_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
ipv4_read_data(net_protocol *_protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;
	RawSocket *raw = protocol->raw;
	if (raw == NULL)
		return B_ERROR;

	TRACE(("read is waiting for data...\n"));
	return raw->Read(numBytes, flags, protocol->socket->receive.timeout, _buffer);
}


ssize_t
ipv4_read_avail(net_protocol *_protocol)
{
	ipv4_protocol *protocol = (ipv4_protocol *)_protocol;
	RawSocket *raw = protocol->raw;
	if (raw == NULL)
		return B_ERROR;

	return raw->BytesAvailable();
}


struct net_domain *
ipv4_get_domain(net_protocol *protocol)
{
	return sDomain;
}


size_t
ipv4_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	net_route *route = sDatalinkModule->get_route(sDomain, address);
	if (route == NULL)
		return 0;

	size_t mtu;
	if (route->mtu != 0)
		mtu = route->mtu;
	else
		mtu = route->interface->mtu;

	sDatalinkModule->put_route(sDomain, route);
	return mtu - sizeof(ipv4_header);
}


status_t
ipv4_receive_data(net_buffer *buffer)
{
	TRACE(("IPv4 received a packet (%p) of %ld size!\n", buffer, buffer->size));

	NetBufferHeader<ipv4_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	ipv4_header &header = bufferHeader.Data();
	bufferHeader.Detach();
	//dump_ipv4_header(header);

	if (header.version != IP_VERSION)
		return B_BAD_TYPE;

	uint16 packetLength = header.TotalLength();
	uint16 headerLength = header.HeaderLength();
	if (packetLength > buffer->size
		|| headerLength < sizeof(ipv4_header))
		return B_BAD_DATA;

	// TODO: would be nice to have a direct checksum function somewhere
	if (gBufferModule->checksum(buffer, 0, headerLength, true) != 0)
		return B_BAD_DATA;

	struct sockaddr_in &source = *(struct sockaddr_in *)&buffer->source;
	struct sockaddr_in &destination = *(struct sockaddr_in *)&buffer->destination;

	source.sin_len = sizeof(sockaddr_in);
	source.sin_family = AF_INET;
	source.sin_addr.s_addr = header.source;

	destination.sin_len = sizeof(sockaddr_in);
	destination.sin_family = AF_INET;
	destination.sin_addr.s_addr = header.destination;

	// test if the packet is really for us
	uint32 matchedAddressType;
	if (!sDatalinkModule->is_local_address(sDomain, (sockaddr*)&destination, 
		&buffer->interface, &matchedAddressType)) {
		TRACE(("this packet was not for us\n"));
		return B_ERROR;
	}
	if (matchedAddressType != 0) {
		// copy over special address types (MSG_BCAST or MSG_MCAST):
		buffer->flags |= matchedAddressType;
	}

	uint8 protocol = buffer->protocol = header.protocol;

	// remove any trailing/padding data
	status_t status = gBufferModule->trim(buffer, packetLength);
	if (status < B_OK)
		return status;

	// check for fragmentation
	uint16 fragmentOffset = ntohs(header.fragment_offset);
	if ((fragmentOffset & IP_MORE_FRAGMENTS) != 0
		|| (fragmentOffset & IP_FRAGMENT_OFFSET_MASK) != 0) {
		// this is a fragment
		TRACE(("   Found a Fragment!\n"));
		status = reassemble_fragments(header, &buffer);
		TRACE(("   -> %s!\n", strerror(status)));
		if (status != B_OK)
			return status;

		if (buffer == NULL) {
			// buffer was put into fragment packet
			TRACE(("   Not yet assembled...\n"));
			return B_OK;
		}
	}

	// Since the buffer might have been changed (reassembled fragment)
	// we must no longer access bufferHeader or header anymore after
	// this point

	if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP) {
		// SOCK_RAW doesn't get all packets
		raw_receive_data(buffer);
	}

	gBufferModule->remove_header(buffer, headerLength);
		// the header is of variable size and may include IP options
		// (that we ignore for now)

	// TODO: since we'll doing this for every packet, we may want to cache the module
	//	(and only put them when we're about to be unloaded)
	net_protocol_module_info *module;
	status = sStackModule->get_domain_receiving_protocol(sDomain, protocol, &module);
	if (status < B_OK) {
		// no handler for this packet
		return status;
	}

	status = module->receive_data(buffer);
	sStackModule->put_domain_receiving_protocol(sDomain, protocol);

	return status;
}


status_t
ipv4_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
ipv4_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


//	#pragma mark -


status_t
init_ipv4()
{
	status_t status = get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);
	if (status < B_OK)
		return status;
	status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK)
		goto err1;
	status = get_module(NET_DATALINK_MODULE_NAME, (module_info **)&sDatalinkModule);
	if (status < B_OK)
		goto err2;

	sPacketID = (int32)system_time();

	status = benaphore_init(&sRawSocketsLock, "raw sockets");
	if (status < B_OK)
		goto err3;

	status = benaphore_init(&sFragmentLock, "IPv4 Fragments");
	if (status < B_OK)
		goto err4;

	sFragmentHash = hash_init(MAX_HASH_FRAGMENTS, FragmentPacket::NextOffset(),
		&FragmentPacket::Compare, &FragmentPacket::Hash);
	if (sFragmentHash == NULL)
		goto err5;

	new (&sRawSockets) RawSocketList;
		// static initializers do not work in the kernel,
		// so we have to do it here, manually
		// TODO: for modules, this shouldn't be required

	status = sStackModule->register_domain_protocols(AF_INET, SOCK_RAW, 0,
		"network/protocols/ipv4/v1", NULL);
	if (status < B_OK)
		goto err6;

	status = sStackModule->register_domain(AF_INET, "internet", &gIPv4Module,
		&gIPv4AddressModule, &sDomain);
	if (status < B_OK)
		goto err6;

	return B_OK;

err6:
	hash_uninit(sFragmentHash);
err5:
	benaphore_destroy(&sFragmentLock);
err4:
	benaphore_destroy(&sRawSocketsLock);
err3:
	put_module(NET_DATALINK_MODULE_NAME);
err2:
	put_module(NET_BUFFER_MODULE_NAME);
err1:
	put_module(NET_STACK_MODULE_NAME);
	return status;
}


status_t
uninit_ipv4()
{
	hash_uninit(sFragmentHash);

	benaphore_destroy(&sFragmentLock);
	benaphore_destroy(&sRawSocketsLock);

	sStackModule->unregister_domain(sDomain);
	put_module(NET_DATALINK_MODULE_NAME);
	put_module(NET_BUFFER_MODULE_NAME);
	put_module(NET_STACK_MODULE_NAME);
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
	ipv4_init_protocol,
	ipv4_uninit_protocol,
	ipv4_open,
	ipv4_close,
	ipv4_free,
	ipv4_connect,
	ipv4_accept,
	ipv4_control,
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
	ipv4_error,
	ipv4_error_reply,
};

module_info *modules[] = {
	(module_info *)&gIPv4Module,
	NULL
};
