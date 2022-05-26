/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


extern "C" void
__riscv_flush_icache(void *start, void *end, unsigned long int flags)
{
	__asm__ volatile ("fence.i");
}
