/*
	Copyright (c) 2002,03 Thomas Kurschel
	

	Part of Radeon accelerant
		
	Monitor detection
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "crtc_regs.h"
#include "dac_regs.h"
#include "pll_regs.h"
#include "tv_out_regs.h"
#include "config_regs.h"
#include "ddc_regs.h"
#include "gpiopad_regs.h"
#include "pll_access.h"
#include "ddc.h"
#include <malloc.h>

typedef struct {
	accelerator_info *ai;
	uint32 port;
} ddc_port_info;


// get I2C signals
static status_t get_signals( void *cookie, int *clk, int *data )
{
	ddc_port_info *info = (ddc_port_info *)cookie;
	vuint8 *regs = info->ai->regs;
	uint32 value;
	
	value = INREG( regs, info->port );
	
	*clk = (value >> RADEON_GPIO_Y_SHIFT_1) & 1;
	*data = (value >> RADEON_GPIO_Y_SHIFT_0) & 1;
	
	return B_OK;
}


// set I2C signals
static status_t set_signals( void *cookie, int clk, int data )
{
	ddc_port_info *info = (ddc_port_info *)cookie;
	vuint8 *regs = info->ai->regs;
	uint32 value;

	value = INREG( regs, info->port );
	value &= ~(RADEON_GPIO_A_1 | RADEON_GPIO_A_0);
	value &= ~(RADEON_GPIO_EN_0 | RADEON_GPIO_EN_1);		
	value |= ((1-clk) << RADEON_GPIO_EN_SHIFT_1) | ((1-data) << RADEON_GPIO_EN_SHIFT_0);

	OUTREG( regs, info->port, value );
	
	return B_OK;
}


/*
// check whether there is a monitor by talking to him via DDC2
// ddc_port - register to use for DDC2 communication
static bool Radeon_DetectMonitorViaDDC( accelerator_info *ai, uint32 ddc_port )
{
	i2c_bus bus;
	ddc_port_info info;
	edid1_info edid;
	void *vdif;
	size_t vdif_len;
	status_t res;
	
	info.ai = ai;
	info.port = ddc_port;
	
	bus.cookie = &info;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;

	res = ddc2_read_edid1( &bus, &edid, &vdif, &vdif_len );
	if( res != B_OK )
		return false;

	if( vdif != NULL )
		free( vdif );
		
	SHOW_INFO( 2, "Found monitor on DDC port 0x%04x", ddc_port );

	return true;
}
*/

// read EDID information from monitor
// ddc_port - register to use for DDC2 communication
bool Radeon_ReadEDID( accelerator_info *ai, uint32 ddc_port, edid1_info *edid )
{
	i2c_bus bus;
	ddc_port_info info;
	void *vdif;
	size_t vdif_len;
	status_t res;
	
	info.ai = ai;
	info.port = ddc_port;
	
	bus.cookie = &info;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;

	res = ddc2_read_edid1( &bus, edid, &vdif, &vdif_len );
	if( res != B_OK )
		return false;
		
	SHOW_FLOW( 2, "Found DDC-capable monitor @0x%04x", ddc_port );

	if( vdif != NULL )
		free( vdif );

	return true;
}


