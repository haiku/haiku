/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	ISA bus manager

	Implementation.
*/


#include <ISA.h>
#include <bus/ISA.h>
#include <KernelExport.h>
#include <device_manager.h>
#include <arch/cpu.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_ISA
#ifdef TRACE_ISA
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// ToDo: this is architecture dependent and should be made differently!
//	(for example, the Pegasos (PPC based) also has an ISA bus)


#define ISA_MODULE_NAME "bus_managers/isa/root"

device_manager_info *pnp;


static uint8
isa_read_io_8(int mapped_io_addr)
{
	uint8 value = in8(mapped_io_addr);

	TRACE(("isa_read8(%x->%x)\n", mapped_io_addr, value));

	return value;
}


static void
isa_write_io_8(int mapped_io_addr, uint8 value)
{
	TRACE(("isa_write8(%x->%x)\n", value, mapped_io_addr));

	out8(value, mapped_io_addr);
}


static uint16
isa_read_io_16(int mapped_io_addr)
{
	return in16(mapped_io_addr);
}


static void
isa_write_io_16(int mapped_io_addr, uint16 value)
{
	out16(value, mapped_io_addr);
}


static uint32
isa_read_io_32(int mapped_io_addr)
{
	return in32(mapped_io_addr);
}


static void
isa_write_io_32(int mapped_io_addr, uint32 value)
{
	out32(value, mapped_io_addr);
}


static void *
ram_address(const void *physical_address_in_system_memory)
{
	// this is what the BeOS kernel does
	return (void *)physical_address_in_system_memory;
}


static long
make_isa_dma_table(const void *buffer, long buffer_size, ulong num_bits,
	isa_dma_entry *table, long num_entries)
{
	return ENOSYS;
}


static status_t
start_isa_dma(long channel, void *buf, long transfer_count,
	uchar mode, uchar e_mode)
{
	// TBD
	// ToDo
	return B_NOT_ALLOWED;
}


static long
start_scattered_isa_dma(long channel, const isa_dma_entry *table,
	uchar mode, uchar emode)
{
	return ENOSYS;
}


static status_t
lock_isa_dma_channel(long channel)
{
	// TBD
	// ToDo
	return B_NOT_ALLOWED;
}


static status_t
unlock_isa_dma_channel(long channel)
{
	// TBD
	// ToDo
	return B_ERROR;
}


static status_t
isa_init_device(pnp_node_handle node, void *user_cookie, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
isa_uninit_device(void *cookie)
{
	return B_OK;
}


static status_t
isa_device_added(pnp_node_handle parent)
{
	static const pnp_node_attr attrs[] = {
		// info about ourself
		{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: ISA_MODULE_NAME }},
		{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: ISA_DEVICE_TYPE_NAME }},
		// unique connection
		{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "ISA" }},
		
		// mark as being a bus
		{ PNP_BUS_IS_BUS, B_UINT8_TYPE, { ui8: 1 }},

		// tell where to look for consumers
		// ToDo: temporary hack to get things started!
		{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: "busses/ide/ide_isa/isa/device/v1" }},
		{ PNP_DRIVER_DYNAMIC_CONSUMER, B_STRING_TYPE, { string: ISA_DRIVERS_DIR "/" }},
		{ NULL }
	};
	
	pnp_node_handle node;
	char *parent_type;
	status_t res;
	
	// make sure parent is really pnp root
	if (pnp->get_attr_string( parent, PNP_DRIVER_TYPE, &parent_type, false))
		return B_ERROR;

	if (strcmp(parent_type, "pnp/root")) {
		free(parent_type);
		return B_ERROR;
	}
	
	free(parent_type);

	res = pnp->register_device(parent, attrs, NULL, &node);
	if (res != B_OK)
		return res;

	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

static isa_module_info isa_module = {
	{
		{
			B_ISA_MODULE_NAME,
			B_KEEP_LOADED,
			std_ops
		},
		NULL	// rescan
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

static isa2_module_info isa2_module = {
	{
		{
			{
				ISA_MODULE_NAME,
				0,
				std_ops
			},

			isa_init_device,
			isa_uninit_device,
			isa_device_added,
			NULL
		},

		// this rescan(); as ISA relies on device drivers to detect their
		// devices themselves, we don't have an universal rescan method
		NULL
	},

	isa_read_io_8, isa_write_io_8,
	isa_read_io_16, isa_write_io_16,
	isa_read_io_32, isa_write_io_32,

	ram_address,

	start_isa_dma,
};


#if !_BUILDING_kernel && !BOOT
_EXPORT 
module_info *modules[] = {
	(module_info *)&isa_module,
	(module_info *)&isa2_module,
	NULL
};
#endif
