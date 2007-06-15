/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright 1995-1997 The XFree86 Project, Inc.

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2007
*/


#include "GlobalData.h"
#include "AccelerantPrototypes.h"
#include "savage.h"


static void 
SavageSetGBD_Twister(DisplayMode* pMode)
{
	uint32 ulTmp;
	uint8 byte;
	int bci_enable;

	TRACE(("SavageSetGBD_Twister\n"));

	if (si->chipset == S3_SAVAGE4)
		bci_enable = BCI_ENABLE;
	else
		bci_enable = BCI_ENABLE_TWISTER;

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0, 0);
	OUTREG32(PRI_STREAM_FBUF_ADDR1, 0);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */

	OUTREG32(PRI_STREAM_STRIDE,
			 (((pMode->bytesPerRow * 2) << 16) & 0x3FFFE000) |
			 (pMode->bytesPerRow & 0x00001fff));

	/*
	 *  CR69, bit 7 = 1
	 *  to use MM streams processor registers to control primary stream.
	 */
	OUTREG8(VGA_CRTC_INDEX, 0x69);
	byte = INREG8(VGA_CRTC_DATA) | 0x80;
	OUTREG8(VGA_CRTC_DATA, byte);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	byte = INREG8(VGA_CRTC_DATA) | 0xC1;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 * if MS1NB style linear tiling mode.
	 * bit MM850C[15] = 0 select NB linear tile mode.
	 * bit MM850C[15] = 1 select MS-1 128-bit non-linear tile mode.
	 */
	ulTmp = INREG32(ADVANCED_FUNC_CTRL) | 0x8000; /* use MS-s style tile mode*/
	OUTREG32(ADVANCED_FUNC_CTRL, ulTmp);

	/*
	 * Set up Tiled Surface Registers
	 *  Bit 25:20 - Surface width in tiles.
	 *  Bit 29 - Y Range Flag.
	 *  Bit 31:30	= 00, 4 bpp.
	 * 				= 01, 8 bpp.
	 * 				= 10, 16 bpp.
	 * 				= 11, 32 bpp.
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C - twister/prosavage
	 *	bit 24~25: tile format
	 * 		 00: linear
	 * 		 01: destination tiling format
	 * 		 10: texture tiling format
	 * 		 11: reserved
	 *	bit 28: block write disble/enable
	 * 		 0: disable
	 * 		 1: enable
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C - savage4
	 *	bit 24~25: tile format
	 * 		 00: linear
	 * 		 01: reserved
	 * 		 10: 16 bpp tiles
	 * 		 11: 32 bpp tiles
	 *	bit 28: block write disable/enable
	 * 		 0: enable
	 * 		 1: disable
	 */

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	si->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR; /* linear */
	si->GlobalBD.bd1.HighPart.ResBWTile |= 0x10; /* disable block write */
	/* HW uses width */
	si->GlobalBD.bd1.HighPart.Stride = (uint16) (pMode->width);	// number of pixels per line
	si->GlobalBD.bd1.HighPart.Bpp = (uint8) (pMode->bpp);
	si->GlobalBD.bd1.Offset = si->frameBufferOffset;

	/*
	 * CR88, bit 4 - Block write enabled/disabled.
	 *
	 * 	 Note:	Block write must be disabled when writing to tiled
	 *			memory. Even when writing to non-tiled memory, block
	 *			write should only be enabled for certain types of SGRAM.
	 */
	OUTREG8(VGA_CRTC_INDEX, 0x88);
	byte = INREG8(VGA_CRTC_DATA) | DISABLE_BLOCK_WRITE_2D;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 * 	  bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 * 				 at A000:0.
	 */
	OUTREG8(VGA_CRTC_INDEX, MEMORY_CONFIG_REG); /* cr31 */
	byte = INREG8(VGA_CRTC_DATA) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(VGA_CRTC_DATA, byte); /* perhaps this should be 0x0c */

	/* turn on screen */
	OUTREG8(VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) & ~0x20;
	OUTREG8(VGA_SEQ_DATA, byte);

	/* program the GBD and SBD's */
	OUTREG32(S3_GLB_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_GLB_BD_HIGH, si->GlobalBD.bd2.HiPart | bci_enable | S3_LITTLE_ENDIAN | S3_BD64);
	OUTREG32(S3_PRI_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH, si->GlobalBD.bd2.HiPart);
}


