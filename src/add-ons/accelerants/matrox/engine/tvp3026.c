/*
	Program the Texas TVP3026
	using Texas Instrument TVP3026 manual SLA098B July 1996

	Author:
	Apsed May 2002 a lot of time after ...
	
	NB BPP24 and BPP32DIR not tested
*/

#define MODULE_BIT 0x00010000

#include <OS.h> // system_time, snooze 
#include "mga_std.h"

#define PIXEL_BUS_WIDTH64 0 // if 0: 32 bits width, else 64
#define PIXEL_BUS_WIDTH64 1 // if 0: 32 bits width, else 64

#define FPLL_REF   14.31818 // MHz
#define FVCO_MAX  220.00000 // MHz, may be 220 or 250, see 3.5
#define FVCO_MIN  110.00000 // MHz
#define FPLL_MCLK 100.00000 // MHz

#define MGAVGA_INSTS0       0x1FC2

// access the TVP3026, this is near G200 DAC but ...

/* direct registers */
#define TVP_PALWTADD     0x3c00
#define TVP_PALDATA      0x3c01
#define TVP_PIXRDMSK     0x3c02
#define TVP_PALRDADD     0x3c03
#define TVP_CUROVRWTADD  0x3c04
#define TVP_CUROVRDATA   0x3c05
#define TVP_CUROVRRDADD  0x3c07
#define TVP_DIRCURCTRL   0x3c09
#define TVP_X_DATAREG    0x3c0a
#define TVP_CURRAMDATA   0x3c0b
#define TVP_CURPOSXL     0x3c0c
#define TVP_CURPOSXH     0x3c0d
#define TVP_CURPOSYL     0x3c0e
#define TVP_CURPOSYH     0x3c0f

/* indirect registers */
#define TVPI_SILICONREV      0x01
#define TVPI_INDCURCTRL      0x06
#define TVPI_LATCHCTRL       0x0f
#define TVPI_TCOLCTRL        0x18
#define TVPI_MULCTRL         0x19
#define TVPI_CLOCKSEL        0x1a
#define TVPI_PALPAGE         0x1c
#define TVPI_GENCTRL         0x1d
#define TVPI_MISCCTRL        0x1e
#define TVPI_GENIOCTRL       0x2a
#define TVPI_GENIODATA       0x2b
#define TVPI_PLLADDR         0x2c
#define TVPI_PIXPLLDATA      0x2d
#define TVPI_MEMPLLDATA      0x2e
#define TVPI_LOOPLLDATA      0x2f
#define TVPI_COLKEYOL        0x30
#define TVPI_COLKEYOH        0x31
#define TVPI_COLKEYRL        0x32
#define TVPI_COLKEYRH        0x33
#define TVPI_COLKEYGL        0x34
#define TVPI_COLKEYGH        0x35
#define TVPI_COLKEYBL        0x36
#define TVPI_COLKEYBH        0x37
#define TVPI_COLKEYCTRL      0x38
#define TVPI_MEMCLKCTRL      0x39
#define TVPI_SENSETEST       0x3a
#define TVPI_TESTMODEDATA    0x3b
#define TVPI_CRCREML         0x3c
#define TVPI_CRCREMH         0x3d
#define TVPI_CRCBITSEL       0x3e
#define TVPI_ID              0x3f
#define TVPI_RESET           0xff

/*read and write from the TVP3026 registers*/
#define TVPR(A)   (MGA_REG8(TVP_##A))
#define TVPW(A,B) (MGA_REG8(TVP_##A)=B)

/*read and write from the TVP3026 indirect register*/
#define TVPIR(A)   (TVPW(PALWTADD,TVPI_##A),TVPR(X_DATAREG))
#define TVPIW(A,B) (TVPW(PALWTADD,TVPI_##A),TVPW(X_DATAREG,B))

