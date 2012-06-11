/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Request.h"


status_t
Request::Send()
{
	return _TrySend();
}


status_t
Request::_TrySend()
{
	RPC::Reply *rpl;
	RPC::Request *rpc;

	status_t result = fServer->SendCallAsync(fBuilder.Request(), &rpl, &rpc);
	if (result != B_OK)
		return result;

	result = fServer->WaitCall(rpc);
	if (result != B_OK) {
		fServer->CancelCall(rpc);
		delete rpc;
		return result;
	}

	return fReply.SetTo(rpl);
}


void
Request::Reset()
{
	fBuilder.Reset();
	fReply.Reset();
}

