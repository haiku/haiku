/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include <ByteOrder.h>

#include "fssh_byte_order.h"


uint16_t
__fssh_swap_int16(uint16_t value)
{
	return __swap_int16(value);
}

uint32_t
__fssh_swap_int32(uint32_t value)
{
	return __swap_int32(value);
}

uint64_t
__fssh_swap_int64(uint64_t value)
{
	return __swap_int64(value);
}
