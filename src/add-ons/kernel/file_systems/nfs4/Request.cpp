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
	switch (fServer->ID().fProtocol) {
		case ProtocolUDP:	return _SendUDP();
		case ProtocolTCP:	return _SendTCP();
	}

	return B_BAD_VALUE;
}


status_t
Request::_SendUDP()
{
	RPC::Reply *rpl;
	RPC::Request *rpc;

	status_t result = fServer->SendCallAsync(fBuilder.Request(), &rpl, &rpc);
	if (result != B_OK)
		return result;

	result = fServer->WaitCall(rpc);
	if (result != B_OK) {
		int attempts = 1;
		while (result != B_OK && attempts++ < kRetryLimit) {
			result = fServer->ResendCallAsync(fBuilder.Request(), rpc);
			if (result != B_OK)
				return result;

			result = fServer->WaitCall(rpc);
		}

		if (result != B_OK) {
			fServer->CancelCall(rpc);
			delete rpc;
			return result;
		}
	}

	return fReply.SetTo(rpl);
}


status_t
Request::_SendTCP()
{
	RPC::Reply *rpl;
	RPC::Request *rpc;

	status_t result;
	int attempts = 0;
	do {
		result = fServer->SendCallAsync(fBuilder.Request(), &rpl, &rpc);
		if (result == B_NO_MEMORY)
			return result;
		else if (result != B_OK) {
			fServer->Repair();
			continue;
		}

		result = fServer->WaitCall(rpc);
		if (result != B_OK) {
			fServer->CancelCall(rpc);
			delete rpc;

			fServer->Repair();	
		}
	} while (result != B_OK && attempts++ < kRetryLimit);

	return fReply.SetTo(rpl);
}


void
Request::Reset()
{
	fBuilder.Reset();
	fReply.Reset();
}

