/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon accelerant
		
	CRTC <-> display routing
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "dac_regs.h"
#include "fp_regs.h"
#include "crtc_regs.h"
#include "tv_out_regs.h"


// read regs needed for display device routing
void Radeon_ReadMonitorRoutingRegs( accelerator_info *ai, physical_head *head, 
	port_regs *values )
{
	vuint8 *regs = ai->regs;
	
	(void)head;

	values->dac_cntl2 = INREG( regs, RADEON_DAC_CNTL2 );
	values->crtc_ext_cntl = INREG( regs, RADEON_CRTC_EXT_CNTL );
	values->disp_output_cntl = INREG( regs, RADEON_DISP_OUTPUT_CNTL );
	
	switch( ai->si->asic ) {
	case rt_r200:
	case rt_r300:
	case rt_r300_4p:
	case rt_rv350:
	case rt_rv360:
	case rt_r350:
	case rt_r360:
		break;
	
	case rt_ve:
	case rt_m6:
	case rt_rv200:	
	case rt_m7:
	case rt_rv250:
	case rt_rv280:
	case rt_m9:
	
	default:
		values->disp_hw_debug = INREG( regs, RADEON_DISP_HW_DEBUG );
	}
	
	if( ai->si->asic > rt_r100 ) {	
		// register introduced after R100
		values->tv_dac_cntl = INREG( regs, RADEON_TV_DAC_CNTL );
	}
	
	values->fp_gen_cntl = INREG( regs, RADEON_FP_GEN_CNTL );
	values->fp2_gen_cntl = INREG( regs, RADEON_FP2_GEN_CNTL );
}


