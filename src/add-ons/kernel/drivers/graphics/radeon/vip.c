/*
	Copyright (c) 2002-04, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Access to VIP
	
	This code must be in kernel because we need for FIFO to become empty
	during VIP access (which in turn requires locking the card, and locking
	is a dangerous thing in user mode as the app can suddenly die, taking
	the lock with it)
*/

#include "radeon_driver.h"
#include "mmio.h"
#include "vip_regs.h"
#include "bios_regs.h"
#include "theatre_regs.h"


// moved to bottom to avoid inlining
static bool Radeon_VIPWaitForIdle( device_info *di );


// read data from VIP
// CP lock must be hold
static bool do_VIPRead( device_info *di, uint channel, uint address, uint32 *data )
{
	vuint8 *regs = di->regs;

	Radeon_WaitForFifo( di, 2 );
	// the 0x2000 is the nameless "register-read" flag
	OUTREG( regs, RADEON_VIPH_REG_ADDR, (channel << 14) | address | 0x2000 );
	
	if( !Radeon_VIPWaitForIdle( di ))
		return false;

	// enable VIP register cycle reads
	Radeon_WaitForFifo( di, 2 );
	OUTREGP( regs, RADEON_VIPH_TIMEOUT_STAT, 0, 
		~RADEON_VIPH_TIMEOUT_STAT_AK_MASK & ~RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS );
	//Radeon_WaitForIdle( di, false, false );
	
	// this read starts a register cycle; the returned value has no meaning
	INREG( regs, RADEON_VIPH_REG_DATA );
	
	if( !Radeon_VIPWaitForIdle( di ))
		return false;
		
	//Radeon_WaitForIdle( di, false, false );
	
	// register cycle is done, so disable any further cycle
	Radeon_WaitForFifo( di, 2 );
	OUTREGP( regs, RADEON_VIPH_TIMEOUT_STAT, RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS, 
		~RADEON_VIPH_TIMEOUT_STAT_AK_MASK & ~RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS );
	//Radeon_WaitForIdle( di, false, false );
	
	// get the data
	*data = INREG( regs, RADEON_VIPH_REG_DATA );
	
	//SHOW_FLOW( 4, "channel=%d, address=%x, data=%lx", channel, address, *data );

	if( !Radeon_VIPWaitForIdle( di ))
		return false;

	// disable register cycle again (according to sample code)
	// IMHO, this is not necessary as it has been done before
	Radeon_WaitForFifo( di, 2 );
	OUTREGP( regs, RADEON_VIPH_TIMEOUT_STAT, RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS, 
		~RADEON_VIPH_TIMEOUT_STAT_AK_MASK & ~RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS );

	return true;
}

// public function: read data from VIP
bool Radeon_VIPRead( device_info *di, uint channel, uint address, uint32 *data )
{
	bool res;
	
	ACQUIRE_BEN( di->si->cp.lock );
	
	res = do_VIPRead( di, channel, address, data );
	
	RELEASE_BEN( di->si->cp.lock );
	return res;
}

// write data to VIP
// not must be hold
static bool do_VIPWrite( device_info *di, uint8 channel, uint address, uint32 data )
{
	vuint8 *regs = di->regs;
	bool res;

	Radeon_WaitForFifo( di, 2 );
	OUTREG( regs, RADEON_VIPH_REG_ADDR, (channel << 14) | (address & ~0x2000) );

	if( !Radeon_VIPWaitForIdle( di ))
		return false;
		
	//SHOW_FLOW( 4, "channel=%d, address=%x, data=%lx", channel, address, data );
		
	Radeon_WaitForFifo( di, 2 );
	OUTREG( regs, RADEON_VIPH_REG_DATA, data );
	
	res = Radeon_VIPWaitForIdle( di );
		
	return res;
}

// public function: write data to VIP
bool Radeon_VIPWrite( device_info *di, uint8 channel, uint address, uint32 data )
{
	bool res;
	
	ACQUIRE_BEN( di->si->cp.lock );
	
	res = do_VIPWrite( di, channel, address, data );
	
	RELEASE_BEN( di->si->cp.lock );
	return res;
}


