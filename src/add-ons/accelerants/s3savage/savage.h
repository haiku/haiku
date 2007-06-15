/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2007
*/


#ifndef __SAVAGE_H__
#define __SAVAGE_H__


#define BASE_FREQ	14.31818

#define FIFO_CONTROL_REG		0x8200
#define MIU_CONTROL_REG			0x8204
#define STREAMS_TIMEOUT_REG		0x8208
#define MISC_TIMEOUT_REG		0x820c

#define	ADVANCED_FUNC_CTRL		0x850C
#define SYSTEM_CONTROL_REG		0x83DA

#define VGA_ATTR_INDEX			0x83C0
#define VGA_ATTR_DATA_W			0x83C0
#define VGA_MISC_OUT_R			0x83CC		/* read */
#define VGA_MISC_OUT_W			0x83C2		/* write */
#define VGA_SEQ_INDEX			0x83C4
#define VGA_SEQ_DATA			0x83C5
#define VGA_GRAPH_INDEX			0x83CE
#define VGA_GRAPH_DATA			0x83CF
#define VGA_CRTC_INDEX			0x83D4
#define VGA_CRTC_DATA			0x83D5
#define VGA_IN_STAT_1			SYSTEM_CONTROL_REG


/* Stream Processor 1 */

/* Primary Stream 1 Frame Buffer Address 0 */
#define PRI_STREAM_FBUF_ADDR0	0x81c0
/* Primary Stream 1 Frame Buffer Address 1 */
#define PRI_STREAM_FBUF_ADDR1	0x81c4
/* Primary Stream 1 Stride */
#define PRI_STREAM_STRIDE		0x81c8

/* Stream Processor 2 */

/* Primary Stream 2 Frame Buffer Address 0 */
#define PRI_STREAM2_FBUF_ADDR0	0x81b0
/* Primary Stream 2 Frame Buffer Address 1 */
#define PRI_STREAM2_FBUF_ADDR1	0x81b4
/* Primary Stream 2 Stride */
#define PRI_STREAM2_STRIDE		0x81b8


/* GX-3 Configuration/Status Registers */
#define S3_BUFFER_THRESHOLD		0x48C10
#define S3_OVERFLOW_BUFFER		0x48C14
#define S3_OVERFLOW_BUFFER_PTR	0x48C18

#define MEMORY_CTRL0_REG		0xCA

#define MEMORY_CONFIG_REG		0x31

/* bitmap descriptor register */
#define S3_GLB_BD_LOW			0x8168
#define S3_GLB_BD_HIGH			0x816C
#define S3_PRI_BD_LOW			0x8170
#define S3_PRI_BD_HIGH			0x8174
#define S3_SEC_BD_LOW			0x8178
#define S3_SEC_BD_HIGH			0x817c

/* duoview */

#define SELECT_IGA1					0x4026
#define SELECT_IGA2_READS_WRITES	0x4f26

#define MEM_PS1					0x10	/*CRCA_4 :Primary stream 1*/
#define MEM_PS2					0x20	/*CRCA_5 :Primary stream 2*/
#define MEM_SS1					0x40	/*CRCA_6 :Secondary stream 1*/
#define MEM_SS2					0x80	/*CRCA_7 :Secondary stream 2*/

#define SRC_BASE				0xa4d4
#define DEST_BASE				0xa4d8
#define CLIP_L_R				0xa4dc
#define CLIP_T_B				0xa4e0
#define DEST_SRC_STR			0xa4e4
#define MONO_PAT_0				0xa4e8
#define MONO_PAT_1				0xa4ec

/*
 * CR88_4 =1 : disable block write
 * the "2D" is partly to set this apart from "BLOCK_WRITE_DISABLE"
 * constant used for bitmap descriptor
 */
#define DISABLE_BLOCK_WRITE_2D	0x10
#define BLOCK_WRITE_DISABLE		0x0

/* CR31[0] set = Enable 8MB display memory through 64K window at A0000H. */
#define ENABLE_CPUA_BASE_A0000	0x01

/*
 * reads from SUBSYS_STAT
 */
#define STATUS_WORD0		(INREG(0x48C00))
#define ALT_STATUS_WORD0	(INREG(0x48C60))
#define MAXLOOP				0xffffff
#define MAXFIFO				0x7f00

// BCI definitions.
//=================

#define TILE_FORMAT_LINEAR	0

/* BD - BCI enable */
#define BCI_ENABLE			8		// savage4, MX, IX, 3D
#define BCI_ENABLE_TWISTER	0		// twister, prosavage, DDR, supersavage, 2000

#define S3_BIG_ENDIAN		4
#define S3_LITTLE_ENDIAN	0
#define S3_BD64				1

#define BCI_BUFFER_OFFSET	0x10000

#define BCI_GET_PTR		vuint32* bci_ptr = ((uint32*)(regs + BCI_BUFFER_OFFSET))
#define BCI_SEND(dw)	(*bci_ptr++ = ((uint32)(dw)))

