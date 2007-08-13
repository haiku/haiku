/*
	Copyright (c) 2002-2005, Thomas Kurschel
	

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

void    _sPrintf(const char *format, ...);
//bool    set_dprintf_enabled(bool);	/* returns old enable flag */

#define dprintf _sPrintf

extern int debug_level_flow;
extern int debug_level_info;
extern int debug_level_error;

/*#define DEBUG_WAIT_ON_MSG 1000000
#define DEBUG_WAIT_ON_ERROR 1000000*/

#define DEBUG_MSG_PREFIX "Radeon - "

#define DEBUG_MAX_LEVEL_FLOW 3

#include "debug_ext.h"


// info about this accelerant
typedef struct accelerator_info {
	shared_info *si;			// info shared between accelerants
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
} accelerator_info;


#define IS_INTERNAL_TV_OUT( tv_chip ) \
	( (tv_chip) == tc_internal_rt1 || (tv_chip) == tc_internal_rt2 )


// vesa_modes.c
extern const display_timing vesa_mode_list[];
extern const size_t vesa_mode_list_count;

// SetDisplayMode.c
uint32 Radeon_RoundVWidth( int virtual_width, int bpp );
status_t Radeon_MoveDisplay( accelerator_info *ai, uint16 h_display_start, uint16 v_display_start );
void Radeon_EnableIRQ( accelerator_info *ai, bool enable );

// multimon.c
void Radeon_HideMultiMode( virtual_card *vc, display_mode *mode );
void Radeon_DetectMultiMode( virtual_card *vc, display_mode *mode );
void Radeon_VerifyMultiMode( virtual_card *vc, shared_info *si, display_mode *mode );
void Radeon_InitMultiModeVars( accelerator_info *ai, display_mode *mode );
status_t Radeon_CheckMultiMonTunnel( virtual_card *vc, display_mode *mode, 
	const display_mode *low, const display_mode *high, bool *isTunnel );
bool Radeon_NeedsSecondPort( display_mode *mode );
bool Radeon_DifferentPorts( display_mode *mode );


// ProposeDisplayMode.c
bool Radeon_GetFormat( int space, int *format, int *bpp );
status_t Radeon_CreateModeList( shared_info *si );

	
// dpms.c
status_t Radeon_SetDPMS( accelerator_info *ai, int crtc_idx, int mode );
uint32 Radeon_GetDPMS( accelerator_info *ai, int crtc_idx );


// Cursor.c
void Radeon_SetCursorColors( accelerator_info *ai, int crtc_idx );
void Radeon_ShowCursor( accelerator_info *ai, int crtc_idx );


// Acceleration.c
void Radeon_Init2D( accelerator_info *ai );
void Radeon_AllocateVirtualCardStateBuffer( accelerator_info *ai );
void Radeon_FreeVirtualCardStateBuffer( accelerator_info *ai );
void Radeon_FillStateBuffer( accelerator_info *ai, uint32 datatype );


// driver_wrapper.c
status_t Radeon_WaitForIdle( accelerator_info *ai, bool keep_lock );
status_t Radeon_WaitForFifo( accelerator_info *ai, int entries );
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
void Radeon_InitOverlay( accelerator_info *ai, int crtc_idx );

// EngineManagement.c
void Radeon_Spin( uint32 delay );


// monitor_detection.c
void Radeon_DetectDisplays( accelerator_info *ai );
bool Radeon_ReadEDID( accelerator_info *ai, uint32 ddc_port, edid1_info *edid );


// palette.c
void Radeon_InitPalette( accelerator_info *ai, int crtc_idx );


// theatre_out.c
void Radeon_DetectTVOut( accelerator_info *ai );


#ifdef __cplusplus
}
#endif

#endif
