/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCCallbackReply.h"

#include <util/kernel_cpp.h>

#include "RPCDefs.h"


using namespace RPC;


CallbackReply::CallbackReply()
{
}


CallbackReply*
CallbackReply::Create(uint32 xid, AcceptStat rpcError)
{
	CallbackReply* reply = new(std::nothrow) CallbackReply;
	if (reply == NULL)
		return NULL;

	reply->fStream.AddUInt(xid);

	reply->fStream.AddInt(REPLY);
	reply->fStream.AddUInt(MSG_ACCEPTED);

	reply->fStream.AddInt(AUTH_NONE);
	reply->fStream.AddOpaque(NULL, 0);

	reply->fStream.AddUInt(rpcError);
	
	return reply;
}


CallbackReply::~CallbackReply()
{
}

