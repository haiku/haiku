/* All this code has been reversed engineered from Xfree86 Project :-) 
   
   Copyright 1999 by

   Rene MacKinney <rene_@freenet.co.uk>

   7.May.99

*/

//------------------------------------------------------------------------
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach64/mach64.c,v 3.62.2.15 19
98/10/18 20:42:04 hohndel Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993,1994,1995,1996,1997 by Kevin E. Martin, Chapel Hill, North Car
olina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL, KEVIN E. MARTIN, AND RICKARD E. FAITH DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Rewritten for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 * Rewritten for the Mach64 by Kevin E. Martin (martin@cs.unc.edu)
 * Support for the Mach64 CT added by David Dawes (dawes@XFree86.org)
 *
 */
/* $XConsortium: mach64.c /main/34 1996/10/28 04:46:47 kaleb $ */


//---------------------------------------------------------------------------

#include <GraphicsDefs.h>

#include "GlobalData.h"
#include "generic.h"
#include "Mach64fifo.h"
#include "Mach64.h"


/*
 * mach64FIFOdepth --
 *	Calculates the correct FIFO depth for the Mach64 depending on the
 *	color depth and clock selected.
 */
int mach64FIFOdepth(cdepth, clock, width)
    int cdepth;
    int clock;
    int width;
{
    int fifo_depth;

    if (si->device_id == MACH64_VT_ID) {
	if (si->revision == 0x48) { /* VTA4 */
	    fifo_depth = mach64FIFOdepthVTA4(cdepth, clock, width);
	} else { /* VTA3 */
	    fifo_depth = mach64FIFOdepthVTA3(cdepth, clock, width);
	}
    } else if (si->device_id == MACH64_GT_ID) {
	fifo_depth = mach64FIFOdepthGT(cdepth, clock, width);
    } else if (si->device_id == MACH64_CT_ID && si->revision == 0x0a) {
	/* CT-D has a larger FIFO and thus requires special code */
	fifo_depth = mach64FIFOdepthCTD(cdepth, clock, width);
    } else if (si->device_id == MACH64_CT_ID ||
	        si->device_id == MACH64_ET_ID) {
	fifo_depth = mach64FIFOdepthCT(cdepth, clock, width);
    } else {
	fifo_depth = mach64FIFOdepthDefault(cdepth, clock, width);
    }

    return(fifo_depth);
}

/*
 * mach64ProgramClkMach64CT --
 *
 */
