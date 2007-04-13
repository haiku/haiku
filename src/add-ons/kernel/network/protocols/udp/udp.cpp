/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Hugo Santos, hugosantos@gmail.com
 */


#include <net_buffer.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/khash.h>

#include <KernelExport.h>

#include <NetBufferUtilities.h>
#include <NetUtilities.h>
#include <ProtocolUtilities.h>

#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <utility>

//#define TRACE_UDP
#ifdef TRACE_UDP
#	define TRACE_BLOCK(x) dump_block x
// do not remove the space before ', ##args' if you want this
// to compile with gcc 2.95
#	define TRACE_EP(format, args...)	dprintf("UDP [%llu] %p " format "\n", \
		system_time(), this , ##args)
#	define TRACE_EPM(format, args...)	dprintf("UDP [%llu] " format "\n", \
		system_time() , ##args)
#	define TRACE_DOMAIN(format, args...)	dprintf("UDP [%llu] (%d) " format \
		"\n", system_time(), Domain()->family , ##args)
#else
#	define TRACE_BLOCK(x)
#	define TRACE_EP(args...)	do { } while (0)
#	define TRACE_EPM(args...)	do { } while (0)
#	define TRACE_DOMAIN(args...)	do { } while (0)
#endif


struct udp_header {
	uint16 source_port;
	uint16 destination_port;
	uint16 udp_length;
	uint16 udp_checksum;
} _PACKED;


typedef NetBufferField<uint16, offsetof(udp_header, udp_checksum)>
	UDPChecksumField;

class UdpDomainSupport;

class UdpEndpoint : public net_protocol, public DatagramSocket<> {
public:
	UdpEndpoint(net_socket *socket);

	status_t				Bind(sockaddr *newAddr);
	status_t				Unbind(sockaddr *newAddr);
	status_t				Connect(const sockaddr *newAddr);

	status_t				Open();
	status_t				Close();
	status_t				Free();

	status_t				SendRoutedData(net_buffer *buffer, net_route *route);
	status_t				SendData(net_buffer *buffer);

	ssize_t					BytesAvailable();
	status_t				FetchData(size_t numBytes, uint32 flags,
								net_buffer **_buffer);

	status_t				StoreData(net_buffer *buffer);

	net_domain *			Domain() const
	{
		return socket->first_protocol->module->get_domain(socket->first_protocol);
	}

	net_address_module_info *AddressModule() const
	{
		return Domain()->address_module;
	}

	UdpDomainSupport		*DomainSupport() const { return fDomain; }

	UdpEndpoint				*hash_link;
								// link required by hash_table (see khash.h)
private:
	status_t				_Activate();
	status_t				_Deactivate();

	UdpDomainSupport		*fDomain;
	bool					fActive;
								// an active UdpEndpoint is part of the endpoint 
								// hash (and it is bound and optionally connected)
};


class UdpDomainSupport : public DoublyLinkedListLinkImpl<UdpDomainSupport> {
public:
	UdpDomainSupport(net_domain *domain);
	~UdpDomainSupport();

	status_t InitCheck() const;

	net_domain *Domain() const { return fDomain; }

	void Ref() { fEndpointCount++; }
	void Put() { fEndpointCount--; }

	bool IsEmpty() const { return fEndpointCount == 0; }

	status_t DemuxIncomingBuffer(net_buffer *buffer);
	status_t CheckBindRequest(sockaddr *address, int socketOptions);
	status_t ActivateEndpoint(UdpEndpoint *endpoint);
	status_t DeactivateEndpoint(UdpEndpoint *endpoint);

	uint16 GetEphemeralPort();

private:
	struct HashKey {
		HashKey() {}
		HashKey(net_address_module_info *module, const sockaddr *_local,
			const sockaddr *_peer)
			: address_module(module), local(_local), peer(_peer) {}

		net_address_module_info *address_module;
		const sockaddr *local;
		const sockaddr *peer;
	};

