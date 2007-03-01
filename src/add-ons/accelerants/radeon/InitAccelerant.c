/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Main init/uninit functions
*/


#include "GlobalData.h"
#include "generic.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include "CP.h"


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
	ai->shared_info_area = clone_area( "Radeon shared info", (void **)&ai->si, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, gpd.shared_info_area );
	if( ai->shared_info_area < 0 ) {
		result = ai->shared_info_area;
		goto err2;
	}
	
	ai->regs_area = clone_area( "Radeon regs area", (void **)&ai->regs, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, ai->si->regs_area );
	if( ai->regs_area < 0 ) {
		result = ai->regs_area;
		goto err3;
	}
	
//rud:
	if( ai->si->memory[mt_PCI].area > 0 ) {
		ai->mapped_memory[mt_PCI].area = clone_area( "Radeon PCI GART area", 
			(void **)&ai->mapped_memory[mt_PCI].data, B_ANY_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, ai->si->memory[mt_PCI].area );
		if( ai->mapped_memory[mt_PCI].area < 0 ) {
			result = ai->mapped_memory[mt_PCI].area;
			goto err4;
		}
	}
	
	if( ai->si->memory[mt_AGP].area > 0 ) {
		ai->mapped_memory[mt_AGP].area = clone_area( "Radeon AGP GART area", 
			(void **)&ai->mapped_memory[mt_AGP].data, B_ANY_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, ai->si->memory[mt_PCI].area );
		if( ai->mapped_memory[mt_AGP].area < 0 ) {
			result = ai->mapped_memory[mt_AGP].area;
			goto err5;
		}
	}

	ai->mapped_memory[mt_nonlocal] = ai->mapped_memory[ai->si->nonlocal_type];
	ai->mapped_memory[mt_local].data = ai->si->local_mem;
	
	return B_OK;

err5:
	if( ai->mapped_memory[mt_PCI].area > 0 )
		delete_area( ai->mapped_memory[mt_PCI].area );
err4:
	delete_area( ai->regs_area );
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
	if( !ai->accelerant_is_clone ) {
		if ( ai->si->acc_dma ) 
			Radeon_FreeVirtualCardStateBuffer( ai );

		// the last accelerant should must wait for the card to become quite,
		// else some nasty command could lunger in some FIFO
		Radeon_WaitForIdle( ai, false );
	}

	if( ai->mapped_memory[mt_AGP].area > 0 )
		delete_area( ai->mapped_memory[mt_AGP].area );
	if( ai->mapped_memory[mt_PCI].area > 0 )
		delete_area( ai->mapped_memory[mt_PCI].area );
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
	/*result = Radeon_InitCP( ai );
	if( result != B_OK )
		goto err2;*/
		
	// this isn't the best place, but has to be done sometime	
	Radeon_ReadSettings( vc );

	// establish connection to TV-Out unit
	Radeon_DetectTVOut( ai );

	// get all possible information about connected display devices	
	Radeon_DetectDisplays( ai );

	// create list of supported modes
	result = Radeon_CreateModeList( si );
	if (result != B_OK) 
		goto err3;

	/* init the shared semaphore */
	(void)INIT_BEN( si->engine.lock, "Radeon engine" );

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
	si->active_overlay.crtc_idx = -1;
	si->pending_overlay.crtc_idx = -1;
	
	// reset list of allocated overlays
	vc->overlay_buffers = NULL;
	
	// mark engine as having no state
	//si->cp.active_state_buffer = -1;
	
	if ( ai->si->acc_dma )
		Radeon_AllocateVirtualCardStateBuffer( ai );

	// everything else is initialized upon set_display_mode
	return B_OK;

err3:
//err2:
	uninit_common();
err:
	return result;
}


// public function: return size of clone info
ssize_t ACCELERANT_CLONE_INFO_SIZE( void )
{
	SHOW_FLOW0( 0, "" );

	// clone info is device name, so return its maximum size
	return MAX_RADEON_DEVICE_NAME_LENGTH;
}


// public function: return clone info
//	data - buffer to contain info (allocated by caller)
void GET_ACCELERANT_CLONE_INFO( void *data ) 
{
	radeon_device_name dn;
	status_t result;

	SHOW_FLOW0( 0, "" );

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
	
	SHOW_FLOW0( 0, "" );

	// create full device name
	strcpy(path, "/dev/");
	strcat(path, (const char *)data);

	// open device; according to Be, permissions aren't important
	// this will probably change once access right are checked properly
	fd = open(path, B_READ_WRITE);
	if( fd < 0 )
		return errno;

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
	
	SHOW_FLOW0( 0, "" );

	// cloned accelerants have mode_list cloned, so deleting is OK
	// primary accelerant owns mode list, so deleting is OK as well
	delete_area( ai->mode_list_area );
	ai->mode_list = 0;

	uninit_common();
}

// public function: get some info about graphics card
status_t GET_ACCELERANT_DEVICE_INFO( accelerant_device_info *di )
{
	SHOW_FLOW0( 0, "" );

	// is there anyone using it?
	
	// TBD: everything apart from memsize
	di->version = B_ACCELERANT_VERSION;
	strcpy( di->name, "Radeon" );
	strcpy( di->chipset, "Radeon" );
	strcpy( di->serial_no, "None" );
	
	di->memory = ai->si->memory[mt_local].size;
	
	// TBD: is max PLL speed really equal to max DAC speed?
	di->dac_speed = ai->si->pll.max_pll_freq;
	
	return B_OK;
}
