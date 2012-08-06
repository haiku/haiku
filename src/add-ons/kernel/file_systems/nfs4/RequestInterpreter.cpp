/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RequestInterpreter.h"

#include <string.h>

#include <util/kernel_cpp.h>


RequestInterpreter::RequestInterpreter(RPC::CallbackRequest* request)
	:
	fRequest(request)
{
	fOperationCount = fRequest->Stream().GetUInt();
}


RequestInterpreter::~RequestInterpreter()
{
	delete fRequest;
}


status_t
RequestInterpreter::Recall(FileHandle* handle, bool& truncate, uint32* stateSeq,
	uint32* stateID)
{
	if (fLastOperation != OpCallbackRecall)
		return B_BAD_VALUE;

	*stateSeq = fRequest->Stream().GetUInt();
	stateID[0] = fRequest->Stream().GetUInt();
	stateID[1] = fRequest->Stream().GetUInt();
	stateID[2] = fRequest->Stream().GetUInt();

	truncate = fRequest->Stream().GetBoolean();

	uint32 size;
	const void* ptr = fRequest->Stream().GetOpaque(&size);
	handle->fSize = size;
	memcpy(handle->fData, ptr, size);

	return fRequest->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}