	UdpEndpoint *_FindActiveEndpoint(sockaddr *ourAddress,
		sockaddr *peerAddress);
	status_t _DemuxBroadcast(net_buffer *buffer);
	status_t _DemuxMulticast(net_buffer *buffer);
	status_t _DemuxUnicast(net_buffer *buffer);

	uint16 _GetNextEphemeral();

	static int _Compare(void *udpEndpoint, const void *_key);
	static uint32 _Hash(void *udpEndpoint, const void *key, uint32 range);

	net_address_module_info *AddressModule() const
	{
		return fDomain->address_module;
	}

	net_domain		*fDomain;
	uint16			fLastUsedEphemeral;
	hash_table		*fActiveEndpoints;
	uint32			fEndpointCount;

	static const uint16		kFirst = 49152;
	static const uint16		kLast = 65535;
	static const uint32		kNumHashBuckets = 0x800;
							// if you change this, adjust the shifting in
							// Hash() accordingly!
};


typedef DoublyLinkedList<UdpDomainSupport> UdpDomainList;


class UdpEndpointManager {
public:
	UdpEndpointManager();
	~UdpEndpointManager();

	status_t		ReceiveData(net_buffer *buffer);

	UdpDomainSupport *OpenEndpoint(UdpEndpoint *endpoint);
	status_t FreeEndpoint(UdpDomainSupport *domain);

	uint16			GetEphemeralPort();

	benaphore		*Locker() { return &fLock; }
	status_t		InitCheck() const;

private:
	UdpDomainSupport *_GetDomain(net_domain *domain, bool create);

	benaphore		fLock;
	status_t		fStatus;
	UdpDomainList	fDomains;
};


static UdpEndpointManager *sUdpEndpointManager;

net_buffer_module_info *gBufferModule;
net_datalink_module_info *gDatalinkModule;
net_stack_module_info *gStackModule;


// #pragma mark -


UdpDomainSupport::UdpDomainSupport(net_domain *domain)
	:
	fDomain(domain),
	fLastUsedEphemeral(kLast),
	fEndpointCount(0)
{
	fActiveEndpoints = hash_init(kNumHashBuckets, offsetof(UdpEndpoint, hash_link),
		&_Compare, &_Hash);
}


UdpDomainSupport::~UdpDomainSupport()
{
	if (fActiveEndpoints)
		hash_uninit(fActiveEndpoints);
}


status_t
UdpDomainSupport::InitCheck() const
{
	return fActiveEndpoints ? B_OK : B_NO_MEMORY;
}


status_t
UdpDomainSupport::DemuxIncomingBuffer(net_buffer *buffer)
{
	if (buffer->flags & MSG_BCAST)
		return _DemuxBroadcast(buffer);
	else if (buffer->flags & MSG_MCAST)
		return _DemuxMulticast(buffer);

	return _DemuxUnicast(buffer);
}


status_t
UdpDomainSupport::CheckBindRequest(sockaddr *address, int socketOptions)
{		// sUdpEndpointManager->Locker() must be locked!
	status_t status = B_OK;
	UdpEndpoint *otherEndpoint;
	sockaddr *otherAddr;
	struct hash_iterator endpointIterator;

	// Iterate over all active UDP-endpoints and check if the requested bind
	// is allowed (see figure 22.24 in [Stevens - TCP2, p735]):
	hash_open(fActiveEndpoints, &endpointIterator);
	TRACE_DOMAIN("CheckBindRequest() for %s...",
		AddressString(fDomain, address, true).Data());
	while(1) {
		otherEndpoint = (UdpEndpoint *)hash_next(fActiveEndpoints, &endpointIterator);
		if (!otherEndpoint)
			break;
		otherAddr = (sockaddr *)&otherEndpoint->socket->address;
		TRACE_DOMAIN("  ...checking endpoint %p (port=%u)...", otherEndpoint,
			ntohs(AddressModule()->get_port(otherAddr)));
		if (AddressModule()->equal_ports(otherAddr, address)) {
			// port is already bound, SO_REUSEADDR or SO_REUSEPORT is required:
			if (otherEndpoint->socket->options & (SO_REUSEADDR | SO_REUSEPORT) == 0
				|| socketOptions & (SO_REUSEADDR | SO_REUSEPORT) == 0) {
				status = EADDRINUSE;
				break;
			}
			// if both addresses are the same, SO_REUSEPORT is required:
			if (AddressModule()->equal_addresses(otherAddr, address)
				&& (otherEndpoint->socket->options & SO_REUSEPORT == 0
					|| socketOptions & SO_REUSEPORT == 0)) {
				status = EADDRINUSE;
				break;
			}
		}
	}
	hash_close(fActiveEndpoints, &endpointIterator, false);

	TRACE_DOMAIN("  CheckBindRequest done (status=%lx)", status);
	return status;
}


