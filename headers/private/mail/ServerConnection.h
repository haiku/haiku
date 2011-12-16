/*
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H


#include "SupportDefs.h"


namespace BPrivate {


class AbstractConnection;


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

			ssize_t				Read(char* buffer, uint32 length);
			ssize_t				Write(const char* buffer, uint32 length);

private:
			AbstractConnection*	fConnection;
};


}	// namespace BPrivate


using BPrivate::ServerConnection;


#endif // SERVER_CONNECTION_H
