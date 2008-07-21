/*
 * Copyright 2002-2004, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*!
	Radeon monitor detection
*/

#include <stdlib.h>
#include <string.h>

#include "radeon_accelerant.h"
#include "mmio.h"
#include "crtc_regs.h"
#include "dac_regs.h"
#include "pll_regs.h"
#include "tv_out_regs.h"
#include "config_regs.h"
#include "ddc_regs.h"
#include "gpiopad_regs.h"
#include "fp_regs.h"
#include "pll_access.h"
#include "theatre_regs.h"
#include "set_mode.h"
#include "ddc.h"


typedef struct {
	accelerator_info *ai;
	uint32 port;
} ddc_port_info;


//! Get I2C signals
static status_t
get_signals(void *cookie, int *clk, int *data)
{
	ddc_port_info *info = (ddc_port_info *)cookie;
	vuint8 *regs = info->ai->regs;
	uint32 value;

	value = INREG(regs, info->port);
	*clk = (value >> RADEON_GPIO_Y_SHIFT_1) & 1;
	*data = (value >> RADEON_GPIO_Y_SHIFT_0) & 1;

	return B_OK;
}


//! Set I2C signals
static status_t
set_signals(void *cookie, int clk, int data)
{
	ddc_port_info *info = (ddc_port_info *)cookie;
	vuint8 *regs = info->ai->regs;
	uint32 value;

	value = INREG(regs, info->port);
	value &= ~(RADEON_GPIO_A_1 | RADEON_GPIO_A_0);
	value &= ~(RADEON_GPIO_EN_0 | RADEON_GPIO_EN_1);		
	value |= ((1-clk) << RADEON_GPIO_EN_SHIFT_1)
		| ((1-data) << RADEON_GPIO_EN_SHIFT_0);

	OUTREG(regs, info->port, value);
	return B_OK;
}


/*! Read EDID information from monitor
	ddc_port - register to use for DDC2 communication
*/
bool
Radeon_ReadEDID(accelerator_info *ai, uint32 ddcPort, edid1_info *edid)
{
	i2c_bus bus;
	ddc_port_info info;
	void *vdif;
	size_t vdifLength;

	info.ai = ai;
	info.port = ddcPort;

	ddc2_init_timing(&bus);
	bus.cookie = &info;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;

	if (ddc2_read_edid1(&bus, edid, &vdif, &vdifLength) != B_OK)
		return false;

	SHOW_FLOW(2, "Found DDC-capable monitor @0x%04x", ddcPort);

	free(vdif);		
	return true;
}


// search for display connect to CRT DAC
// colour - true, if only a colour monitor is to be accepted
static bool
Radeon_DetectCRTInt(accelerator_info *ai, bool colour)
{
	vuint8 *regs = ai->regs;
	uint32 old_crtc_ext_cntl, old_dac_ext_cntl, old_dac_cntl, value;
	bool found;

	// makes sure there is a signal	
	old_crtc_ext_cntl = INREG(regs, RADEON_CRTC_EXT_CNTL);

	value = old_crtc_ext_cntl | RADEON_CRTC_CRT_ON;
	OUTREG(regs, RADEON_CRTC_EXT_CNTL, value);

	// force DAC to output constant voltage
	// for colour monitors, RGB is tested, for B/W only G	
	old_dac_ext_cntl = INREG(regs, RADEON_DAC_EXT_CNTL);

	value = RADEON_DAC_FORCE_BLANK_OFF_EN | RADEON_DAC_FORCE_DATA_EN
		| (colour ? RADEON_DAC_FORCE_DATA_SEL_RGB : RADEON_DAC_FORCE_DATA_SEL_G)
		| (0x1b6 << RADEON_DAC_FORCE_DATA_SHIFT);
	OUTREG(regs, RADEON_DAC_EXT_CNTL, value);

	// enable DAC and tell it to use VGA signals
	old_dac_cntl = INREG(regs, RADEON_DAC_CNTL);

	value = old_dac_cntl & ~(RADEON_DAC_RANGE_CNTL_MASK | RADEON_DAC_PDWN);
	value |= RADEON_DAC_RANGE_CNTL_PS2 | RADEON_DAC_CMP_EN;
	OUTREG(regs, RADEON_DAC_CNTL, value);

	// specs says that we should wait 1µs before checking but sample
	// code uses 2 ms; we use long delay to be on safe side
	// (though we don't want to make it too long as the monitor 
	// gets no sync signal now)	
	snooze(2000);

	// let's see whether there is some	
	found = (INREG(regs, RADEON_DAC_CNTL) & RADEON_DAC_CMP_OUTPUT) != 0;
	if (found) {
		SHOW_INFO(2, "Found %s CRT connected to CRT-DAC",
			colour ? "colour" : "b/w");
	}

	OUTREG(regs, RADEON_DAC_CNTL, old_dac_cntl);
	OUTREG(regs, RADEON_DAC_EXT_CNTL, old_dac_ext_cntl);
	OUTREG(regs, RADEON_CRTC_EXT_CNTL, old_crtc_ext_cntl);

	return found;
}


//! Check whethere there is a CRT connected to CRT DAC
static bool
Radeon_DetectCRT(accelerator_info *ai)
{
	vuint32 old_vclk_ecp_cntl, value;
	bool found;

	// enforce clock so the DAC gets activated	
	old_vclk_ecp_cntl = Radeon_INPLL(ai->regs, ai->si->asic,
		RADEON_VCLK_ECP_CNTL);

	value = old_vclk_ecp_cntl
		& ~(RADEON_PIXCLK_ALWAYS_ONb | RADEON_PIXCLK_DAC_ALWAYS_ONb);
	Radeon_OUTPLL(ai->regs, ai->si->asic, RADEON_VCLK_ECP_CNTL, value);

	// search first for colour, then for B/W monitor
	found = Radeon_DetectCRTInt(ai, true) || Radeon_DetectCRTInt(ai, false);

	Radeon_OUTPLL(ai->regs, ai->si->asic, RADEON_VCLK_ECP_CNTL,
		old_vclk_ecp_cntl);

	return found;
}