static void 
SavageSetGBD_3D(DisplayMode* pMode)
{
	uint32 ulTmp;
	uint8	byte;
	int bci_enable;

	TRACE(("SavageSetGBD_3D\n"));

	bci_enable = BCI_ENABLE;

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0, 0);
	OUTREG32(PRI_STREAM_FBUF_ADDR1, 0);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */

	OUTREG32(PRI_STREAM_STRIDE,
			 (((pMode->bytesPerRow * 2) << 16) & 0x3FFFE000) |
			 (pMode->bytesPerRow & 0x00001fff));

	/*
	 *  CR69, bit 7 = 1
	 *  to use MM streams processor registers to control primary stream.
	 */
	OUTREG8(VGA_CRTC_INDEX, 0x69);
	byte = INREG8(VGA_CRTC_DATA) | 0x80;
	OUTREG8(VGA_CRTC_DATA, byte);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	byte = INREG8(VGA_CRTC_DATA) | 0xC1;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 * if MS1NB style linear tiling mode.
	 * bit MM850C[15] = 0 select NB linear tile mode.
	 * bit MM850C[15] = 1 select MS-1 128-bit non-linear tile mode.
	 */
	ulTmp = INREG32(ADVANCED_FUNC_CTRL) | 0x8000; /* use MS-s style tile mode*/
	OUTREG32(ADVANCED_FUNC_CTRL, ulTmp);

	/*
	 * Tiled Surface 0 Registers MM48C40:
	 *  bit 0~23: tile surface 0 frame buffer offset
	 *  bit 24~29:tile surface 0 width
	 *  bit 30~31:tile surface 0 bits/pixel
	 *			00: reserved
	 *			01, 8 bits
	 *			10, 16 Bits.
	 *			11, 32 Bits.
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C
	 *	 bit 24~25: tile format
	 * 		 00: linear
	 * 		 01: reserved
	 * 		 10: 16 bpp tiles
	 * 		 11: 32 bpp tiles
	 *	 bit 28: block write disable/enable
	 * 		 0: enable
	 * 		 1: disable
	 */

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	si->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR; /* linear */
	si->GlobalBD.bd1.HighPart.ResBWTile |= 0x10; /* disable block write */
	/* HW uses width */
	si->GlobalBD.bd1.HighPart.Stride = (uint16) (pMode->width);	// number of pixels per line
	si->GlobalBD.bd1.HighPart.Bpp = (uint8) (pMode->bpp);
	si->GlobalBD.bd1.Offset = si->frameBufferOffset;

	/*
	 * CR88, bit 4 - Block write enabled/disabled.
	 *
	 *	Note:	Block write must be disabled when writing to tiled
	 *			memory.	Even when writing to non-tiled memory, block
	 *			write should only be enabled for certain types of SGRAM.
	 */
	OUTREG8(VGA_CRTC_INDEX, 0x88);
	byte = INREG8(VGA_CRTC_DATA) | DISABLE_BLOCK_WRITE_2D;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 * 	  bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 * 				 at A000:0.
	 */
	OUTREG8(VGA_CRTC_INDEX, MEMORY_CONFIG_REG); /* cr31 */
	byte = INREG8(VGA_CRTC_DATA) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(VGA_CRTC_DATA, byte); /* perhaps this should be 0x0c */

	/* turn on screen */
	OUTREG8(VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) & ~0x20;
	OUTREG8(VGA_SEQ_DATA, byte);

	/* program the GBD and SBD's */
	OUTREG32(S3_GLB_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_GLB_BD_HIGH, si->GlobalBD.bd2.HiPart | bci_enable | S3_LITTLE_ENDIAN | S3_BD64);
	OUTREG32(S3_PRI_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH, si->GlobalBD.bd2.HiPart);
}