// setup register contents to proper CRTC <-> display device mapping
void Radeon_CalcMonitorRouting( accelerator_info *ai, physical_head *head, 
	port_regs *values )
{
	display_device_e display_devices;
	
	display_devices = head->chosen_displays;
	
	// route VGA-DAC
	if( (display_devices & dd_crt) != 0 ) {
		// the CRT_ON flag seems to directly affect the CRT-DAC, _not_ the CRTC1 signal
		values->crtc_ext_cntl |= RADEON_CRTC_CRT_ON;

		switch( ai->si->asic ) {
		case rt_r200:
		case rt_r300:
		case rt_r300_4p:
		case rt_rv350:
		case rt_rv360:
		case rt_r350:
		case rt_r360:
			values->disp_output_cntl = 
				(values->disp_output_cntl & ~RADEON_DISP_DAC_SOURCE_MASK) |
				(head->is_crtc2 ? RADEON_DISP_DAC_SOURCE_CRTC2 : 0);
			break;
			
		case rt_ve:
		case rt_m6:
		case rt_rv200:	
		case rt_m7:
		case rt_rv250:
		case rt_rv280:
		case rt_m9:
		default:
			values->dac_cntl2 &= ~RADEON_DAC_CLK_SEL_MASK;
			values->dac_cntl2 |= head->is_crtc2 ? RADEON_DAC_CLK_SEL_CRTC2 : 0;
		}
	}
	
	// set CRT mode of TV-DAC if needed
	if( (display_devices & dd_tv_crt) != 0 ) {
		// TODO: this register doesn't exist on r200 as TV DAC is on 
		// external Rage Theatre
		values->dac_cntl2 &= ~RADEON_DAC2_CLK_SEL_MASK;
		values->dac_cntl2 |= RADEON_DAC2_CLK_SEL_CRT;

		// enable TV-DAC
		values->tv_dac_cntl = 
			RADEON_TV_DAC_CNTL_NBLANK |
			RADEON_TV_DAC_CNTL_NHOLD |
			RADEON_TV_DAC_CNTL_STD_PS2;
	}
	
	// set TV mode of TV-DAC if needed
	if( (display_devices & (dd_ctv | dd_stv)) != 0 ) {
		// see above
		values->dac_cntl2 &= ~RADEON_DAC2_CLK_SEL_MASK;
		values->dac_cntl2 |= RADEON_DAC2_CLK_SEL_TV;
	}	
	
	// choose CRTC for TV-DAC
	if( (display_devices & (dd_tv_crt | dd_ctv | dd_stv)) != 0 ) {
		switch( ai->si->asic ) {
		case rt_r200:
		case rt_r300:
		case rt_r300_4p:
		case rt_rv350:
		case rt_rv360:
		case rt_r350:
		case rt_r360:
			// for r200, this register doesn not exist!?
			// according to r300 spec, this is because TV-DAC is on external chip
			values->disp_output_cntl &= ~RADEON_DISP_TVDAC_SOURCE_MASK;
			values->disp_output_cntl |=
				head->is_crtc2 ? RADEON_DISP_TVDAC_SOURCE_CRTC2 : 0;
			break;
			
		case rt_ve:
		case rt_m6:
		case rt_rv200:	
		case rt_m7:
		case rt_rv250:
		case rt_rv280:
		case rt_m9:
		default:
			values->disp_hw_debug &= ~RADEON_CRT2_DISP1_SEL;
			values->disp_hw_debug |= head->is_crtc2 ? RADEON_CRT2_DISP1_SEL : 0;
		}
	}
	
	// choose CRTC for flat panel
	if( (display_devices & (dd_lvds | dd_dvi)) != 0 ) {
		values->fp_gen_cntl |= head->is_crtc2 ? RADEON_FP_SEL_CRTC2 : 0;					
	}

	// enable/disable RMX for crtc1
	// (TODO: this doesn't seem to work)
	// !!! makes trouble on Radeon 9200 Mobility !??
	/*
	if( !head->is_crtc2 ) {
		// use RMX if there is a flat panel
		if( (display_devices & (dd_lvds | dd_dvi)) != 0 ) {
			values->disp_output_cntl &= ~RADEON_DISP_DAC_SOURCE_MASK;
			values->disp_output_cntl |= RADEON_DISP_DAC_SOURCE_RMX;
		}
	}*/

	
	// choose CRTC for secondary flat panel
	if( (display_devices & dd_dvi_ext) != 0 ) {
		// TODO: this list looks a bit magic/wrong for me; I reckon ATI moved the
		// bit starting with ASIC xxx, but I have no specs to verify that
		switch( ai->si->asic ) {
		case rt_r200:
		case rt_r300:
		case rt_r350:
		case rt_rv350:	
		    values->fp2_gen_cntl |= 
		    	head->is_crtc2 ? RADEON_FP2_SOURCE_SEL_CRTC2 : 0;
		    break;
		default:
			values->fp2_gen_cntl |= 
				head->is_crtc2 ? RADEON_FP2_SRC_SEL_CRTC2 : 0;
		}
	}
	
	// we don't set source of TV-OUT unit - it's done in the tv-out code
}

void Radeon_ProgramMonitorRouting( accelerator_info *ai, physical_head *head, port_regs *values )
{
	vuint8 *regs = ai->regs;
	
	(void)head;
	
	OUTREG( regs, RADEON_DAC_CNTL2, values->dac_cntl2 );
	OUTREGP( regs, RADEON_CRTC_EXT_CNTL, values->crtc_ext_cntl,
		~RADEON_CRTC_CRT_ON );
	OUTREG( regs, RADEON_DISP_OUTPUT_CNTL, values->disp_output_cntl );

	switch( ai->si->asic ) {
	case rt_r200:
	case rt_r300:
	case rt_r300_4p:
	case rt_rv350:
	case rt_rv360:
	case rt_r350:
	case rt_r360:
		break;
		
	case rt_ve:
	case rt_m6:
	case rt_rv200:	
	case rt_m7:
	case rt_rv250:
	case rt_rv280:
	case rt_m9:
	default:	
		OUTREG( regs, RADEON_DISP_HW_DEBUG, values->disp_hw_debug );
	}

	if( ai->si->asic > rt_r100 ) {
		// register introduced after R100
		OUTREG( regs, RADEON_TV_DAC_CNTL, values->tv_dac_cntl );
	}
	
	OUTREG( regs, RADEON_FP_GEN_CNTL, values->fp_gen_cntl );
	OUTREG( regs, RADEON_FP2_GEN_CNTL, values->fp2_gen_cntl );
}


