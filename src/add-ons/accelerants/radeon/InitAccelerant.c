/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Main init/uninit functions
*/


#include "GlobalData.h"
#include "generic.h"

#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/ioctl.h>
#include <malloc.h>


// init data used by both primary and cloned accelerant
//	the_fd - file descriptor of kernel driver
//	accelerant_is_clone - if true, this is a cloned accelerant
static status_t init_common( int the_fd, bool accelerant_is_clone ) 
{
	status_t result;
	radeon_get_private_data gpd;
	
	SHOW_FLOW0( 3, "" );
	
	ai = malloc( sizeof( *ai ));
	if( ai == NULL )
		return B_NO_MEMORY;
		
	memset( ai, 0, sizeof( *ai ));
	
	ai->accelerant_is_clone = accelerant_is_clone;
	ai->fd = the_fd;
	
	// get basic info from driver
	gpd.magic = RADEON_PRIVATE_DATA_MAGIC;

	result = ioctl( ai->fd, RADEON_GET_PRIVATE_DATA, &gpd, sizeof(gpd) );
	if (result != B_OK) goto err;

	ai->virtual_card_area = clone_area( "Radeon virtual card", (void **)&ai->vc, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, gpd.virtual_card_area );
	if( ai->virtual_card_area < 0 ) {
		result = ai->virtual_card_area;
		goto err;
	}
	ai->shared_info_area = clone_area("Radeon shared info", (void **)&ai->si, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, gpd.shared_info_area);
	if( ai->shared_info_area < 0 ) {
		result = ai->shared_info_area;
		goto err2;
	}
	
	ai->regs_area = clone_area("Radeon regs area", (void **)&ai->regs, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, ai->si->regs_area);
	if( ai->regs_area < 0 ) {
		result = ai->regs_area;
		goto err3;
	}

	return B_OK;

err3:
	delete_area( ai->shared_info_area );
err2:
	delete_area( ai->virtual_card_area );
err:
	free( ai );
	return result;
}

// clean up data common to both primary and cloned accelerant
static void uninit_common( void )
{
	delete_area( ai->regs_area );
	delete_area( ai->shared_info_area );
	delete_area( ai->virtual_card_area );
	
	ai->regs_area = ai->shared_info_area = ai->virtual_card_area = 0;
	
	ai->regs = 0;
	ai->si = 0;
	ai->vc = 0;
	
	// close the file handle ONLY if we're the clone
	// (this is what Be tells us ;)
	if( ai->accelerant_is_clone ) 
		close( ai->fd );
	
	free( ai );
}

// public function: init primary accelerant
//	the_fd - file handle of kernel driver
status_t INIT_ACCELERANT( int the_fd ) 
{
	shared_info *si;
	virtual_card *vc;
	status_t result;
	
	SHOW_FLOW0( 3, "" );
	
	result = init_common( the_fd, 0 );
	if (result != B_OK) 
		goto err;
	
	si = ai->si;
	vc = ai->vc;

	// init Command Processor
	result = Radeon_InitCP( ai );
	if( result != B_OK )
		goto err2;
		
	// this isn't the best place, but has to be done sometime	
	Radeon_ReadSettings( vc );
	
	// read FP info via DDC
	// (ignore result - if it fails we fall back to BIOS detection)
	if( si->fp_port.disp_type == dt_dvi_1 )
		Radeon_ReadFPEDID( ai, si );

	// create list of supported modes
	result = Radeon_CreateModeList( si );
	if (result != B_OK) 
		goto err3;

	/* init the shared semaphore */
	INIT_BEN( "Radeon engine", si->engine.lock );

	// init engine sync token
	// (count of issued parameters or commands)
	si->engine.last_idle = si->engine.count = 0;
	// set last written count to be very old, so it must be written on first use
	// (see writeSyncToken)
	si->engine.written = -1;
	
	// init overlay
	si->overlay_mgr.token = 0;
	si->overlay_mgr.inuse = 0;
	
	// mark overlay as inactive
	si->active_overlay.port = -1;
	si->pending_overlay.port = -1;
	
	// reset list of allocated overlays
	vc->overlay_buffers = NULL;

	// everything else is initialized upon set_display_mode
	return B_OK;

err3:
err2:
	uninit_common();
err:
	return result;
}


// public function: return size of clone info
ssize_t ACCELERANT_CLONE_INFO_SIZE( void )
{
	// clone info is device name, so return its maximum size
	return MAX_RADEON_DEVICE_NAME_LENGTH;
}


// public function: return clone info
//	data - buffer to contain info (allocated by caller)
void GET_ACCELERANT_CLONE_INFO( void *data ) 
{
	radeon_device_name dn;
	status_t result;

	// clone info is device name - ask device driver
	dn.magic = RADEON_PRIVATE_DATA_MAGIC;
	dn.name = (char *)data;
	
	result = ioctl( ai->fd, RADEON_DEVICE_NAME, &dn, sizeof(dn) );
}

// public function: init cloned accelerant
//	data - clone info from get_accelerant_clone_info
status_t CLONE_ACCELERANT( void *data ) 
{
	status_t result;
	char path[MAXPATHLEN];
	int fd;

	// create full device name
	strcpy(path, "/dev/");//added trailing '/', this fixes cloning accelerant!
	strcat(path, (const char *)data);

	// open device; according to Be, permissions aren't important
	// this will probably change once access right are checked properly
	fd = open(path, B_READ_WRITE);
	if( fd < 0 )
		return fd;

	result = init_common( fd, 1 );
	if( result != B_OK )
		goto err1;

	// get (cloned) copy of supported display modes
	result = ai->mode_list_area = clone_area(
		"Radeon cloned display_modes", (void **)&ai->mode_list,
		B_ANY_ADDRESS, B_READ_AREA, ai->si->mode_list_area );
	if (result < B_OK) 
		goto err2;

	return B_OK;

err2:
	uninit_common();
err1:
	close( fd );
	return result;
}

// public function: uninit primary or cloned accelerant
void UNINIT_ACCELERANT( void )
{
	// TBD:
	// we should put accelerator into stable state first -
	// on my Laptop, you never can boot Windows/Linux after shutting
	// down BeOS; if both ports have been used, even the BIOS screen
	// is completely messed up
	
	// cloned accelerants have mode_list cloned, so deleting is OK
	// primary accelerant owns mode list, so deleting is OK as well
	delete_area( ai->mode_list_area );
	ai->mode_list = 0;

	uninit_common();
}

// public function: get some info about graphics card
status_t GET_ACCELERANT_DEVICE_INFO( accelerant_device_info *di )
{
	// is there anyone using it?
	
	// TBD: everything apart from memsize
	di->version = B_ACCELERANT_VERSION;
	strcpy( di->name, "Radeon" );
	strcpy( di->chipset, "Radeon" );
	strcpy( di->serial_no, "None" );
	
	di->memory = ai->si->local_mem_size;
	
	// TBD: is max PLL speed really equal to max DAC speed?
	di->dac_speed = ai->si->pll.max_pll_freq;
	
	return B_OK;
}
