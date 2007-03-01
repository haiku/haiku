/*
	Copyright (c) 2003, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Kernel driver wrapper
*/

#include "radeon_accelerant.h"
#include <sys/ioctl.h>
#include "rbbm_regs.h"
#include "mmio.h"


status_t Radeon_WaitForIdle( accelerator_info *ai, bool keep_lock )
{
	radeon_wait_for_idle wfi;
	
	wfi.magic = RADEON_PRIVATE_DATA_MAGIC;
	wfi.keep_lock = keep_lock;
	
	return ioctl( ai->fd, RADEON_WAITFORIDLE, &wfi, sizeof( wfi ));
}

// wait until "entries" FIFO entries are empty
// lock must be hold
status_t Radeon_WaitForFifo( accelerator_info *ai, int entries )
{
	while( 1 ) {
		bigtime_t start_time = system_time();
	
		do {
			int slots = INREG( ai->regs, RADEON_RBBM_STATUS ) & RADEON_RBBM_FIFOCNT_MASK;
			
			if ( slots >= entries ) 
				return B_OK;

			snooze( 1 );
		} while( system_time() - start_time < 1000000 );
			
		Radeon_ResetEngine( ai );
	}
	
	return B_ERROR;
}

void Radeon_ResetEngine( accelerator_info *ai )
{
	radeon_no_arg na;
	
	na.magic = RADEON_PRIVATE_DATA_MAGIC;
	
	ioctl( ai->fd, RADEON_RESETENGINE, &na, sizeof( na ));
}


status_t Radeon_VIPRead( accelerator_info *ai, uint channel, uint address, uint32 *data )
{
	radeon_vip_read vr;
	status_t res;
	
	vr.magic = RADEON_PRIVATE_DATA_MAGIC;
	vr.channel = channel;
	vr.address = address;
	vr.lock = false;
	
	res = ioctl( ai->fd, RADEON_VIPREAD, &vr, sizeof( vr ));
	
	if( res == B_OK )
		*data = vr.data;
		
	return res;
}


status_t Radeon_VIPWrite( accelerator_info *ai, uint8 channel, uint address, uint32 data )
{
	radeon_vip_write vw;
	
	vw.magic = RADEON_PRIVATE_DATA_MAGIC;
	vw.channel = channel;
	vw.address = address;
	vw.data = data;
	vw.lock = false;
	
	return ioctl( ai->fd, RADEON_VIPWRITE, &vw, sizeof( vw ));
}

int Radeon_FindVIPDevice( accelerator_info *ai, uint32 device_id )
{
	radeon_find_vip_device fvd;
	status_t res;
	
	fvd.magic = RADEON_PRIVATE_DATA_MAGIC;
	fvd.device_id = device_id;
	
	res = ioctl( ai->fd, RADEON_FINDVIPDEVICE, &fvd, sizeof( fvd ));
	
	if( res == B_OK )
		return fvd.channel;
	else
		return -1;
}
