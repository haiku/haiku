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


struct ServerAddress {
			sockaddr_storage	fAddress;
			int					fProtocol;

			bool				operator==(const ServerAddress& address);
			bool				operator<(const ServerAddress& address);

			ServerAddress&		operator=(const ServerAddress& address);

								ServerAddress();

			const char*			ProtocolString() const;
			char*				UniversalAddress() const;

			socklen_t			AddressSize() const;

			void				SetPort(uint16 port);
			uint16				Port() const;

			const void*			InAddr() const;

	static	status_t			ResolveName(const char* name,
									ServerAddress* address);
};

class ConnectionBase {
public:
								ConnectionBase(const ServerAddress& address);
	virtual						~ConnectionBase();

			status_t			GetLocalAddress(ServerAddress* address);

			void				Disconnect();

protected:
			sem_id				fWaitCancel;
			int					fSocket;
			mutex				fSocketLock;

			const ServerAddress	fServerAddress;
};

class Connection : public ConnectionBase {
public:
	static	status_t			Connect(Connection **connection,
									const ServerAddress& address);
	static	status_t			SetTo(Connection **connection, int socket,
									const ServerAddress& address);

	virtual	status_t			Send(const void* buffer, uint32 size) = 0;
	virtual	status_t			Receive(void** buffer, uint32* size) = 0;

			status_t			Reconnect();

protected:
	static	Connection*			CreateObject(const ServerAddress& address);

								Connection(const ServerAddress& address);
			status_t			Connect();

};

class ConnectionStream : public Connection {
public:
								ConnectionStream(const ServerAddress& address);

	virtual	status_t			Send(const void* buffer, uint32 size);
	virtual	status_t			Receive(void** buffer, uint32* size);
};

class ConnectionPacket : public Connection {
public:
								ConnectionPacket(const ServerAddress& address);

	virtual	status_t			Send(const void* buffer, uint32 size);
	virtual	status_t			Receive(void** buffer, uint32* size);
};

class ConnectionListener : public ConnectionBase {
public:
	static	status_t	Listen(ConnectionListener** listener, uint16 port = 0);

			status_t	AcceptConnection(Connection** connection);

protected:
						ConnectionListener(const ServerAddress& address);
};

#endif	// CONNECTION_H

