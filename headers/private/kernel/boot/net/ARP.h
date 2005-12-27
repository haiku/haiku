/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_ARP_H
#define _BOOT_ARP_H

#include <boot/net/Ethernet.h>

#include <OS.h>

class ARPService : public EthernetSubService {
	// actually ARP for IP over ethernet
public:
	ARPService(EthernetService *ethernet);
	virtual ~ARPService();

	status_t Init();

	virtual uint16 EthernetProtocol() const;

	virtual void HandleEthernetPacket(EthernetService *ethernet,
		const mac_addr_t &targetAddress, const void *data, size_t size);

	status_t GetMACForIP(ip_addr_t ip, mac_addr_t &mac);

private:
	enum { MAP_ENTRY_COUNT = 10 };
	enum { ARP_REQUEST_RETRY_COUNT = 3 };
	enum { ARP_REPLY_TIMEOUT = 5000 };

	struct MapEntry {
		int32		age;
		ip_addr_t	ip;
		mac_addr_t	mac;
	};

	status_t _SendARPPacket(ip_addr_t ip, const mac_addr_t &mac, uint16 opcode);

	MapEntry *_FindEntry(ip_addr_t ip);
	void _PutEntry(ip_addr_t ip, const mac_addr_t &mac);

	EthernetService	*fEthernet;
	int32			fAge;
	MapEntry		fEntries[MAP_ENTRY_COUNT];
};

#endif	// _BOOT_ARP_H
