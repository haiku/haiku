/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon driver and accelerant
		
	Basic access of PLL registers
*/

#include "radeon_interface.h"
#include "pll_access.h"
#include "pll_regs.h"
#include "utils.h"

// read value "val" from PLL-register "addr"
uint32 Radeon_INPLL( vuint8 *regs, radeon_type asic, int addr )
{	
	uint32 res;
	
	OUTREG8( regs, RADEON_CLOCK_CNTL_INDEX, addr & 0x3f );
	res = INREG( regs, RADEON_CLOCK_CNTL_DATA );
	
	R300_PLLFix( regs, asic );
	return res;
}

// write value "val" to PLL-register "addr" 
void Radeon_OUTPLL( vuint8 *regs, radeon_type asic, uint8 addr, uint32 val )
{
	(void)asic;
	
	OUTREG8( regs, RADEON_CLOCK_CNTL_INDEX, ((addr & 0x3f ) |
		RADEON_PLL_WR_EN));

	OUTREG( regs, RADEON_CLOCK_CNTL_DATA, val );
	
	// TBD: on XFree, there is no call of R300_PLLFix here, 
	// though it should as we've accessed CLOCK_CNTL_INDEX
	//R300_PLLFix( ai );
}

// write "val" to PLL-register "addr" keeping bits "mask"
void Radeon_OUTPLLP( vuint8 *regs, radeon_type asic, uint8 addr, 
	uint32 val, uint32 mask )
{
	uint32 tmp = Radeon_INPLL( regs, asic, addr );
	tmp &= mask;
	tmp |= val;
	Radeon_OUTPLL( regs, asic, addr, tmp );
}

// r300: to be called after each CLOCK_CNTL_INDEX access
// (hardware bug fix suggested by XFree86)
void R300_PLLFix( vuint8 *regs, radeon_type asic ) 
{
	uint32 save, tmp;

	if( asic != rt_r300 )
		return;
		
    save = INREG( regs, RADEON_CLOCK_CNTL_INDEX );
    tmp = save & ~(0x3f | RADEON_PLL_WR_EN);
    OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, tmp );
    tmp = INREG( regs, RADEON_CLOCK_CNTL_DATA );
    OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, save );
}
