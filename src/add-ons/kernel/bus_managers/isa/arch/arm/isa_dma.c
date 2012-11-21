/*
 * Copyright 2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * arch-specific config manager
 *
 * Authors (in chronological order):
 *              François Revol (revol@free.fr)
 */

#include <KernelExport.h>
#include "ISA.h"
#include "arch_cpu.h"
#include "isa_arch.h"


status_t
arch_start_isa_dma(long channel, void *buf, long transfer_count,
	uchar mode, uchar e_mode)
{
	// ToDo: implement this?!
	return B_NOT_ALLOWED;
}


