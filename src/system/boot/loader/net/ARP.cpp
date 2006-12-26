/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/ARP.h>

#include <stdio.h>
#include <KernelExport.h>

#include <boot/net/ChainBuffer.h>


//#define TRACE_ARP
#ifdef TRACE_ARP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// constructor
ARPService::ARPService(EthernetService *ethernet)
	: EthernetSubService(kARPServiceName),
		fEthernet(ethernet),
		fAge(0)
{
	// clear table
	for (int i = 0; i < MAP_ENTRY_COUNT; i++)
		fEntries[i].ip = INADDR_ANY;
}

// destructor
ARPService::~ARPService()
{
	if (fEthernet)
		fEthernet->UnregisterEthernetSubService(this);
}

// Init
status_t
ARPService::Init()
{
	if (!fEthernet)
		return B_BAD_VALUE;
	if (!fEthernet->RegisterEthernetSubService(this))
		return B_NO_MEMORY;
	return B_OK;
}

// EthernetProtocol
uint16
ARPService::EthernetProtocol() const
{
	return ETHERTYPE_ARP;
}

// HandleEthernetPacket
void
ARPService::HandleEthernetPacket(EthernetService *ethernet,
	const mac_addr_t &targetAddress, const void *data, size_t size)
{
	TRACE(("ARPService::HandleEthernetPacket(): %lu - %lu bytes\n", size,
		sizeof(ip_header)));

	if (size < sizeof(arp_header))
		return;

	arp_header *header = (arp_header*)data;
	// check packet validity
	if (header->hardware_format != htons(ARPHRD_ETHER)
		|| header->protocol_format != htons(ETHERTYPE_IP)
		|| header->hardware_length != sizeof(mac_addr_t)
		|| header->protocol_length != sizeof(ip_addr_t)
		// valid sender MAC?
		|| header->sender_mac == kNoMACAddress
		|| header->sender_mac == kBroadcastMACAddress
		// do we support the opcode?
		|| (header->opcode != htons(ARPOP_REQUEST)
			&& header->opcode != htons(ARPOP_REPLY))) {
		return;
	}

	// if this is a request, we continue only, if we have the targeted IP
	if (header->opcode == htons(ARPOP_REQUEST)
		&& (fEthernet->IPAddress() == INADDR_ANY
			|| header->target_ip != htonl(fEthernet->IPAddress()))) {
		return;
	}

	// if this is a reqly, we accept it only, if it was directly sent to us
	if (header->opcode == htons(ARPOP_REPLY)
		&& (targetAddress != fEthernet->MACAddress()
			|| header->target_mac != targetAddress)) {
		return;
	}

	// if sender IP looks valid, enter the mapping
	if (header->sender_ip != htonl(INADDR_ANY)
		&& header->sender_ip != htonl(INADDR_BROADCAST)) {
		_PutEntry(ntohl(header->sender_ip), header->sender_mac);
	}

	// if this is a request, send a reply
	if (header->opcode == htons(ARPOP_REQUEST)) {
		_SendARPPacket(ntohl(header->sender_ip), header->sender_mac,
			ARPOP_REPLY);
	}
}

// GetMACForIP
status_t
ARPService::GetMACForIP(ip_addr_t ip, mac_addr_t &mac)
{
	TRACE(("ARPService::GetMACForIP(%08lx)\n", ip));

	if (ip == INADDR_ANY)
		return B_BAD_VALUE;
	if (ip == INADDR_BROADCAST) {
		mac = kBroadcastMACAddress;
		TRACE(("ARPService::GetMACForIP(%08lx) done: %012llx\n", ip,
			mac.ToUInt64()));
		return B_OK;
	}

	// already known?
	if (MapEntry *entry = _FindEntry(ip)) {
		mac = entry->mac;
		TRACE(("ARPService::GetMACForIP(%08lx) done: %012llx\n", ip,
			mac.ToUInt64()));
		return B_OK;
	}

	for (int i = 0; i < ARP_REQUEST_RETRY_COUNT; i++) {
		// send request
		status_t error = _SendARPPacket(ip, kBroadcastMACAddress,
			ARPOP_REQUEST);
		if (error != B_OK) {
			TRACE(("ARPService::GetMACForIP(%08lx) failed: sending failed\n",
				ip));
			return error;
		}
	
		bigtime_t startTime = system_time();
		do {
			fEthernet->ProcessIncomingPackets();

			// received reply?
			if (MapEntry *entry = _FindEntry(ip)) {
				mac = entry->mac;
				TRACE(("ARPService::GetMACForIP(%08lx) done: %012llx\n", ip,
					mac.ToUInt64()));
				return B_OK;
			}
		} while (system_time() - startTime < ARP_REPLY_TIMEOUT);
	}

	TRACE(("ARPService::GetMACForIP(%08lx) failed: no reply\n", ip));

	return EHOSTUNREACH;
}

// _SendARPPacket
status_t
ARPService::_SendARPPacket(ip_addr_t ip, const mac_addr_t &mac, uint16 opcode)
{
	// prepare ARP header
	arp_header header;
	ChainBuffer headerBuffer(&header, sizeof(header));
	header.hardware_format = htons(ARPHRD_ETHER);
	header.protocol_format = htons(ETHERTYPE_IP);
	header.hardware_length = sizeof(mac_addr_t);
	header.protocol_length = sizeof(ip_addr_t);
	header.opcode = htons(opcode);
	header.sender_mac = fEthernet->MACAddress();
	header.sender_ip = htonl(fEthernet->IPAddress());
	header.target_mac = (mac == kBroadcastMACAddress ? kNoMACAddress : mac);
	header.target_ip = htonl(ip);

	return fEthernet->Send(mac, ETHERTYPE_ARP, &headerBuffer);
}

// _FindEntry
ARPService::MapEntry *
ARPService::_FindEntry(ip_addr_t ip)
{
	if (ip == INADDR_ANY)
		return NULL;

	for (int i = 0; i < MAP_ENTRY_COUNT; i++) {
		if (ip == fEntries[i].ip)
			return fEntries + i;
	}

	return NULL;
}

// _PutEntry
void
ARPService::_PutEntry(ip_addr_t ip, const mac_addr_t &mac)
{
	// find empty/oldest slot
	MapEntry *entry = fEntries;
	for (int i = 0; i < MAP_ENTRY_COUNT; i++) {
		if (fEntries[i].ip == INADDR_ANY) {
			entry = fEntries + i;
			break;
		}

		if (fAge - fEntries[i].age > fAge - entry->age)
			entry = fEntries + i;
	}

	entry->age = fAge++;
	entry->ip = ip;
	entry->mac = mac;
}
