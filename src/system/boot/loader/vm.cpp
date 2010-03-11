/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <string.h>


status_t
user_memcpy(void* to, const void* from, size_t size)
{
	memcpy(to, from, size);
	return B_OK;
}
