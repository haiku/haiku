/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon kernel driver
		
	DevFS interface
*/

#include "radeon_driver.h"
#ifdef ENABLE_LOGGING
#include "log_coll.h"
#endif

#include <OS.h>
#include <malloc.h>
#include <graphic_driver.h>
#include <stdio.h>
#include <string.h>
#include <mmio.h>
#include <version.h>

// tell the kernel what revision of the driver API we support
//int32	api_version = 2;
int32	api_version = B_CUR_DRIVER_API_VERSION; // apsed, was 2, is 2 in R5


static status_t open_hook( const char *name, uint32 flags, void **cookie );
static status_t close_hook( void *dev );
static status_t free_hook( void *dev );
static status_t read_hook( void *dev, off_t pos, void *buf, size_t *len );
static status_t write_hook( void *dev, off_t pos, const void *buf, size_t *len );
static status_t control_hook( void *dev, uint32 msg, void *buf, size_t len );


static device_hooks graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};


// public function: check whether there is *any* supported hardware
status_t init_hardware( void )
{
	SHOW_INFO0( 0, RADEON_DRIVER_VERSION );
	if( Radeon_CardDetect() == B_OK )
		return B_OK;
	else
		return B_ERROR;
}


// public function: init driver
status_t init_driver( void )
{
	if( get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK )
		return B_ERROR;

	/* driver private data */
	devices = (radeon_devices *)calloc( 1, sizeof( radeon_devices ));
	if( devices == NULL ) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	
	INIT_BEN( "Radeon Kernel", devices->kernel);
	
	Radeon_ProbeDevices();
	return B_OK;
}


// public function: uninit driver
void uninit_driver( void )
{
	DELETE_BEN( devices->kernel );
	
	free( devices );
	devices = NULL;

	put_module( B_PCI_MODULE_NAME );
}


// public function: return list of device names
const char **publish_devices( void ) 
{
	return (const char **)devices->device_names;
}


// public function: find hooks for specific device given its name
device_hooks *find_device( const char *name )
{
	uint32 index;
	
	// probably, we could always return standard hooks 
	for( index = 0; devices->device_names[index]; ++index ) {
		if( strcmp( name, devices->device_names[index] ) == 0 )
			return &graphics_device_hooks;
	}
	
	return NULL;
}


// public function: open device
static status_t open_hook( const char *name, uint32 flags, void **cookie ) 
{
	int32 index = 0;
	device_info *di;
	status_t	result = B_OK;

	SHOW_FLOW( 3, "name=%s, flags=%ld, cookie=0x%08lx", name, flags, (uint32)cookie );

	// find device info
	while( devices->device_names[index] && 
		strcmp(name, devices->device_names[index] ) != 0 ) 
		index++;

	di = &(devices->di[index]);

	ACQUIRE_BEN( devices->kernel );

	if( !di->is_open )
		result = Radeon_FirstOpen( di );

	if( result == B_OK ) {
		di->is_open++;
		*cookie = di;
	}
		
	RELEASE_BEN( devices->kernel );

	SHOW_FLOW( 3, "returning 0x%08lx", result );
	return result;
}


// public function: read from device (denied)
static status_t read_hook( void *dev, off_t pos, void *buf, size_t *len )
{
	*len = 0;
	return B_NOT_ALLOWED;
}


// public function: write to device (denied)
static status_t write_hook( void *dev, off_t pos, const void *buf, size_t *len )
{
	*len = 0;
	return B_NOT_ALLOWED;
}


// public function: close device (ignored, wait for free_hook instead)
static status_t close_hook( void *dev )
{
	return B_NO_ERROR;
}


// public function: free device
static status_t free_hook( void *dev )
{
	device_info *di = (device_info *)dev;

	SHOW_FLOW0( 3, "" );

	ACQUIRE_BEN( devices->kernel );

	mem_freetag( di->local_memmgr, dev );
	
	if( di->is_open == 1 )
		Radeon_LastClose( di );

	di->is_open--;
	RELEASE_BEN( devices->kernel );

	return B_OK;
}


// public function: ioctl
static status_t control_hook( void *dev, uint32 msg, void *buf, size_t len )
{
	device_info *di = (device_info *)dev;
	status_t result = B_DEV_INVALID_IOCTL;

	switch (msg) {
		// needed by app_server to load accelerant
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			//strcpy(sig, "radeon2.accelerant");
			strcpy(sig, "radeon.accelerant");
			result = B_OK;
		} break;

		// needed to share data between kernel and accelerant		
		case RADEON_GET_PRIVATE_DATA: {
			radeon_get_private_data *gpd = (radeon_get_private_data *)buf;
			
			if (gpd->magic == RADEON_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				gpd->virtual_card_area = di->virtual_card_area;
				result = B_OK;
			}
		} break;

		// needed for cloning
		case RADEON_DEVICE_NAME: {
			radeon_device_name *dn = (radeon_device_name *)buf;
			
			if( dn->magic == RADEON_PRIVATE_DATA_MAGIC ) {
				strncpy( dn->name, di->name, MAX_RADEON_DEVICE_NAME_LENGTH );
				result = B_OK;
			}
		} break;
		
		// graphics mem manager
		case RADEON_ALLOC_LOCAL_MEM: {
			radeon_alloc_local_mem *am = (radeon_alloc_local_mem *)buf;
			
			if( am->magic == RADEON_PRIVATE_DATA_MAGIC )
				result = mem_alloc( di->local_memmgr, am->size, dev, &am->handle, &am->fb_offset );
		} break;
		
		case RADEON_FREE_LOCAL_MEM: {
			radeon_free_local_mem *fm = (radeon_free_local_mem *)buf;
			
			if( fm->magic == RADEON_PRIVATE_DATA_MAGIC )
				result = mem_free( di->local_memmgr, fm->handle, dev );
		} break;
			
		// interface to log data
#ifdef ENABLE_LOGGING
		case RADEON_GET_LOG_SIZE:
			*(uint32 *)buf = log_getsize( di->si->log );
			result = B_OK;
			break;
			
		case RADEON_GET_LOG_DATA:
			log_getcopy( di->si->log, buf, ((uint32 *)buf)[0] );
			result = B_OK;
			break;	
#endif
			
		// interface to i2c-bus
		case RADEON_SET_I2C_SIGNALS: {
			radeon_getset_i2c *data = (radeon_getset_i2c *)buf;
			
			if( data->magic == RADEON_PRIVATE_DATA_MAGIC ) {
				result = B_OK;
				OUTREG( di->regs, data->port, data->value );
			}
			break; }	
		case RADEON_GET_I2C_SIGNALS: {
			radeon_getset_i2c *data = (radeon_getset_i2c *)buf;
			
			if( data->magic == RADEON_PRIVATE_DATA_MAGIC ) {
				result = B_OK;
				data->value = INREG( di->regs, data->port );
			}
			break; }	
	}
	return result;
}
