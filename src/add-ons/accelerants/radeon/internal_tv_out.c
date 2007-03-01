/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Programming of internal TV-out unit
*/

#include "radeon_interface.h"
#include "radeon_accelerant.h"

#include "tv_out_regs.h"
#include "pll_access.h"
#include "mmio.h"
#include "utils.h"
#include "set_mode.h"

#include <stdlib.h>


// mapping of offset in impactv_regs to register address 
typedef struct register_mapping {
	uint16		address;			// register address
	uint16		offset;				// offset in impactv_regs
} register_mapping;


// internal TV-encoder:

// registers to write before programming PLL
static const register_mapping intern_reg_mapping_before_pll[] = {
	{ RADEON_TV_MASTER_CNTL,		offsetof( impactv_regs, tv_master_cntl ) },
	{ RADEON_TV_HRESTART,			offsetof( impactv_regs, tv_hrestart ) },
	{ RADEON_TV_VRESTART,			offsetof( impactv_regs, tv_vrestart ) },
	{ RADEON_TV_FRESTART,			offsetof( impactv_regs, tv_frestart ) },
	{ RADEON_TV_FTOTAL,				offsetof( impactv_regs, tv_ftotal ) },
	{ 0, 0 }
};

// PLL registers to program
static const register_mapping intern_reg_mapping_pll[] = {
	{ RADEON_TV_PLL_CNTL,			offsetof( impactv_regs, tv_tv_pll_cntl ) },
	{ RADEON_TV_PLL_CNTL1,			offsetof( impactv_regs, tv_pll_cntl1 ) },
	{ RADEON_TV_PLL_FINE_CNTL,		offsetof( impactv_regs, tv_pll_fine_cntl ) },
	{ 0, 0 }
};

// registers to write after programming of PLL
static const register_mapping intern_reg_mapping_after_pll[] = {
	{ RADEON_TV_HTOTAL,				offsetof( impactv_regs, tv_htotal ) },
	{ RADEON_TV_HDISP,				offsetof( impactv_regs, tv_hdisp ) },
	{ RADEON_TV_HSTART,				offsetof( impactv_regs, tv_hstart ) },
	{ RADEON_TV_VTOTAL,				offsetof( impactv_regs, tv_vtotal ) },
	{ RADEON_TV_VDISP,				offsetof( impactv_regs, tv_vdisp ) },
	
	{ RADEON_TV_TIMING_CNTL,		offsetof( impactv_regs, tv_timing_cntl ) },

	{ RADEON_TV_VSCALER_CNTL1,		offsetof( impactv_regs, tv_vscaler_cntl1 ) },
	{ RADEON_TV_VSCALER_CNTL2,		offsetof( impactv_regs, tv_vscaler_cntl2 ) },
	
	{ RADEON_TV_Y_SAW_TOOTH_CNTL,	offsetof( impactv_regs, tv_y_saw_tooth_cntl ) },
	{ RADEON_TV_Y_RISE_CNTL,		offsetof( impactv_regs, tv_y_rise_cntl ) },
	{ RADEON_TV_Y_FALL_CNTL,		offsetof( impactv_regs, tv_y_fall_cntl ) },
	
	{ RADEON_TV_MODULATOR_CNTL1,	offsetof( impactv_regs, tv_modulator_cntl1 ) },
	{ RADEON_TV_MODULATOR_CNTL2,	offsetof( impactv_regs, tv_modulator_cntl2 ) },
	{ RADEON_TV_RGB_CNTL,			offsetof( impactv_regs, tv_rgb_cntl ) },
	{ RADEON_TV_UV_ADR,				offsetof( impactv_regs, tv_uv_adr ) },
	{ RADEON_TV_PRE_DAC_MUX_CNTL, 	offsetof( impactv_regs, tv_pre_dac_mux_cntl ) },
	{ RADEON_TV_CRC_CNTL,			offsetof( impactv_regs, tv_crc_cntl ) },
	{ 0, 0 }
};

// registers to write when things settled down
static const register_mapping intern_reg_mapping_finish[] = {
	{ RADEON_TV_GAIN_LIMIT_SETTINGS,  offsetof( impactv_regs, tv_gain_limit_settings ) },
	{ RADEON_TV_LINEAR_GAIN_SETTINGS, offsetof( impactv_regs, tv_linear_gain_settings ) },
	{ RADEON_TV_UPSAMP_AND_GAIN_CNTL, offsetof( impactv_regs, tv_upsamp_and_gain_cntl ) },

	{ RADEON_TV_DAC_CNTL,			offsetof( impactv_regs, tv_dac_cntl ) },
	{ RADEON_TV_MASTER_CNTL,		offsetof( impactv_regs, tv_master_cntl ) },
	{ 0, 0 }
};




