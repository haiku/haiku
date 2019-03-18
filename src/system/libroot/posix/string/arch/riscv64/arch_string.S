/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <asm_defs.h>

#warning RISCV64: optimized assembly memcpy
// for now, a generic C one in:
// * src/system/kernel/lib/arch/riscv64/Jamfile
// * src/system/runtime_loader/arch/riscv64/Jamfile

#if 0

/* that should be enough for now */

.align 4
FUNCTION(memcpy):
/*	void *memcpy(void *dest, const void *src, size_t count) */
FUNCTION_END(memcpy)
#endif