//! CRT on TV-DAC detection for rv200 and below checked for rv200
static bool
Radeon_DetectTVCRT_RV200(accelerator_info *ai)
{
	vuint8 *regs = ai->regs;
	uint32 old_crtc2_gen_cntl, old_tv_dac_cntl, old_dac_cntl2, value;
	bool found;

	// enable CRTC2, setting 8 bpp (we just pick any valid value)
	old_crtc2_gen_cntl = INREG(regs, RADEON_CRTC2_GEN_CNTL);

	value = old_crtc2_gen_cntl & ~RADEON_CRTC2_PIX_WIDTH_MASK;
	value |= RADEON_CRTC2_CRT2_ON | (2 << RADEON_CRTC2_PIX_WIDTH_SHIFT);
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, value);

	// enable TV-DAC, choosing VGA signal level
	old_tv_dac_cntl = INREG(regs, RADEON_TV_DAC_CNTL);

	value = RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD
		| RADEON_TV_DAC_CNTL_DETECT | RADEON_TV_DAC_CNTL_STD_PS2;
	OUTREG(regs, RADEON_TV_DAC_CNTL, value);

	// enforce constant DAC output voltage on RGB
	value = RADEON_DAC2_FORCE_BLANK_OFF_EN | RADEON_DAC2_FORCE_DATA_EN
		| RADEON_DAC_FORCE_DATA_SEL_RGB
		| (0x180 << RADEON_DAC_FORCE_DATA_SHIFT);
	OUTREG(regs, RADEON_DAC_EXT_CNTL, value);

	old_dac_cntl2 = INREG(regs, RADEON_DAC_CNTL2);

	// set DAC in CRT mode and enable detection
	// TODO: make sure we really use CRTC2 - this is ASIC dependant
	value = old_dac_cntl2 | RADEON_DAC2_CLK_SEL_CRT | RADEON_DAC2_CMP_EN;
	OUTREG(regs, RADEON_DAC_CNTL2, value);

	snooze(10000);

	// let's see what we've got!	
	found = (INREG(regs, RADEON_DAC_CNTL2) & RADEON_DAC2_CMP_OUTPUT) != 0;
	if (found)
		SHOW_INFO0(2, "Found CRT connected to TV-DAC, i.e. DVI port");

	OUTREG(regs, RADEON_DAC_CNTL2, old_dac_cntl2);
	OUTREG(regs, RADEON_DAC_EXT_CNTL, 0);
	OUTREG(regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl);
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl);

	return found;
}


//! CRT on TV-DAC detection for r300 checked for r300
static bool
Radeon_DetectTVCRT_R300(accelerator_info *ai)
{
	vuint8 *regs = ai->regs;
	uint32 old_crtc2_gen_cntl, old_tv_dac_cntl, old_dac_cntl2, value;
	uint32 old_radeon_gpiopad_a;
	bool found;

	old_radeon_gpiopad_a = INREG(regs, RADEON_GPIOPAD_A);

	// whatever these flags mean - let's pray they won't get changed
	OUTREGP(regs, RADEON_GPIOPAD_EN, 1, ~1);
	OUTREGP(regs, RADEON_GPIOPAD_MASK, 1, ~1);
	OUTREGP(regs, RADEON_GPIOPAD_A, 1, ~1);

	old_crtc2_gen_cntl = INREG(regs, RADEON_CRTC2_GEN_CNTL);

	// enable DAC, choose valid pixel format and enable DPMS
	// as usual, the code doesn't take into account whether the TV-DAC
	// does really use CRTC2	
	value = old_crtc2_gen_cntl;
	value &= ~RADEON_CRTC2_PIX_WIDTH_MASK;
	value |= (2 << RADEON_CRTC2_PIX_WIDTH_SHIFT) | RADEON_CRTC2_CRT2_ON
		| RADEON_CRTC2_VSYNC_TRISTAT;
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, value);

	old_tv_dac_cntl = INREG(regs, RADEON_TV_DAC_CNTL);

	// enable TV-DAC
	OUTREG(regs, RADEON_TV_DAC_CNTL, RADEON_TV_DAC_CNTL_NBLANK
		| RADEON_TV_DAC_CNTL_NHOLD | RADEON_TV_DAC_CNTL_DETECT
		| RADEON_TV_DAC_CNTL_STD_PS2);
		
	// force constant voltage output of DAC for impedance test
	OUTREG(regs, RADEON_DAC_EXT_CNTL, RADEON_DAC2_FORCE_BLANK_OFF_EN
		| RADEON_DAC2_FORCE_DATA_EN | RADEON_DAC_FORCE_DATA_SEL_RGB
		| (0x1b6 << RADEON_DAC_FORCE_DATA_SHIFT));

	old_dac_cntl2 = INREG(regs, RADEON_DAC_CNTL2);

	// enable CRT mode of TV-DAC and enable comparator    
	OUTREG(regs, RADEON_DAC_CNTL2, old_dac_cntl2 | RADEON_DAC2_CLK_SEL_CRT
		| RADEON_DAC2_CMP_EN);

	snooze(10000);

	// check connection of blue data signal to see whether there is a CRT
	found = (INREG(regs, RADEON_DAC_CNTL2) & RADEON_DAC2_CMP_OUT_B) != 0;

	// clean up the mess
	OUTREG(regs, RADEON_DAC_CNTL2, old_dac_cntl2);
	OUTREG(regs, RADEON_DAC_EXT_CNTL, 0);
	OUTREG(regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl);
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl);

	OUTREGP(regs, RADEON_GPIOPAD_A, old_radeon_gpiopad_a, ~1);

	return found;
}


