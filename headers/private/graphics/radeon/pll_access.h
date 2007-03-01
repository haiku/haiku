/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Basic PLL registers access
*/

#ifndef _PLL_ACCESS_H
#define _PLL_ACCESS_H

#include "mmio.h"


// to be called after each CLOCK_CNTL_INDEX access;
// all functions declared in this header take care of that
// (hardware bug fix suggested by XFree86)
void RADEONPllErrataAfterIndex( vuint8 *regs, radeon_type asic );

// to be called after each CLOCK_CNTL_DATA access;
// all functions declared in this header take care of that
// (hardware bug fix suggested by XFree86)
void RADEONPllErrataAfterData( vuint8 *regs, radeon_type asic );

// in general:
// - the PLL is connected via special port
// - you need first to choose the PLL register and then write/read its value
//
// if atomic updates are not safe we:
// - verify each time whether the right register is chosen
// - verify all values written to PLL-registers


// read value "val" from PLL-register "addr"
uint32 Radeon_INPLL( vuint8 *regs, radeon_type asic, int addr );

// write value "val" to PLL-register "addr" 
void Radeon_OUTPLL( vuint8 *regs, radeon_type asic, uint8 addr, uint32 val );

// write "val" to PLL-register "addr" keeping bits "mask"
void Radeon_OUTPLLP( vuint8 *regs, radeon_type asic, uint8 addr, uint32 val, uint32 mask );

#endif
