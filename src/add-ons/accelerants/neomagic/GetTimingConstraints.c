/*
	Authors:
	Mark Watson - 21/6/00,
	Apsed
*/

#define MODULE_BIT 0x01000000

#include "acc_std.h"

/* Used to help generate mode lines */
status_t GET_TIMING_CONSTRAINTS(display_timing_constraints * dtc) 
{
	// apsed, TODO, is that following card capabilities ?? 
	LOG(4, ("GET_TIMING_CONSTRAINTS\n"));
	
	dtc->h_res=8;
	dtc->h_sync_min=8;
	dtc->h_sync_max=248;
	dtc->h_blank_min=8;
	dtc->h_blank_max=504;
	
	dtc->v_res=1;
	dtc->v_sync_min=1;
	dtc->v_sync_max=15;
	dtc->v_blank_min=1;
	dtc->v_blank_max=255;

	return B_OK;
}
