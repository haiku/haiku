/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <net_buffer.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/khash.h>

#include <KernelExport.h>

#include <NetBufferUtilities.h>
#include <NetUtilities.h>

#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>


#define TRACE_UDP
#ifdef TRACE_UDP
#	define TRACE(x) dprintf x
#	define TRACE_BLOCK(x) dump_block x
#else
#	define TRACE(x)
#	define TRACE_BLOCK(x)
#endif


struct udp_header {
	uint16 source_port;
	uint16 destination_port;
	uint16 udp_length;
	uint16 udp_checksum;
} _PACKED;


class UdpEndpoint : public net_protocol {
public:
	UdpEndpoint(net_socket *socket);
	~UdpEndpoint();

	status_t				Bind(sockaddr *newAddr);
	status_t				Unbind(sockaddr *newAddr);
	status_t				Connect(const sockaddr *newAddr);

	status_t				Open();
	status_t				Close();
	status_t				Free();

	status_t				SendData(net_buffer *buffer, net_route *route);

	ssize_t					BytesAvailable();
	status_t				FetchData(size_t numBytes, uint32 flags, 
								net_buffer **_buffer);

	status_t				StoreData(net_buffer *buffer);

	UdpEndpoint 			*hash_link;
								// link required by hash_table (see khash.h)
private:
	status_t				_Activate();
	status_t				_Deactivate();
	
	bool 					fActive;
								// an active UdpEndpoint is part of the endpoint 
								// hash (and it is bound and optionally connected)
	net_fifo				fFifo;
								// storage space for incoming data
};


class UdpEndpointManager {

	struct hash_key {
		hash_key(sockaddr *ourAddress, sockaddr *peerAddress);

		sockaddr ourAddress;
		sockaddr peerAddress;
	};
	
	class Ephemerals {
	public:
							Ephemerals();
							~Ephemerals();
		
			uint16			GetNext(hash_table *activeEndpoints);
	static 	const uint16	kFirst = 49152;
	static 	const uint16	kLast = 65535;
	private:
			uint16			fLastUsed;
	};
	
public:
	UdpEndpointManager();
	~UdpEndpointManager();

			status_t		DemuxBroadcast(net_buffer *buffer);
			status_t		DemuxMulticast(net_buffer *buffer);
			status_t		DemuxUnicast(net_buffer *buffer);
			status_t		DemuxIncomingBuffer(net_buffer *buffer);
			status_t		ReceiveData(net_buffer *buffer);

	static	int				Compare(void *udpEndpoint, const void *_key);
	static	uint32			ComputeHash(sockaddr *ourAddress, sockaddr *peerAddress);
	static	uint32			Hash(void *udpEndpoint, const void *key, uint32 range);

			UdpEndpoint		*FindActiveEndpoint(sockaddr *ourAddress, 
								sockaddr *peerAddress);
			status_t		CheckBindRequest(sockaddr *address, int socketOptions);

			status_t		ActivateEndpoint(UdpEndpoint *endpoint);
			status_t		DeactivateEndpoint(UdpEndpoint *endpoint);

			status_t		OpenEndpoint(UdpEndpoint *endpoint);
			status_t		CloseEndpoint(UdpEndpoint *endpoint);
			status_t		FreeEndpoint(UdpEndpoint *endpoint);
	
			uint16			GetEphemeralPort();

			benaphore		*Locker();
			status_t		InitCheck() const;
private:
			benaphore		fLock;
			hash_table		*fActiveEndpoints;
	static	const uint32 	kNumHashBuckets = 0x800;
								// if you change this, adjust the shifting in 
								// Hash() accordingly!
			Ephemerals		fEphemerals;
			status_t		fStatus;
			uint32			fEndpointCount;
};


static UdpEndpointManager *sUdpEndpointManager;

static net_domain *sDomain;

static net_address_module_info *sAddressModule;
net_buffer_module_info *sBufferModule;
static net_datalink_module_info *sDatalinkModule;
static net_stack_module_info *sStackModule;


// #pragma mark -