static void 
SavageSetGBD_M7(DisplayMode* pMode)
{
	uint8 byte;
	int bci_enable;

	TRACE(("SavageSetGBD_M7\n"));

	bci_enable = BCI_ENABLE;

	/* following is the enable case */

	/* SR01:turn off screen */
	OUTREG8 (VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) | 0x20;
	OUTREG8(VGA_SEQ_DATA, byte);

	/*
	 * CR67_3:
	 *  = 1  stream processor MMIO address and stride register
	 * 	  are used to control the primary stream
	 *  = 0  standard VGA address and stride registers
	 * 	  are used to control the primary streams
	 */

	OUTREG8(VGA_CRTC_INDEX, 0x67);
	byte = INREG8(VGA_CRTC_DATA) | 0x08;
	OUTREG8(VGA_CRTC_DATA, byte);
	/* IGA 2 */
	OUTREG16(VGA_SEQ_INDEX, SELECT_IGA2_READS_WRITES);
	OUTREG8(VGA_CRTC_INDEX, 0x67);
	byte = INREG8(VGA_CRTC_DATA) | 0x08;
	OUTREG8(VGA_CRTC_DATA, byte);
	OUTREG16(VGA_SEQ_INDEX, SELECT_IGA1);

	/* Set primary stream to bank 0 */
	OUTREG8(VGA_CRTC_INDEX, MEMORY_CTRL0_REG); /* CRCA */
	byte = INREG8(VGA_CRTC_DATA) & ~(MEM_PS1 + MEM_PS2) ;
	OUTREG8(VGA_CRTC_DATA, byte);

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0,  si->frameBufferOffset & 0x7fffff);
	OUTREG32(PRI_STREAM_FBUF_ADDR1,  si->frameBufferOffset & 0x7fffff);
	OUTREG32(PRI_STREAM2_FBUF_ADDR0, si->frameBufferOffset & 0x7fffff);
	OUTREG32(PRI_STREAM2_FBUF_ADDR1, si->frameBufferOffset & 0x7fffff);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */

	OUTREG32(PRI_STREAM_STRIDE,
			 (((pMode->bytesPerRow * 2) << 16) & 0x3FFF0000) |
			 (pMode->bytesPerRow & 0x00003fff));
	OUTREG32(PRI_STREAM2_STRIDE,
			 (((pMode->bytesPerRow * 2) << 16) & 0x3FFF0000) |
			 (pMode->bytesPerRow & 0x00003fff));

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	byte = INREG8(VGA_CRTC_DATA) | 0xC1;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 * CR78, bit 3  - Block write enabled(1)/disabled(0).
	 *		 bit 2  - Block write cycle time(0:2 cycles,1: 1 cycle)
	 *	Note:	Block write must be disabled when writing to tiled
	 *			memory.	Even when writing to non-tiled memory, block
	 *			write should only be enabled for certain types of SGRAM.
	 */
	OUTREG8(VGA_CRTC_INDEX, 0x78);
	byte = INREG8(VGA_CRTC_DATA) | 0xfb;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 * Tiled Surface 0 Registers MM48C40:
	 *  bit 0~23: tile surface 0 frame buffer offset
	 *  bit 24~29:tile surface 0 width
	 *  bit 30~31:tile surface 0 bits/pixel
	 *			00: reserved
	 *			01, 8 bits
	 *			10, 16 Bits.
	 *			11, 32 Bits.
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C
	 *	 bit 24~25: tile format
	 * 		 00: linear
	 * 		 01: reserved
	 * 		 10: 16 bit
	 * 		 11: 32 bit
	 *	 bit 28: block write disble/enable
	 * 		 0: enable
	 * 		 1: disable
	 */

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	si->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR; /* linear */
	si->GlobalBD.bd1.HighPart.ResBWTile |= 0x10; /* disable block write */
	/* HW uses width */
	si->GlobalBD.bd1.HighPart.Stride = (uint16)(pMode->width);	// number of pixels per line
	si->GlobalBD.bd1.HighPart.Bpp = (uint8)(pMode->bpp);
	si->GlobalBD.bd1.Offset = si->frameBufferOffset;

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 * 	  bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 * 				 at A000:0.
	 */

	OUTREG8(VGA_CRTC_INDEX, MEMORY_CONFIG_REG); /* cr31 */
	byte = (INREG8(VGA_CRTC_DATA) | 0x04) & 0xFE;
	OUTREG8(VGA_CRTC_DATA, byte);

	/* program the GBD and SBD's */
	OUTREG32(S3_GLB_BD_LOW, si->GlobalBD.bd2.LoPart );
	/* 8: bci enable */
	OUTREG32(S3_GLB_BD_HIGH, (si->GlobalBD.bd2.HiPart
							  | bci_enable | S3_LITTLE_ENDIAN | S3_BD64));
	OUTREG32(S3_PRI_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH, si->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH, si->GlobalBD.bd2.HiPart);

	/* turn on screen */
	OUTREG8(VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) & ~0x20;
	OUTREG8(VGA_SEQ_DATA, byte);
}


