/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.	All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2007
*/


#include "GlobalData.h"
#include "savage.h"


#define CURSOR_KBYTES	4		// kilobytes used for cursor image in video memory


typedef struct
{
	unsigned char CRTC[25];			// Crtc Controller reg's

	unsigned char SR08, SR0E, SR0F;
	unsigned char SR10, SR11, SR12, SR13, SR15, SR18, SR1B, SR29, SR30;
	unsigned char SR54[8];
	unsigned char CR31, CR32, CR33, CR34, CR36, CR3A, CR3B, CR3C;
	unsigned char CR40, CR41, CR42, CR43, CR45;
	unsigned char CR50, CR51, CR53, CR55, CR58, CR5B, CR5D, CR5E;
	unsigned char CR60, CR63, CR65, CR66, CR67, CR68, CR69, CR6D, CR6F;
	unsigned char CR86, CR88;
	unsigned char CR90, CR91, CRB0;
	unsigned int  MMPR0, MMPR1, MMPR2, MMPR3;
} SavageRegRec;




static void 
InitCrtcTimingValues(DisplayMode* pMode, SavageRegRec* pRegRec)
{
	int hTotal = ((pMode->timing.h_total >> 3) - 5);
	int hDisp_e = ((pMode->timing.h_display >> 3) - 1);
	int hSync_s = (pMode->timing.h_sync_start >> 3);
	int hSync_e = (pMode->timing.h_sync_end >> 3);
	int hBlank_s = hDisp_e;			// start of horizontal blanking
	int hBlank_e = hTotal;			// end of horizontal blanking

	int vTotal = pMode->timing.v_total - 2;
	int vDisp_e = pMode->timing.v_display - 1;
	int vSync_s = pMode->timing.v_sync_start;
	int vSync_e = pMode->timing.v_sync_end;
	int vBlank_s = vDisp_e;			// start of vertical blanking
	int vBlank_e = vTotal;			// end of vertical blanking

	TRACE(("InitCrtcTimingValues\n"));

	// CRTC Controller values

	pRegRec->CRTC[0] = hTotal;
	pRegRec->CRTC[1] = hDisp_e;
	pRegRec->CRTC[2] = hBlank_s;
	pRegRec->CRTC[3] = (hBlank_e & 0x1F) | 0x80;
	pRegRec->CRTC[4] = hSync_s;
	pRegRec->CRTC[5] = ((hSync_e & 0x1F) | ((hBlank_e & 0x20) << 2));
	pRegRec->CRTC[6] = vTotal;
	pRegRec->CRTC[7] = (((vTotal & 0x100) >> 8)
		| ((vDisp_e & 0x100) >> 7)
		| ((vSync_s & 0x100) >> 6)
		| ((vBlank_s & 0x100) >> 5)
		| 0x10
		| ((vTotal & 0x200) >> 4)
		| ((vDisp_e & 0x200) >> 3)
		| ((vSync_s & 0x200) >> 2));

	pRegRec->CRTC[8]  = 0x00;
	pRegRec->CRTC[9]  = ((vBlank_s & 0x200) >> 4) | 0x40;
	pRegRec->CRTC[10] = 0x00;
	pRegRec->CRTC[11] = 0x00;
	pRegRec->CRTC[12] = 0x00;
	pRegRec->CRTC[13] = 0x00;
	pRegRec->CRTC[14] = 0x00;
	pRegRec->CRTC[15] = 0x00;
	pRegRec->CRTC[16] = vSync_s;
	pRegRec->CRTC[17] = (vSync_e & 0x0f) | 0x20;
	pRegRec->CRTC[18] = vDisp_e;
	pRegRec->CRTC[19] = pMode->bytesPerRow / 16;	// should we divide by 8?
	pRegRec->CRTC[20] = 0x00;
	pRegRec->CRTC[21] = vBlank_s;
	pRegRec->CRTC[22] = vBlank_e;
//	pRegRec->CRTC[23] = 0xC3;
	pRegRec->CRTC[23] = 0xEB;
	pRegRec->CRTC[24] = 0xFF;
}


static void 
ResetBCI2K()
{
	uint32 cob = INREG( 0x48c18 );
	/* if BCI is enabled and BCI is busy... */

	if ( (cob & 0x00000008) && ! (ALT_STATUS_WORD0 & 0x00200000) ) {
		TRACE(("Resetting BCI, stat = %08lx...\n", (uint32)ALT_STATUS_WORD0));
		/* Turn off BCI */
		OUTREG( 0x48c18, cob & ~8 );
		snooze(10000);
		/* Turn it back on */
		OUTREG( 0x48c18, cob );
		snooze(10000);
	}
}


/* Wait until "v" queue entries are free */

static bool 
WaitQueue3D(int v)
{
	int loop = 0;
	uint32 slots = MAXFIFO - v;

	while( ((STATUS_WORD0 & 0x0000ffff) > slots) && (loop++ < MAXLOOP) );
	return loop >= MAXLOOP;
}


static bool 
WaitQueue4(int v)
{
	int loop = 0;
	uint32 slots = MAXFIFO - v;

	while( ((ALT_STATUS_WORD0 & 0x001fffff) > slots) && (loop++ < MAXLOOP) );
	return loop >= MAXLOOP;
}


static bool 
WaitQueue2K(int v)
{
	int loop = 0;
	uint32 slots = (MAXFIFO - v) / 4;

	while( ((ALT_STATUS_WORD0 & 0x000fffff) > slots) && (loop++ < MAXLOOP) );

	if (loop >= MAXLOOP)
		ResetBCI2K();

	return loop >= MAXLOOP;
}


/* Wait until GP is idle and queue is empty */