#define WAIT_FOR_PLL_LOCK( pll, on_error) do { \
	bigtime_t start, now; \
	float     delay; \
	int       tmo; \
	\
	start = system_time(); \
	for (tmo = 0; tmo < 100 * 1000 * 1000; tmo++) { \
		int status = TVPIR (pll ## PLLDATA); \
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

#define REREAD_PLL( pll) do { \
	uint8 n, m, p; \
	float f, vco; \
	\
	TVPIW(PLLADDR, 0x00); \
	n = TVPIR(pll ## PLLDATA); \
	TVPIW(PLLADDR, 0x15); \
	m = TVPIR(pll ## PLLDATA); \
	TVPIW(PLLADDR, 0x2a); \
	p = TVPIR(pll ## PLLDATA); \
	vco = 8 * FPLL_REF * (65 - (m & 0x3f)) / (65 - (n & 0x3f)); \
	f   = vco / (1 << (p & 0x03)); \
	LOG(2,("mil2 reread %s PLL, nmp 0x%02x 0x%02x 0x%02x, %fMHz, vco %fMHz\n", \
		#pll, n, m, p, f, vco)); \
} while (0)

#define DUMP_CFG(reg) MSG(( \
	"PCI CONFIG       register 0x%04x %20s 0x%08x\n", MGACFG_##reg, #reg, CFGR(reg)))
#define DUMP_VGA(reg) MSG(( \
	"MGA VGA          register 0x%04x %20s 0x%02x\n", MGAVGA_##reg, #reg, VGAR(reg)))
#define DUMP_VGA_ATTR(reg) MSG(( \
	"MGA VGA ATTR     register 0x%04x %20s 0x%02x\n", 0x##reg, "ATTR" #reg, VGAR_I(ATTR, 0x##reg)))
#define DUMP_VGA_SEQ(reg) MSG(( \
	"MGA VGA SEQ      register 0x%04x %20s 0x%02x\n", 0x##reg, "SEQ" #reg, VGAR_I(SEQ, 0x##reg)))
#define DUMP_VGA_GCTL(reg) MSG(( \
	"MGA VGA GCTL     register 0x%04x %20s 0x%02x\n", 0x##reg, "GCTL" #reg, VGAR_I(GCTL, 0x##reg)))
#define DUMP_VGA_CRTC(reg) MSG(( \
	"MGA VGA CRTC     register 0x%04x %20s 0x%02x\n", 0x##reg, "CRTC" #reg, VGAR_I(CRTC, 0x##reg)))
#define DUMP_VGA_CRTCEXT(reg) MSG(( \
	"MGA VGA CRTCEXT  register 0x%04x %20s 0x%02x\n", 0x##reg, "CRTCEXT" #reg, VGAR_I(CRTCEXT, 0x##reg)))
#define DUMP_TVP(reg) MSG(( \
	"TVP3028          register 0x%04x %20s 0x%02x\n", TVP_##reg,  #reg, TVPR(reg)))
#define DUMP_TVPI(reg) MSG(( \
	"TVP3028 INDIRECT register 0x%04x %20s 0x%02x\n", TVPI_##reg, #reg, TVPIR(reg)))
#define DUMP_MGA(reg) MSG(( \
	"MGA              register 0x%04x %20s 0x%08x\n", MGAACC_##reg, #reg, ACCR(reg)))

static void dump_tvp3026 (void)
{
	/* TVP3026 DAC direct registers */
//	DUMP_TVP (PALWTADD); 
//	DUMP_TVP (PALDATA );
	DUMP_TVP (PIXRDMSK);
//	DUMP_TVP (PALRDADD);
	DUMP_TVP (CUROVRWTADD);
	DUMP_TVP (CUROVRDATA);
	DUMP_TVP (CUROVRRDADD);
	DUMP_TVP (DIRCURCTRL);
//	DUMP_TVP (X_DATAREG);
	DUMP_TVP (CURRAMDATA);
	DUMP_TVP (CURPOSXL);
	DUMP_TVP (CURPOSXH);
	DUMP_TVP (CURPOSYL);
	DUMP_TVP (CURPOSYH);

	/* TVP3026 DAC indirect registers */
	DUMP_TVPI (SILICONREV);
	DUMP_TVPI (INDCURCTRL);
	DUMP_TVPI (LATCHCTRL);
	DUMP_TVPI (TCOLCTRL);
	DUMP_TVPI (MULCTRL);
	DUMP_TVPI (CLOCKSEL);
	DUMP_TVPI (PALPAGE);
	DUMP_TVPI (GENCTRL);
	DUMP_TVPI (MISCCTRL);
	DUMP_TVPI (GENIOCTRL);
	DUMP_TVPI (GENIODATA);
//	DUMP_TVPI (PLLADDR);
//	DUMP_TVPI (PIXPLLDATA);
//	DUMP_TVPI (MEMPLLDATA);
//	DUMP_TVPI (LOOPLLDATA);
	REREAD_PLL (PIX);
	REREAD_PLL (MEM);
	REREAD_PLL (LOO);
	DUMP_TVPI (COLKEYOL);
	DUMP_TVPI (COLKEYOH);
	DUMP_TVPI (COLKEYRL);
	DUMP_TVPI (COLKEYRH);
	DUMP_TVPI (COLKEYGL);
	DUMP_TVPI (COLKEYGH);
	DUMP_TVPI (COLKEYBL);
	DUMP_TVPI (COLKEYBH);
	DUMP_TVPI (COLKEYCTRL);
	DUMP_TVPI (MEMCLKCTRL);
	DUMP_TVPI (SENSETEST);
	DUMP_TVPI (TESTMODEDATA);
	DUMP_TVPI (CRCREML);
	DUMP_TVPI (CRCREMH);
//	DUMP_TVPI (CRCBITSEL); // WO
	DUMP_TVPI (ID);
//	DUMP_TVPI (RESET); // WO
}

static void dump_mil2 (void)
{
	/* PCI_config_space */
	DUMP_CFG (DEVID);
	DUMP_CFG (DEVCTRL);
	DUMP_CFG (CLASS);
	DUMP_CFG (HEADER);
	DUMP_CFG (MGABASE2);
	DUMP_CFG (MGABASE1);
	DUMP_CFG (MGABASE3);
	DUMP_CFG (SUBSYSIDR);
	DUMP_CFG (ROMBASE);
	DUMP_CFG (CAP_PTR);
	DUMP_CFG (INTCTRL);
	DUMP_CFG (OPTION);
	DUMP_CFG (MGA_INDEX);
	DUMP_CFG (MGA_DATA);
	DUMP_CFG (SUBSYSIDW);
	DUMP_CFG (AGP_IDENT);
	DUMP_CFG (AGP_STS);
	DUMP_CFG (AGP_CMD);
	
	/*VGA registers - these are byte wide*/
//	DUMP_VGA (ATTR_I);
//	DUMP_VGA (ATTR_D);
	DUMP_VGA_ATTR (0);
	DUMP_VGA_ATTR (1);
	DUMP_VGA_ATTR (3);
	DUMP_VGA_ATTR (3);
	DUMP_VGA_ATTR (4);
	DUMP_VGA_ATTR (5);
	DUMP_VGA_ATTR (6);
	DUMP_VGA_ATTR (7);
	DUMP_VGA_ATTR (8);
	DUMP_VGA_ATTR (9);
	DUMP_VGA_ATTR (A);
	DUMP_VGA_ATTR (B);
	DUMP_VGA_ATTR (C);
	DUMP_VGA_ATTR (D);
	DUMP_VGA_ATTR (E);
	DUMP_VGA_ATTR (F);
	DUMP_VGA_ATTR (10);
	DUMP_VGA_ATTR (11);
	DUMP_VGA_ATTR (12);
	DUMP_VGA_ATTR (13);
	DUMP_VGA_ATTR (14);
//	DUMP_VGA (CRTC_I);
//	DUMP_VGA (CRTC_D);
	DUMP_VGA_CRTC (0);
	DUMP_VGA_CRTC (1);
	DUMP_VGA_CRTC (2);
	DUMP_VGA_CRTC (3);
	DUMP_VGA_CRTC (4);
	DUMP_VGA_CRTC (5);
	DUMP_VGA_CRTC (6);
	DUMP_VGA_CRTC (7);
	DUMP_VGA_CRTC (8);
	DUMP_VGA_CRTC (9);
	DUMP_VGA_CRTC (A);
	DUMP_VGA_CRTC (B);
	DUMP_VGA_CRTC (C);
	DUMP_VGA_CRTC (D);
	DUMP_VGA_CRTC (E);
	DUMP_VGA_CRTC (F);
	DUMP_VGA_CRTC (10);
	DUMP_VGA_CRTC (11);
	DUMP_VGA_CRTC (12);
	DUMP_VGA_CRTC (13);
	DUMP_VGA_CRTC (14);
	DUMP_VGA_CRTC (15);
	DUMP_VGA_CRTC (16);
	DUMP_VGA_CRTC (17);
	DUMP_VGA_CRTC (18);
	DUMP_VGA_CRTC (22);
	DUMP_VGA_CRTC (24);
	DUMP_VGA_CRTC (26);
//	DUMP_VGA (CRTCEXT_I);
//	DUMP_VGA (CRTCEXT_D);
	DUMP_VGA_CRTCEXT (0);
	DUMP_VGA_CRTCEXT (1);
	DUMP_VGA_CRTCEXT (2);
	DUMP_VGA_CRTCEXT (3);
	DUMP_VGA_CRTCEXT (4);
	DUMP_VGA_CRTCEXT (5);
	DUMP_VGA (DACSTAT);
//	DUMP_VGA (FEATW);
	DUMP_VGA (FEATR);
//	DUMP_VGA (GCTL_I);
//	DUMP_VGA (GCTL_D);
	DUMP_VGA_GCTL (0);
	DUMP_VGA_GCTL (1);
	DUMP_VGA_GCTL (2);
	DUMP_VGA_GCTL (3);
	DUMP_VGA_GCTL (4);
	DUMP_VGA_GCTL (5);
	DUMP_VGA_GCTL (6);
	DUMP_VGA_GCTL (7);
	DUMP_VGA_GCTL (8);
	DUMP_VGA (INSTS0);
	DUMP_VGA (INSTS1);
	DUMP_VGA (MISCR);
	DUMP_VGA (MISCW);
//	DUMP_VGA (SEQ_I);
//	DUMP_VGA (SEQ_D);
	DUMP_VGA_SEQ (0);
	DUMP_VGA_SEQ (1);
	DUMP_VGA_SEQ (2);
	DUMP_VGA_SEQ (3);
	DUMP_VGA_SEQ (4);

	/* MGA registers, only the readable*/
	DUMP_MGA (IEN);
	DUMP_MGA (RST);
	DUMP_MGA (OPMODE);
	DUMP_MGA (STATUS);

	/* TVP3026 registers */
	dump_tvp3026();
}

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t mil2_dac_mode (int mode, float brightness, int hsync_pos, int vsync_pos, int sync_green)
{
	uint8 *r, *g, *b, t[64];
	int    i;
	uint8  miscctrl = 0, latchctrl = 0;
	uint8  tcolctrl = 0, mulctrl   = 0;

	LOG(4,("TVP:Setting screen mode %d brightness %f\n", mode, brightness));
	if (si->settings.logmask & 0x80000000) dump_tvp3026();

	/*set colour arrays to point to space reserved in shared info*/
	r = si->color_data;
	g = r + 256;
	b = g + 256;

	/*init a basic palette for brightness specified*/
brightness = 2.0; // TODO
	for (i=0;i<256;i++)	{
		int ri = i*brightness;
		if (ri > 255) ri = 255;
		r[i]=ri;
	}

	/*modify the palette for the specified mode (&validate mode)*/
	switch(mode) {
		case BPP8:
		case BPP24:case BPP32:
			for (i=0;i<256;i++)	b[i]=g[i]=r[i];
			break;
		case BPP16:
			for (i=0;i<64;i++) t[i]=r[i<<2];
			for (i=0;i<64;i++) g[i]=t[i];
			for (i=0;i<32;i++) b[i]=r[i]=t[i<<1];
			break;
		case BPP15:
			for (i=0;i<32;i++) t[i]=r[i<<3];
			for (i=0;i<32;i++) g[i]=r[i]=b[i]=t[i];
			break;
		case BPP32DIR:
			break;
		default:
			LOG(8,("TVP:Invalid bit depth requested\n"));
			return B_ERROR;
			break;
	}

	if (mil2_dac_palette (r, g, b) != B_OK) return B_ERROR;

	// set the mode
	switch (mode) { // mulctrl for PIXEL_BUS_WIDTH 32
		case BPP8:     miscctrl=0x0c; latchctrl=0x06; tcolctrl=0x80; mulctrl=0x4b; break;
		case BPP15:    miscctrl=0x20; latchctrl=0x06; tcolctrl=0x04; mulctrl=0x53; break;
		case BPP16:    miscctrl=0x20; latchctrl=0x06; tcolctrl=0x05; mulctrl=0x53; break;
		case BPP24:    miscctrl=0x20; latchctrl=0x06; tcolctrl=0x1f; mulctrl=0x5b; break;
		case BPP32:    miscctrl=0x20; latchctrl=0x07; tcolctrl=0x06; mulctrl=0x5b; break;
		case BPP32DIR: miscctrl=0x20; latchctrl=0x07; tcolctrl=0x06; mulctrl=0x5b; break;
	}
	if (PIXEL_BUS_WIDTH64) mulctrl += 1;
	TVPIW(MISCCTRL,(TVPIR(MISCCTRL) & 0xd3) | miscctrl); 
	TVPIW(LATCHCTRL, latchctrl); 
	TVPIW(TCOLCTRL, tcolctrl); 
	TVPIW(MULCTRL, mulctrl);

	// synchros 
	TVPIW (GENCTRL, (TVPIR (GENCTRL) & 0xdc) 
		| (sync_green? 0x00:0x20) // apsed TODO ?
		| (vsync_pos?  0x00:0x02) 
		| (hsync_pos?  0x00:0x01));

//VGAW (MISCW, VGAR(MISCR) & 0x3f); // TODO
//TVPIW (GENCTRL, TVPIR (GENCTRL) & 0xdf);  // TODO

	LOG(2,("TVP: clocksel 0x%02x, miscctrl 0x%02x\n",
		TVPIR(CLOCKSEL), TVPIR(MISCCTRL)));
	LOG(2,("TVP: tcolctrl 0x%02x, mulctrl 0x%02x, pixrdmsk 0x%02x, genctrl 0x%02x\n",
		TVPIR(TCOLCTRL), TVPIR(MULCTRL), TVPR(PIXRDMSK), TVPIR (GENCTRL)));
	return B_OK;
}

// program the palette using the given r,g,b values
status_t mil2_dac_palette (uint8 r[256], uint8 g[256], uint8 b[256])
{
	int i;

	LOG(4,("TVP: setting palette\n"));

	/*clear palwtadd to start programming*/
	TVPW(PALWTADD,0);

	/*loop through all 256 to program LUT*/
	for (i=0;i<256;i++) {
		TVPW(PALDATA,r[i]);
		TVPW(PALDATA,g[i]);
		TVPW(PALDATA,b[i]);
	}
	if (TVPR(PALWTADD)!=0) {
		LOG(8,("TVP: PALWTADD is not 0 after programming\n"));
		return B_ERROR;
	}
if (0) { // apsed: reread LUT
	uint8 R, G, B;
	TVPW(PALRDADD,0);
	for (i=0;i<256;i++)	{
		R = TVPR(PALDATA);
		G = TVPR(PALDATA);
		B = TVPR(PALDATA);
		if ((r[i] != R) || (g[i] != G) || (b[i] != B)) {
			LOG(8,("TVP: palette 0x%02x: w %02x %02x %02x, r %02x %02x %02x\n", 
				i, r[i], g[i], b[i], R, G, B));
		}
	}
}
	return B_OK;
}

// find nearest valid pll parameters, TVP3026 2.4.1 
status_t mil2_dac_pix_pll_find (float f_need, float *f_result, uint8 *m_result, uint8 *n_result, uint8 *p_result)
{
	int   m, n, p;
	float error, best, best_vco;
	float f_vco, f_pll;
	
	LOG(0,("mil2_dac_pix_pll_find for %fMHz\n", f_need));
	best = 999999999;
	// stupid implementation of 2.4.1 and 2.4.2
	for (m = 1; m <= 62; m++) {
		for (n = 40; n <= 62; n++) {
			f_vco = (8.0 * FPLL_REF * (65 - m)) / (65 - n);
			if ((f_vco < FVCO_MIN) || (f_vco >= FVCO_MAX)) continue;
			for (p = 0; p <= 3; p++) {
				f_pll = f_vco / (float)(1 << p);
				error = fabs (f_need - f_pll) / f_need;
				if (error > best) continue;
				best = error;
				best_vco  = f_vco;
				*f_result = f_pll;
				*m_result = m;
				*n_result = n;
				*p_result = p;
				LOG (0,("mil2_dac_pix_pll_find nmp %fMHz %fMHz 0x%02x 0x%02x 0x%02x\n", 
					f_pll, f_vco, *n_result, *m_result, *p_result));
			}
		}
	}
	LOG(0,("mil2_dac_pix_pll_find requested %fMHz got %fMHz, vco %fMHz, nmp 0x%02x 0x%02x 0x%02x\n",
		f_need, *f_result, best_vco, *n_result, *m_result, *p_result));
	return B_OK;
}

/*program the pixpll - frequency in MHz*/
status_t mil2_dac_set_pix_pll (float f_need, int bpp)
{
	uint8    n, m, p, q;
	int      z, k;
	float    fd;
	status_t result;

	LOG(4,("mil2_dac_set_pix_pll need %fMHz, %dbpp\n", f_need, bpp));
	result = mil2_dac_pix_pll_find(f_need,&fd,&m,&n,&p);
	if (result != B_OK) return result;
	LOG(2,("mil2_dac_set_pix_pll need %fMHz got %fMHz, nmp 0x%02x 0x%02x 0x%02x\n",
		f_need, fd, n, m, p));

	// follows (!strictly) Appendix C, extended mode setup 
	// 1st stop the PLLs, 
	switch (bpp) {
		case  8: TVPIW(CLOCKSEL, 0x25); break;
		case 16: TVPIW(CLOCKSEL, 0x15); break;
		case 24: TVPIW(CLOCKSEL, 0x25); break;
		case 32: TVPIW(CLOCKSEL, 0x05); break;
		default: return B_ERROR;
	}
	TVPIW(PLLADDR, 0x2a);        // 0x2c: 2.4 select P to ...
	TVPIW(LOOPLLDATA, 0x00);     // 0x2f: 2.4.1 ... stop the loop PLL
	TVPIW(PIXPLLDATA, 0x00);     // 0x2d: 2.4.1 ... stop the pixel PLL
	VGAW (MISCW, VGAR(MISCR) | 0x0c); // PLLSEL(1,0) set to 1x 
	
	// 2nd setup the pixel PLL
	LOG(2,("mil2_dac_set_pix_pll pix PLL, nmp 0x%02x 0x%02x 0x%02x\n",
		n, m, p));
	TVPIW(PLLADDR, 0x00);        // 0x2c: 2.4 select N to ...
	TVPIW(PIXPLLDATA, n | 0xc0); // 0x2d: ... load n, m, p and ...
	TVPIW(PIXPLLDATA, m);
	TVPIW(PIXPLLDATA, p | 0xb0);
	WAIT_FOR_PLL_LOCK (PIX, return B_ERROR); // ... wait for PLL lock
	if (1) REREAD_PLL (PIX);
	
	// now compute parameters for the loop PLL (24bpp not available) see 2.4.3.1
	k = 1; // ?? external division factor between RCLK and LCLK
	n = (65 - (4 * (PIXEL_BUS_WIDTH64? 64: 32)) / bpp);
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
	TVPIW(MEMCLKCTRL, (TVPIR(MEMCLKCTRL) & 0xf8) | q | 0x20); // 0x39: 2.4.2 table 2.13
	LOG(2,("mil2_dac_set_pix_pll loop PLL, nmpq 0x%02x 0x%02x 0x%02x 0x%02x\n",
		n, m, p, q));

	// now setup the loop PLL
	LOG(2,("mil2_dac_set_pix_pll loop PLL, nmpq 0x%02x 0x%02x 0x%02x 0x%02x\n",
		n, m, p, q));
	TVPIW(PLLADDR, 0x00);        // 0x2c: 2.4 select N to ...
	TVPIW(LOOPLLDATA, n | 0xc0); // 0x2f: ... load n, m, p and ...
	TVPIW(LOOPLLDATA, m);
	TVPIW(LOOPLLDATA, p | 0xf0);
	WAIT_FOR_PLL_LOCK (LOO, return B_ERROR); // ... wait for PLL lock
	if (1) REREAD_PLL (LOO);

	return B_OK;
}

// set MEM clock MCLK , shall be <= 100MHz
static status_t mil2_dac_set_mem_pll (float f_need, float *mclk)
{
	status_t status;
	uint8    n, m, p;
	uint8    n_pix, m_pix, p_pix; 
	uint8    memclkctrl;
	
	status = mil2_dac_pix_pll_find (f_need, mclk, &m, &n, &p);
	if (status != B_OK) return status;
	LOG(2,("mil2_dac_set_mem_pll need %fMHz got %fMHz, nmp 0x%02x 0x%02x 0x%02x\n",
		f_need, *mclk, n, m, p));

	// follows (!strictly) TVP3026 2.4.2.1, extended mode setup 

	// 0) save PIXPLL nmp to restore it at end
	TVPIW(PLLADDR, 0x00);
	n_pix = TVPIR(PIXPLLDATA);
	TVPIW(PLLADDR, 0x15);
	m_pix = TVPIR(PIXPLLDATA);
	TVPIW(PLLADDR, 0x2a);
	p_pix = TVPIR(PIXPLLDATA);
	if (1) REREAD_PLL (PIX);
	
	// 1) disable pixel PLL, set pixel PLL at MCLK freq and poll for lock
	TVPIW(PLLADDR, 0x2a);        // 0x2c: 2.4 select P to ...
	TVPIW(PIXPLLDATA, 0x00);     // 0x2d: 2.4.1 ... stop the PLL
	TVPIW(PLLADDR, 0x00);        // 0x2c: 2.4 select N to ...
	TVPIW(PIXPLLDATA, n | 0xc0); // 0x2d: ... load n, m, p and ...
	TVPIW(PIXPLLDATA, m);
	TVPIW(PIXPLLDATA, p | 0xb0);
	WAIT_FOR_PLL_LOCK (PIX, return B_ERROR); // ... wait for PLL lock
	if (1) REREAD_PLL (PIX);
	
	// 2) select pixel clock as dot clock source 
	VGAW (MISCW, VGAR(MISCR) | 0x0c); // PLLSEL(1,0) set to 1x 
	
	// 3) output dot clock on MCLK pin
	memclkctrl = TVPIR(MEMCLKCTRL) & 0xe7;
	TVPIW(MEMCLKCTRL, memclkctrl | (0x00 << 3));
	TVPIW(MEMCLKCTRL, memclkctrl | (0x01 << 3));
	
	// 4) disable mem PLL, set mem PLL at MCLK freq and poll for lock
	if (1) REREAD_PLL (MEM);
	TVPIW(PLLADDR, 0x2a);
	TVPIW(MEMPLLDATA, 0x00);
	TVPIW(PLLADDR, 0x00);
	TVPIW(MEMPLLDATA, n | 0xc0);
	TVPIW(MEMPLLDATA, m);
	TVPIW(MEMPLLDATA, p | 0xb0);
	WAIT_FOR_PLL_LOCK (MEM, return B_ERROR);
	if (1) REREAD_PLL (MEM);

	// 5) output mem clock on MCLK pin
	TVPIW(MEMCLKCTRL, memclkctrl | (0x02 << 3));
	TVPIW(MEMCLKCTRL, memclkctrl | (0x03 << 3));
	
	// 6) restaure pixel clock as it was
	TVPIW(PLLADDR, 0x2a);
	TVPIW(PIXPLLDATA, 0x00);
	TVPIW(PLLADDR, 0x00);
//	TVPIW(PIXPLLDATA, n_pix);
//	TVPIW(PIXPLLDATA, m_pix);
//	TVPIW(PIXPLLDATA, p_pix);
	TVPIW(PIXPLLDATA, n_pix | 0xc0);
	TVPIW(PIXPLLDATA, m_pix);
	TVPIW(PIXPLLDATA, p_pix | 0xb0);
	WAIT_FOR_PLL_LOCK (PIX, return B_ERROR);
	if (1) REREAD_PLL (PIX);
	
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
	
	dump_mil2();
	
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

	memconfig = 0x00; // 32 bits RAMDAC
	memconfig = 0x01; // 64 bits RAMDAC
	option = CFGR(OPTION) & 0xffd0cfff; 
	CFGW(OPTION, option | (nogscale << 21) | (rfhcnt << 16) | (memconfig << 12));
	LOG(2,("mil2_dac_init: OPTION 0x%08x\n", CFGR(OPTION)));
	
	return B_OK;
}
