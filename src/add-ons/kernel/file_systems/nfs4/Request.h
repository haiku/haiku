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

class Request {
public:
	inline						Request(RPC::Server* serv);

	inline	RequestBuilder&		Builder();
	inline	ReplyInterpreter&	Reply();

			status_t			Send(Cookie* cookie = NULL);
			void				Reset();

private:
			status_t			_SendUDP(Cookie* cookie);
			status_t			_SendTCP(Cookie* cookie);

			RPC::Server*		fServer;

			RequestBuilder		fBuilder;
			ReplyInterpreter	fReply;

	static	const int			kRetryLimit = 5;
};


inline
Request::Request(RPC::Server* serv)
	:
	fServer(serv)
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