static bool 
WaitIdleEmpty3D()
{
	int loop = 0;

	while( ((STATUS_WORD0 & 0x0008ffff) != 0x80000) && (loop++ < MAXLOOP) );
	return loop >= MAXLOOP;
}


static bool 
WaitIdleEmpty4()
{
	int loop = 0;

	while (((ALT_STATUS_WORD0 & 0x00e1ffff) != 0x00e00000) && (loop++ < MAXLOOP)) ;
	return loop >= MAXLOOP;
}


static bool 
WaitIdleEmpty2K()
{
	int loop = 0;

	while( ((ALT_STATUS_WORD0 & 0x009fffff) != 0) && (loop++ < MAXLOOP) );

	if (loop >= MAXLOOP)
		ResetBCI2K();

	return loop >= MAXLOOP;
}


static void
SavageGetPanelInfo()
{
	uint8 cr6b;
	int panelX, panelY;
	char * sTechnology = "Unknown";
	enum ACTIVE_DISPLAYS { /* These are the bits in CR6B */
		ActiveCRT = 0x01,
		ActiveLCD = 0x02,
		ActiveTV	= 0x04,
		ActiveCRT2 = 0x20,
		ActiveDUO = 0x80
	};

	/* Check LCD panel information */

	cr6b = ReadCrtc(0x6b);

	panelX = (ReadSeq(0x61) + ((ReadSeq(0x66) & 0x02) << 7) + 1) * 8;
	panelY =  ReadSeq(0x69) + ((ReadSeq(0x6e) & 0x70) << 4) + 1;

	if ( ! IsDisplaySizeValid(panelX, panelY)) {
		
		// Some chips such as the Savage IX/MV in a Thinkpad T-22 will return
		// a width that is 8 pixels too wide probably because reg SR61 is set
		// to a value +1 higher than it should be.  Subtract 8 from the width,
		// and check if that is a valid width.

		panelX -= 8;
		if ( ! IsDisplaySizeValid(panelX, panelY)) {
			TRACE(("%dx%d LCD panel size invalid.  No matching video mode\n", panelX + 8, panelY));
			si->displayType = MT_CRT;
			return;
		}
	}

	if ((ReadSeq(0x39) & 0x03) == 0)
		sTechnology = "TFT";
	else if ((ReadSeq(0x30) & 0x01) == 0)
		sTechnology = "DSTN";
	else
		sTechnology = "STN";

	TRACE(("%dx%d %s LCD panel detected %s\n", panelX, panelY, sTechnology,
			 cr6b & ActiveLCD ? "and active" : "but not active"));

	if (cr6b & ActiveLCD) {
		TRACE(("Limiting max video mode to %dx%d\n", panelX, panelY));

		si->panelX = panelX;
		si->panelY = panelY;
	} else {
		si->displayType = MT_CRT;
	}
}


bool 
SavagePreInit(void)
{
	bool  bDvi;
	uint8 m, n, n1, n2, sr8;
	uint8 config1;
	uint8 cr66;
	uint8 temp;
	uint8 val;

	TRACE(("SavagePreInit\n"));

	val = INREG8(0x83c3);
	OUTREG8(0x83c3, val | 0x01);

	val = INREG8(VGA_MISC_OUT_R);
	OUTREG8(VGA_MISC_OUT_W, val | 0x01);

	if (si->chipset >= S3_SAVAGE4) {
		OUTREG8(VGA_CRTC_INDEX, 0x40);
		val = INREG8(VGA_CRTC_DATA);
		OUTREG8(VGA_CRTC_DATA, val | 1);
	}

	/* unprotect CRTC[0-7] */
	OUTREG8(VGA_CRTC_INDEX, 0x11);
	temp = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, temp & 0x7f);

	/* unlock extended regs */
	OUTREG16(VGA_CRTC_INDEX, 0x4838);
	OUTREG16(VGA_CRTC_INDEX, 0xa039);
	OUTREG16(VGA_SEQ_INDEX, 0x0608);

	OUTREG8(VGA_CRTC_INDEX, 0x40);
	temp = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, temp & ~0x01);

	/* unlock sys regs */
	OUTREG8(VGA_CRTC_INDEX, 0x38);
	OUTREG8(VGA_CRTC_DATA, 0x48);

	/* Unlock system registers. */
	OUTREG16(VGA_CRTC_INDEX, 0x4838);

	/* Next go on to detect amount of installed ram */

	OUTREG8(VGA_CRTC_INDEX, 0x36);			/* for register CR36 (CONFG_REG1), */
	config1 = INREG8(VGA_CRTC_DATA);		/* get amount of vram installed */

	/* Compute the amount of video memory and offscreen memory. */

	{
		static const uint8 RamSavage3D[] = { 8, 4, 4, 2 };
		static		 uint8 RamSavage4[] =  { 2, 4, 8, 12, 16, 32, 64, 32 };
		static const uint8 RamSavageMX[] = { 2, 8, 4, 16, 8, 16, 4, 16 };
		static const uint8 RamSavageNB[] = { 0, 2, 4, 8, 16, 32, 16, 2 };
		int ramSizeMB = 0;		// memory size in megabytes

		switch (si->chipset) {
		case S3_SAVAGE3D:
			ramSizeMB = RamSavage3D[ (config1 & 0xC0) >> 6 ];
			break;

		case S3_SAVAGE4:
			/*
			 * The Savage4 has one ugly special case to consider.  On
			 * systems with 4 banks of 2Mx32 SDRAM, the BIOS says 4MB
			 * when it really means 8MB.	Why do it the same when you
			 * can do it different...
			 */
			OUTREG8(VGA_CRTC_INDEX, 0x68);	/* memory control 1 */
			if ( (INREG8(VGA_CRTC_DATA) & 0xC0) == (0x01 << 6) )
				RamSavage4[1] = 8;

			/*FALLTHROUGH*/

		case S3_SAVAGE2000:
			ramSizeMB = RamSavage4[ (config1 & 0xE0) >> 5 ];
			break;

		case S3_SAVAGE_MX:
		case S3_SUPERSAVAGE:
			ramSizeMB = RamSavageMX[ (config1 & 0x0E) >> 1 ];
			break;

		case S3_PROSAVAGE:
		case S3_PROSAVAGEDDR:
		case S3_TWISTER:
			ramSizeMB = RamSavageNB[ (config1 & 0xE0) >> 5 ];
			break;

		default:
			/* How did we get here? */
			ramSizeMB = 0;
			break;
		}

		TRACE(("memory size: %d MB\n", ramSizeMB));

		si->videoMemSize = ramSizeMB * 1024 * 1024;
	}


	// Certain Savage4 and ProSavage chips can have coherency problems
	// with respect to the Command Overflow Buffer (COB);	thus, to avoid
	// problems with these chips, set bDisableCOB to true.

	si->bDisableCOB = false;

	/*
	 * Compute the Command Overflow Buffer (COB) location.
	 */
	if ((S3_SAVAGE4_SERIES(si->chipset) || S3_SUPERSAVAGE == si->chipset)
		&& si->bDisableCOB) {
		/*
		 * The Savage4 and ProSavage have COB coherency bugs which render
		 * the buffer useless.
		 */
		si->cobIndex = 0;
		si->cobSize = 0;
		si->bciThresholdHi = 32;
		si->bciThresholdLo = 0;
	} else {
		/* We use 128kB for the COB on all other chips. */
		si->cobSize = 0x20000;
		if (S3_SAVAGE3D_SERIES(si->chipset) || si->chipset == S3_SAVAGE2000)
			si->cobIndex = 7;	/* rev.A savage4 apparently also uses 7 */
		else
			si->cobIndex = 2;

		/* max command size: 2560 entries */
		si->bciThresholdHi = si->cobSize / 4 + 32 - 2560;
		si->bciThresholdLo = si->bciThresholdHi - 2560;
	}

	// Note that the X.org developers stated that the command overflow buffer
	// (COB) must END at a 4MB boundary which for all practical purposes means
	// the very end of the video memory.  However, for Haiku it has been put
	// at the beginning of the video memory and has worked fine for the
	// Savage4, ProSavage, Savage/IX, SuperSavage chipsets.  If any problems
	// are detected in this area with other chipsets, the following 4 lines of
	// code can be uncommented and the 4 lines after those can be commented
	// out to put the COB at the end of video memory.