//! Check whether there is a CRT connected to TV-DAC
static bool
Radeon_DetectTVCRT(accelerator_info *ai)
{
	if (ai->si->is_mobility)
		return dd_none;

	switch (ai->si->asic) {
		case rt_r100:
			// original Radeons have pure DVI only and mobility chips
			// have no DVI connector
			// TBD: can they have a docking station for CRT on TV-DAC?
			return dd_none;

		case rt_rv100:
		case rt_rv200:
		case rt_rv250:
		case rt_rv280:
		// IGP is guessed
		case rt_rs100:
		case rt_rs200:
		case rt_rs300:
			return Radeon_DetectTVCRT_RV200(ai);

		case rt_r300:
		case rt_r350:
		case rt_rv350:
		case rt_rv380:
		case rt_r420:
			return Radeon_DetectTVCRT_R300(ai);

		case rt_r200:
			// r200 has no built-in TV-out and thus no TV-DAC to use for
			// second CRT
			return dd_none;
	}

	return dd_none;
}


//! TV detection for rv200 and below should work for M6 and RV200
static display_device_e
Radeon_DetectTV_RV200(accelerator_info *ai, bool tv_crt_found)
{
	vuint8 *regs = ai->regs;
	uint32 value, old_dac_cntl2, old_crtc_ext_cntl, old_crtc2_gen_cntl;
	uint32 old_tv_master_cntl, old_tv_dac_cntl, old_pre_dac_mux_cntl;
	uint32 config_cntl;
	display_device_e displays = dd_none;

	// give up if there is a CRT connected to TV-DAC
	if (tv_crt_found)
		return dd_none;

	// enable TV mode
	old_dac_cntl2 = INREG(regs, RADEON_DAC_CNTL2);
	value = old_dac_cntl2 & ~RADEON_DAC2_CLK_SEL_CRT;
	OUTREG(regs, RADEON_DAC_CNTL2, value);

	old_crtc_ext_cntl = INREG(regs, RADEON_CRTC_EXT_CNTL);
	old_crtc2_gen_cntl = INREG(regs, RADEON_CRTC2_GEN_CNTL);
	old_tv_master_cntl = INREG(regs, RADEON_TV_MASTER_CNTL);

	// enable TV output
	value = old_tv_master_cntl | RADEON_TV_MASTER_CNTL_TV_ON;
	value &= ~(
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST | 
		RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX | 
		RADEON_TV_MASTER_CNTL_CRT_FIFO_CE_EN | 
		RADEON_TV_MASTER_CNTL_TV_FIFO_CE_EN | 
		RADEON_TV_MASTER_CNTL_RE_SYNC_NOW_SEL_MASK);
	value |= 
		RADEON_TV_MASTER_CNTL_TV_FIFO_ASYNC_RST | 
		RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST;
	OUTREG(regs, RADEON_TV_MASTER_CNTL, value);

	old_tv_dac_cntl = INREG(regs, RADEON_TV_DAC_CNTL);	
	
	config_cntl = INREG(regs, RADEON_CONFIG_CNTL);

	// unlock TV DAC	
	value = 
		RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD | 
		RADEON_TV_DAC_CNTL_DETECT | RADEON_TV_DAC_CNTL_STD_NTSC | 
		(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) | 
		((((config_cntl & RADEON_CFG_ATI_REV_ID_MASK) == 0) ? 8 : 4) << RADEON_TV_DAC_CNTL_DACADJ_SHIFT);
	OUTREG(regs, RADEON_TV_DAC_CNTL, value);
	
	old_pre_dac_mux_cntl = INREG(regs, RADEON_TV_PRE_DAC_MUX_CNTL);
	
	// force constant DAC output voltage
	value = 
		RADEON_TV_PRE_DAC_MUX_CNTL_C_GRN_EN | RADEON_TV_PRE_DAC_MUX_CNTL_CMP_BLU_EN | 
		(RADEON_TV_MUX_FORCE_DAC_DATA << RADEON_TV_PRE_DAC_MUX_CNTL_RED_MX_SHIFT) |
		(RADEON_TV_MUX_FORCE_DAC_DATA << RADEON_TV_PRE_DAC_MUX_CNTL_GRN_MX_SHIFT) |
		(RADEON_TV_MUX_FORCE_DAC_DATA << RADEON_TV_PRE_DAC_MUX_CNTL_BLU_MX_SHIFT) |
		(0x109 << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT);
	OUTREG(regs, RADEON_TV_PRE_DAC_MUX_CNTL, value);
	
	// let things settle a bit
	snooze(3000);

	// now see which wires are connected
	value = INREG(regs, RADEON_TV_DAC_CNTL);
	if ((value & RADEON_TV_DAC_CNTL_GDACDET) != 0) {
		displays |= dd_stv;
		SHOW_INFO0(2, "S-Video TV-Out is connected");
	}

	if ((value & RADEON_TV_DAC_CNTL_BDACDET) != 0) {
		displays |= dd_ctv;
		SHOW_INFO0(2, "Composite TV-Out is connected");
	}

	OUTREG(regs, RADEON_TV_PRE_DAC_MUX_CNTL, old_pre_dac_mux_cntl);
	OUTREG(regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl);
	OUTREG(regs, RADEON_TV_MASTER_CNTL, old_tv_master_cntl);
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl);
	OUTREG(regs, RADEON_CRTC_EXT_CNTL, old_crtc_ext_cntl);
	OUTREG(regs, RADEON_DAC_CNTL2, old_dac_cntl2);

	return displays;
}


