/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Programming of TV-out via Rage Theatre
*/


#include "radeon_interface.h"
#include "radeon_accelerant.h"

#include "theatre_regs.h"
#include "tv_out_regs.h"
#include "set_mode.h"

#include <stdlib.h>


// mapping of offset in impactv_regs to register address 
typedef struct register_mapping {
	uint16		address;			// register address
	uint16		offset;				// offset in impactv_regs
} register_mapping;


// Rage Theatre TV-Out:

// registers to write at first
static const register_mapping theatre_reg_mapping_start[] = {
	{ THEATRE_VIP_MASTER_CNTL,		offsetof( impactv_regs, tv_master_cntl ) },
	{ THEATRE_VIP_TVO_DATA_DELAY_A,	offsetof( impactv_regs, tv_data_delay_a ) },
	{ THEATRE_VIP_TVO_DATA_DELAY_B,	offsetof( impactv_regs, tv_data_delay_b ) },
	
	{ THEATRE_VIP_CLKOUT_CNTL,		offsetof( impactv_regs, tv_clkout_cntl ) },
	{ THEATRE_VIP_PLL_CNTL0,		offsetof( impactv_regs, tv_pll_cntl1 ) },
	
	{ THEATRE_VIP_HRESTART,			offsetof( impactv_regs, tv_hrestart ) },
	{ THEATRE_VIP_VRESTART,			offsetof( impactv_regs, tv_vrestart ) },
	{ THEATRE_VIP_FRESTART,			offsetof( impactv_regs, tv_frestart ) },
	{ THEATRE_VIP_FTOTAL,			offsetof( impactv_regs, tv_ftotal ) },
	
	{ THEATRE_VIP_CLOCK_SEL_CNTL, 	offsetof( impactv_regs, tv_clock_sel_cntl ) },
	{ THEATRE_VIP_TV_PLL_CNTL,		offsetof( impactv_regs, tv_tv_pll_cntl ) },
	{ THEATRE_VIP_CRT_PLL_CNTL,		offsetof( impactv_regs, tv_crt_pll_cntl ) },

	{ THEATRE_VIP_HTOTAL,			offsetof( impactv_regs, tv_htotal ) },
	{ THEATRE_VIP_HSIZE,			offsetof( impactv_regs, tv_hsize ) },
	{ THEATRE_VIP_HDISP,			offsetof( impactv_regs, tv_hdisp ) },
	{ THEATRE_VIP_HSTART,			offsetof( impactv_regs, tv_hstart ) },
	{ THEATRE_VIP_VTOTAL,			offsetof( impactv_regs, tv_vtotal ) },
	{ THEATRE_VIP_VDISP,			offsetof( impactv_regs, tv_vdisp ) },

	{ THEATRE_VIP_TIMING_CNTL,		offsetof( impactv_regs, tv_timing_cntl ) },

	{ THEATRE_VIP_VSCALER_CNTL,		offsetof( impactv_regs, tv_vscaler_cntl1 ) },
	{ THEATRE_VIP_VSCALER_CNTL2,	offsetof( impactv_regs, tv_vscaler_cntl2 ) },
	{ THEATRE_VIP_SYNC_SIZE,		offsetof( impactv_regs, tv_sync_size ) },
	{ THEATRE_VIP_Y_SAW_TOOTH_CNTL, offsetof( impactv_regs, tv_y_saw_tooth_cntl ) },
	{ THEATRE_VIP_Y_RISE_CNTL,		offsetof( impactv_regs, tv_y_rise_cntl ) },
	{ THEATRE_VIP_Y_FALL_CNTL,		offsetof( impactv_regs, tv_y_fall_cntl ) },
	
	{ THEATRE_VIP_MODULATOR_CNTL1,	offsetof( impactv_regs, tv_modulator_cntl1 ) },
	{ THEATRE_VIP_MODULATOR_CNTL2,	offsetof( impactv_regs, tv_modulator_cntl2 ) },
	
	{ THEATRE_VIP_RGB_CNTL,			offsetof( impactv_regs, tv_rgb_cntl ) },
	
	{ THEATRE_VIP_UV_ADR,			offsetof( impactv_regs, tv_uv_adr ) },
	
	{ THEATRE_VIP_PRE_DAC_MUX_CNTL, offsetof( impactv_regs, tv_pre_dac_mux_cntl ) },
	{ THEATRE_VIP_FRAME_LOCK_CNTL,	offsetof( impactv_regs, tv_frame_lock_cntl ) },
	{ THEATRE_VIP_CRC_CNTL,			offsetof( impactv_regs, tv_crc_cntl ) },
	{ 0, 0 }
};