void mach64ProgramClkMach64CT(clkCntl, MHz100)
    int clkCntl;
    int MHz100;
{
    char old_crtc_ext_disp;
#ifdef DEBUG
    extern void mach64PrintCTPLL();
#endif
    int M, N, P, R;
    float Q;
    int postDiv;
    int mhz100 = MHz100;
    unsigned char tmp1, tmp2;
    int ext_div = 0;
    float current_dot_clock;

    old_crtc_ext_disp = inb(CRTC_GEN_CNTL+3);
    outb(CRTC_GEN_CNTL+3, old_crtc_ext_disp | (CRTC_EXT_DISP_EN >> 24));

    M = si->RefDivider;
    R = si->RefFreq;

    if (clkCntl > 3) clkCntl = 3;

    if (mhz100 < si->MinFreq) mhz100 = si->MinFreq;
    if (mhz100 > si->MaxFreq) mhz100 = si->MaxFreq;

    Q = (mhz100 * M)/(2.0 * R);

    if (false) { /* mach64HasDSP) { */
	if (Q > 255) {
	  /*    printf("mach64ProgramClkMach64CT: Warning: Q > 255\n");*/
	    Q = 255;
	    P = 0;
	    postDiv = 1;
	} else if (Q > 127.5) {
	    P = 0;
	    postDiv = 1;
	} else if (Q > 85) {
	    P = 1;
	    postDiv = 2;
	} else if (Q > 63.75) {
	    P = 0;
	    postDiv = 3;
	    ext_div = 1;
	} else if (Q > 42.5) {
	    P = 2;
	    postDiv = 4;
	} else if (Q > 31.875) {
	    P = 2;
	    postDiv = 6;
	    ext_div = 1;
	} else if (Q > 21.25) {
	    P = 3;
	    postDiv = 8;
	} else if (Q >= 10.6666666667) {
	    P = 3;
	    postDiv = 12;
	    ext_div = 1;
	} else {
	  /*	    printf("mach64ProgramClkMach64CT: Warning: Q < 10.66666667\n");*/
	    P = 3;
	    postDiv = 12;
	    ext_div = 1;
	}
    } else {
	if (Q > 255) {
	  /*	    printf("mach64ProgramClkMach64CT: Warning: Q > 255\n");*/
	    Q = 255;
	    P = 0;
	}
	else if (Q > 127.5)
	    P = 0;
	else if (Q > 63.75)
	    P = 1;
	else if (Q > 31.875)
	    P = 2;
	else if (Q >= 16)
	    P = 3;
	else {
	  /*	    printf("mach64ProgramClkMach64CT: Warning: Q < 16\n");*/
	    P = 3;
	}
	postDiv = 1 << P;
    }
    N = (int)(Q * postDiv + 0.5);

    current_dot_clock = (2.0 * R * N)/(M * postDiv);

#ifdef DEBUG_HARNESS
        printf("Q = %f N = %d P = %d, postDiv = %d R = %d M = %d\n", Q, N, P, postDiv, R, M);
	  printf("New freq: %.2f\n", (double)((2 * R * N)/(M * postDiv)) / 100.0);
#endif

    outb(CLOCK_CNTL + 1, PLL_VCLK_CNTL << 2);
#ifdef DEBUG_HARNESS
        printf("CLOCK_CNTL + 1 = 0x%08x\n",PLL_VCLK_CNTL << 2);
#endif    
    tmp1 = inb(CLOCK_CNTL + 2);
    outb(CLOCK_CNTL + 1, (PLL_VCLK_CNTL  << 2) | PLL_WR_EN);
    outb(CLOCK_CNTL + 2, tmp1 | 0x04);
    outb(CLOCK_CNTL + 1, VCLK_POST_DIV << 2);
    tmp2 = inb(CLOCK_CNTL + 2);
    outb(CLOCK_CNTL + 1, ((VCLK0_FB_DIV + clkCntl) << 2) | PLL_WR_EN);
    outb(CLOCK_CNTL + 2, N);
    outb(CLOCK_CNTL + 1, (VCLK_POST_DIV << 2) | PLL_WR_EN);
    outb(CLOCK_CNTL + 2,
	 (tmp2 & ~(0x03 << (2 * clkCntl))) | (P << (2 * clkCntl)));
    outb(CLOCK_CNTL + 1, (PLL_VCLK_CNTL << 2) | PLL_WR_EN);
    outb(CLOCK_CNTL + 2, tmp1 & ~0x04);

    if (false) { /* mach64HasDSP) { */
	outb(CLOCK_CNTL + 1, PLL_XCLK_CNTL << 2);
	tmp1 = inb(CLOCK_CNTL + 2);
	outb(CLOCK_CNTL + 1, (PLL_XCLK_CNTL << 2) | PLL_WR_EN);
	if (ext_div)
	    outb(CLOCK_CNTL + 2, tmp1 | (1 << (clkCntl + 4)));
	else
	    outb(CLOCK_CNTL + 2, tmp1 & ~(1 << (clkCntl + 4)));
    }

    snooze(5000);

    (void)inb(DAC_REGS); /* Clear DAC Counter */
    outb(CRTC_GEN_CNTL+3, old_crtc_ext_disp);

    return;
}

/*
 * CalcCRTCRegs --
 *	Calculate appropiate values for CRTC Registers
 */
