/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Takes of PLL
*/


#include "radeon_accelerant.h"

#include "pll_regs.h"
#include "utils.h"


// read value "val" from PLL-register "addr"
uint32 Radeon_INPLL( accelerator_info *ai, int addr )
{	
	vuint8 *regs = ai->regs;
	uint32 res;
	
	OUTREG8( regs, RADEON_CLOCK_CNTL_INDEX, addr & 0x3f );
	res = INREG( regs, RADEON_CLOCK_CNTL_DATA );
	
	R300_PLLFix( ai );
	return res;
}

// write value "val" to PLL-register "addr" 
void Radeon_OUTPLL( accelerator_info *ai, uint8 addr, uint32 val )
{
	vuint8 *regs = ai->regs;
	
	OUTREG8( regs, RADEON_CLOCK_CNTL_INDEX, ((addr & 0x3f ) |
		RADEON_PLL_WR_EN));

	OUTREG( regs, RADEON_CLOCK_CNTL_DATA, val );
	
	// TBD: on XFree, there is no call of R300_PLLFix here, 
	// though it should as we've accessed CLOCK_CNTL_INDEX
	//R300_PLLFix( ai );
}

// write "val" to PLL-register "addr" keeping bits "mask"
void Radeon_OUTPLLP( accelerator_info *ai, uint8 addr, 
	uint32 val, uint32 mask )
{
	uint32 tmp = Radeon_INPLL( ai, addr );
	tmp &= mask;
	tmp |= val;
	Radeon_OUTPLL( ai, addr, tmp );
}


static void Radeon_PLLWaitForReadUpdateComplete( accelerator_info *ai, virtual_port *port )
{
	int i;
	
	// we should wait forever, but
	// 1. this is unsafe
	// 2. some r300 loop forever (reported by XFree86)
	for( i = 0; i < 10000; ++i ) {
		if( (Radeon_INPLL( ai, port->is_crtc2 ? RADEON_P2PLL_REF_DIV : RADEON_PPLL_REF_DIV ) 
			& RADEON_PPLL_ATOMIC_UPDATE_R) == 0 )
			return;
	}
}

static void Radeon_PLLWriteUpdate( accelerator_info *ai, virtual_port *port )
{
	Radeon_PLLWaitForReadUpdateComplete( ai, port );
	
    Radeon_OUTPLLP( ai, port->is_crtc2 ? RADEON_P2PLL_REF_DIV : RADEON_PPLL_REF_DIV,
    	RADEON_PPLL_ATOMIC_UPDATE_W, 
    	~RADEON_PPLL_ATOMIC_UPDATE_W );
}

// r300: to be called after each CLOCK_CNTL_INDEX access
// (hardware bug fix suggested by XFree86)
void R300_PLLFix( accelerator_info *ai ) 
{
	vuint8 *regs = ai->regs;
	uint32 save, tmp;

	if( ai->si->asic != rt_r300 )
		return;
		
    save = INREG( regs, RADEON_CLOCK_CNTL_INDEX );
    tmp = save & ~(0x3f | RADEON_PLL_WR_EN);
    OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, tmp );
    tmp = INREG( regs, RADEON_CLOCK_CNTL_DATA );
    OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, save );
}


// table to map divider to register value
typedef struct {
	int divider;
	int bitvalue;
} post_div_entry;

static post_div_entry post_divs[] = {
	{  1, 0 },
	{  2, 1 },
	{  4, 2 },
	{  8, 3 },
	{  3, 4 },
	{ 16, 5 },
	{  6, 6 },
	{ 12, 7 },
	{  0, 0 }
};

