/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_ETHERNET_H
#define _BOOT_ETHERNET_H

#include <boot/net/NetDefs.h>
#include <util/Vector.h>

class ChainBuffer;
class EthernetService;

// EthernetInterface
class EthernetInterface {
public:
	EthernetInterface();
	virtual ~EthernetInterface();

	ip_addr_t IPAddress() const;
	void SetIPAddress(ip_addr_t ipAddress);

	virtual mac_addr_t MACAddress() const = 0;

	virtual	void *AllocateSendReceiveBuffer(size_t size) = 0;
	virtual	void FreeSendReceiveBuffer(void *buffer) = 0;

	virtual ssize_t Send(const void *buffer, size_t size) = 0;
	virtual ssize_t Receive(void *buffer, size_t size) = 0;

protected:
	ip_addr_t					fIPAddress;
};

// EthernetSubService
class EthernetSubService : public NetService {
public:
	EthernetSubService(const char *serviceName);
	virtual ~EthernetSubService();

	virtual uint16 EthernetProtocol() const = 0;

	virtual void HandleEthernetPacket(EthernetService *ethernet,
		const mac_addr_t &targetAddress, const void *data, size_t size) = 0;
};


// EthernetService
class EthernetService : public NetService {
public:
	EthernetService();
	virtual ~EthernetService();

	status_t Init(EthernetInterface *interface);

	mac_addr_t MACAddress() const;
	ip_addr_t IPAddress() const;
	void SetIPAddress(ip_addr_t ipAddress);

	status_t Send(const mac_addr_t &destination, uint16 protocol,
		ChainBuffer *buffer);
	void ProcessIncomingPackets();

	bool RegisterEthernetSubService(EthernetSubService *service);
	bool UnregisterEthernetSubService(EthernetSubService *service);

	virtual int CountSubNetServices() const;
	virtual NetService *SubNetServiceAt(int index) const;

private:
	enum {
		SEND_BUFFER_SIZE	= 2048,
		RECEIVE_BUFFER_SIZE	= SEND_BUFFER_SIZE,
	};

	EthernetInterface			*fInterface;
	void						*fSendBuffer;
	void						*fReceiveBuffer;
	Vector<EthernetSubService*>	fServices;
};


#endif	// _BOOT_ETHERNET_H