UdpEndpointManager::hash_key::hash_key(sockaddr *_ourAddress, sockaddr *_peerAddress)
{
	memcpy(&ourAddress, _ourAddress, sizeof(sockaddr));
	memcpy(&peerAddress, _peerAddress, sizeof(sockaddr));
}


// #pragma mark -


UdpEndpointManager::Ephemerals::Ephemerals()
	:
	fLastUsed(kLast)
{
}


UdpEndpointManager::Ephemerals::~Ephemerals()
{
}


uint16
UdpEndpointManager::Ephemerals::GetNext(hash_table *activeEndpoints)
{
	uint16 stop, curr, ncurr;
	if (fLastUsed < kLast) {
		stop = fLastUsed;
		curr = fLastUsed + 1;
	} else {
		stop = kLast;
		curr = kFirst;
	}

	TRACE(("UdpEndpointManager::Ephemerals::GetNext()...\n"));
	// TODO: a free list could be used to avoid the impact of these
	//       two nested loops most of the time... let's see how bad this really is
	UdpEndpoint *endpoint;
	struct hash_iterator endpointIterator;
	hash_open(activeEndpoints, &endpointIterator);
	bool found = false;
	uint16 endpointPort;
	while(!found && curr != stop) {
		TRACE(("...trying port %u...\n", curr));
		ncurr = htons(curr);
		for(hash_rewind(activeEndpoints, &endpointIterator); !found; ) {
			endpoint = (UdpEndpoint *)hash_next(activeEndpoints, &endpointIterator);
			if (!endpoint) {
				found = true;
				break;
			}
			endpointPort = sAddressModule->get_port(
				(sockaddr *)&endpoint->socket->address);
			TRACE(("...checking endpoint %p (port=%u)...\n", endpoint, 
				ntohs(endpointPort)));
			if (endpointPort == ncurr)
				break;
		}
		if (!found) {
			if (curr < kLast)
				curr++;
			else
				curr = kFirst;
		}
	}
	hash_close(activeEndpoints, &endpointIterator, false);
	if (!found)
		return 0;
	TRACE(("...using port %u\n", curr));
	fLastUsed = curr;
	return curr;
}


// #pragma mark -


