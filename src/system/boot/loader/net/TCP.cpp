/*
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


/*
 * NOTE This is a cleanroom TCP implementation with some known issues.
 * Protection Against Wrapping Sequence (PAWS) needs to be added.
 * Congestion control needs to be implemented (slow start, recv. window size).
 * The use of *Packets needs to be re-evaluated in the context of TCP;
 * probably a singly-linked list of received data chunks is more efficient.
 * Debug output should be tuned for better aspect oriented tracing.
 * While Little Endian systems have been considered, this still needs testing.
 */


#include <boot/net/TCP.h>

#include <stdio.h>

#include <KernelExport.h>

#include <boot/net/ChainBuffer.h>
#include <boot/net/NetStack.h>

#include "real_time_clock.h"


//#define TRACE_TCP
//#define TRACE_TCP_RANDOMNESS
//#define TRACE_TCP_CHECKSUM
//#define TRACE_TCP_QUEUE


#ifdef TRACE_TCP
#	define TRACE(x, ...) dprintf(x, ## __VA_ARGS__)
#else
#	define TRACE(x, ...) ;
#endif
#ifdef TRACE_TCP_RANDOMNESS
#	define TRACE_PORT(x, ...) dprintf(x, ## __VA_ARGS__)
#else
#	define TRACE_PORT(x, ...) ;
#endif
#if defined(TRACE_TCP_CHECKSUM)
#	define TRACE_CHECKSUM(x, ...) dprintf(x, ## __VA_ARGS__)
#else
#	define TRACE_CHECKSUM(x, ...) ;
#endif
#if defined(TRACE_TCP_QUEUE)
#	define TRACE_QUEUE(x, ...) dprintf(x, ## __VA_ARGS__)
#else
#	define TRACE_QUEUE(x, ...) ;
#endif


static unsigned int
_rand32(void)
{
	static unsigned int next = 0;
	if (next == 0)
		next = real_time_clock_usecs() / 1000000;

	next = (next >> 1) ^ (unsigned int)((0 - (next & 1U)) & 0xd0000001U);
		// characteristic polynomial: x^32 + x^31 + x^29 + x + 1
	return next;
}


static unsigned short
_rand14(void)
{
	// TODO: Find suitable generator polynomial.
	return _rand32() & 0x3fff;
}


TCPPacket::TCPPacket()
	:
	fData(NULL),
	fNext(NULL)
{
}


TCPPacket::~TCPPacket()
{
	free(fData);
}


status_t
TCPPacket::SetTo(const void* data, size_t size, ip_addr_t sourceAddress,
	uint16 sourcePort, ip_addr_t destinationAddress, uint16 destinationPort,
	uint32 sequenceNumber, uint32 acknowledgmentNumber, uint8 flags)
{
	if (data == NULL && size > 0)
		return B_BAD_VALUE;

	if (size > 0) {
		fData = malloc(size);
		if (fData == NULL)
			return B_NO_MEMORY;
		memcpy(fData, data, size);
	} else
		fData = NULL;

	fSize = size;
	fSourceAddress = sourceAddress;
	fSourcePort = sourcePort;
	fDestinationAddress = destinationAddress;
	fDestinationPort = destinationPort;
	fSequenceNumber = sequenceNumber;
	fAcknowledgmentNumber = acknowledgmentNumber;
	fFlags = flags;

	return B_OK;
}


ip_addr_t
TCPPacket::SourceAddress() const
{
	return fSourceAddress;
}


ip_addr_t
TCPPacket::DestinationAddress() const
{
	return fDestinationAddress;
}


uint16
TCPPacket::SourcePort() const
{
	return fSourcePort;
}


uint16
TCPPacket::DestinationPort() const
{
	return fDestinationPort;
}


uint32
TCPPacket::SequenceNumber() const
{
	return fSequenceNumber;
}


uint32
TCPPacket::AcknowledgmentNumber() const
{
	return fAcknowledgmentNumber;
}


bool
TCPPacket::ProvidesSequenceNumber(uint32 sequenceNumber) const
{
	// TODO PAWS
	return fSequenceNumber <= sequenceNumber
		&& fSequenceNumber + fSize > sequenceNumber;
}


TCPPacket*
TCPPacket::Next() const
{
	return fNext;
}


void
TCPPacket::SetNext(TCPPacket* packet)
{
	fNext = packet;
}




