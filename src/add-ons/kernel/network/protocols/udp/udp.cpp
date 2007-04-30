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
#include <util/OpenHashTable.h>

#include <KernelExport.h>

#include <NetBufferUtilities.h>
#include <NetUtilities.h>
#include <ProtocolUtilities.h>

#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <utility>


// NOTE the locking protocol dictates that we must hold UdpDomainSupport's
//      lock before holding a child UdpEndpoint's lock. This restriction
//      is dictated by the receive path as blind access to the endpoint
//      hash is required when holding the DomainSuppport's lock.


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

	status_t				Bind(const sockaddr *newAddr);
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
	status_t				DeliverData(net_buffer *buffer);

	// only the domain support will change/check the Active flag so
	// we don't really need to protect it with the socket lock.
	bool					IsActive() const { return fActive; }
	void					SetActive(bool newValue) { fActive = newValue; }

	HashTableLink<UdpEndpoint> *HashTableLink() { return &fLink; }

private:
	UdpDomainSupport		*fManager;
	bool					fActive;
								// an active UdpEndpoint is part of the endpoint 
								// hash (and it is bound and optionally connected)

	::HashTableLink<UdpEndpoint> fLink;
};


class UdpDomainSupport;

struct UdpHashDefinition {
	typedef net_address_module_info ParentType;
	typedef std::pair<const sockaddr *, const sockaddr *> KeyType;
	typedef UdpEndpoint ValueType;

	UdpHashDefinition(net_address_module_info *parent)
		: module(parent) {}

	size_t HashKey(const KeyType &key) const
	{
		return _Mix(module->hash_address_pair(key.first, key.second));
	}

	size_t Hash(UdpEndpoint *endpoint) const
	{
		return _Mix(endpoint->LocalAddress().HashPair(*endpoint->PeerAddress()));
	}

	static size_t _Mix(size_t hash)
	{
		// move the bits into the relevant range (as defined by kNumHashBuckets):
		return (hash & 0x000007FF) ^ (hash & 0x003FF800) >> 11
			^ (hash & 0xFFC00000UL) >> 22;
	}

	bool Compare(const KeyType &key, UdpEndpoint *endpoint) const
	{
		return endpoint->LocalAddress().EqualTo(key.first, true)
			&& endpoint->PeerAddress().EqualTo(key.second, true);
	}

	HashTableLink<UdpEndpoint> *GetLink(UdpEndpoint *endpoint) const
	{
		return endpoint->HashTableLink();
	}

	net_address_module_info *module;
};


class UdpDomainSupport : public DoublyLinkedListLinkImpl<UdpDomainSupport> {
public:
	UdpDomainSupport(net_domain *domain);
	~UdpDomainSupport();

	status_t InitCheck() const;

	net_domain *Domain() const { return fDomain; }

	void Ref() { fEndpointCount++; }
	bool Put() { fEndpointCount--; return fEndpointCount == 0; }

	status_t DemuxIncomingBuffer(net_buffer *buffer);

	status_t BindEndpoint(UdpEndpoint *endpoint, const sockaddr *address);
	status_t ConnectEndpoint(UdpEndpoint *endpoint, const sockaddr *address);
	status_t UnbindEndpoint(UdpEndpoint *endpoint);

	void DumpEndpoints() const;

private:
	status_t _BindEndpoint(UdpEndpoint *endpoint, const sockaddr *address);
	status_t _Bind(UdpEndpoint *endpoint, const sockaddr *address);
	status_t _BindToEphemeral(UdpEndpoint *endpoint, const sockaddr *address);
	status_t _FinishBind(UdpEndpoint *endpoint, const sockaddr *address);

	UdpEndpoint *_FindActiveEndpoint(const sockaddr *ourAddress,
		const sockaddr *peerAddress);
	status_t _DemuxBroadcast(net_buffer *buffer);
	status_t _DemuxUnicast(net_buffer *buffer);