UdpEndpointManager::UdpEndpointManager()
	:
	fStatus(B_NO_INIT),
	fEndpointCount(0)
{
	fActiveEndpoints = hash_init(kNumHashBuckets, offsetof(UdpEndpoint, hash_link), 
		&Compare, &Hash);
	if (fActiveEndpoints == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = benaphore_init(&fLock, "UDP endpoints");
	if (fStatus < B_OK)
		hash_uninit(fActiveEndpoints);
}


UdpEndpointManager::~UdpEndpointManager()
{
	benaphore_destroy(&fLock);
	hash_uninit(fActiveEndpoints);
}


inline benaphore *
UdpEndpointManager::Locker()
{
	return &fLock;
}


inline status_t
UdpEndpointManager::InitCheck() const
{
	return fStatus;
}


// #pragma mark - hashing


/*static*/ int
UdpEndpointManager::Compare(void *_udpEndpoint, const void *_key)
{
	struct UdpEndpoint *udpEndpoint = (UdpEndpoint*)_udpEndpoint;
	hash_key *key = (hash_key *)_key;

	sockaddr *ourAddr = (sockaddr *)&udpEndpoint->socket->address;
	sockaddr *peerAddr = (sockaddr *)&udpEndpoint->socket->peer;

	if (sAddressModule->equal_addresses_and_ports(ourAddr, &key->ourAddress)
		&& sAddressModule->equal_addresses_and_ports(peerAddr, &key->peerAddress))
		return 0;

	return 1;
}


/*static*/ inline uint32
UdpEndpointManager::ComputeHash(sockaddr *ourAddress, sockaddr *peerAddress)
{
	return sAddressModule->hash_address_pair(ourAddress, peerAddress);
}


/*static*/ uint32
UdpEndpointManager::Hash(void *_udpEndpoint, const void *_key, uint32 range)
{
	uint32 hash;

	if (_udpEndpoint) {
		struct UdpEndpoint *udpEndpoint = (UdpEndpoint*)_udpEndpoint;
		sockaddr *ourAddr = (sockaddr*)&udpEndpoint->socket->address;
		sockaddr *peerAddr = (sockaddr*)&udpEndpoint->socket->peer;
		hash = ComputeHash(ourAddr, peerAddr);
	} else {
		hash_key *key = (hash_key *)_key;
		hash = ComputeHash(&key->ourAddress, &key->peerAddress);
	}

	// move the bits into the relevant range (as defined by kNumHashBuckets):
	hash = (hash & 0x000007FF) ^ (hash & 0x003FF800) >> 11
			^ (hash & 0xFFC00000UL) >> 22;

	TRACE(("UDP-endpoint hash is %lx\n", hash % range));
	return hash % range;
}


// #pragma mark - inbound


UdpEndpoint *
UdpEndpointManager::FindActiveEndpoint(sockaddr *ourAddress,
	sockaddr *peerAddress)
{
	TRACE(("trying to find UDP-endpoint for (l:%s p:%s)\n",
		AddressString(sDomain, ourAddress, true).Data(),
		AddressString(sDomain, peerAddress, true).Data()));
	hash_key key(ourAddress, peerAddress);
	UdpEndpoint *endpoint = (UdpEndpoint *)hash_lookup(fActiveEndpoints, &key);
	return endpoint;
}


status_t
UdpEndpointManager::DemuxBroadcast(net_buffer *buffer)
{
	sockaddr *peerAddr = (sockaddr *)&buffer->source;
	sockaddr *broadcastAddr = (sockaddr *)&buffer->destination;
	sockaddr *mask = NULL;
	if (buffer->interface)
		mask = (sockaddr *)buffer->interface->mask;

	TRACE(("demuxing buffer %p as broadcast...\n", buffer));

	sockaddr anyAddr;
	sAddressModule->set_to_empty_address(&anyAddr);
	
	uint16 incomingPort = sAddressModule->get_port(broadcastAddr);

	UdpEndpoint *endpoint;
	sockaddr *addr, *connectAddr;
	struct hash_iterator endpointIterator;
	for(hash_open(fActiveEndpoints, &endpointIterator); ; ) {
		endpoint = (UdpEndpoint *)hash_next(fActiveEndpoints, &endpointIterator);
		if (!endpoint)
			break;

		addr = (sockaddr *)&endpoint->socket->address;
		TRACE(("UDP-DemuxBroadcast() is checking endpoint %s...\n",
			AddressString(sDomain, addr, true).Data()));

		if (incomingPort != sAddressModule->get_port(addr)) {
			// ports don't match, so we do not dispatch to this endpoint...
			continue;
		}

		connectAddr = (sockaddr *)&endpoint->socket->peer;
		if (!sAddressModule->is_empty_address(connectAddr, true)) {
			// endpoint is connected to a specific destination, we check if
			// this datagram is from there:
			if (!sAddressModule->equal_addresses_and_ports(connectAddr, peerAddr)) {
				// no, datagram is from another peer, so we do not dispatch to
				// this endpoint...
				continue;
			}
		}

		if (sAddressModule->equal_masked_addresses(addr, broadcastAddr, mask)
			|| sAddressModule->equal_addresses(addr, &anyAddr)) {
				// address matches, dispatch to this endpoint:
			endpoint->StoreData(buffer);
		}
	}
	hash_close(fActiveEndpoints, &endpointIterator, false);
	return B_OK;
}


status_t
UdpEndpointManager::DemuxMulticast(net_buffer *buffer)
{	// TODO: implement!
	return B_ERROR;
}


status_t
UdpEndpointManager::DemuxUnicast(net_buffer *buffer)
{
	struct sockaddr *peerAddr = (struct sockaddr *)&buffer->source;
	struct sockaddr *localAddr = (struct sockaddr *)&buffer->destination;

	TRACE(("demuxing buffer %p as unicast...\n", buffer));

	struct sockaddr anyAddr;
	sAddressModule->set_to_empty_address(&anyAddr);

	UdpEndpoint *endpoint;
	// look for full (most special) match:
	endpoint = FindActiveEndpoint(localAddr, peerAddr);
	if (!endpoint) {
		// look for endpoint matching local address & port:
		endpoint = FindActiveEndpoint(localAddr, &anyAddr);
		if (!endpoint) {
			// look for endpoint matching peer address & port and local port:
			sockaddr localPortAddr;
			sAddressModule->set_to_empty_address(&localPortAddr);
			uint16 localPort = sAddressModule->get_port(localAddr);
			sAddressModule->set_port(&localPortAddr, localPort);
			endpoint = FindActiveEndpoint(&localPortAddr, peerAddr);
			if (!endpoint) {
				// last chance: look for endpoint matching local port only:
				endpoint = FindActiveEndpoint(&localPortAddr, &anyAddr);
			}
		}
	}
	if (!endpoint)
		return B_NAME_NOT_FOUND;

	endpoint->StoreData(buffer);
	return B_OK;
}


status_t
UdpEndpointManager::DemuxIncomingBuffer(net_buffer *buffer)
{
	status_t status;

	if (buffer->flags & MSG_BCAST)
		status = DemuxBroadcast(buffer);
	else if (buffer->flags & MSG_MCAST)
		status = DemuxMulticast(buffer);
	else
		status = DemuxUnicast(buffer);

	return status;
}


status_t
UdpEndpointManager::ReceiveData(net_buffer *buffer)
{
	NetBufferHeader<udp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	udp_header &header = bufferHeader.Data();

	struct sockaddr *source = (struct sockaddr *)&buffer->source;
	struct sockaddr *destination = (struct sockaddr *)&buffer->destination;

	BenaphoreLocker locker(sUdpEndpointManager->Locker());
	if (!sDomain) {
		// domain and address module are not known yet, we copy them from
		// the buffer's interface (if any):
		if (buffer->interface == NULL || buffer->interface->domain == NULL)
			sDomain = sStackModule->get_domain(AF_INET);
		else
			sDomain = buffer->interface->domain;
		if (sDomain == NULL) {
			// this shouldn't occur, of course, but who knows...
			return B_BAD_VALUE;
		}
		sAddressModule = sDomain->address_module;
	}
	sAddressModule->set_port(source, header.source_port);
	sAddressModule->set_port(destination, header.destination_port);
	TRACE(("UDP received data from source %s for destination %s\n",
		AddressString(sDomain, source, true).Data(),
		AddressString(sDomain, destination, true).Data()));

	uint16 udpLength = ntohs(header.udp_length);
	if (udpLength > buffer->size) {
		TRACE(("buffer %p is too short (%lu instead of %u), we drop it!\n", 
			buffer, buffer->size, udpLength));
		return B_MISMATCHED_VALUES;
	}
	if (buffer->size > udpLength) {
		TRACE(("buffer %p is too long (%lu instead of %u), trimming it.\n", 
			buffer, buffer->size, udpLength));
		sBufferModule->trim(buffer, udpLength);
	}

	if (header.udp_checksum != 0) {
		// check UDP-checksum (simulating a so-called "pseudo-header"):
		Checksum udpChecksum;
		sAddressModule->checksum_address(&udpChecksum, source);
		sAddressModule->checksum_address(&udpChecksum, destination);
		udpChecksum 
			<< (uint16)htons(IPPROTO_UDP)
			<< header.udp_length
					// peculiar but correct: UDP-len is used twice for checksum
					// (as it is already contained in udp_header)
			<< Checksum::BufferHelper(buffer, sBufferModule);
		uint16 sum = udpChecksum;
		if (sum != 0) {
			TRACE(("buffer %p has bad checksum (%u), we drop it!\n", buffer, sum));
			return B_BAD_VALUE;
		}
	}

	bufferHeader.Remove();
		// remove UDP-header from buffer before passing it on

	status_t status = DemuxIncomingBuffer(buffer);
	if (status < B_OK) {
		TRACE(("no matching endpoint found for buffer %p, we drop it!", buffer));
		// TODO: send ICMP-error
		return B_ERROR;
	}

	return B_ERROR;
}


// #pragma mark - activation


status_t
UdpEndpointManager::CheckBindRequest(sockaddr *address, int socketOptions)
{		// sUdpEndpointManager->Locker() must be locked!
	status_t status = B_OK;
	UdpEndpoint *otherEndpoint;
	sockaddr *otherAddr;
	struct hash_iterator endpointIterator;

	// Iterate over all active UDP-endpoints and check if the requested bind
	// is allowed (see figure 22.24 in [Stevens - TCP2, p735]):
	hash_open(fActiveEndpoints, &endpointIterator);
	TRACE(("UdpEndpointManager::CheckBindRequest() for %s...\n",
		AddressString(sDomain, address, true).Data()));
	while(1) {
		otherEndpoint = (UdpEndpoint *)hash_next(fActiveEndpoints, &endpointIterator);
		if (!otherEndpoint)
			break;
		otherAddr = (sockaddr *)&otherEndpoint->socket->address;
		TRACE(("...checking endpoint %p (port=%u)...\n", otherEndpoint, 
			ntohs(sAddressModule->get_port(otherAddr))));
		if (sAddressModule->equal_ports(otherAddr, address)) {
			// port is already bound, SO_REUSEADDR or SO_REUSEPORT is required:
			if (otherEndpoint->socket->options & (SO_REUSEADDR | SO_REUSEPORT) == 0
				|| socketOptions & (SO_REUSEADDR | SO_REUSEPORT) == 0) {
				status = EADDRINUSE;
				break;
			}
			// if both addresses are the same, SO_REUSEPORT is required:
			if (sAddressModule->equal_addresses(otherAddr, address)
				&& (otherEndpoint->socket->options & SO_REUSEPORT == 0
					|| socketOptions & SO_REUSEPORT == 0)) {
				status = EADDRINUSE;
				break;
			}
		}
	}
	hash_close(fActiveEndpoints, &endpointIterator, false);

	TRACE(("UdpEndpointManager::CheckBindRequest done (status=%lx)\n", status));
	return status;
}


status_t
UdpEndpointManager::ActivateEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	TRACE(("UDP-endpoint(%s) is activated\n", 
		AddressString(sDomain, (sockaddr *)&endpoint->socket->address, true).Data()));
	return hash_insert(fActiveEndpoints, endpoint);
}


