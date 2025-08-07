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


struct Cookie;
class FileSystem;

class Request {
public:
	inline						Request(RPC::Server* server,
									FileSystem* fileSystem,
									uid_t uid, gid_t gid, bool singleAttempt = false);

	inline	RequestBuilder&		Builder();
	inline	ReplyInterpreter&	Reply();

			status_t			Send(Cookie* cookie = NULL);
			void				Reset(uid_t uid, gid_t gid);

private:
			status_t			_SendUDP(Cookie* cookie);
			status_t			_SendTCP(Cookie* cookie);

			RPC::Server*		fServer;
			FileSystem*			fFileSystem;

			RequestBuilder		fBuilder;
			ReplyInterpreter	fReply;

			bool				fSingleAttempt;
};


inline
Request::Request(RPC::Server* server, FileSystem* fileSystem, uid_t uid, gid_t gid,
	bool singleAttempt)
	:
	fServer(server),
	fFileSystem(fileSystem),
	fBuilder(uid, gid),
	fSingleAttempt(singleAttempt)
{
	ASSERT(server != NULL);
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

