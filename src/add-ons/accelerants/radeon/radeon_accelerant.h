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

#define DEBUG_MSG_PREFIX "Radeon - "

//#define DEBUG_MAX_LEVEL_FLOW 2

#include "debug_ext.h"

typedef struct accelerator_info {
	virtual_card *vc;
	vuint8 *regs;				// pointer to mapped registers
								// !! dont't make it vuint32, access macros rely on 8 bits !!
	area_id shared_info_area;
	area_id regs_area;
	area_id virtual_card_area;
	int accelerant_is_clone;
		
	int fd;						// file descriptor of kernel driver
	struct log_info_t *log;
	
	area_id mode_list_area;		// cloned list of standard display modes
	display_mode *mode_list;
	shared_info *si;
} accelerator_info;


uint32 Radeon_RoundVWidth( int virtual_width, int bpp );
uint16 Radeon_GetHSyncFudge( shared_info *si, physical_port *port, int datatype );
void Radeon_HideMultiMode( virtual_card *vc, display_mode *mode );
bool Radeon_GetFormat( int space, int *format, int *bpp );
status_t Radeon_CreateModeList( shared_info *si );

void Radeon_DetectMultiMode( virtual_card *vc, display_mode *mode );
void Radeon_VerifyMultiMode( virtual_card *vc, shared_info *si, display_mode *mode );
void Radeon_InitMultiModeVars( virtual_card *vc, display_mode *mode );
status_t Radeon_CheckMultiMonTunnel( virtual_card *vc, display_mode *mode, 
	const display_mode *low, const display_mode *high, bool *isTunnel );
bool Radeon_NeedsSecondPort( display_mode *mode );
bool Radeon_DifferentPorts( display_mode *mode );
	
void Radeon_CalcCRTCRegisters( accelerator_info *ai, virtual_port *port, 
	display_mode *mode, port_regs *values );
void Radeon_ProgramCRTCRegisters( accelerator_info *ai, virtual_port *port, 
	port_regs *values );
	
void Radeon_CalcPLLDividers( pll_info *pll, unsigned long freq, port_regs *values );
void Radeon_ProgramPLL( accelerator_info *ai, virtual_port *port, port_regs *values );

void Radeon_CalcFPRegisters( accelerator_info *ai, virtual_port *port, fp_info *fp_port, display_mode *mode, port_regs *values );
void Radeon_ProgramFPRegisters( accelerator_info *ai, fp_info *fp_port, port_regs *values );
status_t Radeon_ReadFPEDID( accelerator_info *ai, shared_info *si );

status_t Radeon_SetDPMS( accelerator_info *ai, virtual_port *port, int mode );
uint32 Radeon_GetDPMS( accelerator_info *ai, virtual_port *port );

void Radeon_SetCursorColors( accelerator_info *ai, virtual_port *port );

void Radeon_Init2D( accelerator_info *ai, uint32 datatype );

int Radeon_WaitForIdle( accelerator_info *ai );
void Radeon_ResetEngine( accelerator_info *ai );
void Radeon_SendWaitUntilIdle( accelerator_info *ai );
void Radeon_SendPurgeCache( accelerator_info *ai );
void Radeon_WaitForFifo( accelerator_info *ai, int entries );
void Radeon_Finish( accelerator_info *ai );

status_t Radeon_InitCP( accelerator_info *ai );
void Radeon_SendCP( accelerator_info *ai, uint32 *buffer, uint32 num_dwords );
void Radeon_WriteRegCP( accelerator_info *ai, uint32 reg, uint32 value );

void Radeon_ActivateVirtualCard( accelerator_info *ai );

void Radeon_ReadSettings( virtual_card *vc );
void Radeon_WriteSettings( virtual_card *vc );

void Radeon_HideOverlay( accelerator_info *ai );
status_t Radeon_UpdateOverlay( accelerator_info *ai );
void Radeon_SetColourKey( accelerator_info *ai, const overlay_window *ow );

status_t Radeon_MoveDisplay( accelerator_info *ai, uint16 h_display_start, uint16 v_display_start );

#ifdef __cplusplus
}
#endif

#endif
