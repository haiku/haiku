/*
	Program the Texas TVP3026
	using Texas Instrument TVP3026 manual SLA098B July 1996

	Authors:
	Apsed May 2002 plus a lot of time after;
	Rudolf Cornelissen 3/2003.
*/

#define MODULE_BIT 0x00010000

#include <OS.h> // system_time, snooze 
#include "mga_std.h"

#define FVCO_MIN  110.00000 // MHz
#define FPLL_MCLK 100.00000 // MHz

//r: ??
#define MGAVGA_INSTS0       0x1FC2

#define WAIT_FOR_PLL_LOCK( pll, on_error) do { \
	bigtime_t start, now; \
	float     delay; \
	int       tmo; \
	\
	start = system_time(); \
	for (tmo = 0; tmo < 100 * 1000 * 1000; tmo++) { \
		int status = DXIR (pll ## PLLDATA); \
		if (status & 0x40) break; \
		/* snooze(10); */ \
	} \
	now = system_time(); \
	delay = (now - start) / 1000.0; \
	LOG(2,("mil2 %s PLL locked in %fms for %d loops\n", #pll, delay, tmo)); \
	if (tmo == 100 * 1000 * 1000) { \
		LOG(8,("mil2 %s PLL not locked in %fms for %d loops\n", #pll, delay, tmo)); \
		on_error; \
	} \
} while (0)

/*program the pixpll - frequency in MHz*/
status_t mil2_dac_set_pix_pll (float f_need, int bpp)
{
	uint8    n, m, p, q;
	int      z, k;
	float    fd;
	status_t result;

	display_mode target;
	target.timing.pixel_clock = (f_need * 1000);
	
	LOG(4,("mil2_dac_set_pix_pll need %fMHz, %dbpp\n", f_need, bpp));
	result = gx00_dac_pix_pll_find(target, &fd, &m, &n, &p, 0);
	if (result != B_OK) return result;

	// follows (!strictly) Appendix C, extended mode setup 
	// 1st stop the PLLs, 
	switch (bpp) {
		case  8: DXIW(TVP_CLOCKSEL, 0x25); break;
		case 16: DXIW(TVP_CLOCKSEL, 0x15); break;
		case 24: DXIW(TVP_CLOCKSEL, 0x25); break;
		case 32: DXIW(TVP_CLOCKSEL, 0x05); break;
		default: return B_ERROR;
	}
	DXIW(TVP_PLLADDR, 0x2a);        // 0x2c: 2.4 select P to ...
	DXIW(TVP_LOOPLLDATA, 0x00);     // 0x2f: 2.4.1 ... stop the loop PLL
	DXIW(TVP_PIXPLLDATA, 0x00);     // 0x2d: 2.4.1 ... stop the pixel PLL
	VGAW (MISCW, VGAR(MISCR) | 0x0c); // PLLSEL(1,0) set to 1x 
	
	// 2nd setup the pixel PLL
	LOG(2,("mil2_dac_set_pix_pll pix PLL, nmp 0x%02x 0x%02x 0x%02x\n",
		n, m, p));
	DXIW(TVP_PLLADDR, 0x00);        // 0x2c: 2.4 select N to ...
	DXIW(TVP_PIXPLLDATA, n | 0xc0); // 0x2d: ... load n, m, p and ...
	DXIW(TVP_PIXPLLDATA, m);
	DXIW(TVP_PIXPLLDATA, p | 0xb0);
	WAIT_FOR_PLL_LOCK (TVP_PIX, return B_ERROR); // ... wait for PLL lock
	
	// now compute parameters for the loop PLL (24bpp not available) see 2.4.3.1
	k = 1; // ?? external division factor between RCLK and LCLK
//	does 32bit DAC path exists for MIL1/2? if so, do this via si->ps...
//	n = (65 - (4 * (PIXEL_BUS_WIDTH64? 64: 32)) / bpp);
	n = (65 - (4 * 64) / bpp);
	m = 61;
	z = (FVCO_MIN * (65 - n)) / (4 * fd * k);
	q = 0;
	if      (z <  2) p = 0; // this is TRUNC (log2 (z))
	else if (z <  4) p = 1;
	else if (z <  8) p = 2; 
	else if (z < 16) p = 3;
	else { // but not this
		p = 3;
		q = 1 + (z - 16) / 16;
	}
	LOG(2,("mil2_dac_set_pix_pll loop PLL, nmpq 0x%02x 0x%02x 0x%02x 0x%02x\n",
		n, m, p, q));
	DXIW(TVP_MEMCLKCTRL, (DXIR(TVP_MEMCLKCTRL) & 0xf8) | q | 0x20); // 0x39: 2.4.2 table 2.13
	LOG(2,("mil2_dac_set_pix_pll loop PLL, nmpq 0x%02x 0x%02x 0x%02x 0x%02x\n",
		n, m, p, q));

	// now setup the loop PLL
	LOG(2,("mil2_dac_set_pix_pll loop PLL, nmpq 0x%02x 0x%02x 0x%02x 0x%02x\n",
		n, m, p, q));
	DXIW(TVP_PLLADDR, 0x00);        // 0x2c: 2.4 select N to ...
	DXIW(TVP_LOOPLLDATA, n | 0xc0); // 0x2f: ... load n, m, p and ...
	DXIW(TVP_LOOPLLDATA, m);
	DXIW(TVP_LOOPLLDATA, p | 0xf0);
	WAIT_FOR_PLL_LOCK (TVP_LOO, return B_ERROR); // ... wait for PLL lock

	return B_OK;
}

// set MEM clock MCLK , shall be <= 100MHz
static status_t mil2_dac_set_mem_pll (float f_need, float *mclk)
{
	status_t status;
	uint8    n, m, p;
	uint8    n_pix, m_pix, p_pix; 
	uint8    memclkctrl;

	display_mode target;
	target.timing.pixel_clock = (f_need * 1000);
	
	LOG(4,("mil2_dac_set_sys_pll need %fMHz\n", f_need));
	//fixme: MIL has same restrictions for pixel and system PLL, so Apsed did this:
	status = gx00_dac_pix_pll_find(target, mclk, &m, &n, &p, 0);
	if (status != B_OK) return status;

	// follows (!strictly) TVP3026 2.4.2.1, extended mode setup 

	// 0) save PIXPLL nmp to restore it at end
	DXIW(TVP_PLLADDR, 0x00);
	n_pix = DXIR(TVP_PIXPLLDATA);
	DXIW(TVP_PLLADDR, 0x15);
	m_pix = DXIR(TVP_PIXPLLDATA);
	DXIW(TVP_PLLADDR, 0x2a);
	p_pix = DXIR(TVP_PIXPLLDATA);
	
	// 1) disable pixel PLL, set pixel PLL at MCLK freq and poll for lock
	DXIW(TVP_PLLADDR, 0x2a);        // 0x2c: 2.4 select P to ...
	DXIW(TVP_PIXPLLDATA, 0x00);     // 0x2d: 2.4.1 ... stop the PLL
	DXIW(TVP_PLLADDR, 0x00);        // 0x2c: 2.4 select N to ...
	DXIW(TVP_PIXPLLDATA, n | 0xc0); // 0x2d: ... load n, m, p and ...
	DXIW(TVP_PIXPLLDATA, m);
	DXIW(TVP_PIXPLLDATA, p | 0xb0);
	WAIT_FOR_PLL_LOCK (TVP_PIX, return B_ERROR); // ... wait for PLL lock
	
	// 2) select pixel clock as dot clock source 
	VGAW (MISCW, VGAR(MISCR) | 0x0c); // PLLSEL(1,0) set to 1x 
	
	// 3) output dot clock on MCLK pin
	memclkctrl = DXIR(TVP_MEMCLKCTRL) & 0xe7;
	DXIW(TVP_MEMCLKCTRL, memclkctrl | (0x00 << 3));
	DXIW(TVP_MEMCLKCTRL, memclkctrl | (0x01 << 3));
	
	// 4) disable mem PLL, set mem PLL at MCLK freq and poll for lock
	DXIW(TVP_PLLADDR, 0x2a);
	DXIW(TVP_MEMPLLDATA, 0x00);
	DXIW(TVP_PLLADDR, 0x00);
	DXIW(TVP_MEMPLLDATA, n | 0xc0);
	DXIW(TVP_MEMPLLDATA, m);
	DXIW(TVP_MEMPLLDATA, p | 0xb0);
	WAIT_FOR_PLL_LOCK (TVP_MEM, return B_ERROR);

	// 5) output mem clock on MCLK pin
	DXIW(TVP_MEMCLKCTRL, memclkctrl | (0x02 << 3));
	DXIW(TVP_MEMCLKCTRL, memclkctrl | (0x03 << 3));
	
	// 6) restaure pixel clock as it was
	DXIW(TVP_PLLADDR, 0x2a);
	DXIW(TVP_PIXPLLDATA, 0x00);
	DXIW(TVP_PLLADDR, 0x00);
	DXIW(TVP_PIXPLLDATA, n_pix | 0xc0);
	DXIW(TVP_PIXPLLDATA, m_pix);
	DXIW(TVP_PIXPLLDATA, p_pix | 0xb0);
	WAIT_FOR_PLL_LOCK (TVP_PIX, return B_ERROR);
	
	return B_OK;
}

/* initialize TVP3026: memory clock PLL ... */
status_t mil2_dac_init (void)
{
	status_t status;
	float    need = 60.0; // MHz FPLL_MCLK
	float    mclk;
	uint32   option;
	uint32   rfhcnt, nogscale, memconfig;
	
	LOG(4, ("mil2_dac_init MISC 0x%02x\n", VGAR(MISCR)));
	CFGW(DEVCTRL,(2|CFGR(DEVCTRL))); // enable device response (already enabled here!)
	VGAW_I(CRTC,0x11,0);             // allow me to change CRTC
	VGAW_I(CRTCEXT,3,0x80);          // use powergraphix (+ trash other bits, they are set later)
	VGAW(MISCW,0x08);                // set MGA pixel clock in MISC - PLLSEL10 of TVP3026 is 1X
	
	// set MEM clock MCLK following TVP3026 2.4.2.1
	status = mil2_dac_set_mem_pll (need, &mclk); //FPLL_MCLK);
	if (status != B_OK) return status;
	
	// update OPTION for refresh count following page 3-18 
	// 33.2mus >= (rfhcnt31*512 + rfhcnt0*64 + 1)*gclk_period*factor
	// rfhcnt31*512 + rfhcnt0*64 <= -1 + 33.2mus*mclk/factor
	if (1) {
		int mclk_p   = 1;
		int nogscale = 1;
		int f_pll    = mclk * 1000 * 333 / (10000 << mclk_p);
		int rfhcnt   = (f_pll - 128) / 256;
		if (rfhcnt > 15) rfhcnt = 15;
		LOG(2,("mil2_dac_init: refresh count %d nogscale %d for %fMHz\n", 
			rfhcnt, nogscale, mclk));
	}
	for (nogscale = 1; nogscale >= 0; nogscale--) {
		int max = -1 + 33.2 * mclk / (nogscale? 1: 4);
		for (rfhcnt = 15; rfhcnt > 0; rfhcnt--) {
			int value = (rfhcnt & 0x0e) * 256 + (rfhcnt & 0x01) * 64;
			LOG(2,("mil2_dac_init factor %d, rfhcnt %2d: %d ?<= %d\n",
				nogscale, rfhcnt, value, max));
			if (value <= max) goto rfhcnt_found;
		}
	}
	LOG(8,("mil2_dac_init: cant get valid refresh count for %fMHz\n", mclk));
	return B_ERROR;
rfhcnt_found:
	LOG(2,("mil2_dac_init: found refresh count %d nogscale %d for %fMHz\n", 
		rfhcnt, nogscale, mclk));

	memconfig = 0x01; // worst case scenario: 64 bits RAMDAC (= tvp3026m) with >2Mb RAM.
	option = CFGR(OPTION) & 0xffd0cfff; 
	CFGW(OPTION, option | (nogscale << 21) | (rfhcnt << 16) | (memconfig << 12));
	LOG(2,("mil2_dac_init: OPTION 0x%08x\n", CFGR(OPTION)));

	//r: select indirect cursor control register and set defaults
	DXIW(CURCTRL, 0x00);	
	return B_OK;
}
