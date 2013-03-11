/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REQUESTINTERPRETER_H
#define REQUESTINTERPRETER_H


#include <SupportDefs.h>

#include "FileInfo.h"
#include "NFS4Defs.h"
#include "RPCCallbackRequest.h"


class RequestInterpreter {
public:
						RequestInterpreter(RPC::CallbackRequest* request);
						~RequestInterpreter();

	inline	uint32		OperationCount();
	inline	uint32		Operation();

			status_t	GetAttr(FileHandle* handle, int* mask);
			status_t	Recall(FileHandle* handle, bool& truncate,
							uint32* stateSeq, uint32* stateID);

private:
			uint32		fOperationCount;
			uint32		fLastOperation;

			RPC::CallbackRequest*	fRequest;
};


inline uint32
RequestInterpreter::OperationCount()
{
	return fOperationCount;
}


inline uint32
RequestInterpreter::Operation()
{
	fLastOperation = fRequest->Stream().GetUInt();
	return fLastOperation;
}


#endif	// REQUESTINTERPRETER_H