status_t
UdpDomainSupport::ActivateEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	TRACE_DOMAIN("Endpoint(%s) was activated",
		AddressString(fDomain, (sockaddr *)&endpoint->socket->address, true).Data());
	return hash_insert(fActiveEndpoints, endpoint);
}


status_t
UdpDomainSupport::DeactivateEndpoint(UdpEndpoint *endpoint)
{		// sUdpEndpointManager->Locker() must be locked!
	TRACE_DOMAIN("Endpoint(%s) was deactivated",
		AddressString(fDomain, (sockaddr *)&endpoint->socket->address, true).Data());
	return hash_remove(fActiveEndpoints, endpoint);
}


uint16
UdpDomainSupport::GetEphemeralPort()
{
	return _GetNextEphemeral();
}


UdpEndpoint *
UdpDomainSupport::_FindActiveEndpoint(sockaddr *ourAddress,
	sockaddr *peerAddress)
{
	TRACE_DOMAIN("finding Endpoint for %s -> %s",
		AddressString(fDomain, ourAddress, true).Data(),
		AddressString(fDomain, peerAddress, true).Data());

	HashKey key(AddressModule(), ourAddress, peerAddress);
	return (UdpEndpoint *)hash_lookup(fActiveEndpoints, &key);
}


status_t
UdpDomainSupport::_DemuxBroadcast(net_buffer *buffer)
{
	sockaddr *peerAddr = (sockaddr *)&buffer->source;
	sockaddr *broadcastAddr = (sockaddr *)&buffer->destination;
	sockaddr *mask = NULL;
	if (buffer->interface)
		mask = (sockaddr *)buffer->interface->mask;

	TRACE_DOMAIN("_DemuxBroadcast(%p)", buffer);

	uint16 incomingPort = AddressModule()->get_port(broadcastAddr);

	UdpEndpoint *endpoint;
	hash_iterator endpointIterator;
	hash_open(fActiveEndpoints, &endpointIterator);
	while ((endpoint = (UdpEndpoint *)hash_next(fActiveEndpoints,
		&endpointIterator)) != NULL) {
		sockaddr *addr = (sockaddr *)&endpoint->socket->address;
		TRACE_DOMAIN("  _DemuxBroadcast(): checking endpoint %s...",
			AddressString(fDomain, addr, true).Data());

		if (incomingPort != AddressModule()->get_port(addr)) {
			// ports don't match, so we do not dispatch to this endpoint...
			continue;
		}

		sockaddr *connectAddr = (sockaddr *)&endpoint->socket->peer;
		if (!AddressModule()->is_empty_address(connectAddr, true)) {
			// endpoint is connected to a specific destination, we check if
			// this datagram is from there:
			if (!AddressModule()->equal_addresses_and_ports(connectAddr, peerAddr)) {
				// no, datagram is from another peer, so we do not dispatch to
				// this endpoint...
				continue;
			}
		}

		if (AddressModule()->equal_masked_addresses(addr, broadcastAddr, mask)
			|| AddressModule()->is_empty_address(addr, false)) {
				// address matches, dispatch to this endpoint:
			endpoint->StoreData(buffer);
		}
	}
	hash_close(fActiveEndpoints, &endpointIterator, false);
	return B_OK;
}