//	si->cobOffset = (si->videoMemSize - si->cobSize) & ~0x1ffff;	// align cob to 128k
//	si->cursorOffset = 0;
//	si->frameBufferOffset = si->cursorOffset + CURSOR_KBYTES * 1024;
//	si->maxFrameBufferSize = si->cobOffset - si->frameBufferOffset;

	si->cobOffset = 0;
	si->cursorOffset = (si->cobSize + 0x3FF) & ~0x3FF;
	si->frameBufferOffset = si->cursorOffset + CURSOR_KBYTES * 1024;
	si->maxFrameBufferSize = si->videoMemSize - si->frameBufferOffset;

	TRACE(("cobIndex: %d	cobSize: %d  cobOffset: 0x%X\n", si->cobIndex, si->cobSize, si->cobOffset));
	TRACE(("cursorOffset: 0x%X  frameBufferOffset: 0x%X\n", si->cursorOffset, si->frameBufferOffset));


	/* reset graphics engine to avoid memory corruption */
	OUTREG8(VGA_CRTC_INDEX, 0x66);
	cr66 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr66 | 0x02);
	snooze(10000);

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	OUTREG8(VGA_CRTC_DATA, cr66 & ~0x02);	/* clear reset flag */
	snooze(10000);

	/* Check for DVI/flat panel */

	bDvi = false;
	if (si->chipset == S3_SAVAGE4) {
		uint8 sr30 = 0;
		OUTREG8(VGA_SEQ_INDEX, 0x30);
		/* clear bit 1 */
		OUTREG8(VGA_SEQ_DATA, INREG8(VGA_SEQ_DATA) & ~0x02);
		sr30 = INREG8(VGA_SEQ_DATA);
		if (sr30 & 0x02 /*0x04 */) {
			 bDvi = true;
			 TRACE(("Digital Flat Panel Detected\n"));
		}
	}

	if (S3_SAVAGE_MOBILE_SERIES(si->chipset) || S3_MOBILE_TWISTER_SERIES(si->chipset)) {
		si->displayType = MT_LCD;
		SavageGetPanelInfo();
	}
	else if (bDvi)
		si->displayType = MT_DFP;
	else
		si->displayType = MT_CRT;

	TRACE(("Display Type: %d\n", si->displayType));

	// Set max pixel clocks for the various color depths.

	si->pix_clk_max8 = 250000;
	si->pix_clk_max16 = 250000;
	si->pix_clk_max32 = 220000;

	/* detect current mclk */

	OUTREG8(VGA_SEQ_INDEX, 0x08);
	sr8 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_DATA, 0x06);
	OUTREG8(VGA_SEQ_INDEX, 0x10);
	n = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x11);
	m = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x08);
	OUTREG8(VGA_SEQ_DATA, sr8);
	m &= 0x7f;
	n1 = n & 0x1f;
	n2 = (n >> 5) & 0x03;
	si->mclk = ((1431818 * (m+2)) / (n1+2) / (1 << n2) + 50) / 100;

	TRACE(("Detected current MCLK value of %1.3f MHz\n", si->mclk / 1000.0));

	// Set the wait functions to be used for the current chip type.

	switch (si->chipset) {
	case S3_SAVAGE3D:
	case S3_SAVAGE_MX:
		 si->WaitQueue		= WaitQueue3D;
		 si->WaitIdleEmpty	= WaitIdleEmpty3D;
		 break;

	case S3_SAVAGE4:
	case S3_PROSAVAGE:
	case S3_SUPERSAVAGE:
	case S3_PROSAVAGEDDR:
	case S3_TWISTER:
		 si->WaitQueue		= WaitQueue4;
		 si->WaitIdleEmpty	= WaitIdleEmpty4;
		 break;

	case S3_SAVAGE2000:
		 si->WaitQueue		= WaitQueue2K;
		 si->WaitIdleEmpty	= WaitIdleEmpty2K;
		 break;
	}

	return true;
}


