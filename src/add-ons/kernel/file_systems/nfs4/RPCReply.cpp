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


Reply::Reply(void *buffer, int size)
	:
	fError(B_OK),
	fStream(buffer, size),
	fBuffer(buffer)
{
	fXID = fStream.GetUInt();
#if 0
	int32 type = fStream.GetInt();
	int32 state = fStream.GetInt();
	int32 auth = fStream.GetInt();
	fStream.GetOpaque(NULL);
#endif
}


Reply::~Reply()
{
	free(fBuffer);
}