status_t
UdpEndpointManager::DeactivateEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	TRACE(("UDP-endpoint(%s) is deactivated\n", 
		AddressString(sDomain, (sockaddr *)&endpoint->socket->address, true).Data()));
	return hash_remove(fActiveEndpoints, endpoint);
}


status_t
UdpEndpointManager::OpenEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	if (fEndpointCount++ == 0) {
		sDomain = sStackModule->get_domain(AF_INET);
		sAddressModule = sDomain->address_module;
		TRACE(("udp: setting domain-pointer to %p.\n", sDomain));
	}
	return B_OK;
}


status_t
UdpEndpointManager::CloseEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	return B_OK;
}


status_t
UdpEndpointManager::FreeEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	if (--fEndpointCount == 0) {
		TRACE(("udp: clearing domain-pointer and address-module.\n"));
		sDomain = NULL;
		sAddressModule = NULL;
	}
	return B_OK;
}


uint16
UdpEndpointManager::GetEphemeralPort()
{
	return fEphemerals.GetNext(fActiveEndpoints);
}


// #pragma mark -


UdpEndpoint::UdpEndpoint(net_socket *socket)
	:
	fActive(false)
{
	status_t status = sStackModule->init_fifo(&fFifo, "UDP endpoint fifo", 
		socket->receive.buffer_size);
	if (status < B_OK)
		fFifo.notify = status;
}