void 
SavageAdjustFrame(int x, int y)
{
	int address;

	TRACE(("SavageAdjustFrame(%d,%d)\n", x, y));

	x &= ~0x3F;
	address = (y * si->fbc.bytes_per_row) + (x * (si->bitsPerPixel >> 3));
	address &= ~0x1F;

	address += si->frameBufferOffset;

	/*
	 * because we align the viewport to the width and height of one tile
	 * we should update the location of frame
	 */
	si->frameX0 = x;
	si->frameY0 = y;

	if (si->chipset == S3_SAVAGE_MX) {
		OUTREG32(PRI_STREAM_FBUF_ADDR0, address & 0xFFFFFFFC);
		OUTREG32(PRI_STREAM_FBUF_ADDR1, address & 0xFFFFFFFC);
	}
	else if (si->chipset == S3_SUPERSAVAGE) {
		OUTREG32(PRI_STREAM_FBUF_ADDR0, 0x80000000);
		OUTREG32(PRI_STREAM_FBUF_ADDR1, address & 0xFFFFFFF8);
	}
	else if (si->chipset == S3_SAVAGE2000) {
		/*  certain Y values seems to cause havoc, not sure why */
		OUTREG32(PRI_STREAM_FBUF_ADDR0, (address & 0xFFFFFFF8));
		OUTREG32(PRI_STREAM2_FBUF_ADDR0, (address & 0xFFFFFFF8));
	} else {
		OUTREG32(PRI_STREAM_FBUF_ADDR0, address | 0xFFFFFFFC);
		OUTREG32(PRI_STREAM_FBUF_ADDR1, address | 0x80000000);
	}

	return;
}


static void 
SavageGEReset(DisplayMode* pMode)
{
	uint8 cr66;
	int r;

	TRACE(("SavageGEReset begin\n"));

	si->WaitIdleEmpty();

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	cr66 = INREG8(VGA_CRTC_DATA);

	snooze(10000);

	for (r = 1; r < 10; r++) {
		bool bSuccess = false;

		OUTREG8(VGA_CRTC_DATA, cr66 | 0x02);
		snooze(10000);
		OUTREG8(VGA_CRTC_DATA, cr66 & ~0x02);
		snooze(10000);

		si->WaitIdleEmpty();

		OUTREG(DEST_SRC_STR, pMode->bytesPerRow << 16 | pMode->bytesPerRow);

		snooze(10000);
		switch (si->chipset) {
		case S3_SAVAGE3D:
		case S3_SAVAGE_MX:
			bSuccess = (STATUS_WORD0 & 0x0008ffff) == 0x00080000;
			break;

		case S3_SAVAGE4:
		case S3_PROSAVAGE:
		case S3_PROSAVAGEDDR:
		case S3_TWISTER:
		case S3_SUPERSAVAGE:
			bSuccess = (ALT_STATUS_WORD0 & 0x0081ffff) == 0x00800000;
			break;

		case S3_SAVAGE2000:
			bSuccess = (ALT_STATUS_WORD0 & 0x008fffff) == 0;
			break;
		}

		if (bSuccess)
			break;

		snooze(10000);
		TRACE(("Restarting S3 graphics engine reset %2d ...\n", r));
	}

	/* At this point, the FIFO is empty and the engine is idle. */

	OUTREG(SRC_BASE, 0);
	OUTREG(DEST_BASE, 0);
	OUTREG(CLIP_L_R, ((0) << 16) | pMode->timing.h_display);
	OUTREG(CLIP_T_B, ((0) << 16) | pMode->timing.v_display);
	OUTREG(MONO_PAT_0, ~0);
	OUTREG(MONO_PAT_1, ~0);

	SavageSetGBD(pMode);

	TRACE(("SavageGEReset end\n"));
}


static void 
SavageCalcClock(long freq, int min_m, int min_n1, int max_n1,
							int min_n2, int max_n2, long freq_min,
							long freq_max, unsigned int *mdiv,
							unsigned int *ndiv, unsigned int *r)
{
	double ffreq, ffreq_min, ffreq_max;
	double div, diff, best_diff;
	int m;
	unsigned char n1, n2, best_n1 = 16 + 2, best_n2 = 2, best_m = 125 + 2;

	ffreq = freq / 1000.0 / BASE_FREQ;
	ffreq_max = freq_max / 1000.0 / BASE_FREQ;
	ffreq_min = freq_min / 1000.0 / BASE_FREQ;

	if (ffreq < ffreq_min / (1 << max_n2)) {
		TRACE(("SavageCalcClock invalid frequency %1.3f Mhz\n", ffreq * BASE_FREQ));
		ffreq = ffreq_min / (1 << max_n2);
	}
	if (ffreq > ffreq_max / (1 << min_n2)) {
		TRACE(("SavageCalcClock invalid frequency %1.3f Mhz\n", ffreq * BASE_FREQ));
		ffreq = ffreq_max / (1 << min_n2);
	}

	/* work out suitable timings */

	best_diff = ffreq;

	for (n2 = min_n2; n2 <= max_n2; n2++) {
		for (n1 = min_n1 + 2; n1 <= max_n1 + 2; n1++) {
			m = (int)(ffreq * n1 * (1 << n2) + 0.5);
			if (m < min_m + 2 || m > 127 + 2)
				continue;

			div = (double)(m) / (double)(n1);
			if ((div >= ffreq_min) && (div <= ffreq_max)) {
				diff = ffreq - div / (1 << n2);
				if (diff < 0.0)
					diff = -diff;
				if (diff < best_diff) {
					best_diff = diff;
					best_m = m;
					best_n1 = n1;
					best_n2 = n2;
				}
			}
		}
	}

	*ndiv = best_n1 - 2;
	*r = best_n2;
	*mdiv = best_m - 2;
}



