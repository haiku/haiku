/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_REMOTE_DISK_H
#define _BOOT_REMOTE_DISK_H

#include <boot/vfs.h>
#include <boot/net/NetDefs.h>
#include <boot/net/RemoteDiskDefs.h>

class UDPPacket;
class UDPSocket;

class RemoteDisk : public Node {
public:
	RemoteDisk();
	~RemoteDisk();

	status_t Init(ip_addr_t serverAddress, uint16 serverPort, off_t imageSize);

	virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
		size_t bufferSize);
	virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
		size_t bufferSize);

	virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
	virtual off_t Size() const;

	ip_addr_t ServerIPAddress() const;
	uint16 ServerPort() const;

	static RemoteDisk *FindAnyRemoteDisk();

private:
	ssize_t _ReadFromPacket(off_t &pos, uint8 *&buffer, size_t &bufferSize);
	
	static status_t _SendRequest(UDPSocket *socket, ip_addr_t serverAddress,
		uint16 serverPort, remote_disk_header *request, size_t size,
		uint8 expectedReply, UDPPacket **packet);
	status_t _SendRequest(remote_disk_header *request, size_t size,
		uint8 expectedReply, UDPPacket **packet);

private:
	ip_addr_t	fServerAddress;
	uint16		fServerPort;
	off_t		fImageSize;
	uint64		fRequestID;
	UDPSocket	*fSocket;
	UDPPacket	*fPacket;
};

#endif	// _BOOT_REMOTE_DISK_H
