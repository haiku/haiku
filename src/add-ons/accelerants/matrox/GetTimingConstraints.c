/*
	Author:
	Rudolf Cornelissen 11/2004
*/

#define MODULE_BIT 0x01000000

#include "acc_std.h"

/* Used to help generate mode lines */
status_t GET_TIMING_CONSTRAINTS(display_timing_constraints * dtc) 
{
	LOG(4, ("GET_TIMING_CONSTRAINTS: returning info\n"));

	/* specs are identical for all Matrox cards on CRTC1 (Mil-1 - G550) */
	/* Note: G400-G550 CRTC2 have wider constraints, so this is OK. */
	dtc->h_res = 8;
	dtc->h_sync_min = 8;
	dtc->h_sync_max = 248;
	/* Note:
	 * h_blank info is used to determine the max. diff. between h_total and h_display! */
	dtc->h_blank_min = 8;
	dtc->h_blank_max = 1016;

	dtc->v_res = 1;
	dtc->v_sync_min = 1;
	dtc->v_sync_max = 15;
	/* Note:
	 * v_blank info is used to determine the max. diff. between v_total and v_display! */
	dtc->v_blank_min = 1;
	dtc->v_blank_max = 255;

	return B_OK;
}