static void 
SavageSave(SavageRegRec* pRegRec)
{
	unsigned char cr3a, cr53, cr66;

	TRACE(("SavageSave()\n"));

	OUTREG16(VGA_CRTC_INDEX, 0x4838);
	OUTREG16(VGA_CRTC_INDEX, 0xa039);
	OUTREG16(VGA_SEQ_INDEX, 0x0608);

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	cr66 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr66 | 0x80);

	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	cr3a = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr3a | 0x80);

	OUTREG8(VGA_CRTC_INDEX, 0x53);
	cr53 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr53 & 0x7f);


	OUTREG8(VGA_CRTC_INDEX, 0x66);
	OUTREG8(VGA_CRTC_DATA, cr66);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	OUTREG8(VGA_CRTC_DATA, cr3a);

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	OUTREG8(VGA_CRTC_DATA, cr66);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	OUTREG8(VGA_CRTC_DATA, cr3a);

	/* unlock extended seq regs */
	OUTREG8(VGA_SEQ_INDEX, 0x08);
	pRegRec->SR08 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_DATA, 0x06);

	/* now save all the extended regs we need */
	OUTREG8(VGA_CRTC_INDEX, 0x31);
	pRegRec->CR31 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x32);
	pRegRec->CR32 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x34);
	pRegRec->CR34 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x36);
	pRegRec->CR36 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	pRegRec->CR3A = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x40);
	pRegRec->CR40 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x42);
	pRegRec->CR42 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x45);
	pRegRec->CR45 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	pRegRec->CR50 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x51);
	pRegRec->CR51 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x53);
	pRegRec->CR53 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x58);
	pRegRec->CR58 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x60);
	pRegRec->CR60 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x66);
	pRegRec->CR66 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x67);
	pRegRec->CR67 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x68);
	pRegRec->CR68 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x69);
	pRegRec->CR69 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x6f);
	pRegRec->CR6F = INREG8(VGA_CRTC_DATA);

	OUTREG8(VGA_CRTC_INDEX, 0x33);
	pRegRec->CR33 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x86);
	pRegRec->CR86 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x88);
	pRegRec->CR88 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x90);
	pRegRec->CR90 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x91);
	pRegRec->CR91 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0xb0);
	pRegRec->CRB0 = INREG8(VGA_CRTC_DATA) | 0x80;

	/* extended mode timing regs */
	OUTREG8(VGA_CRTC_INDEX, 0x3b);
	pRegRec->CR3B = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x3c);
	pRegRec->CR3C = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x43);
	pRegRec->CR43 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x5d);
	pRegRec->CR5D = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x5e);
	pRegRec->CR5E = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x65);
	pRegRec->CR65 = INREG8(VGA_CRTC_DATA);

	/* save seq extended regs for DCLK PLL programming */
	OUTREG8(VGA_SEQ_INDEX, 0x0e);
	pRegRec->SR0E = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x0f);
	pRegRec->SR0F = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x10);
	pRegRec->SR10 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x11);
	pRegRec->SR11 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x12);
	pRegRec->SR12 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x13);
	pRegRec->SR13 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x29);
	pRegRec->SR29 = INREG8(VGA_SEQ_DATA);

	OUTREG8(VGA_SEQ_INDEX, 0x15);
	pRegRec->SR15 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x30);
	pRegRec->SR30 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x18);
	pRegRec->SR18 = INREG8(VGA_SEQ_DATA);
	OUTREG8(VGA_SEQ_INDEX, 0x1b);
	pRegRec->SR1B = INREG8(VGA_SEQ_DATA);

	/* Save flat panel expansion registers. */

	if (S3_SAVAGE_MOBILE_SERIES(si->chipset) || S3_MOBILE_TWISTER_SERIES(si->chipset)) {
		int i;
		for (i = 0; i < 8; i++) {
			OUTREG8(VGA_SEQ_INDEX, 0x54 + i);
			pRegRec->SR54[i] = INREG8(VGA_SEQ_DATA);
		}
	}

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	cr66 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr66 | 0x80);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	cr3a = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr3a | 0x80);

	/* now save MIU regs */
	if (!S3_SAVAGE_MOBILE_SERIES(si->chipset)) {
		pRegRec->MMPR0 = INREG(FIFO_CONTROL_REG);
		pRegRec->MMPR1 = INREG(MIU_CONTROL_REG);
		pRegRec->MMPR2 = INREG(STREAMS_TIMEOUT_REG);
		pRegRec->MMPR3 = INREG(MISC_TIMEOUT_REG);
	}

	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	OUTREG8(VGA_CRTC_DATA, cr3a);
	OUTREG8(VGA_CRTC_INDEX, 0x66);
	OUTREG8(VGA_CRTC_DATA, cr66);

	return;
}


