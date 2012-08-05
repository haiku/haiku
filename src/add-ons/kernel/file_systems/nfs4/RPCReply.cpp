/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCReply.h"

#include <util/kernel_cpp.h>

#include "RPCDefs.h"


using namespace RPC;


Reply::Reply(void *buffer, int size)
	:
	fError(B_OK),
	fStream(buffer, size),
	fBuffer(buffer)
{
	fXID = fStream.GetUInt();
	if (fStream.GetInt() != REPLY) {
		fError = B_BAD_VALUE;
		return;
	}

	if (fStream.GetInt() == MSG_ACCEPTED) {
		fStream.GetInt();
		fStream.GetOpaque(NULL);

		switch (fStream.GetInt()) {
			case SUCCESS:
				return;
			case PROG_UNAVAIL:
			case PROG_MISMATCH:
			case PROC_UNAVAIL:
				fError = B_DEVICE_NOT_FOUND;
				return;
			case GARBAGE_ARGS:
				fError = B_MISMATCHED_VALUES;
				return;
			case SYSTEM_ERR:
				fError = B_ERROR;
				return;
			default:
				fError = B_BAD_VALUE;
				return;
		}
	} else {		// MSG_DENIED
		if (fStream.GetInt() == RPC_MISMATCH) {
			fError = B_DEVICE_NOT_FOUND;
			return;
		} else {	// AUTH_ERROR	
			fError = B_PERMISSION_DENIED;
			return;
		}
	}
}


Reply::~Reply()
{
	free(fBuffer);
}

