/*
 * Copyright 2005-2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BOOT_REMOTE_DISK_H
#define _BOOT_REMOTE_DISK_H

#include <netinet/in.h>

#include <SupportDefs.h>

#include <boot/net/RemoteDiskDefs.h>


class RemoteDisk {
public:
								RemoteDisk();
								~RemoteDisk();

			status_t			Init(uint32 serverAddress, uint16 serverPort,
									off_t imageSize);

			status_t			FindAnyRemoteDisk();

			ssize_t				ReadAt(off_t pos, void *buffer,
									size_t bufferSize);
			ssize_t				WriteAt(off_t pos, const void *buffer,
									size_t bufferSize);

			off_t				Size() const
									{ return fImageSize; }

			bool				IsReadOnly() const
									{ return false; }

// 			uint32				ServerIPAddress() const
// 									{ return fServerAddress; }
// 			uint16				ServerPort() const
// 									{ return fServerPort; }

private:
			status_t			_Init();

			ssize_t				_ReadFromPacket(off_t& pos, uint8*& buffer,
									size_t& bufferSize);
	
			status_t			_SendRequest(remote_disk_header *request,
									size_t size, uint8 expectedReply,
									sockaddr_in* peerAddress = NULL);
			status_t			_SendRequest(remote_disk_header *request,
									size_t size, uint8 expectedReply,
									sockaddr_in* peerAddress,
									void* receiveBuffer,
									size_t receiveBufferSize,
									int32* bytesReceived);

private:
			sockaddr_in			fSocketAddress;
			sockaddr_in			fServerAddress;
			off_t				fImageSize;
			uint64				fRequestID;
			int					fSocket;
			void*				fPacket;
			int32				fPacketSize;
};

#endif	// _BOOT_REMOTE_DISK_H