static void 
SavageWriteMode(DisplayMode* pMode, SavageRegRec* pRegRec)
{
	uint8 temp, cr3a, cr66;
	int j;

	TRACE(("SavageWriteMode\n"));

	si->WaitIdleEmpty();

	if (!S3_SAVAGE_MOBILE_SERIES(si->chipset))
		SavageInitialize2DEngine(pMode);

	OUTREG8(VGA_MISC_OUT_W, 0x23);

	OUTREG16(VGA_CRTC_INDEX, 0x4838);
	OUTREG16(VGA_CRTC_INDEX, 0xa039);
	OUTREG16(VGA_SEQ_INDEX, 0x0608);

	/* reset GE to make sure nothing is going on */
	OUTREG8(VGA_CRTC_INDEX, 0x66);
	if (INREG8(VGA_CRTC_DATA) & 0x01)
		SavageGEReset(pMode);

	/*
	 * Some Savage/MX and /IX systems go nuts when trying to exit the
	 * server after WindowMaker has displayed a gradient background.  I
	 * haven't been able to find what causes it, but a non-destructive
	 * switch to mode 3 here seems to eliminate the issue.
	 */

	OUTREG8(VGA_CRTC_INDEX, 0x67);
	(void) INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR67 & ~0x0e); /* no STREAMS yet old and new */

	/* restore extended regs */
	OUTREG8(VGA_CRTC_INDEX, 0x66);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR66);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR3A);
	OUTREG8(VGA_CRTC_INDEX, 0x31);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR31);
	OUTREG8(VGA_CRTC_INDEX, 0x32);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR32);
	OUTREG8(VGA_CRTC_INDEX, 0x58);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR58);
	OUTREG8(VGA_CRTC_INDEX, 0x53);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR53 & 0x7f);

	OUTREG16(VGA_SEQ_INDEX, 0x0608);

	/* Restore DCLK registers. */

	OUTREG8(VGA_SEQ_INDEX, 0x0e);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR0E);
	OUTREG8(VGA_SEQ_INDEX, 0x0f);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR0F);
	OUTREG8(VGA_SEQ_INDEX, 0x29);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR29);
	OUTREG8(VGA_SEQ_INDEX, 0x15);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR15);

	/* Restore flat panel expansion registers. */

	if (S3_SAVAGE_MOBILE_SERIES(si->chipset) || S3_MOBILE_TWISTER_SERIES(si->chipset)) {
		int i;
		for ( i = 0; i < 8; i++ ) {
			OUTREG8(VGA_SEQ_INDEX, 0x54 + i);
			OUTREG8(VGA_SEQ_DATA, pRegRec->SR54[i]);
		}
	}

	/* Set the standard CRTC vga regs */

	WriteCrtc(17, pRegRec->CRTC[17] & 0x7f);	// unlock CRTC reg's 0-7 by clearing bit 7 of CRTC[17]

	for (j = 0; j < NUM_ELEMENTS(pRegRec->CRTC); j++)
		WriteCrtc(j, pRegRec->CRTC[j]);

	/* setup HSYNC & VSYNC polarity */

	temp = ((INREG8(VGA_MISC_OUT_R) & 0x3f) | 0x0c);

	if (!(pMode->timing.flags & B_POSITIVE_HSYNC))
		temp |= 0x40;
	if (!(pMode->timing.flags & B_POSITIVE_VSYNC))
		temp |= 0x80;

	OUTREG8(VGA_MISC_OUT_W, temp);
	

	/* extended mode timing registers */
	OUTREG8(VGA_CRTC_INDEX, 0x53);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR53);
	OUTREG8(VGA_CRTC_INDEX, 0x5d);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR5D);
	OUTREG8(VGA_CRTC_INDEX, 0x5e);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR5E);
	OUTREG8(VGA_CRTC_INDEX, 0x3b);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR3B);
	OUTREG8(VGA_CRTC_INDEX, 0x3c);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR3C);
	OUTREG8(VGA_CRTC_INDEX, 0x43);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR43);
	OUTREG8(VGA_CRTC_INDEX, 0x65);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR65);

	/* restore the desired video mode with cr67 */
	OUTREG8(VGA_CRTC_INDEX, 0x67);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR67 & ~0x0e); /* no streams for new and old streams engines */

	/* other mode timing and extended regs */
	OUTREG8(VGA_CRTC_INDEX, 0x34);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR34);
	OUTREG8(VGA_CRTC_INDEX, 0x40);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR40);
	OUTREG8(VGA_CRTC_INDEX, 0x42);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR42);
	OUTREG8(VGA_CRTC_INDEX, 0x45);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR45);
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR50);
	OUTREG8(VGA_CRTC_INDEX, 0x51);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR51);

	/* memory timings */
	OUTREG8(VGA_CRTC_INDEX, 0x36);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR36);
	OUTREG8(VGA_CRTC_INDEX, 0x60);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR60);
	OUTREG8(VGA_CRTC_INDEX, 0x68);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR68);
	VerticalRetraceWait();
	OUTREG8(VGA_CRTC_INDEX, 0x69);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR69);
	OUTREG8(VGA_CRTC_INDEX, 0x6f);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR6F);

	OUTREG8(VGA_CRTC_INDEX, 0x33);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR33);
	OUTREG8(VGA_CRTC_INDEX, 0x86);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR86);
	OUTREG8(VGA_CRTC_INDEX, 0x88);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR88);
	OUTREG8(VGA_CRTC_INDEX, 0x90);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR90);
	OUTREG8(VGA_CRTC_INDEX, 0x91);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR91);

	if (si->chipset == S3_SAVAGE4) {
		OUTREG8(VGA_CRTC_INDEX, 0xb0);
		OUTREG8(VGA_CRTC_DATA, pRegRec->CRB0);
	}

	OUTREG8(VGA_CRTC_INDEX, 0x32);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR32);

	/* unlock extended seq regs */
	OUTREG8(VGA_SEQ_INDEX, 0x08);
	OUTREG8(VGA_SEQ_DATA, 0x06);

	/* Restore extended sequencer regs for MCLK. SR10 == 255 indicates that
	 * we should leave the default SR10 and SR11 values there.
	 */

	if (pRegRec->SR10 != 255) {
		OUTREG8(VGA_SEQ_INDEX, 0x10);
		OUTREG8(VGA_SEQ_DATA, pRegRec->SR10);
		OUTREG8(VGA_SEQ_INDEX, 0x11);
		OUTREG8(VGA_SEQ_DATA, pRegRec->SR11);
	}

	/* restore extended seq regs for dclk */
	OUTREG8(VGA_SEQ_INDEX, 0x0e);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR0E);
	OUTREG8(VGA_SEQ_INDEX, 0x0f);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR0F);
	OUTREG8(VGA_SEQ_INDEX, 0x12);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR12);
	OUTREG8(VGA_SEQ_INDEX, 0x13);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR13);
	OUTREG8(VGA_SEQ_INDEX, 0x29);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR29);

	OUTREG8(VGA_SEQ_INDEX, 0x18);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR18);
	OUTREG8(VGA_SEQ_INDEX, 0x1b);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR1B);

	/* load new m, n pll values for dclk & mclk */
	OUTREG8(VGA_SEQ_INDEX, 0x15);
	temp = INREG8(VGA_SEQ_DATA) & ~0x21;

	OUTREG8(VGA_SEQ_DATA, temp | 0x03);
	OUTREG8(VGA_SEQ_DATA, temp | 0x23);
	OUTREG8(VGA_SEQ_DATA, temp | 0x03);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR15);
	snooze( 100 );

	OUTREG8(VGA_SEQ_INDEX, 0x30);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR30);
	OUTREG8(VGA_SEQ_INDEX, 0x08);
	OUTREG8(VGA_SEQ_DATA, pRegRec->SR08);

	/* now write out cr67 in full, possibly starting STREAMS */
	VerticalRetraceWait();
	OUTREG8(VGA_CRTC_INDEX, 0x67);
	OUTREG8(VGA_CRTC_DATA, pRegRec->CR67);

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	cr66 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr66 | 0x80);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	cr3a = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_DATA, cr3a | 0x80);

	SavageGEReset(pMode);

	if (!S3_SAVAGE_MOBILE_SERIES(si->chipset)) {
		VerticalRetraceWait();
		OUTREG(FIFO_CONTROL_REG, pRegRec->MMPR0);
		OUTREG(MIU_CONTROL_REG, pRegRec->MMPR1);
		OUTREG(STREAMS_TIMEOUT_REG, pRegRec->MMPR2);
		OUTREG(MISC_TIMEOUT_REG, pRegRec->MMPR3);
	}

	/* If we're going into graphics mode and acceleration was enabled, */
	/* go set up the BCI buffer and the global bitmap descriptor. */

	OUTREG8(VGA_CRTC_INDEX, 0x66);
	OUTREG8(VGA_CRTC_DATA, cr66);
	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	OUTREG8(VGA_CRTC_DATA, cr3a);

	SavageInitialize2DEngine(pMode);

	OUTREG16(VGA_CRTC_INDEX, 0x0140);

	SavageSetGBD(pMode);

	TRACE(("SavageWriteMode end\n"));

	return;
}


