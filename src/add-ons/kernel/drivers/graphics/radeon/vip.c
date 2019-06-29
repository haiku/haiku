/*
	Copyright (c) 2002-05, Thomas Kurschel
	

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
static status_t RADEON_VIPFifoIdle(device_info *di, uint8 channel);


// read data from VIP
// CP lock must be hold
static bool do_VIPRead( 
	device_info *di, uint channel, uint address, uint32 *data )
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
bool Radeon_VIPRead( 
	device_info *di, uint channel, uint address, uint32 *data, bool lock )
{
	bool res;

	if( lock )
		ACQUIRE_BEN( di->si->cp.lock );
	
	res = do_VIPRead( di, channel, address, data );

	if( lock )
		RELEASE_BEN( di->si->cp.lock );
		
//	SHOW_FLOW( 2, "address=%x, data=%lx, lock=%d", address, *data, lock );

	return res;
}

static bool do_VIPFifoRead(device_info *di, uint8 channel, uint32 address, uint32 count, uint8 *buffer)
{
   	vuint8 *regs = di->regs;
	uint32 status, tmp;

	if(count!=1)
	{
		SHOW_FLOW0( 2, "Attempt to access VIP bus with non-stadard transaction length\n");
		return false;
	}
		
	SHOW_FLOW( 2, "address=%" B_PRIx32 ", count=%" B_PRIu32 " ",
		address, count );
	
	Radeon_WaitForFifo( di, 2);
	SHOW_FLOW0( 2, "1");
	OUTREG( regs, RADEON_VIPH_REG_ADDR,  (channel << 14) | address | 0x3000);
	SHOW_FLOW0( 2, "3");
	while(B_BUSY == (status = RADEON_VIPFifoIdle( di , 0xff)));
	if(B_OK != status) return false;
	
	//	disable VIPH_REGR_DIS to enable VIP cycle.
	//	The LSB of VIPH_TIMEOUT_STAT are set to 0
	//	because 1 would have acknowledged various VIP
	//	interrupts unexpectedly       
	
	SHOW_FLOW0( 2, "4");
	Radeon_WaitForFifo( di, 2 ); // Radeon_WaitForIdle( di, false, false );
	SHOW_FLOW0( 2, "5");
	OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, 
		INREG( regs, RADEON_VIPH_TIMEOUT_STAT) & 
			(0xffffff00 & ~RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS) );
	
	//	the value returned here is garbage.  The read merely initiates
	//	a register cycle
	SHOW_FLOW0( 2, "6");
	Radeon_WaitForFifo( di, 2 ); // Radeon_WaitForIdle( di, false, false );
	INREG( regs, RADEON_VIPH_REG_DATA);
	SHOW_FLOW0( 2, "7");
	while(B_BUSY == (status = RADEON_VIPFifoIdle( di , 0xff)));
	if(B_OK != status)  return false;
	
	//	set VIPH_REGR_DIS so that the read won't take too long.
	SHOW_FLOW0( 2, "8");
	Radeon_WaitForFifo( di, 2 ); // Radeon_WaitForIdle( di, false, false );
	SHOW_FLOW0( 2, "9");
	tmp = INREG( regs, RADEON_VIPH_TIMEOUT_STAT);
	OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, (tmp & 0xffffff00) | RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS);
	
	SHOW_FLOW0( 2, "10");
	Radeon_WaitForFifo( di, 2 ); // Radeon_WaitForIdle( di, false, false );
	switch(count){
	   case 1:
	        *buffer=(uint8)(INREG( regs, RADEON_VIPH_REG_DATA) & 0xff);
	        break;
	   case 2:
	        *(uint16 *)buffer=(uint16) (INREG( regs, RADEON_VIPH_REG_DATA) & 0xffff);
	        break;
	   case 4:
	        *(uint32 *)buffer=(uint32) ( INREG( regs, RADEON_VIPH_REG_DATA) & 0xffffffff);
	        break;
	   }
	SHOW_FLOW0( 2, "11");
	while(B_BUSY == (status = RADEON_VIPFifoIdle( di , 0xff)));
	if(B_OK != status) return false;
	
	// so that reading VIPH_REG_DATA would not trigger unnecessary vip cycles.
	SHOW_FLOW0( 2, "12");
	OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, 
		(INREG( regs, RADEON_VIPH_TIMEOUT_STAT) & 0xffffff00) | RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS);
	return true;
		
}

bool Radeon_VIPFifoRead(device_info *di, uint8 channel, uint32 address, uint32 count, uint8 *buffer, bool lock)
{
	bool res;

	if( lock )
		ACQUIRE_BEN( di->si->cp.lock );
	
	res = do_VIPFifoRead( di, channel, address, count, buffer );
	
	if( lock )
		RELEASE_BEN( di->si->cp.lock );
		
	//SHOW_FLOW( 2, "address=%x, data=%lx, lock=%d", address, *data, lock );

	return res;
}

// write data to VIP
// CP must be hold
static bool do_VIPWrite( device_info *di, uint8 channel, uint address, uint32 data )
{
	vuint8 *regs = di->regs;

	Radeon_WaitForFifo( di, 2 );
	OUTREG( regs, RADEON_VIPH_REG_ADDR, (channel << 14) | (address & ~0x2000) );

	if( !Radeon_VIPWaitForIdle( di )) return false;
		
	//SHOW_FLOW( 4, "channel=%d, address=%x, data=%lx", channel, address, data );
		
	Radeon_WaitForFifo( di, 2 );
	OUTREG( regs, RADEON_VIPH_REG_DATA, data );
	
	return Radeon_VIPWaitForIdle( di );
		
}

// public function: write data to VIP
bool Radeon_VIPWrite(device_info *di, uint8 channel, uint address, uint32 data, bool lock )
{
	bool res;

	//SHOW_FLOW( 2, "address=%x, data=%lx, lock=%d", address, data, lock );

	if( lock )
		ACQUIRE_BEN( di->si->cp.lock );
	
	res = do_VIPWrite( di, channel, address, data );
	
	if( lock )
		RELEASE_BEN( di->si->cp.lock );
	
	return res;
}


static bool do_VIPFifoWrite(device_info *di, uint8 channel, uint32 address,
	uint32 count, uint8 *buffer)
{
	vuint8 *regs = di->regs;

	uint32 status;
	uint32 i;

	SHOW_FLOW( 2, "address=%" B_PRIx32 ", count=%" B_PRIu32 ", ",
		address, count );

	Radeon_WaitForFifo( di, 2 );
	OUTREG( regs, RADEON_VIPH_REG_ADDR,
		((channel << 14) | address | 0x1000) & ~0x2000 );
	SHOW_FLOW0( 2, "1");
	do {
		status = RADEON_VIPFifoIdle(di, 0x0f);
	} while (status == B_BUSY);

	if(B_OK != status){
		SHOW_FLOW( 2 ,"cannot write %x to VIPH_REG_ADDR\n",
			(unsigned int)address);
		return false;
	}

	SHOW_FLOW0( 2, "2");
	for (i = 0; i < count; i+=4)
	{
		Radeon_WaitForFifo( di, 2);
		SHOW_FLOW( 2, "count %" B_PRIu32, count);
		OUTREG( regs, RADEON_VIPH_REG_DATA, *(uint32*)(buffer + i));

		do {
			status = RADEON_VIPFifoIdle(di, 0x0f);
		} while (status == B_BUSY);

    	if(B_OK != status)
		{
    		SHOW_FLOW0( 2 , "cannot write to VIPH_REG_DATA\n");
			return false;
		}
	}
				
	return true;
}

bool Radeon_VIPFifoWrite(device_info *di, uint8 channel, uint32 address, uint32 count, uint8 *buffer, bool lock)
{
    bool res;

	//SHOW_FLOW( 2, "address=%x, data=%lx, lock=%d", address, data, lock );

	if( lock )
		ACQUIRE_BEN( di->si->cp.lock );
	
	Radeon_VIPReset( di, false);
	res = do_VIPFifoWrite( di, channel, address, count, buffer );
	
	if( lock )
		RELEASE_BEN( di->si->cp.lock );
	
	return res;
}


// reset VIP
void Radeon_VIPReset( 
	device_info *di, bool lock )
{
	vuint8 *regs = di->regs;

	if( lock )
		ACQUIRE_BEN( di->si->cp.lock );

	Radeon_WaitForFifo( di, 5 ); // Radeon_WaitForIdle( di, false, false );
	switch(di->asic){
	    case rt_r200:
	    case rt_rs200:
	    case rt_rv200:
	    case rt_rs100:
		case rt_rv100:
		case rt_r100:
	    OUTREG( regs, RADEON_VIPH_CONTROL, 4 | 	(15 << RADEON_VIPH_CONTROL_VIPH_MAX_WAIT_SHIFT) |
			RADEON_VIPH_CONTROL_VIPH_DMA_MODE |	RADEON_VIPH_CONTROL_VIPH_EN ); // slowest, timeout in 16 phases
	    OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, (INREG( regs, RADEON_VIPH_TIMEOUT_STAT) & 0xFFFFFF00) | 
	    	RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS);
	    OUTREG( regs, RADEON_VIPH_DV_LAT, 
	    		0xff |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV0_LAT_SHIFT) |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV1_LAT_SHIFT) |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV2_LAT_SHIFT) |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV3_LAT_SHIFT)); // set timeslice
	    OUTREG( regs, RADEON_VIPH_DMA_CHUNK, 0x151);
	    OUTREG( regs, RADEON_TEST_DEBUG_CNTL, INREG( regs, RADEON_TEST_DEBUG_CNTL) & (~RADEON_TEST_DEBUG_CNTL_OUT_EN));
	default:
		    OUTREG( regs, RADEON_VIPH_CONTROL, 9 | 	(15 << RADEON_VIPH_CONTROL_VIPH_MAX_WAIT_SHIFT) |
				RADEON_VIPH_CONTROL_VIPH_DMA_MODE |	RADEON_VIPH_CONTROL_VIPH_EN ); // slowest, timeout in 16 phases
	    OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, (INREG( regs, RADEON_VIPH_TIMEOUT_STAT) & 0xFFFFFF00) | 
		    	RADEON_VIPH_TIMEOUT_STAT_VIPH_REGR_DIS);
	    OUTREG( regs, RADEON_VIPH_DV_LAT, 
			    0xff |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV0_LAT_SHIFT) |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV1_LAT_SHIFT) |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV2_LAT_SHIFT) |
				(4 << RADEON_VIPH_DV_LAT_VIPH_DV3_LAT_SHIFT)); // set timeslice
	    OUTREG( regs, RADEON_VIPH_DMA_CHUNK, 0x0);
	    OUTREG( regs, RADEON_TEST_DEBUG_CNTL, INREG( regs, RADEON_TEST_DEBUG_CNTL) & (~RADEON_TEST_DEBUG_CNTL_OUT_EN));
	    break;

	} 
		
	if( lock )
		RELEASE_BEN( di->si->cp.lock );
}


// check whether VIP host is idle
// lock must be hold
static status_t Radeon_VIPIdle( 
	device_info *di )
{
	vuint8 *regs = di->regs;
	uint32 timeout;
	
	//Radeon_WaitForIdle( di, false, false );
	
	// if there is a stuck transaction, acknowledge that
	timeout = INREG( regs, RADEON_VIPH_TIMEOUT_STAT );
	if( (timeout & RADEON_VIPH_TIMEOUT_STAT_VIPH_REG_STAT) != 0 ) 
	{
		OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, 
			(timeout & 0xffffff00) | RADEON_VIPH_TIMEOUT_STAT_VIPH_REG_AK);
		return (INREG( regs, RADEON_VIPH_CONTROL) & 0x2000) ? B_BUSY : B_ERROR;
	}
	return (INREG( regs, RADEON_VIPH_CONTROL) & 0x2000) ? B_BUSY : B_OK;
}

static status_t RADEON_VIPFifoIdle(device_info *di, uint8 channel)
{
	vuint8 *regs = di->regs;
	uint32 timeout;

	timeout = INREG( regs, RADEON_VIPH_TIMEOUT_STAT);
	if((timeout & 0x0000000f) & channel) /* lockup ?? */
	{
		OUTREG( regs, RADEON_VIPH_TIMEOUT_STAT, (timeout & 0xfffffff0) | channel);
		return (INREG( regs, RADEON_VIPH_CONTROL) & 0x2000) ? B_BUSY : B_ERROR;
	}
	return (INREG( regs, RADEON_VIPH_CONTROL) & 0x2000) ? B_BUSY : B_OK ;
}
  	 
  	 
// wait until VIP host is idle
// lock must be hold
static bool Radeon_VIPWaitForIdle( 
	device_info *di )
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