// reset VIP
static void VIPReset( device_info *di )
{
	vuint8 *regs = di->regs;

	ACQUIRE_BEN( di->si->cp.lock );

	Radeon_WaitForFifo( di, 5 );
	OUTREG( regs, RADEON_VIPH_CONTROL, 
		4 | 
		(15 << RADEON_VIPH_CONTROL_VIPH_MAX_WAIT_SHIFT) |
		RADEON_VIPH_CONTROL_VIPH_DMA_MODE |
		RADEON_VIPH_CONTROL_VIPH_EN );
	OUTREGP( regs, RADEON_VIPH_TIMEOUT_STAT, RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS, 
		~RADEON_VIPH_TIMEOUT_STAT_AK_MASK & ~RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS);
	OUTREG( regs, RADEON_VIPH_DV_LAT, 
		0xff |
		(4 << RADEON_VIPH_DV_LAT_VIPH_DV0_LAT_SHIFT) |
		(4 << RADEON_VIPH_DV_LAT_VIPH_DV1_LAT_SHIFT) |
		(4 << RADEON_VIPH_DV_LAT_VIPH_DV2_LAT_SHIFT) |
		(4 << RADEON_VIPH_DV_LAT_VIPH_DV3_LAT_SHIFT));
	OUTREG( regs, RADEON_VIPH_DMA_CHUNK,
		1 |
		(1 << RADEON_VIPH_DMA_CHUNK_VIPH_CH1_CHUNK_SHIFT) |
		(1 << RADEON_VIPH_DMA_CHUNK_VIPH_CH2_CHUNK_SHIFT) |
		(1 << RADEON_VIPH_DMA_CHUNK_VIPH_CH3_CHUNK_SHIFT));
	OUTREGP( regs, RADEON_TEST_DEBUG_CNTL, 0, ~RADEON_TEST_DEBUG_CNTL_OUT_EN );

	RELEASE_BEN( di->si->cp.lock );
}


// find VIP channel of a device
// return:	>= 0 channel of device
//			< 0 no device found
int Radeon_FindVIPDevice( device_info *di, uint32 device_id )
{
	uint channel;
	uint32 cur_device_id;
	
	// if card has no VIP port, let hardware detection fail;
	// in this case, noone will bother us again
	if( !di->has_vip )
		return -1;
	
	VIPReset( di );

	// there are up to 4 devices, connected to one of 4 channels	
	for( channel = 0; channel < 4; ++channel ) {
		// read device id
		if( !Radeon_VIPRead( di, channel, RADEON_VIP_VENDOR_DEVICE_ID, &cur_device_id ))
			continue;
		
		// compare device id directly
		if( cur_device_id == device_id )
			return channel;
	}
	
	// couldn't find device
	return -1;
}


// check whether VIP host is idle
// lock must be hold
static status_t Radeon_VIPIdle( device_info *di )
{
	vuint8 *regs = di->regs;
	uint32 timeout;
	
	//Radeon_WaitForIdle( di, false, false );
	
	// if there is a stuck transaction, acknowledge that
	timeout = INREG( regs, RADEON_VIPH_TIMEOUT_STAT );
	if( (timeout & RADEON_VIPH_TIMEOUT_STAT_VIPH_REG_STAT) != 0 ) {
		OUTREGP( regs, RADEON_VIPH_TIMEOUT_STAT, 
			RADEON_VIPH_TIMEOUT_STAT_VIPH_REG_AK, 
			~RADEON_VIPH_TIMEOUT_STAT_VIPH_REG_AK & ~RADEON_VIPH_TIMEOUT_STAT_AK_MASK );
		if( (INREG( regs, RADEON_VIPH_CONTROL) & RADEON_VIPH_CONTROL_VIPH_REG_RDY) != 0 )
			return B_BUSY;
		else
			return B_ERROR;
	}
	
	//Radeon_WaitForIdle( di, false, false );
	
	if( (INREG( regs, RADEON_VIPH_CONTROL) & RADEON_VIPH_CONTROL_VIPH_REG_RDY) != 0 )
		return B_BUSY;
	else
		return B_OK;
}


// wait until VIP host is idle
// lock must be hold
static bool Radeon_VIPWaitForIdle( device_info *di )
{
	int i;
	
	// wait 100x 1ms before giving up
	for( i = 0; i < 100; ++i ) {
		status_t res;
		
		res = Radeon_VIPIdle( di );
		if( res != B_BUSY ) {
			if( res == B_OK )
				return true;
			else
				return false;
		}
			
		snooze( 1000 );
	}
	
	return false;
}