// calculate PLL dividers (freq is in 10kHz)
void Radeon_CalcPLLDividers( pll_info *pll, unsigned long freq, port_regs *values )
{
	post_div_entry *post_div;
  
  	SHOW_FLOW( 2, "freq=%ld", freq );

	// formula is for generated frequency is:
	//   (ref_freq * feedback_div) / (ref_div * post_div )

	// find proper divider by trial-and-error	
	for( post_div = &post_divs[0]; post_div->divider; ++post_div ) {
		values->pll_output_freq = post_div->divider * freq;
		
		if( values->pll_output_freq >= pll->min_pll_freq
			&& values->pll_output_freq <= pll->max_pll_freq )
			break;
	}
	
	if( post_div->divider == 0 )
		SHOW_ERROR( 2, "Frequency (%d kHz) is out of PLL range!", freq );
	
	values->dot_clock_freq = freq;
	values->feedback_div   = RoundDiv( pll->ref_div * values->pll_output_freq,
	    pll->ref_freq);
	values->post_div       = post_div->divider;
	
	values->ppll_ref_div   = pll->ref_div;
	values->ppll_div_3     = (values->feedback_div | (post_div->bitvalue << 16));
	values->htotal_cntl    = 0;
	
	SHOW_FLOW( 2, "dot_clock_freq=%ld, pll_output_freq=%ld, ref_div=%d, feedback_div=%d, post_div=%d",
		values->dot_clock_freq, values->pll_output_freq,
		pll->ref_div, values->feedback_div, values->post_div );
}

// write values into PLL registers
void Radeon_ProgramPLL( accelerator_info *ai, virtual_port *port, port_regs *values )
{
	vuint8 *regs = ai->regs;
	
	SHOW_FLOW0( 2, "" );
	
	// use some other PLL for pixel clock source to not fiddling with PLL
	// while somebody is using it
    Radeon_OUTPLLP( ai, port->is_crtc2 ? RADEON_PIXCLKS_CNTL : RADEON_VCLK_ECP_CNTL, 
    	RADEON_VCLK_SRC_CPU_CLK, ~RADEON_VCLK_SRC_SEL_MASK );

    Radeon_OUTPLLP( ai,
	    port->is_crtc2 ? RADEON_P2PLL_CNTL : RADEON_PPLL_CNTL,
	    RADEON_PPLL_RESET
	    | RADEON_PPLL_ATOMIC_UPDATE_EN
	    | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN,
	    ~(RADEON_PPLL_RESET
		| RADEON_PPLL_ATOMIC_UPDATE_EN
		| RADEON_PPLL_VGA_ATOMIC_UPDATE_EN) );
		
	// select divider 3 (well, only required for first PLL)
    OUTREGP( regs, RADEON_CLOCK_CNTL_INDEX,
	    RADEON_PLL_DIV_SEL_DIV3,
	    ~RADEON_PLL_DIV_SEL_MASK );

	// probably this register doesn't need to be set as is not
	// touched by anyone (anyway - it doesn't hurt)
    Radeon_OUTPLLP( ai, 
    	port->is_crtc2 ? RADEON_P2PLL_REF_DIV : RADEON_PPLL_REF_DIV, 
    	values->ppll_ref_div, 
    	~RADEON_PPLL_REF_DIV_MASK );
    
    Radeon_OUTPLLP( ai, 
    	port->is_crtc2 ? RADEON_P2PLL_DIV_0 : RADEON_PPLL_DIV_3, 
    	values->ppll_div_3, 
    	~RADEON_PPLL_FB3_DIV_MASK );

    Radeon_OUTPLLP( ai, 
    	port->is_crtc2 ? RADEON_P2PLL_DIV_0 : RADEON_PPLL_DIV_3, 
    	values->ppll_div_3, 
    	~RADEON_PPLL_POST3_DIV_MASK );

    Radeon_PLLWriteUpdate( ai, port );
    Radeon_PLLWaitForReadUpdateComplete( ai, port );
    
    Radeon_OUTPLL( ai, 
    	port->is_crtc2 ? RADEON_HTOTAL2_CNTL : RADEON_HTOTAL_CNTL, 
    	values->htotal_cntl );

	Radeon_OUTPLLP( ai, port->is_crtc2 ? RADEON_P2PLL_CNTL : RADEON_PPLL_CNTL, 0, 
		~(RADEON_PPLL_RESET
		| RADEON_PPLL_SLEEP
		| RADEON_PPLL_ATOMIC_UPDATE_EN
		| RADEON_PPLL_VGA_ATOMIC_UPDATE_EN) );

	// there is no way to check whether PLL has settled, so wait a bit
	snooze( 5000 );

	// use PLL for pixel clock again
    Radeon_OUTPLLP( ai, port->is_crtc2 ? RADEON_PIXCLKS_CNTL : RADEON_VCLK_ECP_CNTL, 
    	RADEON_VCLK_SRC_PPLL_CLK, ~RADEON_VCLK_SRC_SEL_MASK );
}