static void 
SavageSetGBD_PM(DisplayMode* pMode)
{
	uint8 byte;
	int bci_enable;

	TRACE(("SavageSetGBD_PM\n"));

	bci_enable = BCI_ENABLE_TWISTER;

	/* following is the enable case */

	/* SR01:turn off screen */
	OUTREG8 (VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) | 0x20;
	OUTREG8(VGA_SEQ_DATA, byte);

	/*
	 * CR67_3:
	 *  = 1  stream processor MMIO address and stride register
	 * 	  are used to control the primary stream
	 *  = 0  standard VGA address and stride registers
	 * 	  are used to control the primary streams
	 */

	OUTREG8(VGA_CRTC_INDEX, 0x67);
	byte = INREG8(VGA_CRTC_DATA) | 0x08;
	OUTREG8(VGA_CRTC_DATA, byte);
	/* IGA 2 */
	OUTREG16(VGA_SEQ_INDEX, SELECT_IGA2_READS_WRITES);

	OUTREG8(VGA_CRTC_INDEX, 0x67);
	byte = INREG8(VGA_CRTC_DATA) | 0x08;
	OUTREG8(VGA_CRTC_DATA, byte);

	OUTREG16(VGA_SEQ_INDEX, SELECT_IGA1);

	/*
	 * load ps1 active registers as determined by MM81C0/81C4
	 * load ps2 active registers as determined by MM81B0/81B4
	 */
	OUTREG8(VGA_CRTC_INDEX, 0x65);
	byte = INREG8(VGA_CRTC_DATA) | 0x03;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */

	OUTREG32(PRI_STREAM_STRIDE,
			 (((pMode->bytesPerRow * 2) << 16) & 0x3FFF0000) |
			 (pMode->bytesPerRow & 0x00001fff));
	OUTREG32(PRI_STREAM2_STRIDE,
			 (((pMode->bytesPerRow * 2) << 16) & 0x3FFF0000) |
			 (pMode->bytesPerRow & 0x00001fff));

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0, si->frameBufferOffset);
	OUTREG32(PRI_STREAM_FBUF_ADDR1, 0x80000000);
	OUTREG32(PRI_STREAM2_FBUF_ADDR0, (si->frameBufferOffset & 0xfffffffc) | 0x80000000);
	OUTREG32(PRI_STREAM2_FBUF_ADDR1, si->frameBufferOffset & 0xfffffffc);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	/* bit 28:block write disable */
	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_BD64 | 0x10000000);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	byte = INREG8(VGA_CRTC_DATA) | 0xC1;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	si->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR; /* linear */
	si->GlobalBD.bd1.HighPart.ResBWTile |= 0x10; /* disable block write */
	/* HW uses width */
	si->GlobalBD.bd1.HighPart.Stride = (uint16) (pMode->width);	// number of pixels per line
	si->GlobalBD.bd1.HighPart.Bpp = (uint8) (pMode->bpp);
	si->GlobalBD.bd1.Offset = si->frameBufferOffset;

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 * 	  bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 * 				 at A000:0.
	 */
	OUTREG8(VGA_CRTC_INDEX, MEMORY_CONFIG_REG);
	byte = INREG8(VGA_CRTC_DATA) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(VGA_CRTC_DATA, byte);

	/* program the GBD and SBDs */
	OUTREG32(S3_GLB_BD_LOW, si->GlobalBD.bd2.LoPart );
	OUTREG32(S3_GLB_BD_HIGH, (si->GlobalBD.bd2.HiPart
							  | bci_enable | S3_LITTLE_ENDIAN | S3_BD64));
	OUTREG32(S3_PRI_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH, si->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH, si->GlobalBD.bd2.HiPart);

	/* turn on screen */
	OUTREG8(VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) & ~0x20;
	OUTREG8(VGA_SEQ_DATA, byte);
}


