/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/UDP.h>

#include <stdio.h>

#include <OS.h>

#include <boot/net/ChainBuffer.h>
#include <boot/net/NetStack.h>


//#define TRACE_UDP
#ifdef TRACE_UDP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - UDPPacket

// constructor
UDPPacket::UDPPacket()
	: fNext(NULL),
		fData(NULL),
		fSize(0)
{
}

// destructor
UDPPacket::~UDPPacket()
{
	free(fData);
}

// SetTo
status_t
UDPPacket::SetTo(const void *data, size_t size, ip_addr_t sourceAddress,
	uint16 sourcePort, ip_addr_t destinationAddress, uint16 destinationPort)
{
	if (!data)
		return B_BAD_VALUE;

	// clone the data
	fData = malloc(size);
	if (!fData)
		return B_NO_MEMORY;
	memcpy(fData, data, size);

	fSize = size;
	fSourceAddress = sourceAddress;
	fDestinationAddress = destinationAddress;
	fSourcePort = sourcePort;
	fDestinationPort = destinationPort;

	return B_OK;
}

// Next
UDPPacket *
UDPPacket::Next() const
{
	return fNext;
}

// SetNext
void
UDPPacket::SetNext(UDPPacket *next)
{
	fNext = next;
}

// Data
const void *
UDPPacket::Data() const
{
	return fData;
}

// DataSize
size_t
UDPPacket::DataSize() const
{
	return fSize;
}

// SourceAddress
ip_addr_t
UDPPacket::SourceAddress() const
{
	return fSourceAddress;
}

// SourcePort
uint16
UDPPacket::SourcePort() const
{
	return fSourcePort;
}

// DestinationAddress
ip_addr_t
UDPPacket::DestinationAddress() const
{
	return fDestinationAddress;
}

// DestinationPort
uint16
UDPPacket::DestinationPort() const
{
	return fDestinationPort;
}


// #pragma mark - UDPSocket

// constructor
UDPSocket::UDPSocket()
	: fUDPService(NetStack::Default()->GetUDPService()),
	  fFirstPacket(NULL),
	  fLastPacket(NULL),
	  fAddress(INADDR_ANY),
	  fPort(0)
{
}

// destructor
UDPSocket::~UDPSocket()
{
	if (fPort != 0 && fUDPService)
		fUDPService->UnbindSocket(this);
}

// Bind
status_t
UDPSocket::Bind(ip_addr_t address, uint16 port)
{
	if (!fUDPService) {
		printf("UDPSocket::Bind(): no UDP service\n");
		return B_NO_INIT;
	}

	if (address == INADDR_BROADCAST || port == 0) {
		printf("UDPSocket::Bind(): broadcast IP or port 0\n");
		return B_BAD_VALUE;
	}

	if (fPort != 0) {
		printf("UDPSocket::Bind(): already bound\n");
		return EALREADY; // correct code?
	}

	status_t error = fUDPService->BindSocket(this, address, port);
	if (error != B_OK) {
		printf("UDPSocket::Bind(): service BindSocket() failed\n");
		return error;
	}

	fAddress = address;
	fPort = port;

	return B_OK;
}

// Send
status_t
UDPSocket::Send(ip_addr_t destinationAddress, uint16 destinationPort,
	ChainBuffer *buffer)
{
	if (!fUDPService)
		return B_NO_INIT;

	return fUDPService->Send(fPort, destinationAddress, destinationPort,
		buffer);
}

// Send
status_t
UDPSocket::Send(ip_addr_t destinationAddress, uint16 destinationPort,
	const void *data, size_t size)
{
	if (!data)
		return B_BAD_VALUE;

	ChainBuffer buffer((void*)data, size);
	return Send(destinationAddress, destinationPort, &buffer);
}

// Receive
status_t
UDPSocket::Receive(UDPPacket **_packet, bigtime_t timeout)
{
	if (!fUDPService)
		return B_NO_INIT;

	if (!_packet)
		return B_BAD_VALUE;

	bigtime_t startTime = system_time();
	for (;;) {
		fUDPService->ProcessIncomingPackets();
		if ((*_packet = PopPacket()))
			return B_OK;
	
		if (system_time() - startTime > timeout)
			return (timeout == 0 ? B_WOULD_BLOCK : B_TIMED_OUT);
	}
}

// PushPacket
void
UDPSocket::PushPacket(UDPPacket *packet)
{
	if (fLastPacket)
		fLastPacket->SetNext(packet);
	else
		fFirstPacket = packet;

	fLastPacket = packet;
	packet->SetNext(NULL);
}

// PopPacket
UDPPacket *
UDPSocket::PopPacket()
{
	if (!fFirstPacket)
		return NULL;

	UDPPacket *packet = fFirstPacket;
	fFirstPacket = packet->Next();

	if (!fFirstPacket)
		fLastPacket = NULL;

	packet->SetNext(NULL);
	return packet;
}


// #pragma mark - UDPService

// constructor
UDPService::UDPService(IPService *ipService)
	: IPSubService(kUDPServiceName),
		fIPService(ipService)
{
}

// destructor
UDPService::~UDPService()
{
	if (fIPService)
		fIPService->UnregisterIPSubService(this);
}

