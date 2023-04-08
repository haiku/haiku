/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include <image.h>


extern "C" void
__riscv_flush_icache(void *start, void *end, unsigned long int flags)
{
	clear_caches(start, (uint8*)end - (uint8*)start, B_INVALIDATE_ICACHE);
}