status_t
UdpDomainSupport::_DemuxMulticast(net_buffer *buffer)
{	// TODO: implement!
	TRACE_DOMAIN("_DemuxMulticast(%p)", buffer);

	return B_ERROR;
}


status_t
UdpDomainSupport::_DemuxUnicast(net_buffer *buffer)
{
	struct sockaddr *peerAddr = (struct sockaddr *)&buffer->source;
	struct sockaddr *localAddr = (struct sockaddr *)&buffer->destination;

	TRACE_DOMAIN("_DemuxUnicast(%p)", buffer);

	UdpEndpoint *endpoint;
	// look for full (most special) match:
	endpoint = _FindActiveEndpoint(localAddr, peerAddr);
	if (!endpoint) {
		// look for endpoint matching local address & port:
		endpoint = _FindActiveEndpoint(localAddr, NULL);
		if (!endpoint) {
			// look for endpoint matching peer address & port and local port:
			sockaddr localPortAddr;
			AddressModule()->set_to_empty_address(&localPortAddr);
			uint16 localPort = AddressModule()->get_port(localAddr);
			AddressModule()->set_port(&localPortAddr, localPort);
			endpoint = _FindActiveEndpoint(&localPortAddr, peerAddr);
			if (!endpoint) {
				// last chance: look for endpoint matching local port only:
				endpoint = _FindActiveEndpoint(&localPortAddr, NULL);
			}
		}
	}

	if (!endpoint)
		return B_NAME_NOT_FOUND;

	endpoint->StoreData(buffer);
	return B_OK;
}


