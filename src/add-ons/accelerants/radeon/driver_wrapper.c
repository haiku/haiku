/*
	Copyright (c) 2003, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Kernel driver wrapper
*/

#include "radeon_accelerant.h"
#include <sys/ioctl.h>

status_t Radeon_WaitForIdle( accelerator_info *ai, bool keep_lock )
{
	radeon_wait_for_idle wfi;
	
	wfi.magic = RADEON_PRIVATE_DATA_MAGIC;
	wfi.keep_lock = keep_lock;
	
	return ioctl( ai->fd, RADEON_WAITFORIDLE, &wfi, sizeof( wfi ));
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