// Setup sensible default monitor routing
// whished_num_heads - number of independant heads current display mode would need
void Radeon_SetupDefaultMonitorRouting( accelerator_info *ai, int whished_num_heads )
{
	display_device_e crtc1_displays = 0, crtc2_displays = 0;
	display_device_e display_devices = ai->si->connected_displays;
	
	// flat panels get always connected to CRTC1 because its RMX unit
	if( (display_devices & dd_lvds) != 0 ) {
		// don't enable Laptop panel if display mode needs one head only 
		// and there is a CRT connected (showing the same on both panel and
		// CRT doesn't make much sense)
		if( !(whished_num_heads == 1 && (display_devices & (dd_crt | dd_tv_crt)) != 0 ))
			crtc1_displays |= dd_lvds;
	} else if( (display_devices & dd_dvi) != 0 )
		crtc1_displays |= dd_dvi;
		
	// TV-Out gets always connected to crtc2...
	if( (display_devices & dd_stv) != 0 )
		crtc2_displays |= dd_stv;
	else if( (display_devices & dd_stv) != 0 )
		crtc2_displays |= dd_ctv;
		
	// ...but if there is no crtc2, they win on crtc1;
	// if the user connects both a flat panel and a TV, he certainly wants to use the TV
	if( ai->si->num_heads == 1 && crtc2_displays != 0 )
		crtc1_displays = crtc2_displays;
		
	// if TV-Out is used, the DAC cannot drive a CRT at the same time
	if( (display_devices & (dd_stv | dd_ctv)) != 0 )
		display_devices &= ~dd_tv_crt;
		
	// CRT on CRT-DAC gets any spare CRTC;
	// if there is none, it can share CRTC with TV-Out
	if( (display_devices & dd_crt) != 0 ) {
		if( crtc1_displays == 0 )
			crtc1_displays |= dd_crt;
		else if( ai->si->num_heads > 1 && crtc2_displays == 0 )
			crtc2_displays |= dd_crt;
		else if( (crtc1_displays & ~(dd_stv | dd_ctv)) == 0 )
			crtc1_displays |= dd_crt;
		else if( ai->si->num_heads > 1 && (crtc2_displays & ~(dd_stv | dd_ctv)) == 0 )
			crtc2_displays |= dd_crt;
	}
	
	// same applies to CRT on TV-DAC;
	// if we cannot find a CRTC, we could clone the content of the CRT-DAC,
	// but I doubt that you really want two CRTs showing the same
	if( (display_devices & dd_tv_crt) != 0 &&
		(display_devices & (dd_ctv | dd_stv)) == 0 ) 
	{
		if( crtc1_displays == 0 )
			crtc1_displays |= dd_tv_crt;
		else if( ai->si->num_heads > 1 && crtc2_displays == 0 )
			crtc2_displays |= dd_tv_crt;
		else if( (crtc1_displays & ~(dd_stv | dd_ctv)) == 0 )
			crtc1_displays |= dd_tv_crt;
		else if( ai->si->num_heads > 1 && (crtc2_displays & ~(dd_stv | dd_ctv)) == 0 )
			crtc2_displays |= dd_tv_crt;
	}
	
	//crtc1_displays = dd_stv | dd_crt;
	//crtc2_displays = 0;
	
	SHOW_FLOW( 2, "CRTC1: 0x%x, CRTC2: 0x%x", crtc1_displays, crtc2_displays );
	
	ai->si->heads[0].chosen_displays = crtc1_displays;
	ai->si->heads[1].chosen_displays = crtc2_displays;
}