uint16
UdpDomainSupport::_GetNextEphemeral()
{
	uint16 stop, curr, ncurr;
	if (fLastUsedEphemeral < kLast) {
		stop = fLastUsedEphemeral;
		curr = fLastUsedEphemeral + 1;
	} else {
		stop = kLast;
		curr = kFirst;
	}

	TRACE_DOMAIN("_GetNextEphemeral()");
	// TODO: a free list could be used to avoid the impact of these
	//       two nested loops most of the time... let's see how bad this really is
	bool found = false;
	uint16 endpointPort;
	hash_iterator endpointIterator;
	hash_open(fActiveEndpoints, &endpointIterator);
	while(!found && curr != stop) {
		TRACE_DOMAIN("  _GetNextEphemeral(): trying port %hu...", curr);
		ncurr = htons(curr);
		hash_rewind(fActiveEndpoints, &endpointIterator);
		while (!found) {
			UdpEndpoint *endpoint = (UdpEndpoint *)hash_next(fActiveEndpoints,
				&endpointIterator);
			if (!endpoint) {
				found = true;
				break;
			}
			endpointPort = AddressModule()->get_port(
				(sockaddr *)&endpoint->socket->address);
			TRACE_DOMAIN("  _GetNextEphemeral(): checking endpoint %p (port %hu)...",
				endpoint, ntohs(endpointPort));
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
	hash_close(fActiveEndpoints, &endpointIterator, false);
	if (!found)
		return 0;
	TRACE_DOMAIN("  _GetNextEphemeral(): ...using port %hu", curr);
	fLastUsedEphemeral = curr;
	return curr;
}


int
UdpDomainSupport::_Compare(void *_udpEndpoint, const void *_key)
{
	struct UdpEndpoint *udpEndpoint = (UdpEndpoint*)_udpEndpoint;
	const HashKey *key = (const HashKey *)_key;

	sockaddr *ourAddr = (sockaddr *)&udpEndpoint->socket->address;
	sockaddr *peerAddr = (sockaddr *)&udpEndpoint->socket->peer;

	net_address_module_info *addressModule = key->address_module;

	if (addressModule->equal_addresses_and_ports(ourAddr, key->local)
		&& addressModule->equal_addresses_and_ports(peerAddr, key->peer))
		return 0;

	return 1;
}


uint32
UdpDomainSupport::_Hash(void *_udpEndpoint, const void *_key, uint32 range)
{
	const HashKey *key = (const HashKey *)_key;
	HashKey key_storage;
	uint32 hash;

	if (_udpEndpoint) {
		const UdpEndpoint *udpEndpoint = (const UdpEndpoint *)_udpEndpoint;
		key_storage = HashKey(udpEndpoint->DomainSupport()->AddressModule(),
			(const sockaddr *)&udpEndpoint->socket->address,
			(const sockaddr *)&udpEndpoint->socket->peer);
		key = &key_storage;
	}

	hash = key->address_module->hash_address_pair(key->local, key->peer);

	// move the bits into the relevant range (as defined by kNumHashBuckets):
	hash = (hash & 0x000007FF) ^ (hash & 0x003FF800) >> 11
			^ (hash & 0xFFC00000UL) >> 22;

	return hash % range;
}


// #pragma mark -


UdpEndpointManager::UdpEndpointManager()
{
	fStatus = benaphore_init(&fLock, "UDP endpoints");
}


UdpEndpointManager::~UdpEndpointManager()
{
	benaphore_destroy(&fLock);
}


status_t
UdpEndpointManager::InitCheck() const
{
	return fStatus;
}


// #pragma mark - inbound


status_t
UdpEndpointManager::ReceiveData(net_buffer *buffer)
{
	TRACE_EPM("ReceiveData(%p [%ld bytes])", buffer, buffer->size);

	NetBufferHeaderReader<udp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	udp_header &header = bufferHeader.Data();

	struct sockaddr *source = (struct sockaddr *)&buffer->source;
	struct sockaddr *destination = (struct sockaddr *)&buffer->destination;

	if (buffer->interface == NULL || buffer->interface->domain == NULL) {
		TRACE_EPM("  ReceiveData(): UDP packed dropped as there was no domain "
			"specified (interface %p).", buffer->interface);
		return B_BAD_VALUE;
	}

	net_domain *domain = buffer->interface->domain;
	net_address_module_info *addressModule = domain->address_module;

	BenaphoreLocker _(fLock);

	addressModule->set_port(source, header.source_port);
	addressModule->set_port(destination, header.destination_port);

	TRACE_EPM("  ReceiveData(): data from %s to %s",
		AddressString(domain, source, true).Data(),
		AddressString(domain, destination, true).Data());

	uint16 udpLength = ntohs(header.udp_length);
	if (udpLength > buffer->size) {
		TRACE_EPM("  ReceiveData(): buffer is too short, expected %hu.",
			udpLength);
		return B_MISMATCHED_VALUES;
	}

	if (buffer->size > udpLength)
		gBufferModule->trim(buffer, udpLength);

	if (header.udp_checksum != 0) {
		// check UDP-checksum (simulating a so-called "pseudo-header"):
		Checksum udpChecksum;
		addressModule->checksum_address(&udpChecksum, source);
		addressModule->checksum_address(&udpChecksum, destination);
		udpChecksum
			<< (uint16)htons(IPPROTO_UDP)
			<< header.udp_length
					// peculiar but correct: UDP-len is used twice for checksum
					// (as it is already contained in udp_header)
			<< Checksum::BufferHelper(buffer, gBufferModule);
		uint16 sum = udpChecksum;
		if (sum != 0) {
			TRACE_EPM("  ReceiveData(): bad checksum 0x%hx.", sum);
			return B_BAD_VALUE;
		}
	}

	bufferHeader.Remove();
		// remove UDP-header from buffer before passing it on

	UdpDomainSupport *domainSupport = _GetDomain(domain, false);
	if (domainSupport == NULL) {
		// we don't instantiate domain supports in the
		// RX path as we are only interested in delivering
		// data to existing sockets.
		return B_ERROR;
	}

	status_t status = domainSupport->DemuxIncomingBuffer(buffer);
	if (status < B_OK) {
		TRACE_EPM("  ReceiveData(): no endpoint.");
		// TODO: send ICMP-error
		return B_ERROR;
	}

	return B_ERROR;
}


UdpDomainSupport *
UdpEndpointManager::OpenEndpoint(UdpEndpoint *endpoint)
{
	BenaphoreLocker _(fLock);

	UdpDomainSupport *domain = _GetDomain(endpoint->Domain(), true);
	if (domain)
		domain->Ref();
	return domain;
}


status_t
UdpEndpointManager::FreeEndpoint(UdpDomainSupport *domain)
{
	BenaphoreLocker _(fLock);

	domain->Put();

	if (domain->IsEmpty()) {
		fDomains.Remove(domain);
		delete domain;
	}

	return B_OK;
}


// #pragma mark -


UdpDomainSupport *
UdpEndpointManager::_GetDomain(net_domain *domain, bool create)
{
	UdpDomainList::Iterator it = fDomains.GetIterator();

	// TODO convert this into a Hashtable or install per-domain
	//      receiver handlers that forward the requests to the
	//      appropriate DemuxIncomingBuffer(). For instance, while
	//      being constructed UdpDomainSupport could call
	//      register_domain_receiving_protocol() with the right
	//      family.
	while (it.HasNext()) {
		UdpDomainSupport *domainSupport = it.Next();
		if (domainSupport->Domain() == domain)
			return domainSupport;
	}

	if (!create)
		return NULL;

	UdpDomainSupport *domainSupport =
		new (std::nothrow) UdpDomainSupport(domain);
	if (domainSupport == NULL || domainSupport->InitCheck() < B_OK) {
		delete domainSupport;
		return NULL;
	}

	fDomains.Add(domainSupport);
	return domainSupport;
}


// #pragma mark -


UdpEndpoint::UdpEndpoint(net_socket *socket)
	:
	DatagramSocket<>("udp endpoint", socket),
	fDomain(NULL),
	fActive(false)
{
}


// #pragma mark - activation


status_t
UdpEndpoint::Bind(sockaddr *address)
{
	TRACE_EP("Bind(%s)", AddressString(Domain(), address, true).Data());

	// let the underlying protocol check whether there is an interface that
	// supports the given address
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	BenaphoreLocker locker(sUdpEndpointManager->Locker());

	if (fActive) {
		// socket module should have called unbind() before!
		return EINVAL;
	}

	if (AddressModule()->get_port(address) == 0) {
		uint16 port = htons(fDomain->GetEphemeralPort());
		if (port == 0)
			return ENOBUFS;
				// whoa, no more ephemeral port available!?!
		AddressModule()->set_port((sockaddr *)&socket->address, port);
	} else {
		status = fDomain->CheckBindRequest((sockaddr *)&socket->address,
			socket->options);
		if (status < B_OK)
			return status;
	}

	return _Activate();
}


status_t
UdpEndpoint::Unbind(sockaddr *address)
{
	TRACE_EP("Unbind()");

	BenaphoreLocker locker(sUdpEndpointManager->Locker());

	return _Deactivate();
}


status_t
UdpEndpoint::Connect(const sockaddr *address)
{
	TRACE_EP("Connect(%s)", AddressString(Domain(), address, true).Data());

	BenaphoreLocker locker(sUdpEndpointManager->Locker());

	if (fActive)
		_Deactivate();

	if (address->sa_family == AF_UNSPEC) {
		// [Stevens-UNP1, p226]: specifying AF_UNSPEC requests a "disconnect",
		// so we reset the peer address:
		AddressModule()->set_to_empty_address((sockaddr *)&socket->peer);
	} else {
		// TODO check if `address' is compatible with AddressModule()
		AddressModule()->set_to((sockaddr *)&socket->peer, address);
	}

	// we need to activate no matter whether or not we have just disconnected,
	// as calling connect() always triggers an implicit bind():
	return _Activate();
}


status_t
UdpEndpoint::Open()
{
	TRACE_EP("Open()");

	// we can't be the first protocol, there must an underlying
	// network protocol that supplies us with an address module.
	if (socket->first_protocol == NULL)
		return EAFNOSUPPORT;

	if (Domain() == NULL || Domain()->address_module == NULL)
		return EAFNOSUPPORT;

	fDomain = sUdpEndpointManager->OpenEndpoint(this);

	if (fDomain == NULL)
		return EAFNOSUPPORT;

	return B_OK;
}


status_t
UdpEndpoint::Close()
{
	TRACE_EP("Close()");

	BenaphoreLocker _(sUdpEndpointManager->Locker());
	if (fActive)
		_Deactivate();

	return B_OK;
}


status_t
UdpEndpoint::Free()
{
	TRACE_EP("Free()");

	return sUdpEndpointManager->FreeEndpoint(fDomain);
}


status_t
UdpEndpoint::_Activate()
{
	if (fActive)
		return B_ERROR;
	status_t status = fDomain->ActivateEndpoint(this);
	fActive = (status == B_OK);
	return status;
}


status_t
UdpEndpoint::_Deactivate()
{
	if (!fActive)
		return B_ERROR;
	fActive = false;
	return fDomain->DeactivateEndpoint(this);
}


// #pragma mark - outbound


status_t
UdpEndpoint::SendRoutedData(net_buffer *buffer, net_route *route)
{
	TRACE_EP("SendRoutedData(%p [%lu bytes], %p)", buffer, buffer->size, route);

	if (buffer->size > (0xffff - sizeof(udp_header)))
		return EMSGSIZE;

	buffer->protocol = IPPROTO_UDP;

	// add and fill UDP-specific header:
	NetBufferPrepend<udp_header> header(buffer);
	if (header.Status() < B_OK)
		return header.Status();

	header->source_port = AddressModule()->get_port((sockaddr *)&buffer->source);
	header->destination_port = AddressModule()->get_port(
		(sockaddr *)&buffer->destination);
	header->udp_length = htons(buffer->size);
		// the udp-header is already included in the buffer-size
	header->udp_checksum = 0;

	header.Sync();

	// generate UDP-checksum (simulating a so-called "pseudo-header"):
	Checksum udpChecksum;
	AddressModule()->checksum_address(&udpChecksum,
		(sockaddr *)route->interface->address);
	AddressModule()->checksum_address(&udpChecksum,
		(sockaddr *)&buffer->destination);
	udpChecksum
		<< (uint16)htons(IPPROTO_UDP)
		<< (uint16)htons(buffer->size)
				// peculiar but correct: UDP-len is used twice for checksum
				// (as it is already contained in udp_header)
		<< Checksum::BufferHelper(buffer, gBufferModule);

	uint16 calculatedChecksum = udpChecksum;
	if (calculatedChecksum == 0)
		calculatedChecksum = 0xffff;

	*UDPChecksumField(buffer) = calculatedChecksum;

	TRACE_BLOCK(((char*)&header, sizeof(udp_header), "udp-hdr: "));

	return next->module->send_routed_data(next, route, buffer);
}


status_t
UdpEndpoint::SendData(net_buffer *buffer)
{
	TRACE_EP("SendData(%p [%lu bytes])", buffer, buffer->size);

	net_route *route = NULL;
	status_t status = gDatalinkModule->get_buffer_route(Domain(),
		buffer,	&route);
	if (status >= B_OK) {
		status = SendRoutedData(buffer, route);
		gDatalinkModule->put_route(Domain(), route);
	}

	return status;
}


// #pragma mark - inbound


ssize_t
UdpEndpoint::BytesAvailable()
{
	size_t bytes = AvailableData();
	TRACE_EP("BytesAvailable(): %lu", bytes);
	return bytes;
}


status_t
UdpEndpoint::FetchData(size_t numBytes, uint32 flags, net_buffer **_buffer)
{
	TRACE_EP("FetchData(%ld, 0x%lx)", numBytes, flags);

	status_t status = SocketDequeue(flags, _buffer);
	TRACE_EP("  FetchData(): returned from fifo status=0x%lx", status);
	if (status < B_OK)
		return status;

	TRACE_EP("  FetchData(): returns buffer with %ld bytes", (*_buffer)->size);
	return B_OK;
}


status_t
UdpEndpoint::StoreData(net_buffer *buffer)
{
	TRACE_EP("StoreData(%p [%ld bytes])", buffer, buffer->size);

	return SocketEnqueue(buffer);
}


// #pragma mark - protocol interface


net_protocol *
udp_init_protocol(net_socket *socket)
{
	socket->protocol = IPPROTO_UDP;

	UdpEndpoint *endpoint = new (std::nothrow) UdpEndpoint(socket);
	if (endpoint == NULL || endpoint->InitCheck() < B_OK) {
		delete endpoint;
		return NULL;
	}

	return endpoint;
}


status_t
udp_uninit_protocol(net_protocol *protocol)
{
	delete (UdpEndpoint *)protocol;
	return B_OK;
}


status_t
udp_open(net_protocol *protocol)
{
	return ((UdpEndpoint *)protocol)->Open();
}


status_t
udp_close(net_protocol *protocol)
{
	return ((UdpEndpoint *)protocol)->Close();
}


status_t
udp_free(net_protocol *protocol)
{
	return ((UdpEndpoint *)protocol)->Free();
}


status_t
udp_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return ((UdpEndpoint *)protocol)->Connect(address);
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
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
udp_bind(net_protocol *protocol, struct sockaddr *address)
{
	return ((UdpEndpoint *)protocol)->Bind(address);
}


status_t
udp_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return ((UdpEndpoint *)protocol)->Unbind(address);
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
	return ((UdpEndpoint *)protocol)->SendRoutedData(buffer, route);
}


status_t
udp_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return ((UdpEndpoint *)protocol)->SendData(buffer);
}