// TV detection for r300 series
// should work for R300
static display_device_e Radeon_DetectTV_R300(accelerator_info *ai)
{
	vuint8 *regs = ai->regs;
	display_device_e displays = dd_none;
	uint32 tmp, old_dac_cntl2, old_crtc2_gen_cntl, old_dac_ext_cntl, old_tv_dac_cntl;
	uint32 old_radeon_gpiopad_a;
	
	old_radeon_gpiopad_a = INREG(regs, RADEON_GPIOPAD_A);

	// whatever these flags mean - let's pray they won't get changed
	OUTREGP(regs, RADEON_GPIOPAD_EN, 1, ~1);
	OUTREGP(regs, RADEON_GPIOPAD_MASK, 1, ~1);
	OUTREGP(regs, RADEON_GPIOPAD_A, 0, ~1);
	
	old_dac_cntl2 = INREG(regs, RADEON_DAC_CNTL2);
	
	// set CRT mode (!) of TV-DAC
	OUTREG(regs, RADEON_DAC_CNTL2, RADEON_DAC2_CLK_SEL_CRT);
	
	old_crtc2_gen_cntl = INREG(regs, RADEON_CRTC2_GEN_CNTL);
	
	// enable TV-Out output, but set DPMS mode 
	// (this seems to be not correct if TV-Out is connected to CRTC1,
	//  but it doesn't really hurt having wrong DPMS mode)
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, 
		RADEON_CRTC2_CRT2_ON | RADEON_CRTC2_VSYNC_TRISTAT);
	
	old_dac_ext_cntl = INREG(regs, RADEON_DAC_EXT_CNTL);
	
	// force constant voltage output of DAC for impedance test
	OUTREG(regs, RADEON_DAC_EXT_CNTL, 
		RADEON_DAC2_FORCE_BLANK_OFF_EN | RADEON_DAC2_FORCE_DATA_EN |
    	RADEON_DAC_FORCE_DATA_SEL_RGB |
    	(0xec << RADEON_DAC_FORCE_DATA_SHIFT));

	old_tv_dac_cntl = INREG(regs, RADEON_TV_DAC_CNTL);

	// get TV-DAC running (or something...)	
	OUTREG(regs, RADEON_TV_DAC_CNTL, 
		RADEON_TV_DAC_CNTL_STD_NTSC |
		(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
		(6 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT));
		
	(void)INREG(regs, RADEON_TV_DAC_CNTL);
	
	snooze(4000);
	
	OUTREG(regs, RADEON_TV_DAC_CNTL, 
		RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD |
		RADEON_TV_DAC_CNTL_DETECT |
		RADEON_TV_DAC_CNTL_STD_NTSC |
		(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
		(6 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT));
		
	(void)INREG(regs, RADEON_TV_DAC_CNTL);
	
	snooze(6000);
	
	// now see which wires are connected
	tmp = INREG(regs, RADEON_TV_DAC_CNTL);
	if ((tmp & RADEON_TV_DAC_CNTL_GDACDET) != 0) {
		displays |= dd_stv;
		SHOW_INFO0(2, "S-Video TV-Out is connected");
	}
	
	if ((tmp & RADEON_TV_DAC_CNTL_BDACDET) != 0) {
		displays |= dd_ctv;
		SHOW_INFO0(2, "Composite TV-Out is connected");
	}

	// clean up the mess we did
	OUTREG(regs, RADEON_TV_DAC_CNTL, old_tv_dac_cntl);
	OUTREG(regs, RADEON_DAC_EXT_CNTL, old_dac_ext_cntl);
	OUTREG(regs, RADEON_CRTC2_GEN_CNTL, old_crtc2_gen_cntl);
	OUTREG(regs, RADEON_DAC_CNTL2, old_dac_cntl2);
	
	OUTREGP(regs, RADEON_GPIOPAD_A, old_radeon_gpiopad_a, ~1);
	
	return displays;
}


// save readout of TV detection comparators
static bool readTVDetect(accelerator_info *ai)
{
	uint32 tmp;
	int i;
	bigtime_t start_time;
	bool detect;

	// make output constant	
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
		RADEON_TV_DAC_CNTL_STD_NTSC | RADEON_TV_DAC_CNTL_DETECT | RADEON_TV_DAC_CNTL_NBLANK);
		
	// check detection result
	Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL, &tmp);
	detect = (tmp & RADEON_TV_DAC_CNTL_CMPOUT) != 0;
	
	//SHOW_FLOW(2, "detect=%d", detect);

	start_time = system_time();
	
	do {
		// wait for stable detect signal
		for (i = 0; i < 5; ++i) {
			bool cur_detect;
			
			Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
				RADEON_TV_DAC_CNTL_STD_NTSC | RADEON_TV_DAC_CNTL_DETECT | RADEON_TV_DAC_CNTL_NBLANK |
				RADEON_TV_DAC_CNTL_NHOLD);
			Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
				RADEON_TV_DAC_CNTL_STD_NTSC | RADEON_TV_DAC_CNTL_DETECT | RADEON_TV_DAC_CNTL_NBLANK);
				
			Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL, &tmp);
			cur_detect = (tmp & RADEON_TV_DAC_CNTL_CMPOUT) != 0;
			
			//SHOW_FLOW(2, "cur_detect=%d", cur_detect);
			
			if (cur_detect != detect)
				break;
				
			detect = cur_detect;
		}
		
		if (i == 5) {
			//SHOW_FLOW(2, "return %d", detect);
			return detect;
		}
	
		// don't wait forever - give up after 1 second		
	} while (system_time() - start_time < 1000000);

	SHOW_FLOW0(2, "timeout");
	return false;
}