UdpEndpoint::~UdpEndpoint()
{
	if (fFifo.notify >= B_OK)
		sStackModule->uninit_fifo(&fFifo);
}


// #pragma mark - activation


status_t
UdpEndpoint::Bind(sockaddr *address)
{
	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	BenaphoreLocker locker(sUdpEndpointManager->Locker());

	if (fActive) {
		// socket module should have called unbind() before!
		return EINVAL;
	}
	
	if (sAddressModule->get_port(address) == 0) {
		uint16 port = htons(sUdpEndpointManager->GetEphemeralPort());
		if (port == 0)
			return ENOBUFS;
				// whoa, no more ephemeral port available!?!
		sAddressModule->set_port((sockaddr *)&socket->address, port);
	} else {
		status = sUdpEndpointManager->CheckBindRequest((sockaddr *)&socket->address, 
			socket->options);
		if (status < B_OK)
			return status;
	}

	return _Activate();
}


status_t
UdpEndpoint::Unbind(sockaddr *address)
{
	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	BenaphoreLocker locker(sUdpEndpointManager->Locker());

	return _Deactivate();
}


status_t
UdpEndpoint::Connect(const sockaddr *address)
{
	if (address->sa_family != AF_INET && address->sa_family != AF_UNSPEC)
		return EAFNOSUPPORT;

	BenaphoreLocker locker(sUdpEndpointManager->Locker());

	if (fActive)
		_Deactivate();

	if (address->sa_family == AF_UNSPEC) {
		// [Stevens-UNP1, p226]: specifying AF_UNSPEC requests a "disconnect",
		// so we reset the peer address:
		sAddressModule->set_to_empty_address((sockaddr *)&socket->peer);
	} else
		sAddressModule->set_to((sockaddr *)&socket->peer, address);

	// we need to activate no matter whether or not we have just disconnected,
	// as calling connect() always triggers an implicit bind():
	return _Activate();
}


