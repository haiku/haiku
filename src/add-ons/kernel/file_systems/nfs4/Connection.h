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

	static	status_t			ResolveName(const char* name,
									ServerAddress* addr);
};

class Connection {
public:
	static	status_t			Connect(Connection **conn,
									const ServerAddress& id);
	virtual						~Connection();

	virtual	status_t			Send(const void* buffer, uint32 size) = 0;
	virtual	status_t			Receive(void** buffer, uint32* size) = 0;

			status_t			GetLocalID(ServerAddress* addr);

			status_t			Reconnect();
			void				Disconnect();

protected:
								Connection(const sockaddr_in& addr,
									Transport proto);
			status_t			_Connect();

			int					fSock;
			mutex				fSockLock;

			const Transport		fProtocol;
			const sockaddr_in	fServerAddress;
};

class ConnectionStream : public Connection {
public:
								ConnectionStream(const sockaddr_in& addr,
									Transport proto);

	virtual	status_t			Send(const void* buffer, uint32 size);
	virtual	status_t			Receive(void** buffer, uint32* size);
};

class ConnectionPacket : public Connection {
public:
								ConnectionPacket(const sockaddr_in& addr,
									Transport proto);

	virtual	status_t			Send(const void* buffer, uint32 size);
	virtual	status_t			Receive(void** buffer, uint32* size);
};

#endif	// CONNECTION_H

