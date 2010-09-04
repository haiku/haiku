/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/net/UDP.h>

#include <stdio.h>

#include <KernelExport.h>

#include <boot/net/ChainBuffer.h>
#include <boot/net/NetStack.h>


//#define TRACE_UDP
#ifdef TRACE_UDP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - UDPPacket


UDPPacket::UDPPacket()
	:
	fNext(NULL),
	fData(NULL),
	fSize(0)
{
}


UDPPacket::~UDPPacket()
{
	free(fData);
}


status_t
UDPPacket::SetTo(const void *data, size_t size, ip_addr_t sourceAddress,
	uint16 sourcePort, ip_addr_t destinationAddress, uint16 destinationPort)
{
	if (data == NULL)
		return B_BAD_VALUE;

	// clone the data
	fData = malloc(size);
	if (fData == NULL)
		return B_NO_MEMORY;
	memcpy(fData, data, size);

	fSize = size;
	fSourceAddress = sourceAddress;
	fDestinationAddress = destinationAddress;
	fSourcePort = sourcePort;
	fDestinationPort = destinationPort;

	return B_OK;
}


UDPPacket *
UDPPacket::Next() const
{
	return fNext;
}


void
UDPPacket::SetNext(UDPPacket *next)
{
	fNext = next;
}


const void *
UDPPacket::Data() const
{
	return fData;
}


size_t
UDPPacket::DataSize() const
{
	return fSize;
}


ip_addr_t
UDPPacket::SourceAddress() const
{
	return fSourceAddress;
}


uint16
UDPPacket::SourcePort() const
{
	return fSourcePort;
}


ip_addr_t
UDPPacket::DestinationAddress() const
{
	return fDestinationAddress;
}


uint16
UDPPacket::DestinationPort() const
{
	return fDestinationPort;
}


// #pragma mark - UDPSocket


UDPSocket::UDPSocket()
	:
	fUDPService(NetStack::Default()->GetUDPService()),
	fFirstPacket(NULL),
	fLastPacket(NULL),
	fAddress(INADDR_ANY),
	fPort(0)
{
}


UDPSocket::~UDPSocket()
{
	if (fPort != 0 && fUDPService != NULL)
		fUDPService->UnbindSocket(this);
}


