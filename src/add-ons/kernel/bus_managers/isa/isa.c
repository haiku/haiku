/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
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

#include "isa_arch.h"

//#define TRACE_ISA
#ifdef TRACE_ISA
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// ToDo: this is architecture dependent and should be made differently!
//	(for example, the Pegasos (PPC based) also has an ISA bus)


#define ISA_MODULE_NAME "bus_managers/isa/root/device_v1"

device_manager_info *pnp;


static long
make_isa_dma_table(const void *buffer, long buffer_size, ulong num_bits,
	isa_dma_entry *table, long num_entries)
{
	// ToDo: implement this?!
	return ENOSYS;
}


static long
start_scattered_isa_dma(long channel, const isa_dma_entry *table,
	uchar mode, uchar emode)
{
	// ToDo: implement this?!
	return ENOSYS;
}


static status_t
lock_isa_dma_channel(long channel)
{
	// ToDo: implement this?!
	return B_NOT_ALLOWED;
}


static status_t
unlock_isa_dma_channel(long channel)
{
	// ToDo: implement this?!
	return B_ERROR;
}


#if 0
static status_t
isa_init_driver(device_node_handle node, void *user_cookie, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
isa_uninit_driver(void *cookie)
{
	return B_OK;
}


static float
isa_supports_device(device_node_handle parent, bool *_noConnection)
{
	char *bus;

	// make sure parent is really pnp root
	if (pnp->get_attr_string(parent, B_DRIVER_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "root")) {
		free(bus);
		return 0.0;
	}

	free(bus);
	return 1.0;
}


static status_t
isa_register_device(device_node_handle parent)
{
	static const device_attr attrs[] = {
		// info about ourself
		{ B_DRIVER_MODULE, B_STRING_TYPE, { string: ISA_MODULE_NAME }},
		// unique connection
		{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "ISA" }},

		// mark as being a bus
		{ PNP_BUS_IS_BUS, B_UINT8_TYPE, { ui8: 1 }},

		// tell where to look for child devices
		{ B_DRIVER_BUS, B_STRING_TYPE, { string: "isa" }},
		{ B_DRIVER_FIND_DEVICES_ON_DEMAND, B_UINT8_TYPE, { ui8: 1 }},
		{ B_DRIVER_EXPLORE_LAST, B_UINT8_TYPE, { ui8: 1 }},
		{ NULL }
	};

	return pnp->register_device(parent, attrs, NULL, NULL);
}


static void
isa_get_paths(const char ***_bus, const char ***_device)
{
	static const char *kBus[] = {"root", NULL};

	*_bus = kBus;
	*_device = NULL;
}
#endif


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return arch_isa_init();
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
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
	&arch_isa_read_io_8,
	&arch_isa_write_io_8,
	&arch_isa_read_io_16,
	&arch_isa_write_io_16,
	&arch_isa_read_io_32,
	&arch_isa_write_io_32,
	&arch_isa_ram_address,
	&make_isa_dma_table,
	&arch_start_isa_dma,
	&start_scattered_isa_dma,
	&lock_isa_dma_channel,
	&unlock_isa_dma_channel
};

#if 0
static isa2_module_info isa2_module = {
	{
		{
			{
				ISA_MODULE_NAME,
				0,
				std_ops
			},

			isa_supports_device,
			isa_register_device,
			isa_init_driver,
			isa_uninit_driver,
			NULL,	// removed device
			NULL,	// cleanup device
			isa_get_paths,
		},

		// as ISA relies on device drivers to detect their devices themselves,
		// we don't have an universal rescan method
		NULL,	// register child devices
		NULL,	// rescan bus
	},

	arch_isa_read_io_8, arch_isa_write_io_8,
	arch_isa_read_io_16, arch_isa_write_io_16,
	arch_isa_read_io_32, arch_isa_write_io_32,

	arch_isa_ram_address,

	arch_start_isa_dma,
};
#endif

module_info *modules[] = {
	(module_info *)&isa_module,
//	(module_info *)&isa2_module,
	NULL
};