static void 
SavageSetGBD_2000(DisplayMode* pMode)
{
	uint32 ulYRange;
	uint8 byte;
	int bci_enable;

	TRACE(("SavageSetGBD_2000\n"));

	bci_enable = BCI_ENABLE_TWISTER;

	if (pMode->width > 1024)
		ulYRange = 0x40000000;
	else
		ulYRange = 0x20000000;

	/* following is the enable case */

	/* SR01:turn off screen */
	OUTREG8 (VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) | 0x20;
	OUTREG8(VGA_SEQ_DATA, byte);

	/* MM81C0 and 81B0 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0, si->frameBufferOffset);
	OUTREG32(PRI_STREAM2_FBUF_ADDR0, si->frameBufferOffset);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */
	OUTREG32(PRI_STREAM_STRIDE,  ((pMode->bytesPerRow << 4) & 0x7ff0));
	OUTREG32(PRI_STREAM2_STRIDE, ((pMode->bytesPerRow << 4) & 0x7ff0));

	/*
	 * CR67_3:
	 *  = 1  stream processor MMIO address and stride register
	 * 	  are used to control the primary stream
	 *  = 0  standard VGA address and stride registers
	 * 	  are used to control the primary streams
	 */

	OUTREG8(VGA_CRTC_INDEX, 0x67);
	byte = INREG8(VGA_CRTC_DATA) | 0x08;
	OUTREG8(VGA_CRTC_DATA, byte);


	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	/* bit 28:block write disable */
	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_BD64 | 0x10000000);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(VGA_CRTC_INDEX, 0x50);
	byte = INREG8(VGA_CRTC_DATA) | 0xC1;
	OUTREG8(VGA_CRTC_DATA, byte);

	/* CR73 bit 5 = 0 block write disable */
	OUTREG8(VGA_CRTC_INDEX, 0x73);
	byte = INREG8(VGA_CRTC_DATA) & ~0x20;
	OUTREG8(VGA_CRTC_DATA, byte);

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	si->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR; /* linear */
	si->GlobalBD.bd1.HighPart.ResBWTile |= 0x10; /* disable block write */
	/* HW uses width */
	si->GlobalBD.bd1.HighPart.Stride = (uint16) (pMode->width);	// number of pixels per line
	si->GlobalBD.bd1.HighPart.Bpp = (uint8) (pMode->bpp);
	si->GlobalBD.bd1.Offset = si->frameBufferOffset;

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 * 	  bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 * 				 at A000:0.
	 */
	OUTREG8(VGA_CRTC_INDEX, MEMORY_CONFIG_REG);
	byte = INREG8(VGA_CRTC_DATA) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(VGA_CRTC_DATA, byte);

	/* program the GBD and SBDs */
	OUTREG32(S3_GLB_BD_LOW, si->GlobalBD.bd2.LoPart );
	OUTREG32(S3_GLB_BD_HIGH, (si->GlobalBD.bd2.HiPart
							  | bci_enable | S3_LITTLE_ENDIAN | S3_BD64));
	OUTREG32(S3_PRI_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH, si->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW, si->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH, si->GlobalBD.bd2.HiPart);

	/* turn on screen */
	OUTREG8(VGA_SEQ_INDEX, 0x01);
	byte = INREG8(VGA_SEQ_DATA) & ~0x20;
	OUTREG8(VGA_SEQ_DATA, byte);
}