// search for display connect to CRT DAC
// colour - true, if only a colour monitor is to be accepted
static bool Radeon_DetectCRTInt( accelerator_info *ai, bool colour )
{
	vuint8 *regs = ai->regs;
	uint32 old_crtc_ext_cntl, old_dac_ext_cntl, old_dac_cntl, tmp;
	bool found;

	// makes sure there is a signal	
	old_crtc_ext_cntl = INREG( regs, RADEON_CRTC_EXT_CNTL );

	tmp = old_crtc_ext_cntl | RADEON_CRTC_CRT_ON;
	OUTREG( regs, RADEON_CRTC_EXT_CNTL, tmp );

	// force DAC to output constant voltage
	// for colour monitors, RGB is tested, for B/W only G	
	old_dac_ext_cntl = INREG( regs, RADEON_DAC_EXT_CNTL );
	
	tmp = 
		RADEON_DAC_FORCE_BLANK_OFF_EN |
		RADEON_DAC_FORCE_DATA_EN |
		(colour ? RADEON_DAC_FORCE_DATA_SEL_RGB : RADEON_DAC_FORCE_DATA_SEL_G) |
		(0x1b6 << RADEON_DAC_FORCE_DATA_SHIFT);
	OUTREG( regs, RADEON_DAC_EXT_CNTL, tmp );
	
	// enable DAC and tell is to use VGA signals
	old_dac_cntl = INREG( regs, RADEON_DAC_CNTL );
	
	tmp = old_dac_cntl & ~(RADEON_DAC_RANGE_CNTL_MASK | RADEON_DAC_PDWN);
	tmp |= RADEON_DAC_RANGE_CNTL_PS2 | RADEON_DAC_CMP_EN;
	OUTREG( regs, RADEON_DAC_CNTL, tmp );

	// specs says that we should wait 1µs before checking but sample
	// code uses 2 ms; we use long delay to be on safe side
	// (though we don't want to make it too long as the monitor 
	// gets no sync signal now)	
	snooze( 2000 );

	// let's see whether there is some	
	found = (INREG( regs, RADEON_DAC_CNTL ) & RADEON_DAC_CMP_OUTPUT) != 0;
	
	if( found )
		SHOW_INFO( 2, "Found %s CRT connected to CRT-DAC", colour ? "colour" : "b/w" );
	
	OUTREG( regs, RADEON_DAC_CNTL, old_dac_cntl );
	OUTREG( regs, RADEON_DAC_EXT_CNTL, old_dac_ext_cntl );
	OUTREG( regs, RADEON_CRTC_EXT_CNTL, old_crtc_ext_cntl );
	
	return found;
}


// check whethere there is a CRT connected to CRT DAC
static bool Radeon_DetectCRT( accelerator_info *ai )
{
	vuint32 old_vclk_ecp_cntl, tmp;
	bool found;

	// enforce clock so the DAC gets activated	
	old_vclk_ecp_cntl = Radeon_INPLL( ai->regs, ai->si->asic, RADEON_VCLK_ECP_CNTL );	

	tmp = old_vclk_ecp_cntl &
		~(RADEON_PIXCLK_ALWAYS_ONb | RADEON_PIXCLK_DAC_ALWAYS_ONb);
	Radeon_OUTPLL( ai->regs, ai->si->asic, RADEON_VCLK_ECP_CNTL, tmp );

	// search first for colour, then for B/W monitor
	found = Radeon_DetectCRTInt( ai, true ) || Radeon_DetectCRTInt( ai, false );
	
	Radeon_OUTPLL( ai->regs, ai->si->asic, RADEON_VCLK_ECP_CNTL, old_vclk_ecp_cntl );
	
	return found;
}


