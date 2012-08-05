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


using namespace RPC;

enum {
	CALL		= 0
};

#define	VERSION		2

enum {
	PROGRAM_NFS_CB	= 0x40000000
};

#define NFS_CB_VERSION	1


CallbackRequest::CallbackRequest(void *buffer, int size)
	:
	fError(B_BAD_VALUE),
	fStream(buffer, size),
	fBuffer(buffer)
{
	fXID = fStream.GetUInt();

	if (fStream.GetUInt() != CALL)
		return;

	if (fStream.GetUInt() != VERSION)
		return;

	if (fStream.GetUInt() != PROGRAM_NFS_CB)
		return;

	if (fStream.GetUInt() != NFS_CB_VERSION)
		return;

	fProcedure = fStream.GetUInt();

	fStream.GetOpaque(NULL);
	fStream.GetOpaque(NULL);

	if (fProcedure == CallbackProcCompound) {
		fStream.GetOpaque(NULL); // TODO: tag may be important
		if (fStream.GetUInt() != 0)
			return;

		fID = fStream.GetUInt();
	}

	fError = B_OK;
}


CallbackRequest::~CallbackRequest()
{
	free(fBuffer);
}


