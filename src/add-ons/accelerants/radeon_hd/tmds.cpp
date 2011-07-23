/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"
#include "tmds.h"


#define TRACE_TMDS
#ifdef TRACE_TMDS
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


bool
TMDSSense(uint8 tmdsIndex)
{
	// For now radeon cards only have TMDSA and no TMDSB

	// Backup current TMDS values
	uint32 loadDetect = Read32(OUT, TMDSA_LOAD_DETECT);

	// Call TMDS load detect on TMDSA
	Write32Mask(OUT, TMDSA_LOAD_DETECT, 0x00000001, 0x00000001);
	snooze(1);

	// Check result of TMDS load detect
	bool result = Read32(OUT, TMDSA_LOAD_DETECT) & 0x00000010;

	// Restore saved value
	Write32Mask(OUT, TMDSA_LOAD_DETECT, loadDetect, 0x00000001);

	return result;
}