// CRT on TV-DAC detection for rv200 and below
// checked for rv200
static bool Radeon_DetectTVCRT_RV200( accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	uint32 old_crtc2_gen_cntl, old_tv_dac_cntl, old_dac_cntl2, tmp;
	bool found;

	// enable CRTC2, setting 8 bpp (we just pick any valid value)
	old_crtc2_gen_cntl = INREG( regs, RADEON_CRTC2_GEN_CNTL );
	
	tmp = old_crtc2_gen_cntl & ~RADEON_CRTC2_PIX_WIDTH_MASK;
	tmp |= 
		RADEON_CRTC2_CRT2_ON |
		(2 << RADEON_CRTC2_PIX_WIDTH_SHIFT);
	OUTREG( regs, RADEON_CRTC2_GEN_CNTL, tmp );
	
	// enable TV-DAC, choosing VGA signal level
	old_tv_dac_cntl = INREG( regs, RADEON_TV_DAC_CNTL );
	
	tmp = 
		RADEON_TV_DAC_CNTL_NBLANK |
		RADEON_TV_DAC_CNTL_NHOLD |
		RADEON_TV_DAC_CNTL_DETECT |
		RADEON_TV_DAC_CNTL_STD_PS2;
	OUTREG( regs, RADEON_TV_DAC_CNTL, tmp );
	
	// enforce constant DAC output voltage on RGB
	tmp =
		RADEON_DAC2_FORCE_BLANK_OFF_EN |
		RADEON_DAC2_FORCE_DATA_EN |
		RADEON_DAC_FORCE_DATA_SEL_RGB |
		(0x180 << RADEON_DAC_FORCE_DATA_SHIFT);
	OUTREG( regs, RADEON_DAC_EXT_CNTL, tmp );
	
	old_dac_cntl2 = INREG( regs, RADEON_DAC_CNTL2 );
	
	// set DAC in CRT mode and enable detection
	// TODO: make sure we really use CRTC2 - this is ASIC dependant
	tmp = old_dac_cntl2 | RADEON_DAC2_CLK_SEL_CRT | RADEON_DAC2_CMP_EN;
	OUTREG( regs, RADEON_DAC_CNTL2, tmp );
	
	snooze( 10000 );

	// let's see what we've got!	
	found = (INREG( regs, RADEON_DAC_CNTL2 ) & RADEON_DAC2_CMP_OUTPUT) != 0;
	
	if( found )
		SHOW_INFO0( 2, "Found CRT connected to TV-DAC, i.e. DVI port" );
	
	OUTREG( regs, RADEON_DAC_CNTL2, old_dac_cntl2 );
	OUTREG( regs, RADEON_DAC_EXT_CNTL, 0 );
	OUTREG( regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl );
	OUTREG( regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl );
	
	return found;
}

// CRT on TV-DAC detection for r300
// checked for r300
static bool Radeon_DetectTVCRT_R300( accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	uint32 old_crtc2_gen_cntl, old_tv_dac_cntl, old_dac_cntl2, tmp;
	bool found;

	// whatever these flags mean - let's pray they won't get changed
	OUTREGP( regs, RADEON_GPIOPAD_EN, 1, ~1 );
	OUTREGP( regs, RADEON_GPIOPAD_MASK, 1, ~1 );
	OUTREGP( regs, RADEON_GPIOPAD_A, 1, ~1 );

	old_crtc2_gen_cntl = INREG( regs, RADEON_CRTC2_GEN_CNTL );

	// enable DAC, choose valid pixel format and enable DPMS
	// as usual, the code doesn't take into account whether the TV-DAC
	// does really use CRTC2	
	tmp = old_crtc2_gen_cntl;
	tmp &= ~RADEON_CRTC2_PIX_WIDTH_MASK;
	tmp |= 
		(2 << RADEON_CRTC2_PIX_WIDTH_SHIFT) |
		RADEON_CRTC2_CRT2_ON | RADEON_CRTC2_VSYNC_TRISTAT;
		
	OUTREG( regs, RADEON_CRTC2_GEN_CNTL, tmp );
	
	old_tv_dac_cntl = INREG( regs, RADEON_TV_DAC_CNTL );
	
	// enable TV-DAC
	OUTREG( regs, RADEON_TV_DAC_CNTL, 
		RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD |
		RADEON_TV_DAC_CNTL_DETECT |
		RADEON_TV_DAC_CNTL_STD_PS2 );
		
	// force constant voltage output of DAC for impedance test
	OUTREG( regs, RADEON_DAC_EXT_CNTL, 
		RADEON_DAC2_FORCE_BLANK_OFF_EN | RADEON_DAC2_FORCE_DATA_EN |
    	RADEON_DAC_FORCE_DATA_SEL_RGB |
    	(0x1b6 << RADEON_DAC_FORCE_DATA_SHIFT ));
    	
    old_dac_cntl2 = INREG( regs, RADEON_DAC_CNTL2 );

	// enable CRT mode of TV-DAC and enable comparator    
    tmp = old_dac_cntl2 | RADEON_DAC2_CLK_SEL_CRT | RADEON_DAC2_CMP_EN;
    
    OUTREG( regs, RADEON_DAC_CNTL2, tmp );
    
    snooze( 10000 );
    
    // check connection of blue data signal to see whether there is a CRT
	found = (INREG( regs, RADEON_DAC_CNTL2 ) & RADEON_DAC2_CMP_OUT_B) != 0;
    
    // clean up the mess
    OUTREG( regs, RADEON_DAC_CNTL2, old_dac_cntl2 );
    OUTREG( regs, RADEON_DAC_EXT_CNTL, 0 );
    OUTREG( regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl );
    OUTREG( regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl );
    
    return found;
}