void 
CalcCRTCRegs(crtcRegs, mode)
     mach64CRTCRegPtr crtcRegs;
     display_mode *mode;
{
    int h_pol = (mode->timing.flags & B_POSITIVE_HSYNC) ? 1 : 0;
    int v_pol = (mode->timing.flags & B_POSITIVE_VSYNC) ? 1 : 0;

    /* Register CRTC_H_TOTAL_DISP 
       bits 23-16 Horizontal total visible in character clocks (8 pixel units)
       bits 7-0 Horizontal display end in character clocks
    */
    crtcRegs->h_total_disp = (((mode->timing.h_display >> 3) - 1) << 16) |
		((mode->timing.h_total >> 3) - 1);

    /* Register CRTC_H_SYNC_STRT_WID
       bits 21    Horizontal sync polarity
       bits 20-16 Horizontal sync width in character clocks 
       bits 10-8  Horizontal sync start delay in pixels
       bits 7-0   Horizontal sync start in character clocks
    */
    crtcRegs->h_sync_strt_wid = h_pol << 21 |
      (((mode->timing.h_sync_end - mode->timing.h_sync_start) >> 3) << 16) |
      /*		((mode->timing.h_sync_start % 8) << 8)  |*/
		((mode->timing.h_sync_start >> 3) - 1);

    /* Register CRTC_V_TOTAL_DISP 
       bits 23-16 Vertical total visible in lines
       bits 7-0 Vertical display end in lines 
    */
    crtcRegs->v_total_disp = ((mode->timing.v_display - 1) << 16) |
		(mode->timing.v_total - 1);

    /* Register CRTC_V_SYNC_STRT_WID
       bits 21    Vertical sync polarity
       bits 20-16 Vertical sync width in lines
       bits 10-0  Vertical sync start in lines
    */
    crtcRegs->v_sync_strt_wid = v_pol << 21 |
      ((mode->timing.v_sync_end - mode->timing.v_sync_start) << 16) |
		(mode->timing.v_sync_start - 1);

    switch (mode->space & 0x0fff) {
       case B_CMAP8:
	 crtcRegs->color_depth = CRTC_PIX_WIDTH_8BPP;
	 break;
       case B_RGB15:
	 crtcRegs->color_depth = CRTC_PIX_WIDTH_15BPP;
	 break;
       case B_RGB16:
	 crtcRegs->color_depth = CRTC_PIX_WIDTH_16BPP;
	 break;
       case B_RGB32:
	 crtcRegs->color_depth = CRTC_PIX_WIDTH_32BPP;
	 break;
       default:
    }

    crtcRegs->crtc_gen_cntl = 0;
    if (mode->timing.flags & B_TIMING_INTERLACED)
	crtcRegs->crtc_gen_cntl |= CRTC_INTERLACE_EN;


    crtcRegs->clock_cntl = 0x03; /* Hack */
    crtcRegs->dot_clock = mode->timing.pixel_clock / 10;

    crtcRegs->fifo_v1 =	mach64FIFOdepth(crtcRegs->color_depth,
					crtcRegs->dot_clock, mode->timing.h_display);

    


}

/*
 * SetCRTCRegs
 * Write values to the CRTC Registers  */
