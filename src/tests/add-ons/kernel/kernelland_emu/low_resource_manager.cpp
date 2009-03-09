/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <low_resource_manager.h>


extern "C" int32
low_resource_state(uint32 resources)
{
	return B_NO_LOW_RESOURCE;
}


extern "C" void
low_resource(uint32 resource, uint64 requirements, uint32 flags,
	uint32 timeout)
{
}


extern "C" status_t
register_low_resource_handler(low_resource_func function, void *data,
	uint32 resources, int32 priority)
{
	return B_OK;
}


extern "C" status_t
unregister_low_resource_handler(low_resource_func function, void *data)
{
	return B_OK;
}