//! Detect TV connected to external Theatre-Out
static display_device_e
Radeon_DetectTV_Theatre(accelerator_info *ai)
{
	uint32 old_tv_dac_cntl, old_pre_dac_mux_cntl, old_modulator_cntl1;
	uint32 old_master_cntl;
	uint32 uv_adr, old_last_fifo_entry, old_mid_fifo_entry, last_fifo_addr;
	display_device_e displays = dd_none;

	if (ai->si->tv_chip != tc_external_rt1)
		return dd_none;

	// save previous values (TV-Out may be running)	
	Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
		&old_tv_dac_cntl);

	// enable DAC and comparators
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
		RADEON_TV_DAC_CNTL_STD_NTSC | RADEON_TV_DAC_CNTL_DETECT
		| RADEON_TV_DAC_CNTL_NHOLD | RADEON_TV_DAC_CNTL_NBLANK);

	Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_PRE_DAC_MUX_CNTL,
		&old_pre_dac_mux_cntl);			
	Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_MODULATOR_CNTL1,
		&old_modulator_cntl1);		
	Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_MASTER_CNTL,
		&old_master_cntl);

	// save output timing
	Radeon_VIPRead(ai, ai->si->theatre_channel, THEATRE_VIP_UV_ADR, &uv_adr);

	last_fifo_addr = (uv_adr & RADEON_TV_UV_ADR_MAX_UV_ADR_MASK) * 2 + 1;

	old_last_fifo_entry = Radeon_TheatreReadFIFO(ai, last_fifo_addr);
	old_mid_fifo_entry = Radeon_TheatreReadFIFO(ai, 0x18f);

	Radeon_TheatreWriteFIFO(ai, last_fifo_addr, 0x20208);
	Radeon_TheatreWriteFIFO(ai, 0x18f, 0x3ff2608);

	// stop TV-Out to savely program it
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_MASTER_CNTL, 
		RADEON_TV_MASTER_CNTL_TV_FIFO_ASYNC_RST
		| RADEON_TV_MASTER_CNTL_TV_ASYNC_RST);

	// set constant base level
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_MODULATOR_CNTL1,
		(0x2c << RADEON_TV_MODULATOR_CNTL1_SET_UP_LEVEL_SHIFT)
		| (0x2c << RADEON_TV_MODULATOR_CNTL1_BLANK_LEVEL_SHIFT));

	// enable output
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_MASTER_CNTL,
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST);

	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_MASTER_CNTL, 0);

	// set constant Composite output
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_PRE_DAC_MUX_CNTL,
		RADEON_TV_PRE_DAC_MUX_CNTL_CMP_BLU_EN
		| RADEON_TV_PRE_DAC_MUX_CNTL_DAC_DITHER_EN
		| (9 << RADEON_TV_PRE_DAC_MUX_CNTL_BLU_MX_SHIFT)
		| (0xa8 << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT));

	// check for S-Video connection
	if (readTVDetect(ai)) {
		SHOW_FLOW0(2, "Composite-Out of Rage Theatre is connected");
		displays |= dd_ctv;
	}

	// enable output changes	
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
		RADEON_TV_DAC_CNTL_STD_NTSC | RADEON_TV_DAC_CNTL_DETECT
		| RADEON_TV_DAC_CNTL_NBLANK | RADEON_TV_DAC_CNTL_NHOLD);

	// set constant Y-output of S-Video adapter
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_PRE_DAC_MUX_CNTL,
		RADEON_TV_PRE_DAC_MUX_CNTL_Y_RED_EN
		| RADEON_TV_PRE_DAC_MUX_CNTL_DAC_DITHER_EN
		| (9 << RADEON_TV_PRE_DAC_MUX_CNTL_RED_MX_SHIFT)
		| (0xa8 << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT));

	// check for composite connection
	if (readTVDetect(ai)) {
		SHOW_FLOW0(2, "S-Video-Out of Rage Theatre is connected");
		displays |= dd_stv;
	}

	// restore everything
	Radeon_TheatreWriteFIFO(ai, last_fifo_addr, old_last_fifo_entry);
	Radeon_TheatreWriteFIFO(ai, 0x18f, old_mid_fifo_entry);

	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_MASTER_CNTL,
		old_master_cntl);
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_MODULATOR_CNTL1,
		old_modulator_cntl1);
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_PRE_DAC_MUX_CNTL,
		old_pre_dac_mux_cntl);
	Radeon_VIPWrite(ai, ai->si->theatre_channel, THEATRE_VIP_TV_DAC_CNTL,
		old_tv_dac_cntl);

	return displays;
}


/*!
	Check whether there is a TV connected to TV-DAC
	returns bit set, i.e. there can be S-Video or composite or both
*/
static display_device_e
Radeon_DetectTV(accelerator_info *ai, bool tv_crt_found)
{
	switch (ai->si->asic) {
		case rt_r100:
		case rt_r200:
			return Radeon_DetectTV_Theatre(ai);

		case rt_rv100:
		case rt_rv200:
		case rt_rv250:
		case rt_rv280:
		// IGP method is guessed
		case rt_rs100:
		case rt_rs200:
		case rt_rs300:
			return Radeon_DetectTV_RV200(ai, tv_crt_found);

		case rt_r300:
		case rt_r350:
		case rt_rv350:
		case rt_rv380:
		case rt_r420:
			return Radeon_DetectTV_R300(ai);
	}

	return dd_none;
}