// registers to write when things settled down
static const register_mapping theatre_reg_mapping_finish[] = {
	{ THEATRE_VIP_UPSAMP_COEFF0_0,	offsetof( impactv_regs, tv_upsample_filter_coeff[0*3+0] ) },
	{ THEATRE_VIP_UPSAMP_COEFF0_1,	offsetof( impactv_regs, tv_upsample_filter_coeff[0*3+1] ) },
	{ THEATRE_VIP_UPSAMP_COEFF0_2,	offsetof( impactv_regs, tv_upsample_filter_coeff[0*3+2] ) },
	{ THEATRE_VIP_UPSAMP_COEFF1_0,	offsetof( impactv_regs, tv_upsample_filter_coeff[1*3+0] ) },
	{ THEATRE_VIP_UPSAMP_COEFF1_1,	offsetof( impactv_regs, tv_upsample_filter_coeff[1*3+1] ) },
	{ THEATRE_VIP_UPSAMP_COEFF1_2,	offsetof( impactv_regs, tv_upsample_filter_coeff[1*3+2] ) },
	{ THEATRE_VIP_UPSAMP_COEFF2_0,	offsetof( impactv_regs, tv_upsample_filter_coeff[2*3+0] ) },
	{ THEATRE_VIP_UPSAMP_COEFF2_1,	offsetof( impactv_regs, tv_upsample_filter_coeff[2*3+1] ) },
	{ THEATRE_VIP_UPSAMP_COEFF2_2,	offsetof( impactv_regs, tv_upsample_filter_coeff[2*3+2] ) },
	{ THEATRE_VIP_UPSAMP_COEFF3_0,	offsetof( impactv_regs, tv_upsample_filter_coeff[3*3+0] ) },
	{ THEATRE_VIP_UPSAMP_COEFF3_1,	offsetof( impactv_regs, tv_upsample_filter_coeff[3*3+1] ) },
	{ THEATRE_VIP_UPSAMP_COEFF3_2,	offsetof( impactv_regs, tv_upsample_filter_coeff[3*3+2] ) },
	{ THEATRE_VIP_UPSAMP_COEFF4_0,	offsetof( impactv_regs, tv_upsample_filter_coeff[4*3+0] ) },
	{ THEATRE_VIP_UPSAMP_COEFF4_1,	offsetof( impactv_regs, tv_upsample_filter_coeff[4*3+1] ) },
	{ THEATRE_VIP_UPSAMP_COEFF4_2,	offsetof( impactv_regs, tv_upsample_filter_coeff[4*3+2] ) },

	{ THEATRE_VIP_GAIN_LIMIT_SETTINGS,  offsetof( impactv_regs, tv_gain_limit_settings ) },
	{ THEATRE_VIP_LINEAR_GAIN_SETTINGS, offsetof( impactv_regs, tv_linear_gain_settings ) },
	{ THEATRE_VIP_UPSAMP_AND_GAIN_CNTL, offsetof( impactv_regs, tv_upsamp_and_gain_cntl ) },

	{ THEATRE_VIP_TV_DAC_CNTL,		offsetof( impactv_regs, tv_dac_cntl ) },
	{ THEATRE_VIP_MASTER_CNTL,		offsetof( impactv_regs, tv_master_cntl ) },
	{ 0, 0 }
};


