/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/Ethernet.h>

#include <stdio.h>
#include <KernelExport.h>

#include <boot/net/ChainBuffer.h>


//#define TRACE_ETHERNET
#ifdef TRACE_ETHERNET
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - EthernetInterface

// constructor	
EthernetInterface::EthernetInterface()
	: fIPAddress(INADDR_ANY)
{
}

// destructor	
EthernetInterface::~EthernetInterface()
{
}

// IPAddress
ip_addr_t
EthernetInterface::IPAddress() const
{
	return fIPAddress;
}

// SetIPAddress
void
EthernetInterface::SetIPAddress(ip_addr_t ipAddress)
{
	fIPAddress = ipAddress;
}


// #pragma mark - EthernetSubService

// constructor
EthernetSubService::EthernetSubService(const char *serviceName)
	: NetService(serviceName)
{
}

// destructor
EthernetSubService::~EthernetSubService()
{
}


// #pragma mark - EthernetService

// constructor
EthernetService::EthernetService()
	: NetService(kEthernetServiceName),
	  fInterface(NULL),
	  fSendBuffer(NULL),
	  fReceiveBuffer(NULL)
{
}

// destructor
EthernetService::~EthernetService()
{
	if (fSendBuffer)
		fInterface->FreeSendReceiveBuffer(fSendBuffer);
}

// Init
status_t
EthernetService::Init(EthernetInterface *interface)
{
	if (!interface)
		return B_BAD_VALUE;

	fInterface = interface;

	fSendBuffer = fInterface->AllocateSendReceiveBuffer(
		SEND_BUFFER_SIZE + RECEIVE_BUFFER_SIZE);
	if (!fSendBuffer)
		return B_NO_MEMORY;
	fReceiveBuffer = (uint8*)fSendBuffer + SEND_BUFFER_SIZE;

	return B_OK;
}

// MACAddress
mac_addr_t
EthernetService::MACAddress() const
{
	return fInterface->MACAddress();
}

// IPAddress
ip_addr_t
EthernetService::IPAddress() const
{
	return fInterface->IPAddress();
}

// SetIPAddress
void
EthernetService::SetIPAddress(ip_addr_t ipAddress)
{
	fInterface->SetIPAddress(ipAddress);
}

// Send
status_t
EthernetService::Send(const mac_addr_t &destination, uint16 protocol,
	ChainBuffer *buffer)
{
	TRACE(("EthernetService::Send(to: %012" B_PRIx64 ", proto: 0x%hx, %"
		PRIu32 " bytes)\n",
		destination.ToUInt64(), protocol, (buffer ? buffer->TotalSize() : 0)));

	if (!fInterface || !fSendBuffer)
		return B_NO_INIT;

	// sending has time, but we need to handle incoming packets as soon as
	// possible
	ProcessIncomingPackets();

	if (!buffer)
		return B_BAD_VALUE;

	// data too long?
	size_t dataSize = buffer->TotalSize();
	if (dataSize > ETHER_MAX_TRANSFER_UNIT)
		return B_BAD_VALUE;

	// prepend ethernet header
	ether_header header;
	ChainBuffer headerBuffer(&header, sizeof(header), buffer);
	header.source = fInterface->MACAddress();
	header.destination = destination;
	header.type = htons(protocol);

	// flatten
	size_t totalSize = headerBuffer.TotalSize();
	headerBuffer.Flatten(fSendBuffer);

	// pad data, if necessary
	if (dataSize < ETHER_MIN_TRANSFER_UNIT) {
		size_t paddingSize = ETHER_MIN_TRANSFER_UNIT - dataSize;
		memset((uint8*)fSendBuffer + totalSize, 0, paddingSize);
		totalSize += paddingSize;
	}

	// send
	ssize_t bytesSent = fInterface->Send(fSendBuffer, totalSize);
	if (bytesSent < 0)
		return bytesSent;
	if (bytesSent != (ssize_t)totalSize)
		return B_ERROR;

	return B_OK;
}

// ProcessIncomingPackets
void
EthernetService::ProcessIncomingPackets()
{
	if (!fInterface || !fReceiveBuffer)
		return;

	for (;;) {
		// read from the interface
		ssize_t bytesReceived = fInterface->Receive(fReceiveBuffer,
			RECEIVE_BUFFER_SIZE);
		if (bytesReceived < 0)
			return;

		// basic sanity checks (packet too small/too big)
		if (bytesReceived
				< (ssize_t)sizeof(ether_header) + ETHER_MIN_TRANSFER_UNIT
			|| bytesReceived
				> (ssize_t)sizeof(ether_header) + ETHER_MAX_TRANSFER_UNIT) {
			continue;
		}

		// is the packet intended for us?
		ether_header *header = (ether_header*)fReceiveBuffer;
		if (header->destination != kBroadcastMACAddress
			&& header->destination != fInterface->MACAddress()) {
			continue;
		}

		TRACE(("EthernetService::ProcessIncomingPackets(): received ethernet "
			"frame: to: %012" B_PRIx64 ", proto: 0x%hx, %" B_PRIuSIZE " bytes\n",
			header->destination.ToUInt64(), ntohs(header->type),
			bytesReceived - (ssize_t)sizeof(ether_header)));

		// find a service handling this kind of packet
		int serviceCount = fServices.Count();
		for (int i = 0; i < serviceCount; i++) {
			EthernetSubService *service = fServices.ElementAt(i);
			if (service->EthernetProtocol() == ntohs(header->type)) {
				service->HandleEthernetPacket(this, header->destination,
					(uint8*)fReceiveBuffer + sizeof(ether_header),
					bytesReceived - sizeof(ether_header));
				break;
			}
		}
	}
}

// RegisterEthernetSubService
bool
EthernetService::RegisterEthernetSubService(EthernetSubService *service)
{
	return (service && fServices.Add(service) == B_OK);
}

// UnregisterEthernetSubService
bool
EthernetService::UnregisterEthernetSubService(EthernetSubService *service)
{
	return (service && fServices.Remove(service) >= 0);
}

// CountSubNetServices
int
EthernetService::CountSubNetServices() const
{
	return fServices.Count();
}

// SubNetServiceAt
NetService *
EthernetService::SubNetServiceAt(int index) const
{
	return fServices.ElementAt(index);
}