// write list of MM I/O registers
static void writeMMIORegList( 
	accelerator_info *ai, impactv_regs *values, const register_mapping *mapping )
{	
	vuint8 *regs = ai->regs;
	
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		/*SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/

		OUTREG( regs, mapping->address, *(uint32 *)((char *)(values) + mapping->offset) );
	}
	
	//snooze( 1000000 );
}


// write list of PLL registers
static void writePLLRegList( 
	accelerator_info *ai, impactv_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		/*SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/

		Radeon_OUTPLL( ai->regs, ai->si->asic,
			mapping->address, *(uint32 *)((char *)(values) + mapping->offset) );
	}
	
	//snooze( 1000000 );
}




// read timing FIFO
#if 0
static uint32 Radeon_InternalTVOutReadFIFO( 
	accelerator_info *ai, uint16 addr )
{
	vuint8 *regs = ai->regs;
	bigtime_t start_time;
	uint32 res = ~0;
	
	//SHOW_FLOW( 2, "addr=%d", addr );
	
	OUTREG( regs, RADEON_TV_HOST_RD_WT_CNTL, addr | RADEON_TV_HOST_RD_WT_CNTL_RD);
	
	start_time = system_time();
	
	do {
		uint32 status;
		
		status = INREG( regs, RADEON_TV_HOST_RD_WT_CNTL );
		
		if( (status & RADEON_TV_HOST_RD_WT_CNTL_RD_ACK) != 0 )
			break;
	} while( system_time() - start_time < 2000000 );
	
	OUTREG( regs, RADEON_TV_HOST_RD_WT_CNTL, 0);
	res = INREG( regs, RADEON_TV_HOST_READ_DATA );
	
	//SHOW_FLOW( 2, "res=%x %x", res >> 14, res & 0x3fff );
	
	return res;
}
#endif

// write to timing FIFO
static void Radeon_InternalTVOutWriteFIFO( 
	accelerator_info *ai, uint16 addr, uint32 value )
{
	vuint8 *regs = ai->regs;
	bigtime_t start_time;
	
	//readFIFO( ai, addr, internal_encoder );
	
	//SHOW_FLOW( 2, "addr=%d, value=%x %x", addr, value >> 14, value & 0x3fff );
	
	OUTREG( regs, RADEON_TV_HOST_WRITE_DATA, value );
	OUTREG( regs, RADEON_TV_HOST_RD_WT_CNTL, addr | RADEON_TV_HOST_RD_WT_CNTL_WT );
	
	start_time = system_time();
	
	do {
		uint32 status;
		
		status = INREG( regs, RADEON_TV_HOST_RD_WT_CNTL );
		
		if( (status & RADEON_TV_HOST_RD_WT_CNTL_WT_ACK) != 0 )
			break;
	} while( system_time() - start_time < 2000000 );
	
	OUTREG( regs, RADEON_TV_HOST_RD_WT_CNTL, 0 );
}


// program TV-Out registers
void Radeon_InternalTVOutProgramRegisters( 
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
		
	writeMMIORegList( ai, values, intern_reg_mapping_before_pll );
	writePLLRegList( ai, values, intern_reg_mapping_pll );
	writeMMIORegList( ai, values, intern_reg_mapping_after_pll );
	
	// un-reset FIFO to access timing table
	OUTREG( ai->regs, RADEON_TV_MASTER_CNTL,
		orig_tv_master_cntl |
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST |
		
		RADEON_TV_MASTER_CNTL_VIN_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_AUD_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_DVS_ASYNC_RST );
	
	Radeon_ImpacTVwriteHorTimingTable( ai, Radeon_InternalTVOutWriteFIFO, values, true );
	Radeon_ImpacTVwriteVertTimingTable( ai, Radeon_InternalTVOutWriteFIFO, values );

	snooze( 50000 );

	values->tv_master_cntl = orig_tv_master_cntl;
	writeMMIORegList( ai, values, intern_reg_mapping_finish );
}


// read list of MM I/O registers
static void readMMIORegList( 
	accelerator_info *ai, impactv_regs *values, const register_mapping *mapping )
{
	vuint8 *regs = ai->regs;
	
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		*(uint32 *)((char *)(values) + mapping->offset) = 
			INREG( regs, mapping->address );
			
		/*SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/
	}
	
	//snooze( 1000000 );
}


// read list of PLL registers
static void readPLLRegList( 
	accelerator_info *ai, impactv_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		*(uint32 *)((char *)(values) + mapping->offset) = 
			Radeon_INPLL( ai->regs, ai->si->asic, mapping->address );
			
		/*SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/
	}
	
	//snooze( 1000000 );
}


// read TV-Out registers
void Radeon_InternalTVOutReadRegisters( 
	accelerator_info *ai, impactv_regs *values )
{
	readMMIORegList( ai, values, intern_reg_mapping_before_pll );
	readPLLRegList( ai, values, intern_reg_mapping_pll );
	readMMIORegList( ai, values, intern_reg_mapping_after_pll );
	readMMIORegList( ai, values, intern_reg_mapping_finish );
	
	//snooze( 1000000 );
}
