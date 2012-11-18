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

//#define TRACE_ISA
#ifdef TRACE_ISA
#       define TRACE(x) dprintf x
#else
#       define TRACE(x) ;
#endif


uint8
arch_isa_read_io_8(int mapped_io_addr)
{
	uint8 value = in8(mapped_io_addr);

	TRACE(("isa_read8(%x->%x)\n", mapped_io_addr, value));

	return value;
}


void
arch_isa_write_io_8(int mapped_io_addr, uint8 value)
{
	TRACE(("isa_write8(%x->%x)\n", value, mapped_io_addr));

	out8(value, mapped_io_addr);
}


uint16
arch_isa_read_io_16(int mapped_io_addr)
{
	return in16(mapped_io_addr);
}


void
arch_isa_write_io_16(int mapped_io_addr, uint16 value)
{
	out16(value, mapped_io_addr);
}


uint32
arch_isa_read_io_32(int mapped_io_addr)
{
	return in32(mapped_io_addr);
}


void
arch_isa_write_io_32(int mapped_io_addr, uint32 value)
{
	out32(value, mapped_io_addr);
}


void *
arch_isa_ram_address(const void *physical_address_in_system_memory)
{
	// this is what the BeOS kernel does
	return (void *)physical_address_in_system_memory;
}


status_t
arch_isa_init(void)
{
	return B_OK;
}