// find VIP channel of a device
// return:	>= 0 channel of device
//			< 0 no device found
int Radeon_FindVIPDevice( 
	device_info *di, uint32 device_id )
{
	uint channel;
	uint32 cur_device_id;
	
	// if card has no VIP port, let hardware detection fail;
	// in this case, noone will bother us again
	if( !di->has_vip ) {
		SHOW_FLOW0( 3, "This Device has no VIP Bus.");
		return -1;
	}

	ACQUIRE_BEN( di->si->cp.lock );

	Radeon_VIPReset( di, false );

	// there are up to 4 devices, connected to one of 4 channels	
	for( channel = 0; channel < 4; ++channel ) {
	
		// read device id
		if( !Radeon_VIPRead( di, channel, RADEON_VIP_VENDOR_DEVICE_ID, &cur_device_id, false ))	{
			SHOW_FLOW( 3, "No device found on channel %d", channel);
			continue;
		}
		
		// compare device id directly
		if( cur_device_id == device_id ) {
			SHOW_FLOW( 3, "Device %08" B_PRIx32 " found on channel %d",
				device_id, channel);
			RELEASE_BEN( di->si->cp.lock );
			return channel;
		}
	}
	
	RELEASE_BEN( di->si->cp.lock );
	
	// couldn't find device
	return -1;
}
