/* 
** Copyright 2002, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <module.h>
#include <Errors.h>
#include <ISA.h>
#include <arch/cpu.h>
#include <debug.h>
#include <KernelExport.h>

static status_t isa_rescan(void)
{
	dprintf("isa_rescan()\n");
	return 0;
}

static uint8 isa_read_io_8(int mapped_io_addr)
{
	return in8(mapped_io_addr);
}

static void isa_write_io_8(int mapped_io_addr, uint8 value)
{
	out8(value, mapped_io_addr);
}

static uint16 isa_read_io_16( int mapped_io_addr )
{
	return in16(mapped_io_addr);
}

static void isa_write_io_16( int mapped_io_addr, uint16 value )
{
	out16(value, mapped_io_addr);
}

static uint32 isa_read_io_32( int mapped_io_addr )
{
	return in32(mapped_io_addr);
}

static void isa_write_io_32( int mapped_io_addr, uint32 value )
{
	out32(value, mapped_io_addr);
}

static void *ram_address(const void *physical_address_in_system_memory)
{
	dprintf("isa: ram_address\n");
	return NULL;
}

static long make_isa_dma_table(const void *buffer, long buffer_size,
                               ulong num_bits, isa_dma_entry *table,
                               long	num_entries)
{
	return ENOSYS;
}

static long start_isa_dma(long	channel, void *buf, long transfer_count, 
                          uchar mode, uchar e_mode)
{
	return ENOSYS;
}

static long start_scattered_isa_dma(long channel, const isa_dma_entry *table,
                                    uchar mode, uchar emode)
{
	return ENOSYS;
}

static long lock_isa_dma_channel(long channel)
{
	return ENOSYS;
}

static long unlock_isa_dma_channel(long channel)
{
	return ENOSYS;
}

static int32 std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "ISA: init\n" );
			break;
		case B_MODULE_UNINIT:
			dprintf( "ISA: uninit\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}

static isa_module_info isa_module = {
	{
		{
			B_ISA_MODULE_NAME,
			B_KEEP_LOADED,
			std_ops
		},
		&isa_rescan
	},
	&isa_read_io_8, 
	&isa_write_io_8,
	&isa_read_io_16, 
	&isa_write_io_16,
	&isa_read_io_32, 
	&isa_write_io_32,
	&ram_address,
	&make_isa_dma_table,
	&start_isa_dma,
	&start_scattered_isa_dma,
	&lock_isa_dma_channel,
	&unlock_isa_dma_channel
};
	
module_info *modules[] = {
	(module_info*) &isa_module,
	NULL
};
