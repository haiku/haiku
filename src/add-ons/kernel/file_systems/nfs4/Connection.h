/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef CONNECTION_H
#define CONNECTION_H


#include <netinet/in.h>

#include <lock.h>
#include <SupportDefs.h>


enum Transport {
		ProtocolTCP		= 6,
		ProtocolUDP		= 11
};

struct ServerAddress {
			uint32				fAddress;
			uint16				fPort;
			Transport			fProtocol;

			bool				operator==(const ServerAddress& x);
			bool				operator<(const ServerAddress& x);

			ServerAddress&		operator=(const ServerAddress& x);
};

class Connection {
public:
	static	status_t			Connect(Connection **conn,
									const ServerAddress& id);
								~Connection();

	inline	status_t			Send(const void* buffer, uint32 size);
	inline	status_t			Receive(void** buffer, uint32* size);

			status_t			GetLocalID(ServerAddress* addr);

			status_t			Reconnect();
			void				Disconnect();

private:
								Connection(const sockaddr_in& addr,
									Transport proto, bool markers);
			status_t			_Connect();

			status_t			_SendStream(const void* buffer, uint32 size);
			status_t			_SendPacket(const void* buffer, uint32 size);

			status_t			_ReceiveStream(void** buffer, uint32* size);
			status_t			_ReceivePacket(void** buffer, uint32* size);

			int					fSock;
			mutex				fSockLock;

			const bool			fUseMarkers;

			const Transport		fProtocol;
			const sockaddr_in	fServerAddress;
};


inline status_t
Connection::Send(const void* buffer, uint32 size)
{
	if (fUseMarkers) 
		return _SendStream(buffer, size);
	else
		return _SendPacket(buffer, size);
}


inline status_t
Connection::Receive(void** buffer, uint32* size)
{
	if (fUseMarkers)
		return _ReceiveStream(buffer, size);
	else
		return _ReceivePacket(buffer, size);
}


#endif	// CONNECTION_H

