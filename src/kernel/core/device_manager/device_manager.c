/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
**
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP Manager

	Main file.
*/

#include "device_manager_private.h"
#include <kdevice_manager.h>

#include <KernelExport.h>
#include <string.h>


//#define TRACE_DEVICE_MANAGER
#ifdef TRACE_DEVICE_MANAGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


pnp_node_info *node_list;

bool disable_useraddons;

int pnp_resource_wait_count;
sem_id pnp_resource_wait_sem;

io_resource_info *io_mem_list, *io_port_list, *isa_dma_list;

#if 0
void
check_settings()
{
	void *settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);

	disable_useraddons = get_driver_boolean_parameter(settings, "disableuseraddons", 0, 1);

	unload_driver_settings(settings);
}
#endif	


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


// ToDo: the device manager exports these for now, so that Thomas's driver
//	can work on both R5 and OpenBeOS without too much hassle

device_manager_info gDeviceManagerModule = {
	{
		DEVICE_MANAGER_MODULE_NAME,
		0,
		std_ops
	},

	pnp_load_driver,
	pnp_unload_driver,

	pnp_rescan,

	pnp_register_device,
	pnp_unregister_device,

	pnp_acquire_io_resources,
	pnp_release_io_resources,

	pnp_find_device,

	pnp_create_id,
	pnp_free_id,

	pnp_get_parent,	
	pnp_get_attr_uint8,
	pnp_get_attr_uint16,
	pnp_get_attr_uint32,
	pnp_get_attr_uint64,
	pnp_get_attr_string,
	pnp_get_attr_raw,

	pnp_get_next_attr,
	pnp_release_attr,
	pnp_retrieve_attr,
	pnp_write_attr
};


//	#pragma mark -


status_t
device_manager_init(struct kernel_args *args)
{
	status_t status;
//	char name[B_FILE_NAME_LENGTH];
//	size_t size = sizeof(name);
//	int32 cookie;

	TRACE(("device manager init\n"));

	status = id_generator_init();
	if (status < B_OK) {
		panic("could not initialize ID generator\n");
		return status;
	}

	status = nodes_init();
	if (status < B_OK) {
		panic("could not initialize device nodes\n");
		return status;
	}

	pnp_resource_wait_sem = create_sem(0, "pnp_resource_wait_sem");
	if (pnp_resource_wait_sem < 0) {
		panic("could not create resource wait sem\n");
		return pnp_resource_wait_sem;
	}

	pnp_resource_wait_count = 0;
	io_mem_list = io_port_list = isa_dma_list = NULL;
	pnp_fs_emulation_nesting = 0;

	pnp_root_init_root();

	// build initial device tree; register all root bus_managers

#if 0
	cookie = 0;
	while (get_next_loaded_module_name(&cookie, name, &size) == B_OK) {
		if (strncmp(name, "bus_managers", 12))
			continue;

		dprintf(": %s\n", name);
		size = sizeof(name);
	}
#endif

	return B_OK;
}

