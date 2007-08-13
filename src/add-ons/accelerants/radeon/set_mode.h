/*
	Copyright (c) 2002-04, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Header file explicitely for display mode changes
*/

#ifndef _SET_MODE_H
#define _SET_MODE_H

// PLL divider values
typedef struct {
	uint32 post_code;			// code for post divider
	uint32 post;				// value of post divider
	uint32 extra_post_code;		// code for extra post divider
	uint32 extra_post;			// value of extra post divider
	uint32 ref;					// reference divider
	uint32 feedback;			// feedback divider
	uint32 freq;				// resulting frequency
} pll_dividers;


// TV-timing
typedef struct {
	uint32 freq;				// TV sub carrier frequency x12
	uint16 h_total;
	uint16 h_sync_len;
	uint16 h_genclk_delay;
	uint16 h_setup_delay;
	uint16 h_active_delay;
	uint16 h_active_len;
	uint16 v_total;
	uint16 v_active_lines;
	uint16 v_field_total;
	uint16 v_fields;
	uint16 f_total;
	uint16 frame_size_adjust;
	uint32 scale;
} tv_timing;


// TV-Out parameters
typedef struct {
	uint16 y_accum_init;
	uint16 uv_accum_init;
	uint16 uv_inc;
	uint16 h_inc;
	uint32 tv_clocks_to_active;
	
	uint16 f_restart;
	uint16 v_restart;
	uint16 h_restart;
	bool mode888;
	
	uint16 y_saw_tooth_slope;
	uint16 y_saw_tooth_amp;
	uint16 y_rise_accum_init;
	uint16 y_fall_accum_init;
	bool y_coeff_enable;
	uint8 y_coeff_value;
	
	pll_dividers	tv_dividers;
	pll_dividers	crt_dividers;
	
	tv_timing		timing;
} impactv_params;


// CRTC register content (for mode change)
typedef struct {
	uint32		crtc_h_total_disp;
	uint32		crtc_h_sync_strt_wid;
	uint32		crtc_v_total_disp;
	uint32		crtc_v_sync_strt_wid;
	uint32		crtc_pitch;
	uint32		crtc_gen_cntl;
	uint32		crtc_offset_cntl;
} crtc_regs;


// PLL register content (for mode change)
typedef struct {
	uint32 		ppll_div_3;
	uint32		ppll_ref_div;
	uint32		htotal_cntl;
	
	// pure information
	uint32		dot_clock_freq;	// in 10 kHz
	uint32		pll_output_freq;// in 10 kHz
	int			feedback_div;
	int			post_div;
} pll_regs;


// Flat Panel register content (for mode change)
typedef struct {
	uint32		fp_gen_cntl;
	uint32		fp_panel_cntl;
	uint32		lvds_gen_cntl;
	uint32		tmds_pll_cntl;
	uint32		tmds_trans_cntl;
	uint32		fp_h_sync_strt_wid;
	uint32		fp_v_sync_strt_wid;
	uint32		fp2_gen_cntl;
    
	uint32		fp2_h_sync_strt_wid;
	uint32		fp2_v_sync_strt_wid;
	
	// RMX registers
	uint32		fp_horz_stretch;
	uint32		fp_vert_stretch;

	// Bios values used by Mobility Asics
	uint32		bios_4_scratch;
	uint32		bios_5_scratch;
	uint32		bios_6_scratch;
} fp_regs;


#define RADEON_TV_TIMING_SIZE 32
#define RADEON_TV_UPSAMP_COEFF_NUM (5*3)


// ImpacTV-Out regs (for mode change)
typedef struct {
	uint32		tv_ftotal;
	uint32		tv_vscaler_cntl1;
	uint32		tv_y_saw_tooth_cntl;
	uint32		tv_y_fall_cntl;
	uint32		tv_y_rise_cntl;
	uint32		tv_vscaler_cntl2;
	uint32 		tv_hrestart;
	uint32 		tv_vrestart;
	uint32 		tv_frestart;
	uint32		tv_tv_pll_cntl;
	uint32 		tv_crt_pll_cntl;
	uint32		tv_clock_sel_cntl;
	uint32 		tv_clkout_cntl;
	uint32		tv_htotal;
	uint32 		tv_hsize;
	uint32		tv_hdisp;
	uint32		tv_hstart;
	uint32		tv_vtotal;
	uint32		tv_vdisp;
	uint32		tv_sync_size;
	uint32		tv_timing_cntl;
	uint32		tv_modulator_cntl1;
	uint32		tv_modulator_cntl2;
	uint32		tv_data_delay_a;
	uint32		tv_data_delay_b;
	uint32		tv_frame_lock_cntl;
	uint32		tv_pll_cntl1;
	uint32		tv_rgb_cntl;
	uint32		tv_pre_dac_mux_cntl;
	uint32		tv_master_cntl;
	uint32		tv_dac_cntl;
	uint32		tv_uv_adr;
	uint32		tv_pll_fine_cntl;
	uint32		tv_gain_limit_settings;
	uint32		tv_linear_gain_settings;
	uint32		tv_upsamp_and_gain_cntl;
	uint32		tv_crc_cntl;

	uint16		tv_hor_timing[RADEON_TV_TIMING_SIZE];
	uint16		tv_vert_timing[RADEON_TV_TIMING_SIZE];
	
	uint32		tv_upsample_filter_coeff[RADEON_TV_UPSAMP_COEFF_NUM];
} impactv_regs;