	uint16 _GetNextEphemeral();
	UdpEndpoint *_EndpointWithPort(uint16 port) const;

	net_address_module_info *AddressModule() const
		{ return fDomain->address_module; }

	typedef OpenHashTable<UdpHashDefinition, false> EndpointTable;

	benaphore		fLock;
	net_domain		*fDomain;
	uint16			fLastUsedEphemeral;
	EndpointTable	fActiveEndpoints;
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
	status_t		Deframe(net_buffer *buffer);

	UdpDomainSupport *OpenEndpoint(UdpEndpoint *endpoint);
	status_t FreeEndpoint(UdpDomainSupport *domain);

	status_t		InitCheck() const;

	static int DumpEndpoints(int argc, char *argv[]);

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
	fActiveEndpoints(domain->address_module, kNumHashBuckets),
	fEndpointCount(0)
{
	benaphore_init(&fLock, "udp domain");

	fLastUsedEphemeral = kFirst + rand() % (kLast - kFirst);
}


UdpDomainSupport::~UdpDomainSupport()
{
	benaphore_destroy(&fLock);
}


status_t
UdpDomainSupport::InitCheck() const
{
	if (fLock.sem < B_OK)
		return fLock.sem;

	return fActiveEndpoints.InitCheck();
}


status_t
UdpDomainSupport::DemuxIncomingBuffer(net_buffer *buffer)
{
	// NOTE multicast is delivered directly to the endpoint

	BenaphoreLocker _(fLock);

	if (buffer->flags & MSG_BCAST)
		return _DemuxBroadcast(buffer);
	else if (buffer->flags & MSG_MCAST)
		return B_ERROR;

	return _DemuxUnicast(buffer);
}


status_t
UdpDomainSupport::BindEndpoint(UdpEndpoint *endpoint,
	const sockaddr *address)
{
	BenaphoreLocker _(fLock);

	if (endpoint->IsActive())
		return EINVAL;

	return _BindEndpoint(endpoint, address);
}


status_t
UdpDomainSupport::ConnectEndpoint(UdpEndpoint *endpoint,
	const sockaddr *address)
{
	BenaphoreLocker _(fLock);

	if (endpoint->IsActive()) {
		fActiveEndpoints.Remove(endpoint);
		endpoint->SetActive(false);
	}

	if (address->sa_family == AF_UNSPEC) {
		// [Stevens-UNP1, p226]: specifying AF_UNSPEC requests a "disconnect",
		// so we reset the peer address:
		endpoint->PeerAddress().SetToEmpty();
	} else {
		status_t status = endpoint->PeerAddress().SetTo(address);
		if (status < B_OK)
			return status;
	}

	// we need to activate no matter whether or not we have just disconnected,
	// as calling connect() always triggers an implicit bind():
	return _BindEndpoint(endpoint, *endpoint->LocalAddress());
}


status_t
UdpDomainSupport::UnbindEndpoint(UdpEndpoint *endpoint)
{
	BenaphoreLocker _(fLock);

	if (endpoint->IsActive())
		fActiveEndpoints.Remove(endpoint);

	endpoint->SetActive(false);

	return B_OK;
}


void
UdpDomainSupport::DumpEndpoints() const
{
	kprintf("-------- UDP Domain %p ---------\n", this);
	kprintf("%10s %20s %20s %8s\n", "address", "local", "peer", "recv-q");

	EndpointTable::Iterator it = fActiveEndpoints.GetIterator();

	while (it.HasNext()) {
		UdpEndpoint *endpoint = it.Next();

		char localBuf[64], peerBuf[64];
		endpoint->LocalAddress().AsString(localBuf, sizeof(localBuf), true);
		endpoint->PeerAddress().AsString(peerBuf, sizeof(peerBuf), true);

		kprintf("%p %20s %20s %8lu\n", endpoint, localBuf, peerBuf,
			endpoint->AvailableData());
	}
}


status_t
UdpDomainSupport::_BindEndpoint(UdpEndpoint *endpoint,
	const sockaddr *address)
{
	if (AddressModule()->get_port(address) == 0)
		return _BindToEphemeral(endpoint, address);

	return _Bind(endpoint, address);
}


status_t
UdpDomainSupport::_Bind(UdpEndpoint *endpoint, const sockaddr *address)
{
	int socketOptions = endpoint->Socket()->options;

	EndpointTable::Iterator it = fActiveEndpoints.GetIterator();

	// Iterate over all active UDP-endpoints and check if the requested bind
	// is allowed (see figure 22.24 in [Stevens - TCP2, p735]):
	TRACE_DOMAIN("CheckBindRequest() for %s...", AddressString(fDomain,
		address, true).Data());

	while (it.HasNext()) {
		UdpEndpoint *otherEndpoint = it.Next();

		TRACE_DOMAIN("  ...checking endpoint %p (port=%u)...", otherEndpoint,
			ntohs(otherEndpoint->LocalAddress().Port()));

		if (otherEndpoint->LocalAddress().EqualPorts(address)) {
			// port is already bound, SO_REUSEADDR or SO_REUSEPORT is required:
			if (otherEndpoint->Socket()->options & (SO_REUSEADDR | SO_REUSEPORT) == 0
				|| socketOptions & (SO_REUSEADDR | SO_REUSEPORT) == 0)
				return EADDRINUSE;

			// if both addresses are the same, SO_REUSEPORT is required:
			if (otherEndpoint->LocalAddress().EqualTo(address, false)
				&& (otherEndpoint->Socket()->options & SO_REUSEPORT == 0
					|| socketOptions & SO_REUSEPORT == 0))
				return EADDRINUSE;
		}
	}

	return _FinishBind(endpoint, address);
}


status_t
UdpDomainSupport::_BindToEphemeral(UdpEndpoint *endpoint,
	const sockaddr *address)
{
	SocketAddressStorage newAddress(AddressModule());
	status_t status = newAddress.SetTo(address);
	if (status < B_OK)
		return status;

	uint16 allocedPort = _GetNextEphemeral();
	if (allocedPort == 0)
		return ENOBUFS;

	newAddress.SetPort(allocedPort);

	return _FinishBind(endpoint, *newAddress);
}


status_t
UdpDomainSupport::_FinishBind(UdpEndpoint *endpoint, const sockaddr *address)
{
	status_t status = endpoint->next->module->bind(endpoint->next, address);
	if (status < B_OK)
		return status;

	fActiveEndpoints.Insert(endpoint);
	endpoint->SetActive(true);

	return B_OK;
}


UdpEndpoint *
UdpDomainSupport::_FindActiveEndpoint(const sockaddr *ourAddress,
	const sockaddr *peerAddress)
{
	TRACE_DOMAIN("finding Endpoint for %s -> %s",
		AddressString(fDomain, ourAddress, true).Data(),
		AddressString(fDomain, peerAddress, true).Data());

	return fActiveEndpoints.Lookup(std::make_pair(ourAddress, peerAddress));
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

	EndpointTable::Iterator it = fActiveEndpoints.GetIterator();

	while (it.HasNext()) {
		UdpEndpoint *endpoint = it.Next();

		TRACE_DOMAIN("  _DemuxBroadcast(): checking endpoint %s...",
			AddressString(fDomain, *endpoint->LocalAddress(), true).Data());

		if (endpoint->LocalAddress().Port() != incomingPort) {
			// ports don't match, so we do not dispatch to this endpoint...
			continue;
		}

		if (!endpoint->PeerAddress().IsEmpty(true)) {
			// endpoint is connected to a specific destination, we check if
			// this datagram is from there:
			if (!endpoint->PeerAddress().EqualTo(peerAddr, true)) {
				// no, datagram is from another peer, so we do not dispatch to
				// this endpoint...
				continue;
			}
		}

		if (endpoint->LocalAddress().MatchMasked(broadcastAddr, mask)
			|| endpoint->LocalAddress().IsEmpty(false)) {
			// address matches, dispatch to this endpoint:
			endpoint->StoreData(buffer);
		}
	}

	return B_OK;
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
			SocketAddressStorage local(AddressModule());
			local.SetToEmpty();
			local.SetPort(AddressModule()->get_port(localAddr));
			endpoint = _FindActiveEndpoint(*local, peerAddr);
			if (!endpoint) {
				// last chance: look for endpoint matching local port only:
				endpoint = _FindActiveEndpoint(*local, NULL);
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
	uint16 stop, curr;
	if (fLastUsedEphemeral < kLast) {
		stop = fLastUsedEphemeral;
		curr = fLastUsedEphemeral + 1;
	} else {
		stop = kLast;
		curr = kFirst;
	}

	// TODO: a free list could be used to avoid the impact of these
	//       two nested loops most of the time... let's see how bad this really is

	TRACE_DOMAIN("_GetNextEphemeral(), last %hu, curr %hu, stop %hu",
		fLastUsedEphemeral, curr, stop);

	for (; curr != stop; curr = (curr < kLast) ? (curr + 1) : kFirst) {
		TRACE_DOMAIN("  _GetNextEphemeral(): trying port %hu...", curr);

		if (_EndpointWithPort(htons(curr)) == NULL) {
			TRACE_DOMAIN("  _GetNextEphemeral(): ...using port %hu", curr);
			fLastUsedEphemeral = curr;
			return curr;
		}
	}

	return 0;
}


UdpEndpoint *
UdpDomainSupport::_EndpointWithPort(uint16 port) const
{
	EndpointTable::Iterator it = fActiveEndpoints.GetIterator();

	while (it.HasNext()) {
		UdpEndpoint *endpoint = it.Next();
		if (endpoint->LocalAddress().Port() == port)
			return endpoint;
	}

	return NULL;
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


int
UdpEndpointManager::DumpEndpoints(int argc, char *argv[])
{
	UdpDomainList::Iterator it = sUdpEndpointManager->fDomains.GetIterator();

	while (it.HasNext())
		it.Next()->DumpEndpoints();

	return 0;
}


// #pragma mark - inbound


status_t
UdpEndpointManager::ReceiveData(net_buffer *buffer)
{
	status_t status = Deframe(buffer);
	if (status < B_OK)
		return status;

	TRACE_EPM("ReceiveData(%p [%ld bytes])", buffer, buffer->size);

	net_domain *domain = buffer->interface->domain;

	UdpDomainSupport *domainSupport = NULL;

	{
		BenaphoreLocker _(fLock);
		domainSupport = _GetDomain(domain, false);
		// TODO we don't want to hold to the manager's lock
		//      during the whole RX path, we may not hold an
		//      endpoint's lock with the manager lock held.
		//      But we should increase the domain's refcount
		//      here.
	}

	if (domainSupport == NULL) {
		// we don't instantiate domain supports in the
		// RX path as we are only interested in delivering
		// data to existing sockets.
		return B_ERROR;
	}

	status = domainSupport->DemuxIncomingBuffer(buffer);
	if (status < B_OK) {
		TRACE_EPM("  ReceiveData(): no endpoint.");
		// TODO: send ICMP-error
		return B_ERROR;
	}

	return B_ERROR;
}


status_t
UdpEndpointManager::Deframe(net_buffer *buffer)
{
	TRACE_EPM("Deframe(%p [%ld bytes])", buffer, buffer->size);

	NetBufferHeaderReader<udp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	udp_header &header = bufferHeader.Data();

	if (buffer->interface == NULL || buffer->interface->domain == NULL) {
		TRACE_EPM("  Deframe(): UDP packed dropped as there was no domain "
			"specified (interface %p).", buffer->interface);
		return B_BAD_VALUE;
	}

	net_domain *domain = buffer->interface->domain;
	net_address_module_info *addressModule = domain->address_module;

	SocketAddress source(addressModule, &buffer->source);
	SocketAddress destination(addressModule, &buffer->destination);

	source.SetPort(header.source_port);
	destination.SetPort(header.destination_port);

	TRACE_EPM("  Deframe(): data from %s to %s", source.AsString(true).Data(),
		destination.AsString(true).Data());

	uint16 udpLength = ntohs(header.udp_length);
	if (udpLength > buffer->size) {
		TRACE_EPM("  Deframe(): buffer is too short, expected %hu.",
			udpLength);
		return B_MISMATCHED_VALUES;
	}

	if (buffer->size > udpLength)
		gBufferModule->trim(buffer, udpLength);

	if (header.udp_checksum != 0) {
		// check UDP-checksum (simulating a so-called "pseudo-header"):
		uint16 sum = Checksum::PseudoHeader(addressModule, gBufferModule,
			buffer, IPPROTO_UDP);
		if (sum != 0) {
			TRACE_EPM("  Deframe(): bad checksum 0x%hx.", sum);
			return B_BAD_VALUE;
		}
	}

	bufferHeader.Remove();
		// remove UDP-header from buffer before passing it on

	return B_OK;
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

	if (domain->Put()) {
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
	: DatagramSocket<>("udp endpoint", socket), fActive(false) {}


// #pragma mark - activation


status_t
UdpEndpoint::Bind(const sockaddr *address)
{
	TRACE_EP("Bind(%s)", AddressString(Domain(), address, true).Data());
	return fManager->BindEndpoint(this, address);
}


status_t
UdpEndpoint::Unbind(sockaddr *address)
{
	TRACE_EP("Unbind()");
	return fManager->UnbindEndpoint(this);
}


status_t
UdpEndpoint::Connect(const sockaddr *address)
{
	TRACE_EP("Connect(%s)", AddressString(Domain(), address, true).Data());
	return fManager->ConnectEndpoint(this, address);
}


status_t
UdpEndpoint::Open()
{
	TRACE_EP("Open()");

	BenaphoreLocker _(fLock);

	status_t status = ProtocolSocket::Open();
	if (status < B_OK)
		return status;

	fManager = sUdpEndpointManager->OpenEndpoint(this);
	if (fManager == NULL)
		return EAFNOSUPPORT;

	return B_OK;
}


status_t
UdpEndpoint::Close()
{
	TRACE_EP("Close()");
	return B_OK;
}


status_t
UdpEndpoint::Free()
{
	TRACE_EP("Free()");
	fManager->UnbindEndpoint(this);
	return sUdpEndpointManager->FreeEndpoint(fManager);
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

	uint16 calculatedChecksum = Checksum::PseudoHeader(AddressModule(),
		gBufferModule, buffer, IPPROTO_UDP);
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

	return gDatalinkModule->send_datagram(this, NULL, buffer);
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


status_t
UdpEndpoint::DeliverData(net_buffer *_buffer)
{
	net_buffer *buffer = gBufferModule->clone(_buffer, false);
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = sUdpEndpointManager->Deframe(buffer);
	if (status < B_OK) {
		gBufferModule->free(buffer);
		return status;
	}

	// we call Enqueue() instead of SocketEnqueue() as there is
	// no need to clone the buffer again.
	return Enqueue(buffer);
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
udp_bind(net_protocol *protocol, const struct sockaddr *address)
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
udp_deliver_data(net_protocol *protocol, net_buffer *buffer)
{
	return ((UdpEndpoint *)protocol)->DeliverData(buffer);
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

	add_debugger_command("udp_endpoints", UdpEndpointManager::DumpEndpoints,
		"lists all open UDP endpoints");

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
	remove_debugger_command("udp_endpoints",
		UdpEndpointManager::DumpEndpoints);
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
	NULL, // getsockopt
	NULL, // setsockopt
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
	udp_deliver_data,
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
