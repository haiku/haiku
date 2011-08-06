/*
 * Copyright 2008 Advanced Micro Devices, Inc.  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author: Stanislaw Skowronek
 */
#ifndef ATOM_H
#define ATOM_H


#include "atombios.h"

#include <String.h>
#include <SupportDefs.h>


struct card_info {
	// Filled by driver
	void (*reg_write)(uint32 offset, uint32 data);
		uint32 (*reg_read)(uint32 offset);
	void (*ioreg_write)(uint32 offset, uint32 data);
		uint32 (*ioreg_read)(uint32 offset);
	void (*mc_write)(uint32 offset, uint32 data);
		uint32 (*mc_read)(uint32 offset);
	void (*pll_write)(uint32 offset, uint32 data);
		uint32 (*pll_read)(uint32 offset);
};


#define ATOM_BIOS_MAGIC		0xAA55
#define ATOM_ATI_MAGIC_PTR	0x30
#define ATOM_ATI_MAGIC		" 761295520"
#define ATOM_ROM_TABLE_PTR	0x48

#define ATOM_ROM_MAGIC		"ATOM"
#define ATOM_ROM_MAGIC_PTR	4

#define ATOM_ROM_MSG_PTR	0x10
#define ATOM_ROM_CMD_PTR	0x1E
#define ATOM_ROM_DATA_PTR	0x20

#define ATOM_CMD_INIT		0
#define ATOM_CMD_SETSCLK	0x0A
#define ATOM_CMD_SETMCLK	0x0B
#define ATOM_CMD_SETPCLK	0x0C

#define ATOM_DATA_FWI_PTR	0xC
#define ATOM_DATA_IIO_PTR	0x32

#define ATOM_FWI_DEFSCLK_PTR	8
#define ATOM_FWI_DEFMCLK_PTR	0xC
#define ATOM_FWI_MAXSCLK_PTR	0x24
#define ATOM_FWI_MAXMCLK_PTR	0x28

#define ATOM_CT_SIZE_PTR	0
#define ATOM_CT_WS_PTR		4
#define ATOM_CT_PS_PTR		5
#define ATOM_CT_PS_MASK		0x7F
#define ATOM_CT_CODE_PTR	6

#define ATOM_OP_CNT		123
#define ATOM_OP_EOT		91

#define ATOM_CASE_MAGIC		0x63
#define ATOM_CASE_END		0x5A5A

#define ATOM_ARG_REG		0
#define ATOM_ARG_PS		1
#define ATOM_ARG_WS		2
#define ATOM_ARG_ID		4
#define ATOM_ARG_FB		3
#define ATOM_ARG_IMM		5
#define ATOM_ARG_PLL		6
#define ATOM_ARG_MC		7

#define ATOM_SRC_DWORD		0
#define ATOM_SRC_WORD0		1
#define ATOM_SRC_WORD8		2
#define ATOM_SRC_WORD16		3
#define ATOM_SRC_BYTE0		4
#define ATOM_SRC_BYTE8		5
#define ATOM_SRC_BYTE16		6
#define ATOM_SRC_BYTE24		7

#define ATOM_WS_QUOTIENT	0x40
#define ATOM_WS_REMAINDER	0x41
#define ATOM_WS_DATAPTR		0x42
#define ATOM_WS_SHIFT		0x43
#define ATOM_WS_OR_MASK		0x44
#define ATOM_WS_AND_MASK	0x45
#define ATOM_WS_FB_WINDOW	0x46
#define ATOM_WS_ATTRIBUTES	0x47
#define ATOM_WS_REGPTR		0x48

#define ATOM_IIO_NOP		0
#define ATOM_IIO_START		1
#define ATOM_IIO_READ		2
#define ATOM_IIO_WRITE		3
#define ATOM_IIO_CLEAR		4
#define ATOM_IIO_SET		5
#define ATOM_IIO_MOVE_INDEX	6
#define ATOM_IIO_MOVE_ATTR	7
#define ATOM_IIO_MOVE_DATA	8
#define ATOM_IIO_END		9

#define ATOM_IO_MM		0
#define ATOM_IO_PCI		1
#define ATOM_IO_SYSIO		2
#define ATOM_IO_IIO		0x80

typedef struct atom_context_s {
	card_info *card;
	void *bios;
	uint32 cmd_table, data_table;
	uint16 *iio;

	uint16 data_block;
	uint32 fb_base;
	uint32 divmul[2];
	uint16 io_attr;
	uint16 reg_block;
	uint8 shift;
	int cs_equal, cs_above;
	int io_mode;
	uint32 *scratch;
} atom_context;

extern int atom_debug;

atom_context *atom_parse(card_info *, void *);
void atom_execute_table(atom_context *, int, uint32 *);
int atom_asic_init(atom_context *);
void atom_destroy(atom_context *);

#endif