// check whether there is a CRT connected to TV-DAC
static bool Radeon_DetectTVCRT( accelerator_info *ai )
{
	switch( ai->si->asic ) {
	case rt_r100:
	case rt_m6:
	case rt_m7:
		// original Radeons have pure DVI only and mobility chips
		// have no DVI connector
		// TBD: can they have a docking station for CRT on TV-DAC?
		return dd_none;
		
	case rt_ve:
	case rt_rv200:
	case rt_rv250:
	case rt_rv280:
		return Radeon_DetectTVCRT_RV200( ai );
		
	case rt_r300:
	case rt_r300_4p:
	case rt_rv350:
	case rt_rv360:
	case rt_r350:
	case rt_r360:
		return Radeon_DetectTVCRT_R300( ai );
		
	default:
		// don't know about IGP
		;
	}
			
	return dd_none;
}


// TV detection for rv200 and below
// should work for M6 and RV200
static display_device_e Radeon_DetectTV_RV200( accelerator_info *ai, bool tv_crt_found )
{
	vuint8 *regs = ai->regs;
	uint32 
		tmp, old_dac_cntl2, old_crtc_ext_cntl, old_crtc2_gen_cntl, old_tv_master_cntl, 
		old_tv_dac_cntl, old_pre_dac_mux_cntl, config_cntl;
	display_device_e displays = dd_none;
	
	// give up if there is a CRT connected to TV-DAC
	if( tv_crt_found )
		return dd_none;

	// enable TV mode
	old_dac_cntl2 = INREG( regs, RADEON_DAC_CNTL2 );
	tmp = old_dac_cntl2 & ~RADEON_DAC2_CLK_SEL_CRT;
	OUTREG( regs, RADEON_DAC_CNTL2, tmp );
	
	old_crtc_ext_cntl = INREG( regs, RADEON_CRTC_EXT_CNTL );
	old_crtc2_gen_cntl = INREG( regs, RADEON_CRTC2_GEN_CNTL );
	old_tv_master_cntl = INREG( regs, RADEON_TV_MASTER_CNTL );
	
	// enable TV output
	tmp = old_tv_master_cntl | RADEON_TV_MASTER_CNTL_TV_ON;
	tmp &= ~(
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST | 
		RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX | 
		RADEON_TV_MASTER_CNTL_CRT_FIFO_CE_EN | 
		RADEON_TV_MASTER_CNTL_TV_FIFO_CE_EN | 
		RADEON_TV_MASTER_CNTL_RE_SYNC_NOW_SEL_MASK);
	tmp |= 
		RADEON_TV_MASTER_CNTL_TV_FIFO_ASYNC_RST | 
		RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST;
	OUTREG( regs, RADEON_TV_MASTER_CNTL, tmp );

	old_tv_dac_cntl = INREG( regs, RADEON_TV_DAC_CNTL );	
	
	config_cntl = INREG( regs, RADEON_CONFIG_CNTL );

	// unlock TV DAC	
	tmp = 
		RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD | 
		RADEON_TV_DAC_CNTL_DETECT | RADEON_TV_DAC_CNTL_STD_NTSC | 
		(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) | 
		((((config_cntl & RADEON_CFG_ATI_REV_ID_MASK) == 0) ? 8 : 4) << RADEON_TV_DAC_CNTL_DACADJ_SHIFT);
	OUTREG( regs, RADEON_TV_DAC_CNTL, tmp );
	
	old_pre_dac_mux_cntl = INREG( regs, RADEON_TV_PRE_DAC_MUX_CNTL );
	
	// force constant DAC output voltage
	tmp = 
		RADEON_TV_PRE_DAC_MUX_CNTL_C_GRN_EN | RADEON_TV_PRE_DAC_MUX_CNTL_CMP_BLU_EN | 
		(RADEON_TV_MUX_FORCE_DAC_DATA << RADEON_TV_PRE_DAC_MUX_CNTL_RED_MX_SHIFT) |
		(RADEON_TV_MUX_FORCE_DAC_DATA << RADEON_TV_PRE_DAC_MUX_CNTL_GRN_MX_SHIFT) |
		(RADEON_TV_MUX_FORCE_DAC_DATA << RADEON_TV_PRE_DAC_MUX_CNTL_BLU_MX_SHIFT) |
		(0x109 << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT);
	OUTREG( regs, RADEON_TV_PRE_DAC_MUX_CNTL, tmp );
	
	// let things settle a bit
	snooze( 3000 );
	
	// now see which wires are connected
	tmp = INREG( regs, RADEON_TV_DAC_CNTL );
	if( (tmp & RADEON_TV_DAC_CNTL_GDACDET) != 0 ) {
		displays |= dd_stv;
		SHOW_INFO0( 2, "S-Video TV-Out is connected" );
	}
	
	if( (tmp & RADEON_TV_DAC_CNTL_BDACDET) != 0 ) {
		displays |= dd_ctv;
		SHOW_INFO0( 2, "Composite TV-Out is connected" );
	}
		
	OUTREG( regs, RADEON_TV_PRE_DAC_MUX_CNTL, old_pre_dac_mux_cntl );
	OUTREG( regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl );
	OUTREG( regs, RADEON_TV_MASTER_CNTL, old_tv_master_cntl );
	OUTREG( regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl );
	OUTREG( regs, RADEON_CRTC_EXT_CNTL, old_crtc_ext_cntl );
	OUTREG( regs, RADEON_DAC_CNTL2, old_dac_cntl2 );
	
	return displays;
}