bool 
SavageModeInit(DisplayMode* pMode)
{
	int width, dclk, i, j;
	unsigned int m, n, r;
	uint8 temp = 0;
	SavageRegRec regRec;

	InitCrtcTimingValues(pMode, &regRec);
	SavageSave(&regRec);

	TRACE(("SavageModeInit(%dx%d, %dHz)\n",
			pMode->timing.h_display, pMode->timing.v_display, pMode->timing.pixel_clock));

	/* We need to set CR67 whether or not we use the BIOS. */

	dclk = pMode->timing.pixel_clock;
	regRec.CR67 = 0x00;

	switch (pMode->bpp) {
	case 8:
		if ((si->chipset == S3_SAVAGE2000) && (dclk >= 230000))
			regRec.CR67 = 0x10;	/* 8bpp, 2 pixels/clock */
		else
			regRec.CR67 = 0x00;	/* 8bpp, 1 pixel/clock */
		break;

	case 15:
		if (S3_SAVAGE_MOBILE_SERIES(si->chipset)
			|| ((si->chipset == S3_SAVAGE2000) && (dclk >= 230000)))
			regRec.CR67 = 0x30;	/* 15bpp, 2 pixel/clock */
		else
			regRec.CR67 = 0x20;	/* 15bpp, 1 pixels/clock */
		break;

	case 16:
		if (S3_SAVAGE_MOBILE_SERIES(si->chipset)
			|| ((si->chipset == S3_SAVAGE2000) && (dclk >= 230000)))
			regRec.CR67 = 0x50;	/* 16bpp, 2 pixel/clock */
		else
			regRec.CR67 = 0x40;	/* 16bpp, 1 pixels/clock */
		break;

	case 24:
		regRec.CR67 = 0x70;
		break;

	case 32:
		regRec.CR67 = 0xd0;
		break;
	}


	// Use traditional register-crunching to set mode.

	OUTREG8(VGA_CRTC_INDEX, 0x3a);
	temp = INREG8(VGA_CRTC_DATA);
	if (true /*psav->pci_burst*/)
		regRec.CR3A = (temp & 0x7f) | 0x15;
	else
		regRec.CR3A = temp | 0x95;

	regRec.CR53 = 0x00;
	regRec.CR31 = 0x8c;
	regRec.CR66 = 0x89;

	OUTREG8(VGA_CRTC_INDEX, 0x58);
	regRec.CR58 = INREG8(VGA_CRTC_DATA) & 0x80;
	regRec.CR58 |= 0x13;

	regRec.SR15 = 0x03 | 0x80;
	regRec.SR18 = 0x00;

	/* set 8-bit CLUT */
	regRec.SR1B |= 0x10;

	regRec.CR43 = regRec.CR45 = regRec.CR65 = 0x00;

	OUTREG8(VGA_CRTC_INDEX, 0x40);
	regRec.CR40 = INREG8(VGA_CRTC_DATA) & ~0x01;

	regRec.MMPR0 = 0x010400;
	regRec.MMPR1 = 0x00;
	regRec.MMPR2 = 0x0808;
	regRec.MMPR3 = 0x08080810;

	if (si->mclk <= 0) {
		regRec.SR10 = 255;
		regRec.SR11 = 255;
	}

	SavageCalcClock(dclk, 1, 1, 127, 0, 4, 180000, 360000, &m, &n, &r);
	regRec.SR12 = (r << 6) | (n & 0x3f);
	regRec.SR13 = m & 0xff;
	regRec.SR29 = (r & 4) | (m & 0x100) >> 5 | (n & 0x40) >> 2;

	TRACE(("CalcClock, m: %d  n: %d  r: %d\n", m, n, r));

	if (pMode->bpp < 24)
		regRec.MMPR0 -= 0x8000;
	else
		regRec.MMPR0 -= 0x4000;

	regRec.CR42 = 0x00;
	regRec.CR34 = 0x10;

	i = ((((pMode->timing.h_total >> 3) - 5) & 0x100) >> 8) |
		((((pMode->timing.h_display >> 3) - 1) & 0x100) >> 7) |
		((((pMode->timing.h_sync_start >> 3) - 1) & 0x100) >> 6) |
		((pMode->timing.h_sync_start & 0x800) >> 7);

	if ((pMode->timing.h_sync_end >> 3) - (pMode->timing.h_sync_start >> 3) > 64)
		i |= 0x08;
	if ((pMode->timing.h_sync_end >> 3) - (pMode->timing.h_sync_start >> 3) > 32)
		i |= 0x20;

	j = (regRec.CRTC[0] + ((i & 0x01) << 8) + regRec.CRTC[4] + ((i & 0x10) << 4) + 1) / 2;

	if (j - (regRec.CRTC[4] + ((i & 0x10) << 4)) < 4) {
		if (regRec.CRTC[4] + ((i & 0x10) << 4) + 4 <= regRec.CRTC[0] + ((i & 0x01) << 8))
			j = regRec.CRTC[4] + ((i & 0x10) << 4) + 4;
		else
			j = regRec.CRTC[0] + ((i & 0x01) << 8) + 1;
	}

	regRec.CR3B = j & 0xff;
	i |= (j & 0x100) >> 2;
	regRec.CR3C = (regRec.CRTC[0] + ((i & 0x01) << 8)) / 2 ;
	regRec.CR5D = i;
	regRec.CR5E = (((pMode->timing.v_total - 2) & 0x400) >> 10) |
				(((pMode->timing.v_display - 1) & 0x400) >> 9) |
				(((pMode->timing.v_sync_start) & 0x400) >> 8) |
				(((pMode->timing.v_sync_start) & 0x400) >> 6) | 0x40;
	width = pMode->bytesPerRow >> 3;
	regRec.CR91 = 0xff & width;
	regRec.CR51 = (0x300 & width) >> 4;
	regRec.CR90 = 0x80 | (width >> 8);

	/* Set frame buffer description. */

	if (pMode->bpp <= 8)
		regRec.CR50 = 0;
	else if (pMode->bpp <= 16)
		regRec.CR50 = 0x10;
	else
		regRec.CR50 = 0x30;

	if (pMode->width == 640)
		regRec.CR50 |= 0x40;
	else if (pMode->width == 800)
		regRec.CR50 |= 0x80;
	else if (pMode->width == 1024)
		regRec.CR50 |= 0x00;
	else if (pMode->width == 1152)
		regRec.CR50 |= 0x01;
	else if (pMode->width == 1280)
		regRec.CR50 |= 0xc0;
	else if (pMode->width == 1600)
		regRec.CR50 |= 0x81;
	else
		regRec.CR50 |= 0xc1;	/* Use GBD */

	if (S3_SAVAGE_MOBILE_SERIES(si->chipset))
		regRec.CR33 = 0x00;
	else
		regRec.CR33 = 0x08;

	regRec.CR67 |= 1;

	OUTREG8(VGA_CRTC_INDEX, 0x36);
	regRec.CR36 = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x68);
	regRec.CR68 = INREG8(VGA_CRTC_DATA);

	regRec.CR69 = 0;
	OUTREG8(VGA_CRTC_INDEX, 0x6f);
	regRec.CR6F = INREG8(VGA_CRTC_DATA);
	OUTREG8(VGA_CRTC_INDEX, 0x86);
	regRec.CR86 = INREG8(VGA_CRTC_DATA) | 0x08;
	OUTREG8(VGA_CRTC_INDEX, 0x88);
	regRec.CR88 = INREG8(VGA_CRTC_DATA) | DISABLE_BLOCK_WRITE_2D;
	OUTREG8(VGA_CRTC_INDEX, 0xb0);
	regRec.CRB0 = INREG8(VGA_CRTC_DATA) | 0x80;

	/* do it! */
	SavageWriteMode(pMode, &regRec);

	return true;
}

