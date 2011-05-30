/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H


#include "SupportDefs.h"


class AbstractConnection {
public:
	virtual						~AbstractConnection();

	virtual status_t			Connect(const char* server, uint32 port) = 0;
	virtual status_t			Disconnect() = 0;

	virtual status_t			WaitForData(bigtime_t timeout) = 0;

	virtual int32				Read(char* buffer, uint32 nBytes) = 0;
	virtual int32				Write(const char* buffer, uint32 nBytes) = 0;
};


class ServerConnection {
public:
								ServerConnection();
								~ServerConnection();

			status_t			ConnectSSL(const char* server,
									uint32 port = 993);

			status_t			ConnectSocket(const char* server,
									uint32 port = 143);
			status_t			Disconnect();

			status_t			WaitForData(bigtime_t timeout);

			int32				Read(char* buffer, uint32 nBytes);
			int32				Write(const char* buffer, uint32 nBytes);

private:
			AbstractConnection*	fConnection;
};

#endif // SERVER_CONNECTION_H