// TV detection for r300 series
// should work for R300
static display_device_e Radeon_DetectTV_R300( accelerator_info *ai )
{
	vuint8 *regs = ai->regs;
	display_device_e displays = dd_none;
	uint32 tmp, old_dac_cntl2, old_crtc2_gen_cntl, old_dac_ext_cntl, old_tv_dac_cntl;

	// whatever these flags mean - let's pray they won't get changed
	OUTREGP( regs, RADEON_GPIOPAD_EN, 1, ~1 );
	OUTREGP( regs, RADEON_GPIOPAD_MASK, 1, ~1 );
	OUTREGP( regs, RADEON_GPIOPAD_A, 0, ~1 );
	
	old_dac_cntl2 = INREG( regs, RADEON_DAC_CNTL2 );
	
	// set CRT mode (!) of TV-DAC
	OUTREG( regs, RADEON_DAC_CNTL2, RADEON_DAC2_CLK_SEL_CRT );
	
	old_crtc2_gen_cntl = INREG( regs, RADEON_CRTC2_GEN_CNTL );
	
	// enable TV-Out output, but set DPMS mode 
	// (this seems to be not correct if TV-Out is connected to CRTC1,
	//  but it doesn't really hurt having wrong DPMS mode)
	OUTREG( regs, RADEON_CRTC2_GEN_CNTL, 
		RADEON_CRTC2_CRT2_ON | RADEON_CRTC2_VSYNC_TRISTAT );
	
	old_dac_ext_cntl = INREG( regs, RADEON_DAC_EXT_CNTL );
	
	// force constant voltage output of DAC for impedance test
	OUTREG( regs, RADEON_DAC_EXT_CNTL, 
		RADEON_DAC2_FORCE_BLANK_OFF_EN | RADEON_DAC2_FORCE_DATA_EN |
    	RADEON_DAC_FORCE_DATA_SEL_RGB |
    	(0xec << RADEON_DAC_FORCE_DATA_SHIFT ));

	old_tv_dac_cntl = INREG( regs, RADEON_TV_DAC_CNTL );

	// get TV-DAC running (or something...)	
	OUTREG( regs, RADEON_TV_DAC_CNTL, 
		RADEON_TV_DAC_CNTL_STD_NTSC |
		(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
		(6 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT ));
		
	(void)INREG( regs, RADEON_TV_DAC_CNTL );
	
	snooze( 4000 );
	
	OUTREG( regs, RADEON_TV_DAC_CNTL, 
		RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD |
		RADEON_TV_DAC_CNTL_DETECT |
		RADEON_TV_DAC_CNTL_STD_NTSC |
		(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
		(6 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT ));
		
	(void)INREG( regs, RADEON_TV_DAC_CNTL );
	
	snooze( 6000 );
	
	// now see which wires are connected
	tmp = INREG( regs, RADEON_TV_DAC_CNTL );
	if( (tmp & RADEON_TV_DAC_CNTL_GDACDET) != 0 ) {
		displays |= dd_stv;
		SHOW_INFO0( 2, "S-Video TV-Out is connected" );
	}
	
	if( (tmp & RADEON_TV_DAC_CNTL_BDACDET) != 0 ) {
		displays |= dd_ctv;
		SHOW_INFO0( 2, "Composite TV-Out is connected" );
	}

	// clean up the mess we did
	OUTREG( regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl );
	OUTREG( regs, RADEON_DAC_EXT_CNTL, old_dac_ext_cntl );
	OUTREG( regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl );
	OUTREG( regs, RADEON_DAC_CNTL2, old_dac_cntl2 );
	
	// again the magic wire
	// !if you uncomment this, TV-out gets disabled
	//OUTREGP( regs, RADEON_GPIOPAD_A, 1, ~1 );
	
	return displays;
}


// check whether there is a TV connected to TV-DAC
// returns bit set, i.e. there can be S-Video or composite or both
static display_device_e Radeon_DetectTV( accelerator_info *ai, bool tv_crt_found )
{
	switch( ai->si->asic ) {
	case rt_ve:
	case rt_m6:
	case rt_rv200:
	case rt_m7:
	case rt_rv250:
	case rt_rv280:
		return Radeon_DetectTV_RV200( ai, tv_crt_found );
		
	case rt_r300:
	case rt_r300_4p:
	case rt_rv350:
	case rt_rv360:
	case rt_r350:
	case rt_r360:
		return Radeon_DetectTV_R300( ai );
		
	default:
		// don't know about IGP
		;
	}
			
	return dd_none;
}

// read edid data of flat panel and setup its timing accordingly
static status_t Radeon_StoreFPEDID( accelerator_info *ai, edid1_info *edid )
{
	fp_info *fp = &ai->si->flatpanels[0];
	uint32 max_hsize, max_vsize;
	int i;

	SHOW_FLOW0( 2, "EDID data read from DVI port via DDC2:" );
	edid_dump( edid );

	// find detailed timing with maximum resolution
	max_hsize = max_vsize = 0;
		
	for( i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i ) {
		if( edid->detailed_monitor[i].monitor_desc_type == edid1_is_detailed_timing ) {
			edid1_detailed_timing *timing = &edid->detailed_monitor[i].data.detailed_timing;
			
			if( timing->h_size > max_hsize && timing->v_size > max_vsize ) {
				SHOW_FLOW( 2, "Found DDC data for mode %dx%d", 
					(int)timing->h_active, (int)timing->v_active );
					
				max_hsize = timing->h_active;
				max_vsize = timing->v_active;

				// copy it to timing specification			
				fp->panel_xres = timing->h_active;
				fp->h_blank = timing->h_blank;
				fp->h_over_plus = timing->h_sync_off;
				fp->h_sync_width = timing->h_sync_width;
				
				fp->panel_yres = timing->v_active;
				fp->v_blank = timing->v_blank;
				fp->v_over_plus = timing->v_sync_off;
				fp->v_sync_width = timing->v_sync_width;

				// BeOS uses kHz, but the timing is in 10 kHz
				fp->dot_clock = timing->pixel_clock * 10;
			}
		}
	}

	if( max_hsize == 0 )
		return B_ERROR;
			
	SHOW_INFO( 2, "h_disp=%d, h_blank=%d, h_over_plus=%d, h_sync_width=%d", 
		fp->panel_xres, fp->h_blank, fp->h_over_plus, fp->h_sync_width );
	SHOW_INFO( 2, "v_disp=%d, v_blank=%d, v_over_plus=%d, v_sync_width=%d", 
		fp->panel_yres, fp->v_blank, fp->v_over_plus, fp->v_sync_width );			
	SHOW_INFO( 2, "pixel_clock=%d kHz", fp->dot_clock );

	return B_OK;
}


// detect connected displays devices
// whished_num_heads - how many heads the requested display mode needs
void Radeon_DetectDisplays( accelerator_info *ai )
{
	shared_info *si = ai->si;
	display_device_e displays = 0;
	edid1_info edid;

	// mobile chips are for use in laptops - there must be a laptop panel	
	if( si->is_mobility )
		displays |= dd_lvds;

	// use DDC to detect monitors - if we can read DDC, there must be a monitor
	
	// all non-mobility versions have a DVI port
	if( (displays & dd_lvds) == 0 && 
		Radeon_ReadEDID( ai, RADEON_GPIO_DVI_DDC, &edid ))
	{
		SHOW_FLOW0( 2, "Found monitor on DVI DDC port" );
		// there may be an analog monitor connected to DVI-I;
		// we must check EDID to see whether it's really a digital monitor
		if( edid.display.input_type == 1 ) {
			SHOW_FLOW0( 2, "Must be a DVI monitor" );
			
			// store info about DVI-connected flat-panel
			if( Radeon_StoreFPEDID( ai, &edid ) == B_OK ) {
				displays |= dd_dvi;
			} else {
				SHOW_ERROR0( 2, "Disabled DVI - invalid EDID" );
			}
		} else {
			// must be the analog portion of DVI
			// I'm not sure about Radeons with one CRTC - do they have DVI-I or DVI-D?
			// anyway - if there are two CRTC, analog portion must be connected 
			// to TV-DAC, if there is one CRTC, it must be the normal VGA-DAC
			if( si->num_heads > 1 ) {
				SHOW_FLOW0( 2, "Must be an analog monitor on DVI port" );
				displays |= dd_tv_crt;
			} else {
				SHOW_FLOW0( 2, "Seems to be a CRT on VGA port!?" );
				displays |= dd_crt;
			}
		}
	}
	
	// all chips have a standard VGA port
	if( Radeon_ReadEDID( ai, RADEON_GPIO_VGA_DDC, &edid ))
		displays |= dd_crt;
		
	// we may have overseen monitors if they don't support DDC or 
	// have broken DDC data (like mine);
	// time to do a physical wire test; this test is more reliable, but it
	// leads to distortions on screen, which is not very nice to look at
	
	// for DVI, there is no mercy if no DDC data is there - we wouldn't
	// even know the native resolution of the panel!

	// all versions have a standard VGA port	
	if( (displays & dd_crt) == 0 &&
		Radeon_DetectCRT( ai ))
		displays |= dd_crt;
		
	// check VGA signal routed to DVI port
	// (the detection code checks whether there is hardware for that)
	if( (displays & dd_tv_crt) == 0 &&
		Radeon_DetectTVCRT( ai ))
		displays |= dd_tv_crt;

	// TV-Out doesn't work, so don't detect that	
#if 0
	// check TV-out connector
	// (this is the only one where we cannot use DDC)
	displays |= Radeon_DetectTV( ai, (displays & dd_tv_crt) != 0 );
#endif

	SHOW_INFO( 0, "Detected monitors: 0x%x", displays );
		
	// if no monitor found, we define to have a CRT connected to CRT-DAC
	if( displays == 0 )
		displays = dd_crt;
		
	si->connected_displays = displays;
}