#define BCI_CMD_NOP					0x40000000
#define BCI_CMD_RECT				0x48000000
#define BCI_CMD_RECT_XP				0x01000000
#define BCI_CMD_RECT_YP				0x02000000
#define BCI_CMD_GET_ROP(cmd)		(((cmd) >> 16) & 0xFF)
#define BCI_CMD_SET_ROP(cmd, rop)	((cmd) |= ((rop & 0xFF) << 16))

#define BCI_CMD_SEND_COLOR			0x00008000
#define BCI_CMD_DEST_PBD_NEW		0x00000C00
#define BCI_CMD_SRC_SOLID			0x00000000
#define BCI_CMD_SRC_SBD_COLOR_NEW	0x00000140

#define BCI_BD_BW_DISABLE			0x10000000
#define BCI_BD_SET_BPP(bd, bpp)		((bd) |= (((bpp) & 0xFF) << 16))
#define BCI_BD_SET_STRIDE(bd, st)	((bd) |= ((st) & 0xFFFF))

#define BCI_W_H(w, h)				((((h) << 16) | (w)) & 0x0FFF0FFF)
#define BCI_X_Y(x, y)				((((y) << 16) | (x)) & 0x0FFF0FFF)


// Macros for memory mapped I/O.
//==============================

#define INREG8(addr)		*((vuint8*)(regs + addr))
#define INREG16(addr)		*((vuint16*)(regs + addr))
#define INREG32(addr)		*((vuint32*)(regs + addr))

#define OUTREG8(addr, val)	*((vuint8*)(regs + addr)) = val
#define OUTREG16(addr, val)	*((vuint16*)(regs + addr)) = val
#define OUTREG32(addr, val)	*((vuint32*)(regs + addr)) = val

#define INREG(addr)			INREG32(addr)
#define OUTREG(addr, val)	OUTREG32(addr, val)


static inline uint8 ReadCrtc(uint8 index)
{
	OUTREG8(VGA_CRTC_INDEX, index);
	return INREG8(VGA_CRTC_DATA);
}

static inline void WriteCrtc(uint8 index, uint8 value)
{
	OUTREG8(VGA_CRTC_INDEX, index);
	OUTREG8(VGA_CRTC_DATA, value);
}

static inline uint8 ReadSeq(uint8 index)
{
	OUTREG8(VGA_SEQ_INDEX, index);
	return INREG8(VGA_SEQ_DATA);
}

static inline void WriteSeq(uint8 index, uint8 value)
{
	OUTREG8(VGA_SEQ_INDEX, index);
	OUTREG8(VGA_SEQ_DATA, value);
}

static inline uint8 ReadST01()
{
	return INREG8(VGA_IN_STAT_1);
}

/*
 * unprotect CRTC[0-7]
 * CR11_7 = 0: Writing to all CRT Controller registers enabled
 *		  = 1: Writing to all bits of CR0~CR7 except CR7_4 disabled
 */
static inline void UnProtectCRTC()
{
	unsigned char byte;
	OUTREG8(VGA_CRTC_INDEX, 0x11);
	byte = INREG8(VGA_CRTC_DATA) & 0x7F;
	OUTREG16(VGA_CRTC_INDEX, byte << 8 | 0x11);
}

/*
 * unlock extended regs
 * CR38:unlock CR20~CR3F
 * CR39:unlock CR40~CRFF
 * SR08:unlock SR09~SRFF
 */
static inline void UnLockExtRegs()
{
	OUTREG16(VGA_CRTC_INDEX, 0x4838);
	OUTREG16(VGA_CRTC_INDEX, 0xA039);
	OUTREG16(VGA_SEQ_INDEX, 0x0608);
}


static inline void VerticalRetraceWait()
{
	INREG8(VGA_CRTC_INDEX);
	OUTREG8(VGA_CRTC_INDEX, 0x17);
	if (INREG8(VGA_CRTC_DATA) & 0x80)
	{
		int i = 0x10000;
		while ((INREG8(SYSTEM_CONTROL_REG) & 0x08) == 0x08 && i--) ;
		i = 0x10000;
		while ((INREG8(SYSTEM_CONTROL_REG) & 0x08) == 0x00 && i--) ;
	}
}


typedef struct
{
	display_timing	timing;			// CRTC info
	int				bpp;			// bits/pixel
	int				width;			// screen width in pixels
	int				bytesPerRow;	// number of bytes in one line/row
} DisplayMode;



bool SavageLoadCursorImage(int width, int height, uint8* and_mask, uint8* xor_mask);
void SavageSetCursorColors(int bg, int fg);
void SavageSetCursorPosition(int x, int y);
void SavageShowCursor(void);
void SavageHideCursor(void);

bool SavageDPMS(int mode);

bool SavagePreInit(void);
bool SavageModeInit(DisplayMode* pMode);
void SavageInitialize2DEngine(DisplayMode* pMode);
void SavageSetGBD(DisplayMode* pMode);
void SavageAdjustFrame(int x, int y);


#endif // __SAVAGE_H__
