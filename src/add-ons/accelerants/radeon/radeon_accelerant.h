/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Internal header file
*/

#ifndef _RADEON_ACCELERANT_H
#define _RADEON_ACCELERANT_H


#include "radeon_interface.h"
#include "accelerant_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

void    _kdprintf_(const char *format, ...);
//bool    set_dprintf_enabled(bool);	/* returns old enable flag */

#define dprintf _kdprintf_

extern int debug_level_flow;
extern int debug_level_info;
extern int debug_level_error;

/*#define DEBUG_WAIT_ON_MSG 1000000
#define DEBUG_WAIT_ON_ERROR 1000000*/

#define DEBUG_MSG_PREFIX "Radeon - "

#define DEBUG_MAX_LEVEL_FLOW 2

#include "debug_ext.h"


// info about this accelerant
typedef struct accelerator_info {
	virtual_card *vc;			// associated virtual card
	vuint8 *regs;				// pointer to mapped registers
								// !! dont't make it vuint32, access macros rely on 8 bits !!
	area_id shared_info_area;	// info shared between accelerants
	area_id regs_area;			// MM I/O registers
	area_id virtual_card_area;	// info about virtual card
	
	// mapped-in (non)-local memory;
	// as si->local_mem contains a pointer to the local frame buffer,
	// only mt_pci and mt_agp are filled directly, mt_nonlocal contains
	// a copy of either mt_pci or mt_agp, mt_local a copy of si->local_mem
	struct {
		area_id 	area;		// area of clone
		char 		*data;		// CPU address of area
	} mapped_memory[mt_last+1];
	
	int accelerant_is_clone;	// true, if this is a cloned accelerant
		
	int fd;						// file descriptor of kernel driver
	struct log_info_t *log;
	
	area_id mode_list_area;		// cloned list of standard display modes
	display_mode *mode_list;	// list of standard display modes
	shared_info *si;			// info shared between accelerants
} accelerator_info;


// SetDisplayMode.c
uint32 Radeon_RoundVWidth( int virtual_width, int bpp );
status_t Radeon_MoveDisplay( accelerator_info *ai, uint16 h_display_start, uint16 v_display_start );


// crtc.c
void Radeon_ReadCRTCRegisters( accelerator_info *ai, physical_head *head, 
	port_regs *values );
uint16 Radeon_GetHSyncFudge( physical_head *head, int datatype );
void Radeon_CalcCRTCRegisters( accelerator_info *ai, physical_head *head, 
	display_mode *mode, port_regs *values );
void Radeon_ProgramCRTCRegisters( accelerator_info *ai, physical_head *head, 
	port_regs *values );


// multimon.c
void Radeon_HideMultiMode( virtual_card *vc, display_mode *mode );
void Radeon_DetectMultiMode( virtual_card *vc, display_mode *mode );
void Radeon_VerifyMultiMode( virtual_card *vc, shared_info *si, display_mode *mode );
void Radeon_InitMultiModeVars( virtual_card *vc, display_mode *mode );
status_t Radeon_CheckMultiMonTunnel( virtual_card *vc, display_mode *mode, 
	const display_mode *low, const display_mode *high, bool *isTunnel );
bool Radeon_NeedsSecondPort( display_mode *mode );
bool Radeon_DifferentPorts( display_mode *mode );


// ProposeDisplayMode.c
bool Radeon_GetFormat( int space, int *format, int *bpp );
status_t Radeon_CreateModeList( shared_info *si );

	
// pll.c	
void Radeon_CalcPLLRegisters( general_pll_info *pll, const display_mode *mode, pll_dividers *fixed_dividers, port_regs *values );
void Radeon_ProgramPLL( accelerator_info *ai, physical_head *head, port_regs *values );
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
void Radeon_ReadRMXRegisters( accelerator_info *ai, port_regs *values );
void Radeon_CalcRMXRegisters( fp_info *flatpanel, display_mode *mode, bool use_rmx, port_regs *values );
void Radeon_ProgramRMXRegisters( accelerator_info *ai, port_regs *values );

void Radeon_ReadFPRegisters( accelerator_info *ai, port_regs *values );
void Radeon_CalcFPRegisters( accelerator_info *ai, physical_head *head, 
	fp_info *fp_port, port_regs *values );
void Radeon_ProgramFPRegisters( accelerator_info *ai, physical_head *head,
	fp_info *fp_port, port_regs *values );


// dpms.c
status_t Radeon_SetDPMS( accelerator_info *ai, physical_head *head, int mode );
uint32 Radeon_GetDPMS( accelerator_info *ai, physical_head *head );


// Cursor.c
void Radeon_SetCursorColors( accelerator_info *ai, physical_head *head );


// Acceleration.c
void Radeon_Init2D( accelerator_info *ai );
void Radeon_AllocateVirtualCardStateBuffer( accelerator_info *ai );
void Radeon_FreeVirtualCardStateBuffer( accelerator_info *ai );
void Radeon_FillStateBuffer( accelerator_info *ai, uint32 datatype );


// driver_wrapper.c
status_t Radeon_WaitForIdle( accelerator_info *ai, bool keep_lock );
void Radeon_ResetEngine( accelerator_info *ai );

status_t Radeon_VIPRead( accelerator_info *ai, uint channel, uint address, uint32 *data );
status_t Radeon_VIPWrite( accelerator_info *ai, uint8 channel, uint address, uint32 data );
int Radeon_FindVIPDevice( accelerator_info *ai, uint32 device_id );


// settings.cpp
void Radeon_ReadSettings( virtual_card *vc );
void Radeon_WriteSettings( virtual_card *vc );


// overlay.c
void Radeon_HideOverlay( accelerator_info *ai );
status_t Radeon_UpdateOverlay( accelerator_info *ai );


// EngineManagement.c
void Radeon_Spin( uint32 delay );


// monitor_detection.c
void Radeon_DetectDisplays( accelerator_info *ai );


// tv_out.c
void Radeon_DetectTVOut( accelerator_info *ai );
void Radeon_CalcTVParams( const general_pll_info *general_pll, tv_params *params, 
	const tv_timing *tv_timing, bool internal_encoder, 
	const display_mode *mode, display_mode *tweaked_mode );
void Radeon_CalcTVRegisters( accelerator_info *ai, display_mode *mode, tv_timing *timing, 
	tv_params *params, port_regs *values, physical_head *head, 
	bool internal_encoder, tv_standard tv_format );
void Radeon_ProgramTVRegisters( accelerator_info *ai, port_regs *values, bool internal_encoder );
void Radeon_ReadTVRegisters( accelerator_info *ai, port_regs *values, bool internal_encoder );

extern tv_timing Radeon_std_tv_timing[6];


// palette.c
void Radeon_InitPalette( accelerator_info *ai, physical_head *head );


// monitor_routing.h
void Radeon_ReadMonitorRoutingRegs( accelerator_info *ai, physical_head *head, 
	port_regs *values );
void Radeon_CalcMonitorRouting( accelerator_info *ai, physical_head *head, 
	port_regs *values );
void Radeon_ProgramMonitorRouting( accelerator_info *ai, physical_head *head, port_regs *values );
void Radeon_SetupDefaultMonitorRouting( accelerator_info *ai, int whished_num_heads );
	

#ifdef __cplusplus
}
#endif

#endif
