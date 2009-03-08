/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <io_requests.h>

#include "IORequest.h"


bool
io_request_is_write(const io_request* request)
{
	return request->IsWrite();
}
