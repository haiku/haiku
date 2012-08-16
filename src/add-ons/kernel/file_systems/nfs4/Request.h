/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REQUEST_H
#define REQUEST_H


#include "ReplyInterpreter.h"
#include "RequestBuilder.h"
#include "RPCServer.h"


class Cookie;
class FileSystem;

class Request {
public:
	inline						Request(RPC::Server* server,
									FileSystem* fileSystem);

	inline	RequestBuilder&		Builder();
	inline	ReplyInterpreter&	Reply();

			status_t			Send(Cookie* cookie = NULL);
			void				Reset();

private:
			status_t			_SendUDP(Cookie* cookie);
			status_t			_SendTCP(Cookie* cookie);

			RPC::Server*		fServer;
			FileSystem*			fFileSystem;

			RequestBuilder		fBuilder;
			ReplyInterpreter	fReply;
};


inline
Request::Request(RPC::Server* server, FileSystem* fileSystem)
	:
	fServer(server),
	fFileSystem(fileSystem)
{
}


inline RequestBuilder&
Request::Builder()
{
	return fBuilder;
}


inline ReplyInterpreter&
Request::Reply()
{
	return fReply;
}


#endif	// REQUEST_H