TCPSocket::TCPSocket()
	:
	fTCPService(NetStack::Default()->GetTCPService()),
	fAddress(INADDR_ANY),
	fPort(0),
	fSequenceNumber(0),
	fFirstPacket(NULL),
	fLastPacket(NULL),
	fFirstSentPacket(NULL),
	fLastSentPacket(NULL),
	fState(TCP_SOCKET_STATE_INITIAL),
	fRemoteState(TCP_SOCKET_STATE_INITIAL)
{
}


TCPSocket::~TCPSocket()
{
	if (fTCPService != NULL && fPort != 0)
		fTCPService->UnbindSocket(this);
}


uint16
TCPSocket::WindowSize() const
{
	// TODO A large window size leads to read timeouts
	// due to resends occuring too late.
#if 0
	size_t windowSize = 0xffff;
	for (TCPPacket* packet = fFirstPacket;
			packet != NULL && windowSize > packet->DataSize();
			packet = packet->Next())
		windowSize -= packet->DataSize();
	return windowSize;
#else
	return 4096;
#endif
}


status_t
TCPSocket::Connect(ip_addr_t address, uint16 port)
{
	fRemoteAddress = address;
	fRemotePort = port;
	fSequenceNumber = _rand32();
	fPort = 0xC000 + (_rand14() & ~0xc000);
	TRACE_PORT("TCPSocket::Connect(): connecting from port %u\n", fPort);
	fAcknowledgeNumber = 0;
	fNextSequence = 0;

	status_t error = fTCPService->BindSocket(this);
	if (error != B_OK)
		return error;

	// send SYN
	TCPPacket* packet = new(nothrow) TCPPacket();
	if (packet == NULL)
		return B_NO_MEMORY;
	error = packet->SetTo(NULL, 0, fAddress, fPort, address, port,
		fSequenceNumber, fAcknowledgeNumber, TCP_SYN);
	if (error != B_OK) {
		delete packet;
		return error;
	}
	error = _Send(packet);
	if (error != B_OK)
		return error;
	fState = TCP_SOCKET_STATE_SYN_SENT;
	fSequenceNumber++;
	TRACE("SYN sent\n");

	// receive SYN-ACK
	error = _WaitForState(TCP_SOCKET_STATE_OPEN, 1000000LL);
	if (error != B_OK) {
		TRACE("no SYN-ACK received\n");
		return error;
	}
	TRACE("SYN-ACK received\n");

	return B_OK;
}