status_t
UdpEndpoint::Open()
{
	BenaphoreLocker locker(sUdpEndpointManager->Locker());
	return sUdpEndpointManager->OpenEndpoint(this);
}


status_t
UdpEndpoint::Close()
{
	BenaphoreLocker locker(sUdpEndpointManager->Locker());
	if (fActive)
		_Deactivate();
	return sUdpEndpointManager->CloseEndpoint(this);
}


status_t
UdpEndpoint::Free()
{
	BenaphoreLocker locker(sUdpEndpointManager->Locker());
	return sUdpEndpointManager->FreeEndpoint(this);
}


status_t
UdpEndpoint::_Activate()
{
	if (fActive)
		return B_ERROR;
	status_t status = sUdpEndpointManager->ActivateEndpoint(this);
	fActive = (status == B_OK);
	return status;
}


status_t
UdpEndpoint::_Deactivate()
{
	if (!fActive)
		return B_ERROR;
	status_t status = sUdpEndpointManager->DeactivateEndpoint(this);
	fActive = false;
	return status;
}


// #pragma mark - outbound


status_t
UdpEndpoint::SendData(net_buffer *buffer, net_route *route)
{
	if (buffer->size > socket->send.buffer_size)
		return EMSGSIZE;
	
	buffer->protocol = IPPROTO_UDP;

	{	// scope for lifetime of bufferHeader

		// add and fill UDP-specific header:
		NetBufferPrepend<udp_header> bufferHeader(buffer);
		if (bufferHeader.Status() < B_OK)
			return bufferHeader.Status();
	
		udp_header &header = bufferHeader.Data();
	
		header.source_port = sAddressModule->get_port((sockaddr *)&buffer->source);
		header.destination_port = sAddressModule->get_port(
			(sockaddr *)&buffer->destination);
		header.udp_length = htons(buffer->size);
			// the udp-header is already included in the buffer-size
		header.udp_checksum = 0;

		// generate UDP-checksum (simulating a so-called "pseudo-header"):
		Checksum udpChecksum;
		sAddressModule->checksum_address(&udpChecksum, 
			(sockaddr *)route->interface->address);
		sAddressModule->checksum_address(&udpChecksum, 
			(sockaddr *)&buffer->destination);
		udpChecksum 
			<< (uint16)htons(IPPROTO_UDP)
			<< (uint16)htons(buffer->size)
					// peculiar but correct: UDP-len is used twice for checksum
					// (as it is already contained in udp_header)
			<< Checksum::BufferHelper(buffer, sBufferModule);
		header.udp_checksum = udpChecksum;
		if (header.udp_checksum == 0)
			header.udp_checksum = 0xFFFF;
	
		TRACE_BLOCK(((char*)&header, sizeof(udp_header), "udp-hdr: "));
	}
	return next->module->send_routed_data(next, route, buffer);
}


