/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCReply.h"

#include <util/kernel_cpp.h>


using namespace RPC;

enum {
	REPLY	= 1
};

enum {
	MSG_ACCEPTED	= 0,
	MSG_DENIED		= 1
};

enum accept_stat {
	SUCCESS       = 0, /* RPC executed successfully       */
	PROG_UNAVAIL  = 1, /* remote hasn't exported program  */
	PROG_MISMATCH = 2, /* remote can't support version #  */
	PROC_UNAVAIL  = 3, /* program can't support procedure */
	GARBAGE_ARGS  = 4, /* procedure can't decode params   */
	SYSTEM_ERR    = 5  /* e.g. memory allocation failure  */
};

enum reject_stat {
	RPC_MISMATCH = 0, /* RPC version number != 2          */
	AUTH_ERROR = 1    /* remote can't authenticate caller */
};


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