//! Get native monitor timing, using Detailed Monitor Description
static void
Radeon_FindFPTiming_DetailedMonitorDesc(const edid1_info *edid, fp_info *fp,
	uint32 *max_hsize, uint32 *max_vsize)
{
	int i;

	for (i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
		if (edid->detailed_monitor[i].monitor_desc_type == EDID1_IS_DETAILED_TIMING) {
			const edid1_detailed_timing *timing = &edid->detailed_monitor[i].data.detailed_timing;

			SHOW_FLOW(2, "Found detailed timing for mode %dx%d in DDC data", 
				(int)timing->h_active, (int)timing->v_active);

			if (timing->h_active > *max_hsize && timing->v_active > *max_vsize) {
				*max_hsize = timing->h_active;
				*max_vsize = timing->v_active;

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
}


/*!	Get native monitor timing, using Standard Timing table;
	this table doesn't contain the actual timing, so we try to find a
	appropriate VESA modes for the resolutions given in the table
*/
static void
Radeon_FindFPTiming_StandardTiming(const edid1_info *edid, fp_info *fp,
	uint32 *max_hsize, uint32 *max_vsize)
{
	int i;

	for (i = 0; i < EDID1_NUM_STD_TIMING; ++i) {
		const edid1_std_timing *std_timing = &edid->std_timing[i];
		int best_fit = -1;
		int best_refresh_deviation = 10000;
		int j;

		if (std_timing->h_size <= 256)
			continue;

		for (j = 0; j < (int)vesa_mode_list_count; ++j) {
			int refresh_rate, cur_refresh_deviation;

			if (vesa_mode_list[j].h_display != std_timing->h_size
				|| vesa_mode_list[j].v_display != std_timing->v_size)
				continue;

			// take pixel_clock times 1000 because is is in kHz
			// further, take it times 1000 again, to get 1/1000 frames 
			// as refresh rate
			refresh_rate = (int64)vesa_mode_list[j].pixel_clock * 1000*1000
				/ (vesa_mode_list[j].h_total * vesa_mode_list[j].v_total);

			// standard timing is in frames, so multiple by it to get 1/1000 frames
			// result is scaled by 100 to get difference in percentage;
			cur_refresh_deviation = (100 * (refresh_rate - std_timing->refresh
				* 1000)) / refresh_rate;

			if (cur_refresh_deviation < 0)
				cur_refresh_deviation = -cur_refresh_deviation;

			// less then 1 percent difference is (hopefully) OK, 
			// if there are multiple, we take best one
			// (if the screen is that picky, it should have defined an enhanced timing)
			if (cur_refresh_deviation < 1
				&& cur_refresh_deviation < best_refresh_deviation) {
				best_fit = j;
				best_refresh_deviation = cur_refresh_deviation;
			}
		}

		if (best_fit < 0) {
			SHOW_FLOW(2, "Unsupported standard mode %dx%d@%dHz (not VESA)", 
				std_timing->h_size, std_timing->v_size, std_timing->refresh);
			continue;
		}

		if (std_timing->h_size > *max_hsize && std_timing->h_size > *max_vsize) {
			const display_timing *timing = &vesa_mode_list[best_fit];

			SHOW_FLOW(2, "Found DDC data for standard mode %dx%d", 
				(int)timing->h_display, (int)timing->v_display);

			*max_hsize = timing->h_display;
			*max_vsize = timing->h_display;

			// copy it to timing specification
			fp->panel_xres = timing->h_display;
			fp->h_blank = timing->h_total - timing->h_display;
			fp->h_over_plus = timing->h_sync_start - timing->h_display;
			fp->h_sync_width = timing->h_sync_end - timing->h_sync_start;

			fp->panel_yres = timing->v_display;
			fp->v_blank = timing->v_total - timing->v_display;
			fp->v_over_plus = timing->v_sync_start - timing->v_display;
			fp->v_sync_width = timing->v_sync_end - timing->v_sync_start;

			fp->dot_clock = timing->pixel_clock;
		}
	}
}
	

//! Read edid data of flat panel and setup its timing accordingly
static status_t
Radeon_StoreFPEDID(accelerator_info *ai, int port, const edid1_info *edid)
{
	fp_info *fp = &ai->si->flatpanels[port];
	uint32 max_hsize, max_vsize;

	//SHOW_FLOW0(2, "EDID data read from DVI port via DDC2:");
	//edid_dump(edid);

	// find detailed timing with maximum resolution
	max_hsize = max_vsize = 0;
	Radeon_FindFPTiming_DetailedMonitorDesc(edid, fp, &max_hsize, &max_vsize);

	if (max_hsize == 0) {
		SHOW_FLOW0(2, "Timing is not explicitely defined in DDC - checking standard modes");

		Radeon_FindFPTiming_StandardTiming(edid, fp, &max_hsize, &max_vsize);
		if (max_hsize == 0) {
			SHOW_FLOW0(2, "Still found no valid native mode, disabling DVI");
			return B_ERROR;
		}
	}

	SHOW_INFO(2, "h_disp=%d, h_blank=%d, h_over_plus=%d, h_sync_width=%d", 
		fp->panel_xres, fp->h_blank, fp->h_over_plus, fp->h_sync_width);
	SHOW_INFO(2, "v_disp=%d, v_blank=%d, v_over_plus=%d, v_sync_width=%d", 
		fp->panel_yres, fp->v_blank, fp->v_over_plus, fp->v_sync_width);			
	SHOW_INFO(2, "pixel_clock=%d kHz", fp->dot_clock);

	return B_OK;
}


static void
Radeon_ConnectorInfo(accelerator_info *ai, int port, disp_entity* ptr_entity)
{
	const char* mon;
	const char* ddc = ptr_entity->port_info[port].ddc_type == ddc_none_detected
		? "None" : ptr_entity->port_info[port].ddc_type == ddc_monid
		? "Mon ID" : ptr_entity->port_info[port].ddc_type == ddc_dvi
		? "DVI DDC" : ptr_entity->port_info[port].ddc_type == ddc_vga
		? "VGA DDC" : ptr_entity->port_info[port].ddc_type == ddc_crt2
		? "CRT2 DDC" : "Error";					
	const char* tmds = ptr_entity->port_info[port].tmds_type == tmds_unknown
		? "None" : ptr_entity->port_info[port].tmds_type == tmds_int
		? "Internal" : ptr_entity->port_info[port].tmds_type == tmds_ext
		? "External" : "??? ";
	const char* dac = ptr_entity->port_info[port].dac_type == dac_unknown
		? "Unknown" : ptr_entity->port_info[port].dac_type == dac_primary
		? "Primary" : ptr_entity->port_info[port].dac_type == dac_tvdac
		? "TV / External" : "Error";
	const char* con;

	if (ai->si->is_atombios) {
		con = ptr_entity->port_info[port].connector_type == connector_none_atom
			? "None" : ptr_entity->port_info[port].connector_type == connector_vga_atom
			? "VGA" : ptr_entity->port_info[port].connector_type == connector_dvi_i_atom
			? "DVI-I" : ptr_entity->port_info[port].connector_type == connector_dvi_d_atom
			? "DVI-D" : ptr_entity->port_info[port].connector_type == connector_dvi_a_atom
			? "DVI-A" : ptr_entity->port_info[port].connector_type == connector_stv_atom
			? "S-Video TV" : ptr_entity->port_info[port].connector_type == connector_ctv_atom
			? "Composite TV" : ptr_entity->port_info[port].connector_type == connector_lvds_atom
			? "LVDS" : ptr_entity->port_info[port].connector_type == connector_digital_atom
			? "Digital" : ptr_entity->port_info[port].connector_type == connector_unsupported_atom
			? "N/A  " : "Err  ";
	} else {
		con = ptr_entity->port_info[port].connector_type == connector_none
			? "None" : ptr_entity->port_info[port].connector_type == connector_crt
			? "VGA" : ptr_entity->port_info[port].connector_type == connector_dvi_i
			? "DVI-I" : ptr_entity->port_info[port].connector_type == connector_dvi_d
			? "DVI-D" : ptr_entity->port_info[port].connector_type == connector_proprietary
			? "Proprietary" : ptr_entity->port_info[port].connector_type == connector_stv
			? "S-Video TV" : ptr_entity->port_info[port].connector_type == connector_ctv
			? "Composite TV" : ptr_entity->port_info[port].connector_type == connector_unsupported
			? "N/A" : "Err";
	}

	mon = ptr_entity->port_info[port].mon_type == mt_unknown ? "???"
		: ptr_entity->port_info[port].mon_type == mt_none ? "None"
		: ptr_entity->port_info[port].mon_type == mt_crt ? "CRT "
		: ptr_entity->port_info[port].mon_type == mt_lcd ? "LCD "
		: ptr_entity->port_info[port].mon_type == mt_dfp ? "DVI "
		: ptr_entity->port_info[port].mon_type == mt_ctv ? "Composite TV"
		: ptr_entity->port_info[port].mon_type == mt_stv ? "S-Video TV"
		: "Err ?";

	SHOW_INFO(2, "Port %d:- \nMonitor:    %s\nConn Type:  %s\nDDC Port:   %s\nTMDS Type:  %s\nDAC Type:   %s",
		port, mon, con, ddc, tmds, dac);
}


/*!
	Detect connected displays devices
	whished_num_heads - how many heads the requested display mode needs
*/
void
Radeon_DetectDisplays(accelerator_info *ai)
{
	shared_info *si = ai->si;

	disp_entity* routes = &si->routing;
	display_device_e displays = 0;
	display_device_e controlled_displays = ai->vc->controlled_displays;
	int i;

	uint32 edid_regs[] = {
		0, 
		RADEON_GPIO_MONID, 
		RADEON_GPIO_DVI_DDC,
		RADEON_GPIO_VGA_DDC,
		RADEON_GPIO_CRT2_DDC
	};

	// lock hardware so noone bothers us
	Radeon_WaitForIdle(ai, true);

	// alwats make TMDS_INT port first
	if (routes->port_info[1].tmds_type == tmds_int) {
		radeon_connector swap_entity;
		swap_entity = routes->port_info[0];
		routes->port_info[0] = routes->port_info[1];
		routes->port_info[1] = swap_entity;
		SHOW_FLOW0(2, "Swapping TMDS_INT to first port");
	} else if (routes->port_info[0].tmds_type != tmds_int
		&& routes->port_info[1].tmds_type != tmds_int) {
		// no TMDS_INT port, make primary DAC port first
		// On my Inspiron 8600 both internal and external ports are
		// marked DAC_PRIMARY in BIOS. So be extra careful - only
		// swap when the first port is not DAC_PRIMARY
		if (routes->port_info[1].dac_type == dac_primary
			&& routes->port_info[0].dac_type != dac_primary) {	
			radeon_connector swap_entity;
			swap_entity = routes->port_info[0];
			routes->port_info[0] = routes->port_info[1];
			routes->port_info[1] = swap_entity;
			SHOW_FLOW0(2, "Swapping Primary Dac to front");
		}
	}

	if (si->asic == rt_rs300) {
		// RS300 only has single Dac of TV type  
		// For RS300/RS350/RS400 chips, there is no primary DAC.
		// Force VGA port to use TVDAC
		if (routes->port_info[0].connector_type == connector_crt) {
			routes->port_info[0].dac_type = dac_tvdac;
			routes->port_info[1].dac_type = dac_primary;
		} else {
			routes->port_info[1].dac_type = dac_primary;
			routes->port_info[0].dac_type = dac_tvdac;
		}
	} else if (si->num_crtc == 1) {
		routes->port_info[0].dac_type = dac_primary;	
	}

	// use DDC to detect monitors - if we can read DDC, there must be
	// a monitor
	for (i = 0; i < 2; i++) {
		//TODO could skip edid reading instead if we already have it, but what
		//if monitors have been hot swapped?  Also rely on edid for DVI-D detection
		//if (routes->port_info[i].mon_type != mt_unknown) {
		//	SHOW_FLOW0(2, "known type, skpping detection");	
		//	continue;
		//}
		memset(&routes->port_info[i].edid , 0, sizeof(edid1_info));
		switch (routes->port_info[i].ddc_type) {
			case ddc_monid:
			case ddc_dvi:
			case ddc_vga:
			case ddc_crt2:
				if (Radeon_ReadEDID(ai,
						edid_regs[routes->port_info[i].ddc_type],
						&routes->port_info[i].edid)) {
					routes->port_info[i].edid_valid = true;
					SHOW_FLOW(2, "Edid Data for CRTC %d on line %d", i, routes->port_info[i].ddc_type);
					edid_dump(&routes->port_info[i].edid);
				} else {
					routes->port_info[i].mon_type = mt_none;
				}
				break;

			default:
				SHOW_FLOW(2, "No Edid Pin Assigned to CRTC %d ", i);
				routes->port_info[i].mon_type = mt_none;
		}

		if (routes->port_info[i].edid_valid) {
			if (routes->port_info[i].edid.display.input_type == 1) {
				SHOW_FLOW0(2, "Must be a DVI monitor");

				// Note some laptops have a DVI output that uses internal TMDS,
				// when its DVI is enabled by hotkey, LVDS panel is not used.
				// In this case, the laptop is configured as DVI+VGA as a normal 
				// desktop card.
				// Also for laptop, when X starts with lid closed (no DVI connection)
				// both LDVS and TMDS are disable, we still need to treat it as a LVDS panel.
				if (routes->port_info[i].tmds_type == tmds_ext){
					// store info about DVI-connected flat-panel
					if (Radeon_StoreFPEDID(ai, i, &routes->port_info[i].edid) == B_OK) {
						SHOW_INFO0(2, "Found Ext Laptop DVI");
						routes->port_info[i].mon_type = mt_dfp;
						ai->si->flatpanels[i].is_fp2 = true;
						displays |= dd_dvi_ext;
					} else {
						SHOW_ERROR0(2, "Disabled Ext DVI - invalid EDID");
					}
				} else {
					if (INREG(ai->regs, RADEON_FP_GEN_CNTL) & (1 << 7) || (!si->is_mobility)) {
						// store info about DVI-connected flat-panel
						if (Radeon_StoreFPEDID(ai, i, &routes->port_info[i].edid) == B_OK) {
							SHOW_INFO0(2, "Found DVI");
							routes->port_info[i].mon_type = mt_dfp;
							displays |= dd_dvi;
						} else {
							SHOW_ERROR0(2, "Disabled DVI - invalid EDID");
						}		
					} else {
						SHOW_INFO0(2, "Laptop Panel Found");
						routes->port_info[i].mon_type = mt_lcd;
						displays |= dd_lvds;
					}
					ai->si->flatpanels[1].is_fp2 = FALSE;
				}
			} else {
				// must be the analog portion of DVI
				// I'm not sure about Radeons with one CRTC - do they have DVI-I or DVI-D?
				// anyway - if there are two CRTC, analog portion must be connected 
				// to TV-DAC; if there is one CRTC, it must be the normal VGA-DAC
				if (si->num_crtc > 1) {
					SHOW_FLOW0(2, "Must be an analog monitor on DVI port");
					routes->port_info[i].mon_type = mt_crt;
					displays |= dd_tv_crt;
				} else {
					SHOW_FLOW0(2, "Seems to be a CRT on VGA port!?");
					routes->port_info[i].mon_type = mt_crt;
					displays |= dd_crt;
				}
			}
		}
	}

	if (!routes->port_info[0].edid_valid) {
		SHOW_INFO0(2, "Searching port 0");
		if (si->is_mobility && (INREG(ai->regs, RADEON_BIOS_4_SCRATCH) & 4)) {
			SHOW_INFO0(2, "Found Laptop Panel");
			routes->port_info[0].mon_type = mt_lcd;
			displays |= dd_lvds;
		}
	}

	if (!routes->port_info[1].edid_valid) {
		if (si->is_mobility && (INREG(ai->regs, RADEON_FP2_GEN_CNTL) & RADEON_FP2_FPON)) {
			SHOW_INFO0(2, "Found Ext Laptop DVI");
			routes->port_info[1].mon_type = mt_dfp;
			displays |= dd_dvi_ext;
		}
	}

	if (routes->port_info[0].mon_type == mt_none) {
		if (routes->port_info[1].mon_type == mt_none) {
			routes->port_info[0].mon_type = mt_crt;
		} else {
			radeon_connector portSwapEntity;
			fp_info panelInfoSwapEntity;

			portSwapEntity = routes->port_info[0];
			routes->port_info[0] = routes->port_info[1];
			routes->port_info[1] = portSwapEntity;

			panelInfoSwapEntity = ai->si->flatpanels[0];
			ai->si->flatpanels[0] = ai->si->flatpanels[1];
			ai->si->flatpanels[1] = panelInfoSwapEntity;

			SHOW_ERROR0(2, "swapping active port 2 to free port 1");
		}
			
	}

	routes->reversed_DAC = false;
	if (routes->port_info[1].dac_type == dac_tvdac) {
		SHOW_ERROR0(2, "Reversed dac detected (not impl. yet)");
		routes->reversed_DAC = true;
	}

	// we may have overseen monitors if they don't support DDC or 
	// have broken DDC data (like mine);
	// time to do a physical wire test; this test is more reliable, but it
	// leads to distortions on screen, which is not very nice to look at

	// for DVI, there is no mercy if no DDC data is there - we wouldn't
	// even know the native resolution of the panel!

	// all versions have a standard VGA port	
	if ((displays & dd_crt) == 0 && (controlled_displays & dd_crt) != 0
		&& Radeon_DetectCRT(ai))
		displays |= dd_crt;

	// check VGA signal routed to DVI port
	// (the detection code checks whether there is hardware for that)
	if ((displays & dd_tv_crt) == 0
		&& (controlled_displays & dd_tv_crt) != 0
		&& Radeon_DetectTVCRT(ai))
		displays |= dd_tv_crt;

	// check TV-out connector
	if ((controlled_displays && (dd_ctv | dd_stv)) != 0)
		displays |= Radeon_DetectTV(ai, (displays & dd_tv_crt) != 0);

	SHOW_INFO(0, "Detected monitors: 0x%x", displays);

	displays &= controlled_displays;

	// if no monitor found, we define to have a CRT connected to CRT-DAC
	if (displays == 0)
		displays = dd_crt;

	Radeon_ConnectorInfo(ai, 0, routes);
	Radeon_ConnectorInfo(ai, 1, routes);	

	ai->vc->connected_displays = displays;

	RELEASE_BEN(si->cp.lock);
}