// #pragma mark - inbound


ssize_t
UdpEndpoint::BytesAvailable()
{
	return fFifo.current_bytes;
}


status_t
UdpEndpoint::FetchData(size_t numBytes, uint32 flags, net_buffer **_buffer)
{
	net_buffer *buffer;
	AddressString addressString(sDomain, (sockaddr *)&socket->address, true);
	TRACE(("FetchData() with size=%ld called for endpoint with (%s)\n",	
		numBytes, addressString.Data()));

	status_t status = sStackModule->fifo_dequeue_buffer(&fFifo,	flags,
		socket->receive.timeout, &buffer);
	TRACE(("Endpoint with (%s) returned from fifo status=%lx\n", 
		addressString.Data(), status));
	if (status < B_OK)
		return status;

	if (numBytes < buffer->size) {
		// discard any data behind the amount requested
		sBufferModule->trim(buffer, numBytes);
			// TODO: we should indicate MSG_TRUNC to application!
	}

	TRACE(("FetchData() returns buffer with %ld data bytes\n", buffer->size));
	*_buffer = buffer;
	return B_OK;
}


status_t
UdpEndpoint::StoreData(net_buffer *_buffer)
{
	TRACE(("buffer %p passed to endpoint with (%s)\n", _buffer,
		AddressString(sDomain, (sockaddr *)&socket->address, true).Data()));
	net_buffer *buffer = sBufferModule->clone(_buffer, false);
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = sStackModule->fifo_enqueue_buffer(&fFifo, buffer);
	if (status < B_OK)
		sBufferModule->free(buffer);

	return status;
}


// #pragma mark - protocol interface


net_protocol *
udp_init_protocol(net_socket *socket)
{
	socket->protocol = IPPROTO_UDP;
	socket->send.buffer_size = 65535 - 20 - 8;
		// subtract lengths of IP and UDP headers (NOTE: IP headers could be 
		// larger if IP options are used, but we do not currently care for that)

	UdpEndpoint *endpoint = new (std::nothrow) UdpEndpoint(socket);
	TRACE(("udp_init_protocol(%p) created endpoint %p\n", socket, endpoint));
	return endpoint;
}


status_t
udp_uninit_protocol(net_protocol *protocol)
{
	TRACE(("udp_uninit_protocol(%p)\n", protocol));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	delete udpEndpoint;
	return B_OK;
}


status_t
udp_open(net_protocol *protocol)
{
	TRACE(("udp_open(%p)\n", protocol));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->Open();
}


status_t
udp_close(net_protocol *protocol)
{
	TRACE(("udp_close(%p)\n", protocol));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->Close();
}


status_t
udp_free(net_protocol *protocol)
{
	TRACE(("udp_free(%p)\n", protocol));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->Free();
}


status_t
udp_connect(net_protocol *protocol, const struct sockaddr *address)
{
	TRACE(("udp_connect(%p) on address %s\n", protocol,
		AddressString(sDomain, address, true).Data()));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->Connect(address);
}


status_t
udp_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
udp_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	TRACE(("udp_control(%p)\n", protocol));
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
udp_bind(net_protocol *protocol, struct sockaddr *address)
{
	TRACE(("udp_bind(%p) on address %s\n", protocol,
		AddressString(sDomain, address, true).Data()));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->Bind(address);
}


status_t
udp_unbind(net_protocol *protocol, struct sockaddr *address)
{
	TRACE(("udp_unbind(%p) on address %s\n", protocol, 
		AddressString(sDomain, address, true).Data()));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->Unbind(address);
}


status_t
udp_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
udp_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
udp_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	TRACE(("udp_send_routed_data(%p) size=%lu\n", protocol, buffer->size));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->SendData(buffer, route);
}


