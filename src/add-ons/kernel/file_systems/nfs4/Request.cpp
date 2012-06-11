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
	RPC::Reply *rpl;
	status_t result = fServer->SendCall(fBuilder.Request(), &rpl);
	if (result != B_OK)
		return result;

	return fReply.SetTo(rpl);
}


void
Request::Reset()
{
	fBuilder.Reset();
	fReply.Reset();
}

