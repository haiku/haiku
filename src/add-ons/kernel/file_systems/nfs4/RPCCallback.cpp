/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCCallback.h"

#include "RPCCallbackRequest.h"
#include "RPCServer.h"


using namespace RPC;


Callback::Callback(Server* server)
	:
	fServer(server)
{
}


status_t
Callback::EnqueueRequest(CallbackRequest* request, Connection* connection)
{
	return fServer->PrivateData()->ProcessCallback(request, connection);
}