status_t
udp_send_data(net_protocol *protocol, net_buffer *buffer)
{
	TRACE(("udp_send_data(%p) size=%lu\n", protocol, buffer->size));

	struct net_route *route = sDatalinkModule->get_route(sDomain,
		(sockaddr *)&buffer->destination);
	if (route == NULL)
		return ENETUNREACH;

	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	status_t status = udpEndpoint->SendData(buffer, route);
	sDatalinkModule->put_route(sDomain, route);
	return status;
}


ssize_t
udp_send_avail(net_protocol *protocol)
{
	ssize_t avail = protocol->socket->send.buffer_size;
	TRACE(("udp_send_avail(%p) result=%lu\n", protocol, avail));
	return avail;
}


status_t
udp_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	TRACE(("udp_read_data(%p) size=%lu flags=%lx\n", protocol, numBytes, flags));
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->FetchData(numBytes, flags, _buffer);
}


ssize_t
udp_read_avail(net_protocol *protocol)
{
	UdpEndpoint *udpEndpoint = (UdpEndpoint *)protocol;
	return udpEndpoint->BytesAvailable();
}


struct net_domain *
udp_get_domain(net_protocol *protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
udp_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


status_t
udp_receive_data(net_buffer *buffer)
{
	TRACE(("udp_receive_data() size=%lu\n", buffer->size));
	return sUdpEndpointManager->ReceiveData(buffer);
}


status_t
udp_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
udp_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


//	#pragma mark - module interface


static status_t
init_udp()
{
	status_t status;
	TRACE(("init_udp()\n"));

	status = get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);
	if (status < B_OK)
		return status;
	status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&sBufferModule);
	if (status < B_OK)
		goto err1;
	status = get_module(NET_DATALINK_MODULE_NAME, (module_info **)&sDatalinkModule);
	if (status < B_OK)
		goto err2;

	sUdpEndpointManager = new (std::nothrow) UdpEndpointManager;
	if (sUdpEndpointManager == NULL) {
		status = ENOBUFS;
		goto err3;
	}
	status = sUdpEndpointManager->InitCheck();
	if (status != B_OK)
		goto err3;

	status = sStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM, IPPROTO_IP,
		"network/protocols/udp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err4;
	status = sStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
		"network/protocols/udp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err4;

	status = sStackModule->register_domain_receiving_protocol(AF_INET, IPPROTO_UDP,
		"network/protocols/udp/v1");
	if (status < B_OK)
		goto err4;

	return B_OK;

err4:
	delete sUdpEndpointManager;
err3:
	put_module(NET_DATALINK_MODULE_NAME);
err2:
	put_module(NET_BUFFER_MODULE_NAME);
err1:
	put_module(NET_STACK_MODULE_NAME);

	TRACE(("init_udp() fails with %lx (%s)\n", status, strerror(status)));
	return status;
}


static status_t
uninit_udp()
{
	TRACE(("uninit_udp()\n"));
	delete sUdpEndpointManager;
	put_module(NET_DATALINK_MODULE_NAME);
	put_module(NET_BUFFER_MODULE_NAME);
	put_module(NET_STACK_MODULE_NAME);
	return B_OK;
}


static status_t
udp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_udp();

		case B_MODULE_UNINIT:
			return uninit_udp();

		default:
			return B_ERROR;
	}
}


net_protocol_module_info sUDPModule = {
	{
		"network/protocols/udp/v1",
		0,
		udp_std_ops
	},
	udp_init_protocol,
	udp_uninit_protocol,
	udp_open,
	udp_close,
	udp_free,
	udp_connect,
	udp_accept,
	udp_control,
	udp_bind,
	udp_unbind,
	udp_listen,
	udp_shutdown,
	udp_send_data,
	udp_send_routed_data,
	udp_send_avail,
	udp_read_data,
	udp_read_avail,
	udp_get_domain,
	udp_get_mtu,
	udp_receive_data,
	udp_error,
	udp_error_reply,
};

module_info *modules[] = {
	(module_info *)&sUDPModule,
	NULL
};
