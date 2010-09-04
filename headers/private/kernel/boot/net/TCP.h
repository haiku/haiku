/*
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BOOT_NET_TCP_H
#define BOOT_NET_TCP_H


#include <boot/net/IP.h>


class TCPPacket {
public:
	TCPPacket();
	~TCPPacket();

	status_t SetTo(const void* data, size_t size, ip_addr_t sourceAddress,
		uint16 sourcePort, ip_addr_t destinationAddress,
		uint16 destinationPort, uint32 sequenceNumber,
		uint32 acknowledgmentNumber, uint8 flags);

	ip_addr_t SourceAddress() const;
	ip_addr_t DestinationAddress() const;
	uint16 SourcePort() const;
	uint16 DestinationPort() const;
	uint32 SequenceNumber() const;
	uint32 AcknowledgmentNumber() const;
	const void* Data() const { return fData; }
	size_t DataSize() const { return fSize; }
	uint8 Flags() const { return fFlags; }

	bool ProvidesSequenceNumber(uint32 sequenceNo) const;

	TCPPacket* Next() const;
	void SetNext(TCPPacket* packet);

private:
	ip_addr_t	fSourceAddress;
	ip_addr_t	fDestinationAddress;
	uint16		fSourcePort;
	uint16		fDestinationPort;
	uint32		fSequenceNumber;
	uint32		fAcknowledgmentNumber;
	void*		fData;
	size_t		fSize;
	uint8		fFlags;
	TCPPacket*	fNext;
};

class TCPService;

enum TCPSocketState {
	TCP_SOCKET_STATE_INITIAL,
	TCP_SOCKET_STATE_SYN_SENT,
	TCP_SOCKET_STATE_SYN_RECEIVED,
	TCP_SOCKET_STATE_OPEN,
	TCP_SOCKET_STATE_FIN_SENT,
	TCP_SOCKET_STATE_CLOSED
};

class TCPSocket {
public:
	TCPSocket();
	~TCPSocket();

	ip_addr_t Address() const	{ return fAddress; }
	uint16 Port() const			{ return fPort; }
	uint16 WindowSize() const;

	status_t Connect(ip_addr_t address, uint16 port);
	status_t Close();
	status_t Read(void* buffer, size_t bufferSize, size_t* bytesRead, bigtime_t timeout = 0);
	status_t Write(const void* buffer, size_t bufferSize);

	void Acknowledge(uint32 number);
	void ProcessPacket(TCPPacket* packet);

private:
	TCPPacket* _PeekPacket();
	TCPPacket* _DequeuePacket();
	status_t _Send(TCPPacket* packet, bool enqueue = true);
	status_t _ResendQueue();
	void _EnqueueOutgoingPacket(TCPPacket* packet);
	inline void	_DumpQueue();
	status_t _WaitForState(TCPSocketState state, bigtime_t timeout = 0);
	status_t _Ack();

	TCPService*	fTCPService;
	ip_addr_t	fAddress;
	uint16		fPort;
	ip_addr_t	fRemoteAddress;
	uint16		fRemotePort;
	uint32		fSequenceNumber;
	uint32		fAcknowledgeNumber;
	uint32		fNextSequence;
	TCPPacket*	fFirstPacket;
	TCPPacket*	fLastPacket;
	TCPPacket*	fFirstSentPacket;
	TCPPacket*	fLastSentPacket;
	TCPSocketState fState;
	TCPSocketState fRemoteState;
};

class TCPService : public IPSubService {
public:
	TCPService(IPService* ipService);
	virtual ~TCPService();

	status_t Init();

	virtual uint8 IPProtocol() const;

	virtual void HandleIPPacket(IPService* ipService, ip_addr_t sourceIP,
		ip_addr_t destinationIP, const void* data, size_t size);

	status_t Send(uint16 sourcePort, ip_addr_t destinationAddress,
		uint16 destinationPort, uint32 sequenceNumber,
		uint32 acknowledgmentNumber, uint8 flags, uint16 windowSize,
		ChainBuffer* buffer);

	void ProcessIncomingPackets();

	status_t BindSocket(TCPSocket* socket);
	void UnbindSocket(TCPSocket* socket);

private:
	uint16 _ChecksumBuffer(ChainBuffer* buffer, ip_addr_t source,
		ip_addr_t destination, uint16 length);
	uint16 _ChecksumData(const void* data, uint16 length, ip_addr_t source,
		ip_addr_t destination);

	TCPSocket* _FindSocket(ip_addr_t address, uint16 port);

	IPService*			fIPService;
	Vector<TCPSocket*>	fSockets;
};


#endif