status_t
UDPSocket::Bind(ip_addr_t address, uint16 port)
{
	if (fUDPService == NULL) {
		printf("UDPSocket::Bind(): no UDP service\n");
		return B_NO_INIT;
	}

	if (address == INADDR_BROADCAST || port == 0) {
		printf("UDPSocket::Bind(): broadcast IP or port 0\n");
		return B_BAD_VALUE;
	}

	if (fPort != 0) {
		printf("UDPSocket::Bind(): already bound\n");
		return EALREADY;
			// correct code?
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


status_t
UDPSocket::Send(ip_addr_t destinationAddress, uint16 destinationPort,
	ChainBuffer *buffer)
{
	if (fUDPService == NULL)
		return B_NO_INIT;

	return fUDPService->Send(fPort, destinationAddress, destinationPort,
		buffer);
}


status_t
UDPSocket::Send(ip_addr_t destinationAddress, uint16 destinationPort,
	const void *data, size_t size)
{
	if (data == NULL)
		return B_BAD_VALUE;

	ChainBuffer buffer((void*)data, size);
	return Send(destinationAddress, destinationPort, &buffer);
}


status_t
UDPSocket::Receive(UDPPacket **_packet, bigtime_t timeout)
{
	if (fUDPService == NULL)
		return B_NO_INIT;

	if (_packet == NULL)
		return B_BAD_VALUE;

	bigtime_t startTime = system_time();
	for (;;) {
		fUDPService->ProcessIncomingPackets();
		*_packet = PopPacket();
		if (*_packet != NULL)
			return B_OK;
	
		if (system_time() - startTime > timeout)
			return (timeout == 0 ? B_WOULD_BLOCK : B_TIMED_OUT);
	}
}


void
UDPSocket::PushPacket(UDPPacket *packet)
{
	if (fLastPacket != NULL)
		fLastPacket->SetNext(packet);
	else
		fFirstPacket = packet;

	fLastPacket = packet;
	packet->SetNext(NULL);
}


UDPPacket *
UDPSocket::PopPacket()
{
	if (fFirstPacket == NULL)
		return NULL;

	UDPPacket *packet = fFirstPacket;
	fFirstPacket = packet->Next();

	if (fFirstPacket == NULL)
		fLastPacket = NULL;

	packet->SetNext(NULL);
	return packet;
}


// #pragma mark - UDPService


UDPService::UDPService(IPService *ipService)
	:
	IPSubService(kUDPServiceName),
	fIPService(ipService)
{
}


UDPService::~UDPService()
{
	if (fIPService != NULL)
		fIPService->UnregisterIPSubService(this);
}


status_t
UDPService::Init()
{
	if (fIPService == NULL)
		return B_BAD_VALUE;
	if (!fIPService->RegisterIPSubService(this))
		return B_NO_MEMORY;
	return B_OK;
}


uint8
UDPService::IPProtocol() const
{
	return IPPROTO_UDP;
}


void
UDPService::HandleIPPacket(IPService *ipService, ip_addr_t sourceIP,
	ip_addr_t destinationIP, const void *data, size_t size)
{
	TRACE(("UDPService::HandleIPPacket(): source: %08lx, destination: %08lx, "
		"%lu - %lu bytes\n", sourceIP, destinationIP, size,
		sizeof(udp_header)));

	if (data == NULL || size < sizeof(udp_header))
		return;

	const udp_header *header = (const udp_header*)data;
	uint16 source = ntohs(header->source);
	uint16 destination = ntohs(header->destination);
	uint16 length = ntohs(header->length);

	// check the header
	if (length < sizeof(udp_header) || length > size
		|| (header->checksum != 0	// 0 => checksum disabled
			&& _ChecksumData(data, length, sourceIP, destinationIP) != 0)) {
		TRACE(("UDPService::HandleIPPacket(): dropping packet -- invalid size "
			"or checksum\n"));
		return;
	}

	// find the target socket
	UDPSocket *socket = _FindSocket(destinationIP, destination);
	if (socket == NULL)
		return;

	// create a UDPPacket and queue it in the socket
	UDPPacket *packet = new(nothrow) UDPPacket;
	if (packet == NULL)
		return;
	status_t error = packet->SetTo((uint8*)data + sizeof(udp_header),
		length - sizeof(udp_header), sourceIP, source, destinationIP,
		destination);
	if (error == B_OK)
		socket->PushPacket(packet);
	else
		delete packet;
}


status_t
UDPService::Send(uint16 sourcePort, ip_addr_t destinationAddress,
	uint16 destinationPort, ChainBuffer *buffer)
{
	TRACE(("UDPService::Send(source port: %hu, to: %08lx:%hu, %lu bytes)\n",
		sourcePort, destinationAddress, destinationPort,
		(buffer != NULL ? buffer->TotalSize() : 0)));

	if (fIPService == NULL)
		return B_NO_INIT;

	if (buffer == NULL)
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


void
UDPService::ProcessIncomingPackets()
{
	if (fIPService != NULL)
		fIPService->ProcessIncomingPackets();
}


status_t
UDPService::BindSocket(UDPSocket *socket, ip_addr_t address, uint16 port)
{
	if (socket == NULL)
		return B_BAD_VALUE;

	if (_FindSocket(address, port) != NULL) {
		printf("UDPService::BindSocket(): address in use\n");
		return EADDRINUSE;
	}

	return fSockets.Add(socket);
}


void
UDPService::UnbindSocket(UDPSocket *socket)
{
	fSockets.Remove(socket);
}


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


uint16
UDPService::_ChecksumData(const void *data, uint16 length, ip_addr_t source,
	ip_addr_t destination)
{
	ChainBuffer buffer((void*)data, length);
	return _ChecksumBuffer(&buffer, source, destination, length);
}


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
