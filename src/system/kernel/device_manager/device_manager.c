/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	Main file.
*/

#include "device_manager_private.h"
#include <kdevice_manager.h>

#include <generic_syscall.h>
#include <KernelExport.h>
#include <string.h>


//#define TRACE_DEVICE_MANAGER
#ifdef TRACE_DEVICE_MANAGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


device_node_info *gRootNode;

bool disable_useraddons;

int pnp_resource_wait_count;
sem_id pnp_resource_wait_sem;

io_resource_info *io_mem_list, *io_port_list, *isa_dma_list;


static int
dump_device_nodes(int argc, char **argv)
{
	dm_dump_node(gRootNode, 0);
	return 0;
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


// ToDo: the device manager exports these for now, so that Thomas's driver
//	can work on both R5 and Haiku without too much hassle.
//	We might also want to keep it in order to have the possibility to make
//	substantial changes without breaking compatibility, and eventually
//	separate it from the kernel for whatever reason.

device_manager_info gDeviceManagerModule = {
	{
		B_DEVICE_MANAGER_MODULE_NAME,
		0,
		std_ops
	},

	pnp_load_driver,
	pnp_unload_driver,

	dm_rescan,

	dm_register_node,
	dm_unregister_node,

	dm_get_root,
	dm_get_next_child_node,
	dm_get_parent,
	dm_put_node,

	dm_acquire_io_resources,
	dm_release_io_resources,

	dm_create_id,
	dm_free_id,

	pnp_get_attr_uint8,
	pnp_get_attr_uint16,
	pnp_get_attr_uint32,
	pnp_get_attr_uint64,
	pnp_get_attr_string,
	pnp_get_attr_raw,

	pnp_get_next_attr,
	pnp_release_attr,
	pnp_retrieve_attr,
	pnp_write_attr,
	NULL	// remove_attr
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

	status = dm_init_id_generator();
	if (status < B_OK) {
		panic("could not initialize ID generator\n");
		return status;
	}

	status = dm_init_nodes();
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

	dm_init_root_node();

#ifdef TRACE_DEVICE_MANAGER
	{
		// dump root node
		device_node_info *node = gRootNode;
		while (node && node->parent != NULL)
			node = node->parent;
		dm_dump_node(node, 0);
	}
#endif

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

	add_debugger_command("dm_tree", &dump_device_nodes, "dump device node tree");
	register_generic_syscall(DEVICE_MANAGER_SYSCALLS, device_manager_control, 1, 0);
	return B_OK;
}