void 
SetCRTCRegs(crtcRegs, mode)
     mach64CRTCRegPtr crtcRegs;
     display_mode *mode;
{
    int crtcGenCntl;
    uint32 offset;

    WaitIdleEmpty();
    crtcGenCntl = inw(CRTC_GEN_CNTL);
    outw(CRTC_GEN_CNTL, crtcGenCntl & ~(CRTC_EXT_EN | CRTC_LOCK_REGS));
    
    /* Program the clock chip */
    mach64ProgramClkMach64CT(0x03, crtcRegs->dot_clock);


    /* Horizontal CRTC registers */
    outw(CRTC_H_TOTAL_DISP,    crtcRegs->h_total_disp);
    outw(CRTC_H_SYNC_STRT_WID, crtcRegs->h_sync_strt_wid);

    /* Vertical CRTC registers */
    outw(CRTC_V_TOTAL_DISP,    crtcRegs->v_total_disp);
    outw(CRTC_V_SYNC_STRT_WID, crtcRegs->v_sync_strt_wid);

    /* Clock select register */
    outw(CLOCK_CNTL, crtcRegs->clock_cntl | CLOCK_STROBE);

    /* Zero overscan register to insure proper color */
    /*    outw(OVR_CLR, 0);
    outw(OVR_WID_LEFT_RIGHT, 0);
    outw(OVR_WID_TOP_BOTTOM, 0);
    */
    /* Set the offset and width of the display 
       We could just use the offset value we already know (1024) but this
       will deal with any changes
    */
    offset = (si->fbc.frame_buffer - si->framebuffer) >> 3;

    outw(CRTC_OFF_PITCH, ((mode->virtual_width >> 3) << 22) | offset);
    outw(DST_OFF_PITCH, ((mode->virtual_width >> 3) << 22) | offset);
    outw(SRC_OFF_PITCH, ((mode->virtual_width >> 3) << 22) | offset);



    /* Display control register -- this one turns on the display */

    outw(CRTC_GEN_CNTL,
         (crtcGenCntl & 0xff0000ff &
                   ~(CRTC_PIX_BY_2_EN | CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
                     CRTC_HSYNC_DIS | CRTC_VSYNC_DIS)) |
           (crtcRegs->crtc_gen_cntl & ~CRTC_PIX_BY_2_EN) |
           crtcRegs->color_depth |
           (((crtcRegs->fifo_v1 & 0x0f) << 16) |
           CRTC_EXT_DISP_EN | CRTC_EXT_EN));

    /*    printf(" crtcGenCntl 0x%08x\n", (crtcGenCntl & 0xff0000ff &
                   ~(CRTC_PIX_BY_2_EN | CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
                     CRTC_HSYNC_DIS | CRTC_VSYNC_DIS)) |
           (crtcRegs->crtc_gen_cntl & ~CRTC_PIX_BY_2_EN) |
           crtcRegs->color_depth |
           (((crtcRegs->fifo_v1 & 0x0f) << 16) |
           CRTC_EXT_DISP_EN | CRTC_EXT_EN));*/
 

    /* Set the DAC for the currect mode */
    /*    mach64SetRamdac(crtcRegs->color_depth, TRUE, crtcRegs->dot_clock);
     */
    WaitIdleEmpty();
}

/*
 * mach64InitAperture --
 *	Initialize the aperture for the Mach64.
 */
void mach64InitAperture()
{
    ulong apaddr = (ulong) si->framebuffer_pci;

   outw(CONFIG_CNTL, ((apaddr/(4*1024*1024)) << 4) | 2);

}

/*
 * mach64ResetEngine --
 *	Resets the GUI engine and clears any FIFO errors.
 */
void mach64ResetEngine()
{
    int temp;

    /* Ensure engine is not locked up by clearing any FIFO errors */
    outw(BUS_CNTL, inw(BUS_CNTL) | BUS_HOST_ERR_ACK | BUS_FIFO_ERR_ACK);

    /* Reset engine */
    temp = inw(GEN_TEST_CNTL);
    outw(GEN_TEST_CNTL, temp & ~GUI_ENGINE_ENABLE);

    outw(GEN_TEST_CNTL, temp | GUI_ENGINE_ENABLE);

    WaitIdleEmpty();
}



void
InitRAMDAC(uint32 colorDepth, uint32 flags)
{
  int count;
  uint8 val;

  WaitIdleEmpty();

  /* Initialize Acceleration Mode */

  outb(CRTC_GEN_CNTL+3, ((CRTC_EXT_DISP_EN | CRTC_EXT_EN) >> 24));
  
  switch(colorDepth){
      case 8:
        val = inb(DAC_CNTL+1);
        if (flags & B_8_BIT_DAC)
          outb(DAC_CNTL + 1, val | 0x01);
        else
          outb(DAC_CNTL + 1, val & ~0x01);
        break;
      case 15:
      case 16:
      case 24:
      case 32:
        val = inb(DAC_CNTL + 1);
        outb(DAC_CNTL + 1, val | 0x01);
        outb(DAC_CNTL + 2, 0xff);
        outb(DAC_REGS, 0x00);
        for(count = 0; count < 256; count ++) {
          outb(DAC_REGS+1, count);
          outb(DAC_REGS+1, count);
          outb(DAC_REGS+1, count);
        }
        
        break;
  }
  
}
