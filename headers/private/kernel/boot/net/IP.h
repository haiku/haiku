/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_IP_H
#define _BOOT_IP_H

#include <boot/net/Ethernet.h>

class ARPService;
class IPService;

// IPSubService
class IPSubService : public NetService {
public:
	IPSubService(const char *serviceName);
	virtual ~IPSubService();

	virtual uint8 IPProtocol() const = 0;

	virtual void HandleIPPacket(IPService *ipService, ip_addr_t sourceIP,
		ip_addr_t destinationIP, const void *data, size_t size) = 0;
};


// IPService
class IPService : public EthernetSubService {
	// actually IP over ethernet
public:
	IPService(EthernetService *ethernet, ARPService *arpService);
	virtual ~IPService();

	status_t Init();

	ip_addr_t IPAddress() const;

	virtual uint16 EthernetProtocol() const;

	virtual void HandleEthernetPacket(EthernetService *ethernet,
		const mac_addr_t &targetAddress, const void *data, size_t size);

	status_t Send(ip_addr_t destination, uint8 protocol, ChainBuffer *buffer);

	void ProcessIncomingPackets();

	bool RegisterIPSubService(IPSubService *service);
	bool UnregisterIPSubService(IPSubService *service);

	virtual int CountSubNetServices() const;
	virtual NetService *SubNetServiceAt(int index) const;

private:
	uint16 _Checksum(const ip_header &header);

	EthernetService			*fEthernet;
	ARPService				*fARPService;
	Vector<IPSubService*>	fServices;
};

uint16 ip_checksum(ChainBuffer *buffer);
ip_addr_t ip_parse_address(const char* address);


#endif	// _BOOT_IP_H
