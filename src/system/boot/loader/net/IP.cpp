/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/IP.h>

#include <stdio.h>
#include <KernelExport.h>

#include <boot/net/ARP.h>
#include <boot/net/ChainBuffer.h>


//#define TRACE_IP
#ifdef TRACE_IP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - IPSubService

// constructor
IPSubService::IPSubService(const char *serviceName)
	: NetService(serviceName)
{
}

// destructor
IPSubService::~IPSubService()
{
}


// #pragma mark - IPService

// constructor
IPService::IPService(EthernetService *ethernet, ARPService *arpService)
	: EthernetSubService(kIPServiceName),
		fEthernet(ethernet),
		fARPService(arpService)
{
}

// destructor
IPService::~IPService()
{
	if (fEthernet)
		fEthernet->UnregisterEthernetSubService(this);
}

// Init
status_t
IPService::Init()
{
	if (!fEthernet)
		return B_BAD_VALUE;
	if (!fEthernet->RegisterEthernetSubService(this))
		return B_NO_MEMORY;
	return B_OK;
}

// IPAddress
ip_addr_t
IPService::IPAddress() const
{
	return (fEthernet ? fEthernet->IPAddress() : INADDR_ANY);
}

// EthernetProtocol
uint16
IPService::EthernetProtocol() const
{
	return ETHERTYPE_IP;
}

// HandleEthernetPacket
void
IPService::HandleEthernetPacket(EthernetService *ethernet,
	const mac_addr_t &targetAddress, const void *data, size_t size)
{
	TRACE(("IPService::HandleEthernetPacket(): %lu - %lu bytes\n", size,
		sizeof(ip_header)));

	if (!data || size < sizeof(ip_header))
		return;

	// check header
	const ip_header *header = (const ip_header*)data;
	// header length OK?
	int headerLength = header->header_length * 4;
	if (headerLength < 20 || headerLength > (int)size
		// IP V4?
		|| header->version != IP_PROTOCOL_VERSION_4
		// length OK?
		|| ntohs(header->total_length) > size
		// broadcast or our IP?
		|| (header->destination != htonl(INADDR_BROADCAST)
			&& (fEthernet->IPAddress() == INADDR_ANY
				|| header->destination != htonl(fEthernet->IPAddress())))
		// checksum OK?
		|| _Checksum(*header) != 0) {
		return;
	}

	// find a service handling this kind of packet
	int serviceCount = fServices.Count();
	for (int i = 0; i < serviceCount; i++) {
		IPSubService *service = fServices.ElementAt(i);
		if (service->IPProtocol() == header->protocol) {
			service->HandleIPPacket(this, ntohl(header->source),
				ntohl(header->destination),
				(uint8*)data + headerLength,
				ntohs(header->total_length) - headerLength);
			break;
		}
	}
}

// Send
status_t
IPService::Send(ip_addr_t destination, uint8 protocol, ChainBuffer *buffer)
{
	TRACE(("IPService::Send(to: %08lx, proto: %lu, %lu bytes)\n", destination,
		(uint32)protocol, (buffer ? buffer->TotalSize() : 0)));

	if (!buffer)
		return B_BAD_VALUE;

	if (!fEthernet || !fARPService)
		return B_NO_INIT;

	// prepare header
	ip_header header;
	ChainBuffer headerBuffer(&header, sizeof(header), buffer);
	header.header_length = 5;	// 5 32 bit words, no options
	header.version = IP_PROTOCOL_VERSION_4;
	header.type_of_service = 0;
	header.total_length = htons(headerBuffer.TotalSize());
	header.identifier = 0;
	header.fragment_offset = htons(IP_DONT_FRAGMENT);
	header.time_to_live = IP_DEFAULT_TIME_TO_LIVE;
	header.protocol = protocol;
	header.checksum = 0;
	header.source = htonl(fEthernet->IPAddress());
	header.destination = htonl(destination);

	// compute check sum
	header.checksum = htons(_Checksum(header));

	// get target MAC address
	mac_addr_t targetMAC;
	status_t error = fARPService->GetMACForIP(destination, targetMAC);
	if (error != B_OK)
		return error;

	// send the packet
	return fEthernet->Send(targetMAC, ETHERTYPE_IP, &headerBuffer);
}

// ProcessIncomingPackets
void
IPService::ProcessIncomingPackets()
{
	if (fEthernet)
		fEthernet->ProcessIncomingPackets();
}

// RegisterIPSubService
bool
IPService::RegisterIPSubService(IPSubService *service)
{
	return (service && fServices.Add(service) == B_OK);
}

// UnregisterIPSubService
bool
IPService::UnregisterIPSubService(IPSubService *service)
{
	return (service && fServices.Remove(service) >= 0);
}

// CountSubNetServices
int
IPService::CountSubNetServices() const
{
	return fServices.Count();
}

// SubNetServiceAt
NetService *
IPService::SubNetServiceAt(int index) const
{
	return fServices.ElementAt(index);
}

// _Checksum
uint16
IPService::_Checksum(const ip_header &header)
{
	ChainBuffer buffer((void*)&header, header.header_length * 4);
	return ip_checksum(&buffer);
}


// #pragma mark -

// ip_checksum
uint16
ip_checksum(ChainBuffer *buffer)
{
	// ChainBuffer iterator returning a stream of uint16 (big endian).
	struct Iterator {
		Iterator(ChainBuffer *buffer)
			: fBuffer(buffer),
			  fOffset(-1)
		{
			_Next();
		}

		bool HasNext() const
		{
			return fBuffer;
		}

		uint16 Next()
		{
			uint16 byte = _NextByte();
			return (byte << 8) | _NextByte();
		}

	private:
		void _Next()
		{
			while (fBuffer) {
				fOffset++;
				if (fOffset < (int)fBuffer->Size())
					break;

				fOffset = -1;
				fBuffer = fBuffer->Next();
			}
		}

		uint8 _NextByte()
		{
			uint8 byte = (fBuffer ? ((uint8*)fBuffer->Data())[fOffset] : 0);
			_Next();
			return byte;
		}

		ChainBuffer	*fBuffer;
		int			fOffset;
	};

	Iterator it(buffer);

	uint32 checksum = 0;
	while (it.HasNext()) {
		checksum += it.Next();
		while (checksum >> 16)
			checksum = (checksum & 0xffff) + (checksum >> 16);
	}

	return ~checksum;
}


ip_addr_t
ip_parse_address(const char *string)
{
	ip_addr_t address = 0;
	int components = 0;

	// TODO: Handles only IPv4 addresses for now.
	while (components < 4) {
		address |= strtol(string, NULL, 0) << ((4 - components - 1) * 8);

		const char *dot = strchr(string, '.');
		if (dot == NULL)
			break;

		string = dot + 1;
		components++;
	}

	return address;
}
