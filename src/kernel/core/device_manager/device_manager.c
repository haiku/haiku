/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP Manager

	Main file.
*/

#include "device_manager_private.h"

#include <KernelExport.h>
//#include <driver_settings.h>

//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include <dirent.h>

//#define TRACE_DEVICE_MANAGER
#ifdef TRACE_DEVICE_MANAGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


benaphore node_lock;
pnp_node_info *node_list;

bool disable_useraddons;

//int pnp_load_wait_count;
//sem_id pnp_load_wait_sem;
int pnp_resource_wait_count;
sem_id pnp_resource_wait_sem;

io_resource_info *io_mem_list, *io_port_list, *isa_dma_list;

#if 0
void
check_settings()
{
	void *settings;

	settings = load_driver_settings( B_SAFEMODE_DRIVER_SETTINGS );
	disable_useraddons = get_driver_boolean_parameter( settings, "disableuseraddons", 0, 1 );
	unload_driver_settings(settings);
	
//	if( disable_useraddons ) 
//		SHOW_INFO0( 2, "User pnp drivers disabled" );
}
#endif	

static status_t
init()
{
	status_t status;

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

//	node_list = NULL;
/*	pnp_load_wait_count = 0;*/
	pnp_resource_wait_count = 0;
	io_mem_list = io_port_list = isa_dma_list = NULL;
	pnp_fs_emulation_nesting = 0;

	return B_OK;
/*
err3:
	delete_sem( pnp_load_wait_sem );
err2:
	DELETE_BEN( &node_lock );
err:
	DELETE_BEN( &generator_lock );
	return res;*/
}

#if 0
static status_t
uninit()
{
	SHOW_FLOW0( 0, "" );

	DELETE_BEN( &node_lock );
	DELETE_BEN( &generator_lock );
/*	delete_sem( pnp_load_wait_sem );*/
	delete_sem( pnp_resource_wait_sem );
	return B_OK;
}
#endif


static status_t
std_ops(int32 op, ...)
{
	switch( op ) {
	case B_MODULE_INIT:
		return init();
		
	case B_MODULE_UNINIT:
		return B_OK;
//		return uninit();
		
	default:
		return B_ERROR;
	}
}


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

