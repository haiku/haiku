/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCCallbackRequest.h"

#include <stdlib.h>

#include "NFS4Defs.h"
#include "RPCDefs.h"


using namespace RPC;


CallbackRequest::CallbackRequest(void *buffer, int size)
	:
	fError(B_BAD_VALUE),
	fRPCError(GARBAGE_ARGS),
	fStream(buffer, size),
	fBuffer(buffer)
{
	fXID = fStream.GetUInt();

	if (fStream.GetUInt() != CALL)
		return;

	if (fStream.GetUInt() != VERSION)
		return;

	if (fStream.GetUInt() != PROGRAM_NFS_CB) {
		fRPCError = PROG_UNAVAIL;
		return;
	}

	if (fStream.GetUInt() != NFS_CB_VERSION) {
		fRPCError = PROG_MISMATCH;
		return;
	}

	fProcedure = fStream.GetUInt();

	fStream.GetUInt();
	fStream.GetOpaque(NULL);

	fStream.GetUInt();
	fStream.GetOpaque(NULL);

	if (fProcedure == CallbackProcCompound) {
		fStream.GetOpaque(NULL); // TODO: tag may be important
		if (fStream.GetUInt() != 0)
			return;

		fID = fStream.GetUInt();

		fRPCError = SUCCESS;
		fError = B_OK;
	} else if (fProcedure == CallbackProcNull) {
		fRPCError = SUCCESS;
		fError = B_OK;
	} else
		fRPCError = PROC_UNAVAIL;
}


CallbackRequest::~CallbackRequest()
{
	free(fBuffer);
}