// Monitor Signal Routing regs (for mode change)
// (they collide with many other *_regs, so take
// care to set only the bits really used for routing)
typedef struct {
	// DAC registers
	uint32		dac_cntl2;
	uint32		dac_cntl;
	uint32		tv_master_cntl;
	uint32		tv_dac_cntl;
	bool		skip_tv_dac;	// if true, don't write tv_dac_cntl
	
	// Display path registers
	uint32		disp_hw_debug;
	uint32		disp_output_cntl;
	uint32		disp_tv_out_cntl;

	// CRTC registers 
	uint32		crtc_ext_cntl;
	uint32		crtc2_gen_cntl;

	// PLL regs	
	uint32		vclk_ecp_cntl;
	uint32		pixclks_cntl;
	
	// GP IO-pad
	uint32		gpiopad_a;
	
	// flat panel registers
	uint32		fp_gen_cntl;
	uint32		fp2_gen_cntl;
} routing_regs;


// crtc.c
uint16 Radeon_GetHSyncFudge( crtc_info *crtc, int datatype );
void Radeon_CalcCRTCRegisters( accelerator_info *ai, crtc_info *crtc, 
	display_mode *mode, crtc_regs *values );
void Radeon_ProgramCRTCRegisters( accelerator_info *ai, int crtc_idx, 
	crtc_regs *values );


// pll.c	
void Radeon_CalcCRTPLLDividers( const general_pll_info *general_pll, const display_mode *mode, pll_dividers *dividers );
void Radeon_CalcPLLRegisters( const display_mode *mode, const pll_dividers *dividers, pll_regs *values );
void Radeon_ProgramPLL( accelerator_info *ai, int crtc_idx, pll_regs *values );
void Radeon_CalcPLLDividers( const pll_info *pll, uint32 freq, uint fixed_post_div, pll_dividers *dividers );
void Radeon_MatchCRTPLL( 
	const pll_info *pll, 
	uint32 tv_v_total, uint32 tv_h_total, uint32 tv_frame_size_adjust, uint32 freq, 
	const display_mode *mode, uint32 max_v_tweak, uint32 max_h_tweak, 
	uint32 max_frame_rate_drift, uint32 fixed_post_div, 
	pll_dividers *dividers,
	display_mode *tweaked_mode );
void Radeon_GetTVPLLConfiguration( const general_pll_info *general_pll, pll_info *pll, 
	bool internal_encoder );
void Radeon_GetTVCRTPLLConfiguration( const general_pll_info *general_pll, pll_info *pll,
	bool internal_tv_encoder );


// flat_panel.c
void Radeon_ReadRMXRegisters( accelerator_info *ai, fp_regs *values );
void Radeon_CalcRMXRegisters( fp_info *flatpanel, display_mode *mode, bool use_rmx, fp_regs *values );
void Radeon_ProgramRMXRegisters( accelerator_info *ai, fp_regs *values );

void Radeon_ReadFPRegisters( accelerator_info *ai, fp_regs *values );
void Radeon_CalcFPRegisters( accelerator_info *ai, crtc_info *crtc, 
	fp_info *fp_port, crtc_regs *crtc_values, fp_regs *values );
void Radeon_ProgramFPRegisters( accelerator_info *ai, crtc_info *crtc,
	fp_info *fp_port, fp_regs *values );


// monitor_routing.h
void Radeon_ReadMonitorRoutingRegs( 
	accelerator_info *ai, routing_regs *values );
void Radeon_CalcMonitorRouting( 
	accelerator_info *ai, const impactv_params *tv_parameters, routing_regs *values );
void Radeon_ProgramMonitorRouting( 
	accelerator_info *ai, routing_regs *values );
void Radeon_SetupDefaultMonitorRouting( 
	accelerator_info *ai, int whished_num_heads, bool use_laptop_panel );


// impactv.c

typedef void (*impactv_write_FIFO) ( 
	accelerator_info *ai, uint16 addr, uint32 value );
typedef uint32 (*impactv_read_FIFO) ( 
	accelerator_info *ai, uint16 addr );

void Radeon_CalcImpacTVParams( 
	const general_pll_info *general_pll, impactv_params *params, 
	tv_standard_e tv_format, bool internal_encoder, 
	const display_mode *mode, display_mode *tweaked_mode );
void Radeon_CalcImpacTVRegisters( 
	accelerator_info *ai, display_mode *mode,
	impactv_params *params, impactv_regs *values, int crtc_idx, 
	bool internal_encoder, tv_standard_e tv_format, display_device_e display_device );
void Radeon_ImpacTVwriteHorTimingTable( 
	accelerator_info *ai, impactv_write_FIFO write, impactv_regs *values, bool internal_encoder );
void Radeon_ImpacTVwriteVertTimingTable( 
	accelerator_info *ai, impactv_write_FIFO write, impactv_regs *values );
	

// theatre_out.c
void Radeon_TheatreProgramTVRegisters( accelerator_info *ai, impactv_regs *values );
void Radeon_TheatreReadTVRegisters( accelerator_info *ai, impactv_regs *values );
uint32 Radeon_TheatreReadFIFO( accelerator_info *ai, uint16 addr );
void Radeon_TheatreWriteFIFO( accelerator_info *ai, uint16 addr, uint32 value );

// internal_tv_out.c
void Radeon_InternalTVOutProgramRegisters( accelerator_info *ai, impactv_regs *values );
void Radeon_InternalTVOutReadRegisters( accelerator_info *ai, impactv_regs *values );


#endif