// write list of Rage Theatre registers
static void writeTheatreRegList( 
	accelerator_info *ai, impactv_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 || mapping->offset != 0; ++mapping ) {
		Radeon_VIPWrite( ai, ai->si->theatre_channel, mapping->address,
			*(uint32 *)((char *)(values) + mapping->offset) );
			
/*		SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/
	}
}


// read timing FIFO
uint32 Radeon_TheatreReadFIFO( 
	accelerator_info *ai, uint16 addr )
{
	bigtime_t start_time;
	uint32 res = ~0;
	
	//SHOW_FLOW( 2, "addr=%d", addr );
	
	Radeon_VIPWrite( ai, ai->si->theatre_channel, 
		THEATRE_VIP_HOST_RD_WT_CNTL, addr | RADEON_TV_HOST_RD_WT_CNTL_RD );
	
	start_time = system_time();
	
	do {
		uint32 status;
		
		Radeon_VIPRead( ai, ai->si->theatre_channel, THEATRE_VIP_HOST_RD_WT_CNTL, &status );
		
		if( (status & RADEON_TV_HOST_RD_WT_CNTL_RD_ACK) != 0 )
			break;
	} while( system_time() - start_time < 2000000 );
	
	Radeon_VIPWrite( ai, ai->si->theatre_channel, THEATRE_VIP_HOST_RD_WT_CNTL, 0);
	Radeon_VIPRead( ai, ai->si->theatre_channel, THEATRE_VIP_HOST_READ_DATA, &res );

	return res;
}


// write to timing FIFO
void Radeon_TheatreWriteFIFO( 
	accelerator_info *ai, uint16 addr, uint32 value )
{
	bigtime_t start_time;
	
	//readFIFO( ai, addr, internal_encoder );
	
	//SHOW_FLOW( 2, "addr=%d, value=%x %x", addr, value >> 14, value & 0x3fff );
	
	Radeon_VIPWrite( ai, ai->si->theatre_channel, THEATRE_VIP_HOST_WRITE_DATA, value);
	Radeon_VIPWrite( ai, ai->si->theatre_channel, 
		THEATRE_VIP_HOST_RD_WT_CNTL, addr | RADEON_TV_HOST_RD_WT_CNTL_WT);
	
	start_time = system_time();
	
	do {
		uint32 status;
		
		Radeon_VIPRead( ai, ai->si->theatre_channel, THEATRE_VIP_HOST_RD_WT_CNTL, &status );
		
		if( (status & RADEON_TV_HOST_RD_WT_CNTL_WT_ACK) != 0 )
			break;
	} while( system_time() - start_time < 2000000 );
	
	Radeon_VIPWrite( ai, ai->si->theatre_channel, THEATRE_VIP_HOST_RD_WT_CNTL, 0 );
}


// program TV-Out registers
void Radeon_TheatreProgramTVRegisters( 
	accelerator_info *ai, impactv_regs *values )
{	
	uint32 orig_tv_master_cntl = values->tv_master_cntl;
	
	SHOW_FLOW0( 2, "" );
	
	// disable TV-out when registers are setup
	// it gets enabled again when things have settled down		
	values->tv_master_cntl |=
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_TV_FIFO_ASYNC_RST |
		
		RADEON_TV_MASTER_CNTL_VIN_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_AUD_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_DVS_ASYNC_RST;
		
	writeTheatreRegList( ai, values, theatre_reg_mapping_start );

	// un-reset FIFO to access timing table
	Radeon_VIPWrite( ai, ai->si->theatre_channel, THEATRE_VIP_MASTER_CNTL,
		orig_tv_master_cntl |
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST |
		
		RADEON_TV_MASTER_CNTL_VIN_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_AUD_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_DVS_ASYNC_RST );

	Radeon_ImpacTVwriteHorTimingTable( ai, Radeon_TheatreWriteFIFO, values, false );
	Radeon_ImpacTVwriteVertTimingTable( ai, Radeon_TheatreWriteFIFO, values );

	snooze( 50000 );

	values->tv_master_cntl = orig_tv_master_cntl;
	writeTheatreRegList( ai, values, theatre_reg_mapping_finish );
}


// read list of Rage Theatre registers
static void readTheatreRegList( 
	accelerator_info *ai, impactv_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 || mapping->offset != 0; ++mapping ) {
		Radeon_VIPRead( ai, ai->si->theatre_channel, mapping->address,
			(uint32 *)((char *)(values) + mapping->offset) );
			
		/*SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/
	}
	
	//snooze( 1000000 );
}


// read TV-Out registers
void Radeon_TheatreReadTVRegisters( 
	accelerator_info *ai, impactv_regs *values )
{
	readTheatreRegList( ai, values, theatre_reg_mapping_start );
	readTheatreRegList( ai, values, theatre_reg_mapping_finish );
	
	//snooze( 1000000 );
}


// detect TV-Out encoder
void Radeon_DetectTVOut( 
	accelerator_info *ai )
{
	shared_info *si = ai->si;
	
	SHOW_FLOW0( 0, "" );
	
	switch( si->tv_chip ) {
	case tc_external_rt1: {
		// for external encoder, we need the VIP channel
		int channel = Radeon_FindVIPDevice( ai, THEATRE_ID );
		
		if( channel < 0 ) {
			SHOW_ERROR0( 2, "This card needs a Rage Theatre for TV-Out, but there is none." );
			si->tv_chip = tc_none;
		} else {
			SHOW_INFO( 2, "Rage Theatre found on VIP channel %d", channel );
			si->theatre_channel = channel;
		}
		break; }
	default:
		// for internal encoder, we don't have to look farther - it must be there
		;
	}
}
