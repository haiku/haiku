/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_CALLBACK_H
#define IO_CALLBACK_H


#include "IORequest.h"


typedef status_t (*io_callback)(void* data, io_operation* operation);


class IOCallback {
public:
	virtual						~IOCallback();

	virtual	status_t			DoIO(IOOperation* operation) = 0;

	static	status_t			WrapperFunction(void* data,
									io_operation* operation);
};


#endif	// IO_CALLBACK_H
