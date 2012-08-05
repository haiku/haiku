/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCCallback.h"

#include "RPCCallbackRequest.h"


using namespace RPC;


status_t
Callback::EnqueueRequest(CallbackRequest* request, Connection* connection)
{
	dprintf("GOT A CALLBACK REQUEST %x\n", (int)request->XID());
	return B_OK;
}

