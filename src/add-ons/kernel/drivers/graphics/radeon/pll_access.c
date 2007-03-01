/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon driver and accelerant
		
	Basic access of PLL registers
*/

#include "radeon_interface.h"
#include "pll_access.h"
#include "pll_regs.h"
#include "crtc_regs.h"
#include "utils.h"

void RADEONPllErrataAfterIndex( vuint8 *regs, radeon_type asic )
{
	if (! ((asic == rt_rv200) || (asic == rt_rs200)))
	return;

    /* This workaround is necessary on rv200 and RS200 or PLL
     * reads may return garbage (among others...)
     */
    INREG( regs, RADEON_CLOCK_CNTL_DATA);
    INREG( regs, RADEON_CRTC_GEN_CNTL);
}

void RADEONPllErrataAfterData( vuint8 *regs, radeon_type asic )
{
	uint32 save, tmp;

    /* This workarounds is necessary on RV100, RS100 and RS200 chips
     * or the chip could hang on a subsequent access
     */
    if ((asic == rt_rv100) || (asic == rt_rs100) || (asic == rt_rs200))
    {
		/* we can't deal with posted writes here ... */
		snooze(5000);
    }

    /* This function is required to workaround a hardware bug in some (all?)
     * revisions of the R300.  This workaround should be called after every
     * CLOCK_CNTL_INDEX register access.  If not, register reads afterward
     * may not be correct.
     */
 
	if( asic != rt_r300 )
		return;
		
    save = INREG( regs, RADEON_CLOCK_CNTL_INDEX );
    tmp = save & ~(0x3f | RADEON_PLL_WR_EN);
    OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, tmp );
    tmp = INREG( regs, RADEON_CLOCK_CNTL_DATA );
    OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, save );
    
}

// read value "val" from PLL-register "addr"
uint32 Radeon_INPLL( vuint8 *regs, radeon_type asic, int addr )
{	
	uint32 res;
	
	OUTREG8( regs, RADEON_CLOCK_CNTL_INDEX, addr & 0x3f );
	RADEONPllErrataAfterIndex(regs, asic);
	res = INREG( regs, RADEON_CLOCK_CNTL_DATA );
	RADEONPllErrataAfterData(regs, asic);
	return res;
}

// write value "val" to PLL-register "addr" 
void Radeon_OUTPLL( vuint8 *regs, radeon_type asic, uint8 addr, uint32 val )
{
	(void)asic;
	
	OUTREG8( regs, RADEON_CLOCK_CNTL_INDEX, ((addr & 0x3f ) |
		RADEON_PLL_WR_EN));
	RADEONPllErrataAfterIndex(regs, asic);
	OUTREG( regs, RADEON_CLOCK_CNTL_DATA, val );
	RADEONPllErrataAfterData(regs, asic);

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