void 
SavageSetGBD(DisplayMode* pMode)
{
	TRACE(("SavageSetGBD\n"));

	UnProtectCRTC();
	UnLockExtRegs();
	VerticalRetraceWait();

	switch (si->chipset) {
	case S3_SAVAGE3D:
		SavageSetGBD_3D(pMode);
		break;

	case S3_SAVAGE_MX:
		SavageSetGBD_M7(pMode);
		break;

	case S3_SAVAGE4:
	case S3_PROSAVAGE:
	case S3_TWISTER:
	case S3_PROSAVAGEDDR:
		SavageSetGBD_Twister(pMode);
		break;

	case S3_SUPERSAVAGE:
		SavageSetGBD_PM(pMode);
		break;

	case S3_SAVAGE2000:
		SavageSetGBD_2000(pMode);
		break;
	}
}


void 
SavageInitialize2DEngine(DisplayMode* pMode)
{
	uint32 thresholds;

	TRACE(("SavageInitialize2DEngine\n"));

	OUTREG16(VGA_CRTC_INDEX, 0x0140);
	OUTREG8(VGA_CRTC_INDEX, 0x31);
	OUTREG8(VGA_CRTC_DATA, 0x0c);

	/* Setup plane masks */
	OUTREG(0x8128, ~0); /* enable all write planes */
	OUTREG(0x812C, ~0); /* enable all read planes */
	OUTREG16(0x8134, 0x27);
	OUTREG16(0x8136, 0x07);

	switch (si->chipset) {
	case S3_SAVAGE3D:
	case S3_SAVAGE_MX:

		/* Disable BCI */
		OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);
		/* Setup BCI command overflow buffer */
		OUTREG(0x48C14, (si->cobOffset >> 11) | (si->cobIndex << 29)); /* tim */
		/* Program shadow status update. */
		thresholds = ((si->bciThresholdLo & 0xffff) << 16) |
					 (si->bciThresholdHi & 0xffff);
		OUTREG(0x48C10, thresholds);

		OUTREG(0x48C0C, 0);
		/* Enable BCI and command overflow buffer */
		OUTREG(0x48C18, INREG(0x48C18) | 0x0C);
		break;

	case S3_SAVAGE4:
	case S3_PROSAVAGE:
	case S3_TWISTER:
	case S3_PROSAVAGEDDR:
	case S3_SUPERSAVAGE:

		/* Disable BCI */
		OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);

		if (!si->bDisableCOB) {
			/* Setup BCI command overflow buffer */
			OUTREG(0x48C14, (si->cobOffset >> 11) | (si->cobIndex << 29));
		}
		/* Program shadow status update */ /* AGD: what should this be? */
		thresholds = ((si->bciThresholdLo & 0x1fffe0) << 11)
					| ((si->bciThresholdHi & 0x1fffe0) >> 5);
		OUTREG(0x48C10, thresholds);

		OUTREG(0x48C0C, 0);
		if (si->bDisableCOB)
			OUTREG(0x48C18, INREG(0x48C18) | 0x08);		// enable BCI without COB
		else
			OUTREG(0x48C18, INREG(0x48C18) | 0x0C);		// enable BCI with COB

		break;

	case S3_SAVAGE2000:

		/* Disable BCI */
		OUTREG(0x48C18, 0);
		/* Setup BCI command overflow buffer */
		OUTREG(0x48C18, (si->cobOffset >> 7) | (si->cobIndex));
		/* Disable shadow status update */
		OUTREG(0x48A30, 0);
		/* Enable BCI and command overflow buffer */
		OUTREG(0x48C18, INREG(0x48C18) | 0x00280000 );

		break;
	}

	/* Use and set global bitmap descriptor. */

	/* For reasons I do not fully understand yet, on the Savage4, the */
	/* write to the GBD register, MM816C, does not "take" at this time. */
	/* Only the low-order byte is acknowledged, resulting in an incorrect */
	/* stride.  Writing the register later, after the mode switch, works */
	/* correctly.	This needs to get resolved. */

	SavageSetGBD(pMode);
}

