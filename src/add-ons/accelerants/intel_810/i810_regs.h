/*
 * Copyright 2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

/*!
	Haiku Intel-810 video driver was adapted from the X.org intel driver which
	has the following copyright.

	Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
	All Rights Reserved.
 */

#ifndef __I810_REGS_H__
#define __I810_REGS_H__


// CRT Controller Registers.
#define START_ADDR_HI			0x0C
#define START_ADDR_LO			0x0D
#define VERT_SYNC_END			0x11
#define EXT_VERT_TOTAL			0x30
#define EXT_VERT_DISPLAY		0x31
#define EXT_VERT_SYNC_START		0x32
#define EXT_VERT_BLANK_START	0x33
#define EXT_HORIZ_TOTAL			0x35
#define EXT_HORIZ_BLANK			0x39
#define EXT_START_ADDR			0x40
#define EXT_START_ADDR_ENABLE	0x80 
#define EXT_OFFSET				0x41
#define EXT_START_ADDR_HI		0x42
#define INTERLACE_CNTL			0x70
#define INTERLACE_ENABLE		0x80
#define INTERLACE_DISABLE		0x00

// CR80 - IO Control
#define IO_CTNL					0x80
#define EXTENDED_ATTR_CNTL		0x02
#define EXTENDED_CRTC_CNTL		0x01

// GR10 - Address mapping
#define ADDRESS_MAPPING			0x10
#define PAGE_TO_LOCAL_MEM_ENABLE 0x10
#define GTT_MEM_MAP_ENABLE		0x08
#define PACKED_MODE_ENABLE		0x04
#define LINEAR_MODE_ENABLE		0x02
#define PAGE_MAPPING_ENABLE		0x01

#define FENCE			0x2000

#define INST_DONE		0x2090

// General error reporting regs.
#define EIR				0x20B0
#define EMR				0x20B4
#define ESR				0x20B8

// FIFO Watermark and Burst Length Control Register.
#define FWATER_BLC			0x20d8
#define MM_BURST_LENGTH			0x00700000
#define MM_FIFO_WATERMARK		0x0001F000
#define LM_BURST_LENGTH			0x00000700
#define LM_FIFO_WATERMARK		0x0000001F

#define MEM_MODE			0x020DC

#define DRAM_ROW_CNTL_HI	0x3002
#define DRAM_REFRESH_RATE		0x18
#define DRAM_REFRESH_DISABLE	0x00
#define DRAM_REFRESH_60HZ		0x08

#define VCLK2_VCO_M			0x6008
#define VCLK2_VCO_N			0x600a
#define VCLK2_VCO_DIV_SEL	0x6012

#define PIXPIPE_CONFIG		0x70008
#define NO_BLANK_DELAY			0x100000
#define DISPLAY_8BPP_MODE		0x020000
#define DISPLAY_15BPP_MODE		0x040000
#define DISPLAY_16BPP_MODE		0x050000
#define DAC_8_BIT				0x008000
#define HIRES_MODE				0x000001

// Blitter control.
#define BITBLT_CNTL			0x7000c
#define COLEXP_MODE				0x30
#define COLEXP_8BPP				0x00
#define COLEXP_16BPP			0x10

// Color Palette Registers.
#define DAC_MASK		0x3C6
#define DAC_W_INDEX		0x3C8
#define DAC_DATA		0x3C9


#define MISC_OUT_R		0x3CC		// read
#define MISC_OUT_W		0x3C2		// write
#define SEQ_INDEX		0x3C4
#define SEQ_DATA		0x3C5
#define GRAPH_INDEX		0x3CE
#define GRAPH_DATA		0x3CF
#define CRTC_INDEX		0x3D4
#define CRTC_DATA		0x3D5


// Macros for memory mapped I/O.
//==============================

#define INREG8(addr)		(*((vuint8*)(gInfo.regs + (addr))))
#define INREG16(addr)		(*((vuint16*)(gInfo.regs + (addr))))
#define INREG32(addr)		(*((vuint32*)(gInfo.regs + (addr))))

#define OUTREG8(addr, val)	(*((vuint8*)(gInfo.regs + (addr))) = (val))
#define OUTREG16(addr, val)	(*((vuint16*)(gInfo.regs + (addr))) = (val))
#define OUTREG32(addr, val)	(*((vuint32*)(gInfo.regs + (addr))) = (val))

// Write a value to an 32-bit reg using a mask.  The mask selects the
// bits to be modified.
#define OUTREGM(addr, value, mask)	\
	(OUTREG(addr, (INREG(addr) & ~mask) | (value & mask)))


static inline uint8 ReadCrtcReg(uint8 index)
{
	OUTREG8(CRTC_INDEX, index);
	return INREG8(CRTC_DATA);
}

static inline void WriteCrtcReg(uint8 index, uint8 value)
{
	OUTREG8(CRTC_INDEX, index);
	OUTREG8(CRTC_DATA, value);
}

static inline uint8 ReadGraphReg(uint8 index)
{
	OUTREG8(GRAPH_INDEX, index);
	return INREG8(GRAPH_DATA);
}

static inline void WriteGraphReg(uint8 index, uint8 value)
{
	OUTREG8(GRAPH_INDEX, index);
	OUTREG8(GRAPH_DATA, value);
}

static inline uint8 ReadSeqReg(uint8 index)
{
	OUTREG8(SEQ_INDEX, index);
	return INREG8(SEQ_DATA);
}

static inline void WriteSeqReg(uint8 index, uint8 value)
{
	OUTREG8(SEQ_INDEX, index);
	OUTREG8(SEQ_DATA, value);
}


#endif	// __I810_REGS_H__
