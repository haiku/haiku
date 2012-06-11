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


class Request {
public:
	inline						Request(RPC::Server* serv);

	inline	RequestBuilder&		Builder();
	inline	ReplyInterpreter&	Reply();

			status_t			Send();
			void				Reset();

private:
			status_t			_SendUDP();
			status_t			_SendTCP();

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