// Init
status_t
UDPService::Init()
{
	if (!fIPService)
		return B_BAD_VALUE;
	if (!fIPService->RegisterIPSubService(this))
		return B_NO_MEMORY;
	return B_OK;
}

// IPProtocol
uint8
UDPService::IPProtocol() const
{
	return IPPROTO_UDP;
}

// HandleIPPacket
void
UDPService::HandleIPPacket(IPService *ipService, ip_addr_t sourceIP,
	ip_addr_t destinationIP, const void *data, size_t size)
{
	TRACE(("UDPService::HandleIPPacket(): source: %08lx, destination: %08lx, "
		"%lu - %lu bytes\n", sourceIP, destinationIP, size,
		sizeof(udp_header)));

	if (!data || size < sizeof(udp_header))
		return;

	// check the header
	const udp_header *header = (const udp_header*)data;
	uint16 length = ntohs(header->length);
	if (length < sizeof(udp_header) || length > size
		|| (header->checksum != 0	// 0 => checksum disabled
			&& _ChecksumData(data, length, sourceIP, destinationIP) != 0)) {
		TRACE(("UDPService::HandleIPPacket(): dropping packet -- invalid size "
			"or checksum\n"));
		return;
	}

	// find the target socket
	UDPSocket *socket = _FindSocket(destinationIP, header->destination);
	if (!socket)
		return;

	// create a UDPPacket and queue it in the socket
	UDPPacket *packet = new(nothrow) UDPPacket;
	if (!packet)
		return;
	status_t error = packet->SetTo((uint8*)data + sizeof(udp_header),
		length - sizeof(udp_header), sourceIP, header->source, destinationIP,
		header->destination);
	if (error == B_OK)
		socket->PushPacket(packet);
	else
		delete packet;
}

// Send
status_t
UDPService::Send(uint16 sourcePort, ip_addr_t destinationAddress,
	uint16 destinationPort, ChainBuffer *buffer)
{
	TRACE(("UDPService::Send(source port: %hu, to: %08lx:%hu, %lu bytes)\n",
		sourcePort, destinationAddress, destinationPort,
		(buffer ? buffer->TotalSize() : 0)));

	if (!fIPService)
		return B_NO_INIT;

	if (!buffer)
		return B_BAD_VALUE;

	// prepend the UDP header
	udp_header header;
	ChainBuffer headerBuffer(&header, sizeof(header), buffer);
	header.source = htons(sourcePort);
	header.destination = htons(destinationPort);
	header.length = htons(headerBuffer.TotalSize());

	// compute the checksum
	header.checksum = 0;
	header.checksum = htons(_ChecksumBuffer(&headerBuffer,
		fIPService->IPAddress(), destinationAddress,
		headerBuffer.TotalSize()));
	// 0 means checksum disabled; 0xffff is equivalent in this case
	if (header.checksum == 0)
		header.checksum = 0xffff;

	return fIPService->Send(destinationAddress, IPPROTO_UDP, &headerBuffer);
}

// ProcessIncomingPackets
void
UDPService::ProcessIncomingPackets()
{
	if (fIPService)
		fIPService->ProcessIncomingPackets();
}

// BindSocket
status_t
UDPService::BindSocket(UDPSocket *socket, ip_addr_t address, uint16 port)
{
	if (!socket)
		return B_BAD_VALUE;

	if (_FindSocket(address, port)) {
		printf("UDPService::BindSocket(): address in use\n");
		return EADDRINUSE;
	}

	return fSockets.Add(socket);
}

// UnbindSocket
void
UDPService::UnbindSocket(UDPSocket *socket)
{
	fSockets.Remove(socket);
}

// _ChecksumBuffer
uint16
UDPService::_ChecksumBuffer(ChainBuffer *buffer, ip_addr_t source,
	ip_addr_t destination, uint16 length)
{
	// The checksum is calculated over a pseudo-header plus the UDP packet.
	// So we temporarily prepend the pseudo-header.
	struct pseudo_header {
		ip_addr_t	source;
		ip_addr_t	destination;
		uint8		pad;
		uint8		protocol;
		uint16		length;
	} __attribute__ ((__packed__));
	pseudo_header header = {
		htonl(source),
		htonl(destination),
		0,
		IPPROTO_UDP,
		htons(length)
	};

	ChainBuffer headerBuffer(&header, sizeof(header), buffer);
	uint16 checksum = ip_checksum(&headerBuffer);
	headerBuffer.DetachNext();
	return checksum;
}

// _ChecksumData
uint16
UDPService::_ChecksumData(const void *data, uint16 length, ip_addr_t source,
	ip_addr_t destination)
{
	ChainBuffer buffer((void*)data, length);
	return _ChecksumBuffer(&buffer, source, destination, length);
}

// _FindSocket
UDPSocket *
UDPService::_FindSocket(ip_addr_t address, uint16 port)
{
	int count = fSockets.Count();
	for (int i = 0; i < count; i++) {
		UDPSocket *socket = fSockets.ElementAt(i);
		if ((address == INADDR_ANY || socket->Address() == INADDR_ANY
				|| socket->Address() == address)
			&& port == socket->Port()) {
			return socket;
		}
	}

	return NULL;
}
