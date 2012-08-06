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
RequestInterpreter::GetAttr(FileHandle* handle, int* _mask)
{
	if (fLastOperation != OpCallbackGetAttr)
		return B_BAD_VALUE;

	uint32 size;
	const void* ptr = fRequest->Stream().GetOpaque(&size);
	handle->fSize = size;
	memcpy(handle->fData, ptr, size);

	uint32 count = fRequest->Stream().GetUInt();
	if (count < 1) {
		*_mask = 0;
		return fRequest->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
	}

	uint32 bitmap = fRequest->Stream().GetUInt();
	uint32 mask = 0;

	if ((bitmap & (1 << FATTR4_CHANGE)) != 0)
		mask |= CallbackAttrChange;
	if ((bitmap & (1 << FATTR4_SIZE)) != 0)
		mask |= CallbackAttrSize;

	*_mask = mask;

	for (uint32 i = 1; i < count; i++)
		fRequest->Stream().GetUInt();

	return fRequest->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
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

