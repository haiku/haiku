/*
 * Copyright 2006-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/generic/debug_uart.h>


void
DebugUART::Out8(int reg, uint8 value)
{
#ifdef __ARM__
	// 32-bit aligned
	*((uint8 *)Base() + reg * sizeof(uint32)) = value;
#else
	*((uint8 *)Base() + reg) = value;
#endif
}


uint8
DebugUART::In8(int reg)
{
#ifdef __ARM__
	// 32-bit aligned
	return *((uint8 *)Base() + reg * sizeof(uint32));
#else
	return *((uint8 *)Base() + reg);
#endif
}


void
DebugUART::Barrier()
{
	// Simple memory barriers
#if defined(__POWERPC__)
	asm volatile("eieio; sync");
#elif defined(__ARM__)
	asm volatile ("" : : : "memory");
#endif
}