status_t
TCPSocket::Close()
{
	// send FIN
	TCPPacket* packet = new(nothrow) TCPPacket();
	if (packet == NULL)
		return B_NO_MEMORY;
	status_t error = packet->SetTo(NULL, 0, fAddress, fPort, fRemoteAddress,
		fRemotePort, fSequenceNumber, fAcknowledgeNumber, TCP_FIN | TCP_ACK);
	if (error != B_OK) {
		delete packet;
		return error;
	}
	error = _Send(packet);
	if (error != B_OK)
		return error;
	fState = TCP_SOCKET_STATE_FIN_SENT;
	TRACE("FIN sent\n");

	error = _WaitForState(TCP_SOCKET_STATE_CLOSED, 1000000LL);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
TCPSocket::Read(void* buffer, size_t bufferSize, size_t* bytesRead,
	bigtime_t timeout)
{
	TRACE("TCPSocket::Read(): size = %lu\n", bufferSize);
	if (bytesRead == NULL)
		return B_BAD_VALUE;

	*bytesRead = 0;
	TCPPacket* packet = NULL;
	
	bigtime_t startTime = system_time();
	do {
		fTCPService->ProcessIncomingPackets();
		//_ResendQueue();
		packet = _PeekPacket();
		if (packet == NULL && fRemoteState != TCP_SOCKET_STATE_OPEN)
			return B_ERROR;
		if (packet == NULL && timeout > 0LL)
			_Ack();
	} while (packet == NULL && system_time() - startTime < timeout);
	if (packet == NULL) {
#ifdef TRACE_TCP_QUEUE
		_DumpQueue();
#endif
		return (timeout == 0) ? B_WOULD_BLOCK : B_TIMED_OUT;
	}
	uint32 packetOffset = fNextSequence - packet->SequenceNumber();
	size_t readBytes = packet->DataSize() - packetOffset;
	if (readBytes > bufferSize)
		readBytes = bufferSize;
	if (buffer != NULL)
		memcpy(buffer, (uint8*)packet->Data() + packetOffset, readBytes);
	*bytesRead = readBytes;
	if (!packet->ProvidesSequenceNumber(fNextSequence + readBytes)) {
		_DequeuePacket();
		delete packet;
		packet = NULL;
	}
	fNextSequence += readBytes;

	if (packet == NULL && *bytesRead < bufferSize) {
		do {
			if (buffer != NULL)
				buffer = (uint8*)buffer + readBytes;
			bufferSize -= readBytes;
			fTCPService->ProcessIncomingPackets();
			packet = _PeekPacket();
			if (packet == NULL && fRemoteState != TCP_SOCKET_STATE_OPEN)
				break;
			readBytes = 0;
			if (packet == NULL) {
				_Ack();
				continue;
			}
			readBytes = packet->DataSize();
			if (readBytes > bufferSize)
				readBytes = bufferSize;
			if (buffer != NULL)
				memcpy(buffer, packet->Data(), readBytes);
			*bytesRead += readBytes;
			if (readBytes == packet->DataSize()) {
				_DequeuePacket();
				delete packet;
			}
			fNextSequence += readBytes;
		} while (readBytes < bufferSize &&
			system_time() - startTime < timeout);
#ifdef TRACE_TCP_QUEUE
		if (readBytes < bufferSize) {
			TRACE_QUEUE("TCP: Unable to deliver more data!\n");
			_DumpQueue();
		}
#endif
	}

	return B_OK;
}


status_t
TCPSocket::Write(const void* buffer, size_t bufferSize)
{
	if (buffer == NULL || bufferSize == 0)
		return B_BAD_VALUE;

	// TODO: Check for MTU and create multiple packets if necessary.

	TCPPacket* packet = new(nothrow) TCPPacket();
	if (packet == NULL)
		return B_NO_MEMORY;
	status_t error = packet->SetTo(buffer, bufferSize, fAddress, fPort,
		fRemoteAddress, fRemotePort, fSequenceNumber, fAcknowledgeNumber,
		TCP_ACK);
	if (error != B_OK) {
		delete packet;
		return error;
	}
	return _Send(packet);
}


void
TCPSocket::Acknowledge(uint32 number)
{
	TRACE("TCPSocket::Acknowledge(): %lu\n", number);
	// dequeue packets
	for (TCPPacket* packet = fFirstSentPacket; packet != NULL;
			packet = fFirstSentPacket) {
		if (packet->SequenceNumber() >= number)
			return;
		fFirstSentPacket = packet->Next();
		delete packet;
	}
	fLastSentPacket = NULL;
}


void
TCPSocket::ProcessPacket(TCPPacket* packet)
{
	TRACE("TCPSocket::ProcessPacket()\n");

	if ((packet->Flags() & TCP_FIN) != 0) {
		fRemoteState = TCP_SOCKET_STATE_FIN_SENT;
		TRACE("FIN received\n");
		_Ack();
	}

	if (fState == TCP_SOCKET_STATE_SYN_SENT) {
		if ((packet->Flags() & TCP_SYN) != 0
				&& (packet->Flags() & TCP_ACK) != 0) {
			fNextSequence = fAcknowledgeNumber = packet->SequenceNumber() + 1;
			fRemoteState = TCP_SOCKET_STATE_SYN_SENT;
			delete packet;
			_Ack();
			fState = fRemoteState = TCP_SOCKET_STATE_OPEN;
			return;
		}
	} else if (fState == TCP_SOCKET_STATE_OPEN) {
	} else if (fState == TCP_SOCKET_STATE_FIN_SENT) {
		if ((packet->Flags() & TCP_ACK) != 0) {
			TRACE("FIN-ACK received\n");
			if (fRemoteState == TCP_SOCKET_STATE_FIN_SENT)
				fState = TCP_SOCKET_STATE_CLOSED;
		}
	}

	if (packet->DataSize() == 0) {
		TRACE("TCPSocket::ProcessPacket(): not queuing due to lack of data\n");
		delete packet;
		return;
	}

	// For now rather protect us against being flooded with packets already
	// acknowledged. "If it's important, they'll send it again."
	// TODO PAWS
	if (packet->SequenceNumber() < fAcknowledgeNumber) {
		TRACE_QUEUE("TCPSocket::ProcessPacket(): not queuing due to wraparound\n");
		delete packet;
		return;
	}

	if (fLastPacket == NULL) {
		// no packets enqueued
		TRACE("TCPSocket::ProcessPacket(): first in queue\n");
		packet->SetNext(NULL);
		fFirstPacket = fLastPacket = packet;
	} else if (fLastPacket->SequenceNumber() < packet->SequenceNumber()) {
		// enqueue in back
		TRACE("TCPSocket::ProcessPacket(): enqueue in back\n");
		packet->SetNext(NULL);
		fLastPacket->SetNext(packet);
		fLastPacket = packet;
	} else if (fFirstPacket->SequenceNumber() > packet->SequenceNumber()) {
		// enqueue in front
		TRACE("TCPSocket::ProcessPacket(): enqueue in front\n");
		TRACE_QUEUE("TCP: Enqueuing %lx - %lx in front! (next is %lx)\n",
			packet->SequenceNumber(),
			packet->SequenceNumber() + packet->DataSize() - 1,
			fNextSequence);
		packet->SetNext(fFirstPacket);
		fFirstPacket = packet;
	} else if (fFirstPacket->SequenceNumber() == packet->SequenceNumber()) {
		TRACE_QUEUE("%s(): dropping due to identical first packet\n", __func__);
		delete packet;
		return;
	} else {
		// enqueue in middle
		TRACE("TCPSocket::ProcessPacket(): enqueue in middle\n");
		for (TCPPacket* queuedPacket = fFirstPacket; queuedPacket != NULL;
				queuedPacket = queuedPacket->Next()) {
			if (queuedPacket->SequenceNumber() == packet->SequenceNumber()) {
				TRACE_QUEUE("TCPSocket::EnqueuePacket(): TCP packet dropped\n");
				// we may be waiting for a previous packet
				delete packet;
				return;
			}
			if (queuedPacket->Next()->SequenceNumber()
					> packet->SequenceNumber()) {
				packet->SetNext(queuedPacket->Next());
				queuedPacket->SetNext(packet);
				break;
			}
		}
	}
	while (packet != NULL && packet->SequenceNumber() == fAcknowledgeNumber) {
		fAcknowledgeNumber = packet->SequenceNumber() + packet->DataSize();
		packet = packet->Next();
	}
}


TCPPacket*
TCPSocket::_PeekPacket()
{
	TRACE("TCPSocket::_PeekPacket(): fNextSequence = %lu\n", fNextSequence);
	for (TCPPacket* packet = fFirstPacket; packet != NULL;
			packet = packet->Next()) {
		if (packet->ProvidesSequenceNumber(fNextSequence))
			return packet;
	}
	return NULL;
}


TCPPacket*
TCPSocket::_DequeuePacket()
{
	//TRACE("TCPSocket::DequeuePacket()\n");
	if (fFirstPacket == NULL)
		return NULL;

	if (fFirstPacket->ProvidesSequenceNumber(fNextSequence)) {
		TCPPacket* packet = fFirstPacket;
		fFirstPacket = packet->Next();
		if (fFirstPacket == NULL)
			fLastPacket = NULL;
		packet->SetNext(NULL);
		TRACE("TCP: Dequeuing %lx - %lx from front.\n",
			packet->SequenceNumber(),
			packet->SequenceNumber() + packet->DataSize() - 1);
		return packet;
	}

	for (TCPPacket* packet = fFirstPacket;
			packet != NULL && packet->Next() != NULL;
			packet = packet->Next()) {
		if (packet->Next()->ProvidesSequenceNumber(fNextSequence)) {
			TCPPacket* nextPacket = packet->Next();
			packet->SetNext(nextPacket->Next());
			if (fLastPacket == nextPacket)
				fLastPacket = packet;
			TRACE("TCP: Dequeuing %lx - %lx.\n",
				nextPacket->SequenceNumber(),
				nextPacket->SequenceNumber() + nextPacket->DataSize() - 1);
			return nextPacket;
		}
	}
	TRACE_QUEUE("dequeue failed!\n");
	return NULL;
}


status_t
TCPSocket::_Send(TCPPacket* packet, bool enqueue)
{
	ChainBuffer buffer((void*)packet->Data(), packet->DataSize());
	status_t error = fTCPService->Send(fPort, fRemoteAddress, fRemotePort,
		packet->SequenceNumber(), fAcknowledgeNumber, packet->Flags(),
		WindowSize(), &buffer);
	if (error != B_OK)
		return error;
	if (packet->SequenceNumber() == fSequenceNumber)
		fSequenceNumber += packet->DataSize();

	if (enqueue)
		_EnqueueOutgoingPacket(packet);

	return B_OK;
}


status_t
TCPSocket::_ResendQueue()
{
	TRACE("resending queue\n");
	for (TCPPacket* packet = fFirstSentPacket; packet != NULL;
			packet = packet->Next()) {
		ChainBuffer buffer((void*)packet->Data(), packet->DataSize());
		status_t error = fTCPService->Send(fPort, fRemoteAddress, fRemotePort,
			packet->SequenceNumber(), fAcknowledgeNumber, packet->Flags(),
			WindowSize(), &buffer);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}


void
TCPSocket::_EnqueueOutgoingPacket(TCPPacket* packet)
{
	if (fLastSentPacket != NULL) {
		fLastSentPacket->SetNext(packet);
		fLastSentPacket = packet;
	} else {
		fFirstSentPacket = fLastSentPacket = packet;
	}
}


#ifdef TRACE_TCP_QUEUE

inline void
TCPSocket::_DumpQueue()
{
	TRACE_QUEUE("TCP: waiting for %lx (ack'ed %lx)\n", fNextSequence, fAcknowledgeNumber);
	if (fFirstPacket == NULL)
		TRACE_QUEUE("TCP: Queue is empty.\n");
	else {
		for (TCPPacket* packet = fFirstPacket; packet != NULL;
				packet = packet->Next()) {
			TRACE_QUEUE("TCP: Queue: %lx\n", packet->SequenceNumber());
		}
	}
	if (fFirstSentPacket != NULL)
		TRACE_QUEUE("TCP: Send queue is non-empty.\n");
	else
		TRACE_QUEUE("TCP: Send queue is empty.\n");
}

#endif


status_t
TCPSocket::_Ack()
{
	TCPPacket* packet = new(nothrow) TCPPacket();
	if (packet == NULL)
		return B_NO_MEMORY;
	status_t error = packet->SetTo(NULL, 0, fAddress, fPort, fRemoteAddress,
		fRemotePort, fSequenceNumber, fAcknowledgeNumber, TCP_ACK);
	if (error != B_OK) {
		delete packet;
		return error;
	}
	error = _Send(packet, false);
	delete packet;
	if (error != B_OK)
		return error;
	return B_OK;
}


status_t
TCPSocket::_WaitForState(TCPSocketState state, bigtime_t timeout)
{
	if (fTCPService == NULL)
		return B_NO_INIT;

	bigtime_t startTime = system_time();
	do {
		fTCPService->ProcessIncomingPackets();
		if (fState == state)
			return B_OK;
	} while (system_time() - startTime < timeout);
	return timeout == 0 ? B_WOULD_BLOCK : B_TIMED_OUT;
}




TCPService::TCPService(IPService* ipService)
	:
	IPSubService(kTCPServiceName),
	fIPService(ipService)
{
}


TCPService::~TCPService()
{
	if (fIPService != NULL)
		fIPService->UnregisterIPSubService(this);
}


status_t
TCPService::Init()
{
	if (fIPService == NULL)
		return B_BAD_VALUE;

	if (!fIPService->RegisterIPSubService(this))
		return B_NO_MEMORY;

	return B_OK;
}


uint8
TCPService::IPProtocol() const
{
	return IPPROTO_TCP;
}


void
TCPService::HandleIPPacket(IPService* ipService, ip_addr_t sourceIP,
	ip_addr_t destinationIP, const void* data, size_t size)
{
	TRACE("TCPService::HandleIPPacket(): source = %08lx, "
		"destination = %08lx, %lu - %lu bytes\n", sourceIP, destinationIP,
		size, sizeof(tcp_header));

	if (data == NULL || size < sizeof(tcp_header))
		return;

	const tcp_header* header = (const tcp_header*)data;

	uint16 chksum = _ChecksumData(data, size, sourceIP, destinationIP);
	if (chksum != 0) {
		TRACE_CHECKSUM("TCPService::HandleIPPacket(): invalid checksum "
			"(%04x vs. %04x), padding %lu\n",
			header->checksum, chksum, size % 2);
		return;
	}

	uint16 source = ntohs(header->source);
	uint16 destination = ntohs(header->destination);
	uint32 sequenceNumber = ntohl(header->seqNumber);
	uint32 ackedNumber = ntohl(header->ackNumber);
	TRACE("\tsource = %u, dest = %u, seq = %lu, ack = %lu, dataOffset = %u, "
		"flags %s %s %s %s\n", source, destination, sequenceNumber,
		ackedNumber, header->dataOffset,
		(header->flags & TCP_ACK) != 0 ? "ACK" : "",
		(header->flags & TCP_SYN) != 0 ? "SYN" : "",
		(header->flags & TCP_FIN) != 0 ? "FIN" : "",
		(header->flags & TCP_RST) != 0 ? "RST" : "");
	if (header->dataOffset > 5) {
		uint8* option = (uint8*)data + sizeof(tcp_header);
		while ((uint32*)option < (uint32*)data + header->dataOffset) {
			uint8 optionKind = option[0];
			if (optionKind == 0)
				break;
			uint8 optionLength = 1;
			if (optionKind > 1) {
				optionLength = option[1];
				TRACE("\tTCP option kind %u, length %u\n",
					optionKind, optionLength);
				if (optionKind == 2)
					TRACE("\tTCP MSS = %04hu\n", *(uint16_t*)&option[2]);
			}
			option += optionLength;
		}
	}

	TCPSocket* socket = _FindSocket(destinationIP, destination);
	if (socket == NULL) {
		// TODO If SYN, answer with RST?
		TRACE("TCPService::HandleIPPacket(): no socket\n");
		return;
	}

	if ((header->flags & TCP_ACK) != 0) {
		socket->Acknowledge(ackedNumber);
	}

	TCPPacket* packet = new(nothrow) TCPPacket();
	if (packet == NULL)
		return;
	status_t error = packet->SetTo((uint32*)data + header->dataOffset,
		size - header->dataOffset * 4, sourceIP, source, destinationIP,
		destination, sequenceNumber, ackedNumber, header->flags);
	if (error == B_OK)
		socket->ProcessPacket(packet);
	else
		delete packet;
}


status_t
TCPService::Send(uint16 sourcePort, ip_addr_t destinationAddress,
	uint16 destinationPort, uint32 sequenceNumber,
	uint32 acknowledgmentNumber, uint8 flags, uint16 windowSize,
	ChainBuffer* buffer)
{
	TRACE("TCPService::Send(): seq = %lu, ack = %lu\n",
		sequenceNumber, acknowledgmentNumber);
	if (fIPService == NULL)
		return B_NO_INIT;
	if (buffer == NULL)
		return B_BAD_VALUE;

	tcp_header header;
	ChainBuffer headerBuffer(&header, sizeof(header), buffer);
	memset(&header, 0, sizeof(header));
	header.source = htons(sourcePort);
	header.destination = htons(destinationPort);
	header.seqNumber = htonl(sequenceNumber);
	header.ackNumber = htonl(acknowledgmentNumber);
	header.dataOffset = 5;
	header.flags = flags;
	header.window = htons(windowSize);

	header.checksum = 0;
	header.checksum = htons(_ChecksumBuffer(&headerBuffer,
		fIPService->IPAddress(), destinationAddress,
		headerBuffer.TotalSize()));

	return fIPService->Send(destinationAddress, IPPROTO_TCP, &headerBuffer);
}


void
TCPService::ProcessIncomingPackets()
{
	if (fIPService != NULL)
		fIPService->ProcessIncomingPackets();
}


status_t
TCPService::BindSocket(TCPSocket* socket)
{
	if (socket == NULL)
		return B_BAD_VALUE;

	if (_FindSocket(socket->Address(), socket->Port()) != NULL)
		return EADDRINUSE;

	return fSockets.Add(socket);
}


void
TCPService::UnbindSocket(TCPSocket* socket)
{
	fSockets.Remove(socket);
}


uint16
TCPService::_ChecksumBuffer(ChainBuffer* buffer, ip_addr_t source,
	ip_addr_t destination, uint16 length)
{
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
		IPPROTO_TCP,
		htons(length)
	};

	ChainBuffer headerBuffer(&header, sizeof(header), buffer);
	uint16 checksum = ip_checksum(&headerBuffer);
	headerBuffer.DetachNext();
	return checksum;
}


uint16
TCPService::_ChecksumData(const void* data, uint16 length, ip_addr_t source,
	ip_addr_t destination)
{
	ChainBuffer buffer((void*)data, length);
	return _ChecksumBuffer(&buffer, source, destination, length);
}


TCPSocket*
TCPService::_FindSocket(ip_addr_t address, uint16 port)
{
	for (int i = 0; i < fSockets.Count(); i++) {
		TCPSocket* socket = fSockets.ElementAt(i);
		// TODO Remove socket->Address() INADDR_ANY check once the socket is
		// aware of both its IP addresses (local one is INADDR_ANY for now).
		if ((address == INADDR_ANY || socket->Address() == INADDR_ANY
					|| socket->Address() == address)
				&& socket->Port() == port) {
			return socket;
		}
	}
	return NULL;
}