ssize_t
udp_send_avail(net_protocol *protocol)
{
	return protocol->socket->send.buffer_size;
}


status_t
udp_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return ((UdpEndpoint *)protocol)->FetchData(numBytes, flags, _buffer);
}


ssize_t
udp_read_avail(net_protocol *protocol)
{
	return ((UdpEndpoint *)protocol)->BytesAvailable();
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
	TRACE_EPM("init_udp()");

	sUdpEndpointManager = new (std::nothrow) UdpEndpointManager;
	if (sUdpEndpointManager == NULL)
		return B_NO_MEMORY;

	status = sUdpEndpointManager->InitCheck();
	if (status != B_OK)
		goto err1;

	status = gStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM, IPPROTO_IP,
		"network/protocols/udp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err1;
	status = gStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
		"network/protocols/udp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err1;

	status = gStackModule->register_domain_receiving_protocol(AF_INET, IPPROTO_UDP,
		"network/protocols/udp/v1");
	if (status < B_OK)
		goto err1;

	return B_OK;

err1:
	delete sUdpEndpointManager;

	TRACE_EPM("init_udp() fails with %lx (%s)", status, strerror(status));
	return status;
}


static status_t
uninit_udp()
{
	TRACE_EPM("uninit_udp()");
	delete sUdpEndpointManager;
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

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
	{NET_DATALINK_MODULE_NAME, (module_info **)&gDatalinkModule},
	{}
};

module_info *modules[] = {
	(module_info *)&sUDPModule,
	NULL
};
