/*
	Copyright (c) 2002-04, Thomas Kurschel
	

	Part of Radeon accelerant
		
	CRTC <-> display routing
	
	This stuff is highly ASIC dependant and is probably the most ASIC-specific
	code of the entire project.
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "dac_regs.h"
#include "fp_regs.h"
#include "crtc_regs.h"
#include "tv_out_regs.h"
#include "pll_regs.h"
#include "gpiopad_regs.h"
#include "pll_access.h"
#include "set_mode.h"


// read regs needed for display device routing
void Radeon_ReadMonitorRoutingRegs( 
	accelerator_info *ai, routing_regs *values )
{
	vuint8 *regs = ai->regs;
	
	values->dac_cntl = INREG( regs, RADEON_DAC_CNTL );
	values->dac_cntl2 = INREG( regs, RADEON_DAC_CNTL2 );
	values->crtc_ext_cntl = INREG( regs, RADEON_CRTC_EXT_CNTL );
	values->crtc2_gen_cntl = INREG( regs, RADEON_CRTC2_GEN_CNTL );
	values->disp_output_cntl = INREG( regs, RADEON_DISP_OUTPUT_CNTL );
	values->pixclks_cntl = Radeon_INPLL( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL );
	values->vclk_ecp_cntl = Radeon_INPLL( ai->regs, ai->si->asic, RADEON_VCLK_ECP_CNTL );
	
	switch( ai->si->asic ) {
	case rt_rv100:
	case rt_rv200:	
	case rt_rv250:
	case rt_rv280:
	case rt_rs100:
	case rt_rs200:
	case rt_rs300:
		values->disp_hw_debug = INREG( regs, RADEON_DISP_HW_DEBUG );
		break;

	case rt_r200:
		values->disp_tv_out_cntl = INREG( regs, RADEON_DISP_TV_OUT_CNTL );
		break;

	case rt_r300:
	case rt_rv350:
	case rt_r350:
	case rt_rv380:
	case rt_r420:
		values->gpiopad_a = INREG( regs, RADEON_GPIOPAD_A );
		break;
		
	case rt_r100:
		break;
	}
	
	if( ai->si->asic > rt_r100 ) {	
		// register introduced after R100
		values->tv_dac_cntl = INREG( regs, RADEON_TV_DAC_CNTL );
	}
	
	if( IS_INTERNAL_TV_OUT( ai->si->tv_chip ))
		values->tv_master_cntl = INREG( regs, RADEON_TV_MASTER_CNTL );
	
	values->fp_gen_cntl = INREG( regs, RADEON_FP_GEN_CNTL );
	values->fp2_gen_cntl = INREG( regs, RADEON_FP2_GEN_CNTL );
}


// setup register contents to proper CRTC <-> display device mapping
void Radeon_CalcMonitorRouting( 
	accelerator_info *ai, const impactv_params *tv_parameters, routing_regs *values )
{
	display_device_e display_devices[2], total_devices, controlled_devices;
	
	if( ai->vc->used_crtc[0] )
		display_devices[0] = ai->si->crtc[0].chosen_displays;
	else
		display_devices[0] = dd_none;
		
	if( ai->vc->used_crtc[1] )
		display_devices[1] = ai->si->crtc[1].chosen_displays;
	else
		display_devices[1] = dd_none;
		
	total_devices = display_devices[0] | display_devices[1];
	controlled_devices = ai->vc->controlled_displays;

	// enable 8 bit DAC 
	// (could be moved to boot initialization)
	values->dac_cntl |= 
		RADEON_DAC_MASK_ALL | RADEON_DAC_VGA_ADR_EN | RADEON_DAC_8BIT_EN;
		
	// enable frame buffer access and extended CRTC counter
	// (again: something for boot init.)
	values->crtc_ext_cntl =
		RADEON_VGA_ATI_LINEAR | RADEON_XCRT_CNT_EN;
		
	// set VGA signal style (not sure whether this affects 
	// CRTC1 or CRT-DAC, so we better always set it)
	values->dac_cntl &= ~(RADEON_DAC_RANGE_CNTL_MASK | RADEON_DAC_BLANKING);
	values->dac_cntl |= RADEON_DAC_RANGE_CNTL_PS2;

	// disable all the magic CRTC shadowing
	values->fp_gen_cntl &= 
		~(RADEON_FP_RMX_HVSYNC_CONTROL_EN |
		  RADEON_FP_DFP_SYNC_SEL |	
		  RADEON_FP_CRT_SYNC_SEL | 
		  RADEON_FP_CRTC_LOCK_8DOT |
		  RADEON_FP_USE_SHADOW_EN |
		  RADEON_FP_CRTC_USE_SHADOW_VEND |
		  RADEON_FP_CRT_SYNC_ALT);
	values->fp_gen_cntl |= 
		RADEON_FP_CRTC_DONT_SHADOW_VPAR |
		RADEON_FP_CRTC_DONT_SHADOW_HEND;
	
	// route VGA-DAC
	if( (total_devices & dd_crt) != 0 ) {
		int crtc_idx = (display_devices[1] & dd_crt) != 0;
		
		// the CRT_ON flag seems to directly affect the CRT-DAC, _not_ the CRTC1 signal
		values->crtc_ext_cntl |= RADEON_CRTC_CRT_ON;

		switch( ai->si->asic ) {
		case rt_rv100:
		case rt_rv200:
		case rt_rv250:
		case rt_rv280:
		case rt_rs100:
		case rt_rs200:
		case rt_rs300:
			values->dac_cntl2 &= ~RADEON_DAC_CLK_SEL_MASK;
			values->dac_cntl2 |= crtc_idx == 0 ? 0 : RADEON_DAC_CLK_SEL_CRTC2;
			break;

		case rt_r200:
		case rt_r300:
		case rt_rv350:
		case rt_r350:
		case rt_rv380:
		case rt_r420:
			values->disp_output_cntl &= ~RADEON_DISP_DAC_SOURCE_MASK;
			values->disp_output_cntl |=
				(crtc_idx == 0 ? 0 : RADEON_DISP_DAC_SOURCE_CRTC2);
			break;
		
		case rt_r100:
			break;
		}
		
	} else if( (controlled_devices & dd_crt) != 0 ) {
		values->crtc_ext_cntl &= ~RADEON_CRTC_CRT_ON;
	}
	

	if( (total_devices & (dd_tv_crt | dd_ctv | dd_stv)) != 0 ) {
		// power down TV-DAC
		// (but only if TV-Out _and_ TV-CRT is controlled by us)
		// this will be undone if needed
		values->tv_dac_cntl |= 
			RADEON_TV_DAC_CNTL_RDACPD |
			RADEON_TV_DAC_CNTL_GDACPD |
			RADEON_TV_DAC_CNTL_BDACPD;
	}
	
	
	// set CRT mode of TV-DAC if needed
	// (doesn't work on r200, but there is no TV-DAC used for CRT anyway)
	if( (total_devices & dd_tv_crt) != 0 ) {
		// enable CRT via TV-DAC (ignored if TV-DAC is in TV-Out mode)
		values->crtc2_gen_cntl |= RADEON_CRTC2_CRT2_ON;

		values->dac_cntl2 &= ~RADEON_DAC2_CLK_SEL_MASK;
		values->dac_cntl2 |= RADEON_DAC2_CLK_SEL_CRT;

		// enable TV-DAC and set PS2 signal level
		values->tv_dac_cntl = 
			RADEON_TV_DAC_CNTL_NBLANK |
			RADEON_TV_DAC_CNTL_NHOLD |
			RADEON_TV_DAC_CNTL_STD_PS2 |
			(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
			(2 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT);
		
		// at least r300 needs magic bit set to switch between TV-CRT and TV-Out
		if (IS_R300_VARIANT)
				values->gpiopad_a |= 1;
				
	} else if( (controlled_devices & dd_tv_crt) != 0 ) {
		values->crtc2_gen_cntl &= ~RADEON_CRTC2_CRT2_ON;
	}
	
	values->skip_tv_dac = false;


	// disable forwarding data to TV-Out unit
	// (will be enabled on demand later on)	
	if( (controlled_devices & (dd_ctv | dd_stv)) != 0 )
		values->dac_cntl &= ~RADEON_DAC_TVO_EN;
		
	
	// set TV mode of TV-DAC if needed
	if( (total_devices & (dd_ctv | dd_stv)) != 0 ) {
		// see above
		values->dac_cntl2 &= ~RADEON_DAC2_CLK_SEL_MASK;
		values->dac_cntl2 |= RADEON_DAC2_CLK_SEL_TV;
		
		// at least r300 needs magic bit set to switch between TV-CRT and TV-Out
		if(IS_R300_VARIANT)
				values->gpiopad_a &= ~1;
		
		
		// the TV-DAC itself is under control of the TV-Out code
		values->skip_tv_dac = true;
		
		if( !IS_INTERNAL_TV_OUT( ai->si->tv_chip )) {
			// tell DAC to forward data to external chip
			values->dac_cntl |= RADEON_DAC_TVO_EN;
			
			// set Output Linear Transform Unit as source
			// (TODO: is this unit initialized properly?)
			// disable overlay sync (could be a good idea to enable it)
			// set 8 BPP mode
			values->disp_output_cntl &= 
				~(RADEON_DISP_TV_SOURCE |
				RADEON_DISP_TV_MODE_MASK |
				RADEON_DISP_TV_YG_DITH_EN |
				RADEON_DISP_TV_CBB_CRR_DITH_EN |
				RADEON_DISP_TV_BIT_WIDTH |
				RADEON_DISP_TV_SYNC_MODE_MASK |
				RADEON_DISP_TV_SYNC_COLOR_MASK);
			
			// enable dithering
			values->disp_output_cntl |= 
				RADEON_DISP_TV_YG_DITH_EN |
				RADEON_DISP_TV_CBB_CRR_DITH_EN;
			
			// set output data format	
			values->disp_output_cntl |= tv_parameters->mode888 ? 
				RADEON_DISP_TV_MODE_888 : RADEON_DISP_TV_MODE_565;
				
			switch( ai->si->asic ) {
			case rt_r200:
				// disable downfiltering and scaling, set RGB mode, 
				// don't transmit overlay indicator;
				// I don't really know whether this is a good choice
				values->disp_tv_out_cntl &= 
					(RADEON_DISP_TV_OUT_YG_FILTER_MASK |
					 RADEON_DISP_TV_OUT_YG_SAMPLE |
					 RADEON_DISP_TV_OUT_CrR_FILTER_MASK |
					 RADEON_DISP_TV_OUT_CrR_SAMPLE |
					 RADEON_DISP_TV_OUT_CbB_FILTER_MASK |
					 RADEON_DISP_TV_OUT_CbB_SAMPLE |
					 RADEON_DISP_TV_SUBSAMPLE_CNTL_MASK |
					 RADEON_DISP_TV_H_DOWNSCALE |
					 RADEON_DISP_TV_COLOR_SPACE |
					 RADEON_DISP_TV_DITH_MODE |
					 RADEON_DISP_TV_DATA_ZERO_SEL |
					 RADEON_DISP_TV_CLKO_SEL |
					 RADEON_DISP_TV_CLKO_OUT_EN |
					 RADEON_DISP_TV_DOWNSCALE_CNTL);
					 
				// enable TVOCLKO (is this needed?)
				values->disp_tv_out_cntl |= RADEON_DISP_TV_CLKO_OUT_EN;
				break;
			
			default:
				;
			}
		}
		
	} else if( (controlled_devices & (dd_ctv | dd_stv)) != 0 ) {
		if( IS_INTERNAL_TV_OUT( ai->si->tv_chip )) {
			// disable clock of TV-out units
			values->tv_master_cntl = 
				RADEON_TV_MASTER_CNTL_TV_ASYNC_RST | 
				RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST |
				RADEON_TV_MASTER_CNTL_TV_FIFO_ASYNC_RST | 
				RADEON_TV_MASTER_CNTL_TVCLK_ALWAYS_ONb;
		}
	}
	
	// choose CRTC for TV-DAC
	if( (total_devices & (dd_tv_crt | dd_ctv | dd_stv)) != 0 ) {
		int crtc_idx = (display_devices[1] & (dd_tv_crt | dd_ctv | dd_stv)) != 0;
		
		switch( ai->si->asic ) {
		case rt_rv100:
		case rt_rv200:	
		case rt_rv250:
		case rt_rv280:
		case rt_rs100:
		case rt_rs200:
		case rt_rs300:
					values->disp_hw_debug &= ~RADEON_CRT2_DISP1_SEL;
			// warning: meaning is wrong way around - 0 means crtc2, 1 means crtc1
			values->disp_hw_debug |= crtc_idx == 0 ? RADEON_CRT2_DISP1_SEL : 0;
			break;
		
		case rt_r200:
			// TV-Out data comes directly from CRTC (i.e. with Linear Transform Unit)
			values->disp_output_cntl |= RADEON_DISP_TV_SOURCE;
			// choose CRTC
			values->disp_tv_out_cntl &= ~RADEON_DISP_TV_PATH_SRC;
			values->disp_tv_out_cntl |= crtc_idx == 0 ? 0 : RADEON_DISP_TV_PATH_SRC;
			break;

		case rt_r300:
		case rt_rv350:
		case rt_r350:
		case rt_rv380:
		case rt_r420:
			values->disp_output_cntl &= ~RADEON_DISP_TVDAC_SOURCE_MASK;
			values->disp_output_cntl |=
				crtc_idx == 0 ? 0 : RADEON_DISP_TVDAC_SOURCE_CRTC2;
			break;
			
		case rt_r100:
			break;
		}
	}
	
	// choose clock source for (internal) TV-out unit
	if( (total_devices & (dd_ctv | dd_stv)) != 0 ) {
		int crtc_idx = (display_devices[1] & (dd_ctv | dd_stv)) != 0;
		
		values->pixclks_cntl &= ~RADEON_PIXCLK_TV_SRC_SEL_MASK;
		values->pixclks_cntl |= crtc_idx == 0 ?
			RADEON_PIXCLK_TV_SRC_SEL_PIXCLK : RADEON_PIXCLK_TV_SRC_SEL_PIX2CLK;
	}

	// choose CRTC clock source;
	// normally, CRTC1 uses PLL1 and CRTC2 uses PLL2, but if an external TV-Out
	// chip is used, the clock is retrieved from this chip to stay in perfect sync
	if( (display_devices[0] & (dd_ctv | dd_stv)) != 0
		&& !IS_INTERNAL_TV_OUT( ai->si->tv_chip ))
	{
		// select BYTCLK input pin as pixel src
		values->vclk_ecp_cntl &=
			~(RADEON_VCLK_ECP_CNTL_BYTE_CLK_POST_DIV_MASK | RADEON_VCLK_SRC_SEL_MASK);
		
		values->vclk_ecp_cntl |= RADEON_VCLK_SRC_BYTE_CLK;
		values->vclk_ecp_cntl |= 0 << RADEON_VCLK_ECP_CNTL_BYTE_CLK_POST_DIV_SHIFT;
		
		// disable clock if pixel format in CRTC_GEN_CNTL is zero;
		// disable (DAC?) during blank
		values->vclk_ecp_cntl |= RADEON_PIXCLK_ALWAYS_ONb | RADEON_PIXCLK_DAC_ALWAYS_ONb;

	} else {
		// select PLL as pixel clock
		values->vclk_ecp_cntl &= ~RADEON_VCLK_SRC_SEL_MASK;
		values->vclk_ecp_cntl |= RADEON_VCLK_SRC_PPLL_CLK;
		
		// disable clock if pixel format in CRTC_GEN_CNTL is zero
		values->vclk_ecp_cntl |= RADEON_PIXCLK_ALWAYS_ONb;
	}

	values->pixclks_cntl &= ~RADEON_PIX2CLK_SRC_SEL_MASK;
	if( (display_devices[1] & (dd_ctv | dd_stv)) != 0
		&& !IS_INTERNAL_TV_OUT( ai->si->tv_chip ))
	{
		// r200 spec misses everything regarding second CRTC, so
		// this is guessing
		values->pixclks_cntl |= 2;
	} else
		values->pixclks_cntl |= RADEON_PIX2CLK_SRC_SEL_P2PLL_CLK;
	
	// choose CRTC for flat panel
	if( (total_devices & (dd_lvds | dd_dvi)) != 0 ) {
		int crtc_idx = (display_devices[1] & (dd_lvds | dd_dvi)) != 0;
		
		values->fp_gen_cntl &= ~RADEON_FP_SEL_CRTC2;
		values->fp_gen_cntl |= crtc_idx == 0 ? 0 : RADEON_FP_SEL_CRTC2;
	}

	// enable/disable RMX for crtc1 if there is a flat panel
	// (TODO: this doesn't seem to work)
	// !!! makes trouble on Radeon 9200 Mobility !??
	if( (display_devices[1] & (dd_lvds | dd_dvi)) != 0 ) {
		values->disp_output_cntl &= ~RADEON_DISP_DAC_SOURCE_MASK;
		values->disp_output_cntl |= RADEON_DISP_DAC_SOURCE_RMX;
	}

	
	// choose CRTC for secondary flat panel
	if( (total_devices & dd_dvi_ext) != 0 ) {
		int crtc_idx = (display_devices[1] & (dd_dvi_ext)) != 0;

		// TODO: this list looks a bit magic/wrong for me; I reckon ATI moved the
		// bit starting with ASIC xxx, but I have no specs to verify that
		switch( ai->si->asic ) {
		case rt_r200:
		case rt_r300:
		case rt_r350:
		case rt_rv350:	
		case rt_rv380:
		case rt_r420:
			values->fp2_gen_cntl &= ~RADEON_FP2_SOURCE_SEL_CRTC2;
		    values->fp2_gen_cntl |= 
		    	crtc_idx == 0 ? 0 : RADEON_FP2_SOURCE_SEL_CRTC2;
		    break;
		    
		default:
			values->fp2_gen_cntl &= ~RADEON_FP2_SRC_SEL_CRTC2;
			values->fp2_gen_cntl |= 
				crtc_idx == 0 ? 0 : RADEON_FP2_SRC_SEL_CRTC2;
		}
	}
}

void Radeon_ProgramMonitorRouting( 
	accelerator_info *ai, routing_regs *values )
{
	vuint8 *regs = ai->regs;
	
	OUTREG( regs, RADEON_DAC_CNTL, values->dac_cntl );
	OUTREG( regs, RADEON_DAC_CNTL2, values->dac_cntl2 );
	OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, values->crtc2_gen_cntl,
		~RADEON_CRTC2_CRT2_ON );
	OUTREG( regs, RADEON_DISP_OUTPUT_CNTL, values->disp_output_cntl );

	switch( ai->si->asic ) {
	case rt_rv100:
	case rt_rv200:	
	case rt_rv250:
	case rt_rv280:
	case rt_rs100:
	case rt_rs200:
	case rt_rs300:
		OUTREG( regs, RADEON_DISP_HW_DEBUG, values->disp_hw_debug );
		break;
		
	case rt_r200:
		OUTREG( regs, RADEON_DISP_TV_OUT_CNTL, values->disp_tv_out_cntl );
		break;

	case rt_r300:
	case rt_rv350:
	case rt_r350:
	case rt_rv380:
	case rt_r420:
		OUTREGP( regs, RADEON_GPIOPAD_A, values->gpiopad_a, ~1 );
		break;
		
	case rt_r100:
		break;
	}

	if( ai->si->asic > rt_r100 ) {
		// register introduced after R100;
		// only set it when necessary (more precisely: if TV-Out is used,
		// this register is set by the TV-Out code)
		if( !values->skip_tv_dac )
			OUTREG( regs, RADEON_TV_DAC_CNTL, values->tv_dac_cntl );
	}
	
	if( IS_INTERNAL_TV_OUT( ai->si->tv_chip ))
		OUTREG( regs, RADEON_TV_MASTER_CNTL, values->tv_master_cntl );
	
	OUTREGP( regs, RADEON_FP_GEN_CNTL, values->fp_gen_cntl, ~(
		RADEON_FP_SEL_CRTC2 |
		RADEON_FP_RMX_HVSYNC_CONTROL_EN |
		RADEON_FP_DFP_SYNC_SEL |	
		RADEON_FP_CRT_SYNC_SEL | 
		RADEON_FP_CRTC_LOCK_8DOT |
		RADEON_FP_USE_SHADOW_EN |
		RADEON_FP_CRTC_USE_SHADOW_VEND |
		RADEON_FP_CRT_SYNC_ALT |
		RADEON_FP_CRTC_DONT_SHADOW_VPAR |
		RADEON_FP_CRTC_DONT_SHADOW_HEND ));
		
		
	OUTREGP( regs, RADEON_FP2_GEN_CNTL, values->fp2_gen_cntl, 
		~(RADEON_FP2_SOURCE_SEL_CRTC2 | RADEON_FP2_SRC_SEL_CRTC2 ));

	if( ai->vc->used_crtc[0] ) {
		Radeon_OUTPLLP( ai->regs, ai->si->asic, 
			RADEON_VCLK_ECP_CNTL, values->vclk_ecp_cntl, 
			~RADEON_VCLK_SRC_SEL_MASK );
	}
	
	if( ai->vc->used_crtc[1] ) {
		Radeon_OUTPLLP( ai->regs, ai->si->asic, 
			RADEON_PIXCLKS_CNTL, values->pixclks_cntl, 
			~RADEON_PIX2CLK_SRC_SEL_MASK );
	}
	
	Radeon_OUTPLLP( ai->regs, ai->si->asic, 
		RADEON_PIXCLKS_CNTL, values->pixclks_cntl, 
		~RADEON_PIXCLK_TV_SRC_SEL_MASK );

	// enable/disable CRTC1
	if( ai->vc->assigned_crtc[0] ) {
		uint32 crtc_gen_cntl;
		
		crtc_gen_cntl = INREG( regs, RADEON_CRTC_GEN_CNTL );
		
		if( ai->vc->used_crtc[0] ) {
			crtc_gen_cntl |= RADEON_CRTC_EN;
		} else {
			crtc_gen_cntl &= ~RADEON_CRTC_EN;
			crtc_gen_cntl &= ~RADEON_CRTC_PIX_WIDTH_MASK;
		}
			
		OUTREGP( regs, RADEON_CRTC_GEN_CNTL, crtc_gen_cntl, 
			~(RADEON_CRTC_PIX_WIDTH_MASK | RADEON_CRTC_EN) );
	}

	// enable/disable CRTC2
	if( ai->vc->assigned_crtc[1] ) {
		uint32 crtc2_gen_cntl;
		
		crtc2_gen_cntl = INREG( regs, RADEON_CRTC2_GEN_CNTL );
		
		if( ai->vc->used_crtc[1] ) {
			crtc2_gen_cntl |= RADEON_CRTC2_EN;
		} else {
			crtc2_gen_cntl &= ~RADEON_CRTC2_EN;
			crtc2_gen_cntl &= ~RADEON_CRTC2_PIX_WIDTH_MASK;
		}
			
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, crtc2_gen_cntl, 
			~(RADEON_CRTC2_PIX_WIDTH_MASK | RADEON_CRTC2_EN) );
	}
		
	// XFree says that crtc_ext_cntl must be restored after CRTC2 in dual-screen mode
	OUTREGP( regs, RADEON_CRTC_EXT_CNTL, values->crtc_ext_cntl,
		RADEON_CRTC_VSYNC_DIS |
		RADEON_CRTC_HSYNC_DIS |
		RADEON_CRTC_DISPLAY_DIS );
}


// internal version of SetupDefaultMonitorRouting;
// input and output are written to local variables
static void assignDefaultMonitorRoute( 
	accelerator_info *ai,
	display_device_e display_devices, int whished_num_heads, bool use_laptop_panel,
	display_device_e *crtc1, display_device_e *crtc2 )
{
	virtual_card *vc = ai->vc;
	display_device_e crtc1_displays = 0, crtc2_displays = 0;
	
	SHOW_FLOW( 2, "display_devices=%x, whished_num_heads=%d", 
		display_devices, whished_num_heads );
	
	// restrict to allowed devices	
	display_devices &= ai->vc->controlled_displays;
	
	// if CRTC1 is not ours, we cannot use flat panels
	if( !ai->vc->assigned_crtc[0] ) {
		display_devices &= ~(dd_lvds | dd_dvi);
	}
	
	SHOW_FLOW( 2, "after restriction: %x", display_devices );
	
	// flat panels get always connected to CRTC1 because its RMX unit
	if( (display_devices & dd_lvds) != 0 ) {
		// if user requests it, laptop panels are always used
		if( use_laptop_panel ) {
			crtc1_displays |= dd_lvds;
			
		} else {
			// if he doesn't request it, we try to not use it
			display_device_e tmp_crtc1, tmp_crtc2;
			int effective_num_heads;
			
			// determine routing with laptop panel ignored
			assignDefaultMonitorRoute( ai, display_devices & ~dd_lvds, 
				whished_num_heads, use_laptop_panel, &tmp_crtc1, &tmp_crtc2 );

			effective_num_heads = (tmp_crtc1 != 0) + (tmp_crtc2 != 0);
			
			// only use laptop panel if we cannot satisfy the requested 
			// number of heads without it
			if( effective_num_heads < whished_num_heads )
				crtc1_displays |= dd_lvds;
		}
		
	} else if( (display_devices & dd_dvi) != 0 )
		crtc1_displays |= dd_dvi;
		
	// TV-Out gets always connected to crtc2...
	if( (display_devices & dd_stv) != 0 )
		crtc2_displays |= dd_stv;
	else if( (display_devices & dd_ctv) != 0 )
		crtc2_displays |= dd_ctv;
		
	// ...but if there is no crtc2, they win on crtc1;
	// if the user connects both a flat panel and a TV, he usually 
	// wants to use the TV
	if( !vc->assigned_crtc[1] && crtc2_displays != 0 ) {
		crtc1_displays = crtc2_displays;
		crtc2_displays = dd_none;
	}
		
	// if internal TV-Out is used, the DAC cannot drive a CRT at the same time
	if( IS_INTERNAL_TV_OUT( ai->si->tv_chip ) && (display_devices & (dd_stv | dd_ctv)) != 0 )
		display_devices &= ~dd_tv_crt;
		
	// CRT on CRT-DAC gets any spare CRTC;
	// if there is none, it can share CRTC with TV-Out;
	// this sharing may be dangerous as TV-Out uses strange timings, so
	// we should perhaps forbid sharing
	if( (display_devices & dd_crt) != 0 ) {
		if( crtc1_displays == 0 && vc->assigned_crtc[0] )
			crtc1_displays |= dd_crt;
		else if( ai->si->num_crtc > 1 && crtc2_displays == 0 && vc->assigned_crtc[1] )
			crtc2_displays |= dd_crt;
		else if( (crtc1_displays & ~(dd_stv | dd_ctv)) == 0 && vc->assigned_crtc[0] )
			crtc1_displays |= dd_crt;
		else if( ai->si->num_crtc > 1 && (crtc2_displays & ~(dd_stv | dd_ctv)) == 0 && vc->assigned_crtc[1] )
			crtc2_displays |= dd_crt;
	}
	
	// same applies to CRT on TV-DAC;
	// if we cannot find a CRTC, we could clone the content of the CRT-DAC,
	// but I doubt that you really want two CRTs showing the same
	if( (display_devices & dd_tv_crt) != 0 ) {
		if( crtc1_displays == 0 && vc->assigned_crtc[0] )
			crtc1_displays |= dd_tv_crt;
		else if( ai->si->num_crtc > 1 && crtc2_displays == 0 && vc->assigned_crtc[1] )
			crtc2_displays |= dd_tv_crt;
		else if( (crtc1_displays & ~(dd_stv | dd_ctv)) == 0 && vc->assigned_crtc[0] )
			crtc1_displays |= dd_tv_crt;
		else if( ai->si->num_crtc > 1 && (crtc2_displays & ~(dd_stv | dd_ctv)) == 0 && vc->assigned_crtc[1] )
			crtc2_displays |= dd_tv_crt;
	}

	if( (display_devices & dd_dvi_ext) != 0 )
		crtc2_displays |= dd_dvi_ext;

	SHOW_FLOW( 2, "CRTC1: 0x%x, CRTC2: 0x%x", crtc1_displays, crtc2_displays );
	
	*crtc1 = crtc1_displays;
	*crtc2 = crtc2_displays;
}

// Setup sensible default monitor routing
// whished_num_heads - number of independant heads current display mode would need
// use_laptop_panel - if true, always use laptop panel
void Radeon_SetupDefaultMonitorRouting( 
	accelerator_info *ai, int whished_num_heads, bool use_laptop_panel )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	display_device_e display_devices = vc->connected_displays;

	if (ai->si->settings.force_lcd) {
		use_laptop_panel = true;
		SHOW_FLOW0( 2, 	"LCD Forced Used by Kernel Settings");
	}
	
	SHOW_FLOW( 2, "display_devices=%x, whished_num_heads=%d, use_laptop_panel=%d", 
		display_devices, whished_num_heads, use_laptop_panel );
		
	// ignore TV if standard is set to "off"
	if( vc->tv_standard == ts_off )
		display_devices &= ~(dd_ctv | dd_stv);
	
	assignDefaultMonitorRoute( 
		ai, display_devices, whished_num_heads, use_laptop_panel,
		&si->crtc[0].chosen_displays, &si->crtc[1].chosen_displays );
		
/*	si->crtc[0].chosen_displays = dd_none;
	si->crtc[1].chosen_displays = dd_tv_crt;*/
		
	/*vc->used_crtc[0] = si->crtc[0].chosen_displays != dd_none;
	vc->used_crtc[1] = si->crtc[1].chosen_displays != dd_none;*/
		
	SHOW_FLOW( 2, "num_crtc: %d, CRTC1 (%s): 0x%x, CRTC2 (%s): 0x%x", 
		si->num_crtc,
		vc->assigned_crtc[0] ? "assigned" : "not assigned", si->crtc[0].chosen_displays, 
		vc->assigned_crtc[0] ? "assigned" : "not assigned", si->crtc[1].chosen_displays );
}
