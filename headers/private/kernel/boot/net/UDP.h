/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_UDP_H
#define _BOOT_UDP_H

#include <boot/net/IP.h>

// UDPPacket
class UDPPacket {
public:
	UDPPacket();
	~UDPPacket();

	status_t SetTo(const void *data, size_t size, ip_addr_t sourceAddress,
		uint16 sourcePort, ip_addr_t destinationAddress,
		uint16 destinationPort);

	UDPPacket *Next() const;
	void SetNext(UDPPacket *next);

	const void *Data() const;
	size_t DataSize() const;

	ip_addr_t SourceAddress() const;
	uint16 SourcePort() const;
	ip_addr_t DestinationAddress() const;
	uint16 DestinationPort() const;

private:
	UDPPacket	*fNext;
	void		*fData;
	size_t		fSize;
	ip_addr_t	fSourceAddress;
	ip_addr_t	fDestinationAddress;
	uint16		fSourcePort;
	uint16		fDestinationPort;
};


class UDPService;

// UDPSocket
class UDPSocket {
public:
	UDPSocket();
	~UDPSocket();

	ip_addr_t Address() const	{ return fAddress; }
	uint16 Port() const			{ return fPort; }

	status_t Bind(ip_addr_t address, uint16 port);

	status_t Send(ip_addr_t destinationAddress, uint16 destinationPort,
		ChainBuffer *buffer);
	status_t Send(ip_addr_t destinationAddress, uint16 destinationPort,
		const void *data, size_t size);
	status_t Receive(UDPPacket **packet, bigtime_t timeout = 0);

	void PushPacket(UDPPacket *packet);
	UDPPacket *PopPacket();

private:
	UDPService	*fUDPService;
	UDPPacket	*fFirstPacket;
	UDPPacket	*fLastPacket;
	ip_addr_t	fAddress;
	uint16		fPort;
};


// UDPService
class UDPService : public IPSubService {
public:
	UDPService(IPService *ipService);
	virtual ~UDPService();

	status_t Init();

	virtual uint8 IPProtocol() const;

	virtual void HandleIPPacket(IPService *ipService, ip_addr_t sourceIP,
		ip_addr_t destinationIP, const void *data, size_t size);

	status_t Send(uint16 sourcePort, ip_addr_t destinationAddress,
		uint16 destinationPort, ChainBuffer *buffer);

	void ProcessIncomingPackets();

	status_t BindSocket(UDPSocket *socket, ip_addr_t address, uint16 port);
	void UnbindSocket(UDPSocket *socket);

private:
	uint16 _ChecksumBuffer(ChainBuffer *buffer, ip_addr_t source,
		ip_addr_t destination, uint16 length);
	uint16 _ChecksumData(const void *data, uint16 length, ip_addr_t source,
		ip_addr_t destination);

	UDPSocket *_FindSocket(ip_addr_t address, uint16 port);

	IPService			*fIPService;
	Vector<UDPSocket*>	fSockets;
};

#endif	// _BOOT_UDP_H
