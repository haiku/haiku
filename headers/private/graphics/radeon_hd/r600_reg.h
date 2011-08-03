/*
 * RadeonHD R6xx, R7xx Register documentation
 *
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 * Copyright (C) 2008-2009  Matthias Hopf
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _R600_REG_H_
#define _R600_REG_H_

/*
 * Register definitions
 */

#include "r600_reg_auto_r6xx.h"
#include "r600_reg_r6xx.h"
#include "r600_reg_r7xx.h"


/* From Linux DRM Radeon driver for AtomBIOS */
#define RADEON_SEPROM_CNTL1			0x01c0
#define RADEON_SCK_PRESCALE_SHIFT	24
#define RADEON_SCK_PRESCALE_MASK	(0xff << 24)

#define RADEON_VIPH_CONTROL			0x0c40
#define RADEON_VIPH_EN				(1 << 21)

#define RADEON_GPIOPAD_MASK			0x0198
#define RADEON_GPIOPAD_A            0x019c
#define RADEON_GPIOPAD_EN			0x01a0
#define RADEON_GPIOPAD_Y			0x01a4
#define RADEON_MDGPIO_MASK			0x01a8
#define RADEON_MDGPIO_A				0x01ac
#define RADEON_MDGPIO_EN			0x01b0
#define RADEON_MDGPIO_Y				0x01b4

#define RV370_BUS_CNTL				0x004c

#define R600_CG_SPLL_FUNC_CNTL		0x600
#define R600_CG_SPLL_STATUS			0x60c
#define R600_ROM_CNTL				0x1600
#define R600_BUS_CNTL				0x5420

#define R600_BIOS_ROM_DIS			(1 << 1)
#define R600_SCK_OVERWRITE			(1 << 1)
#define R600_SPLL_CHG_STATUS		(1 << 1)
#define R600_SPLL_BYPASS_EN			(1 << 3)
#define DVGA_CONTROL_MODE_ENABLE	(1 << 0)
#define DVGA_CONTROL_TIMING_SELECT	(1 << 8)
#define VGA_VSTATUS_CNTL_MASK		(3 << 16)


/* SET_*_REG offsets + ends */
enum {
    SET_CONFIG_REG_offset          = 0x00008000,
    SET_CONFIG_REG_end             = 0x0000ac00,
    SET_CONTEXT_REG_offset         = 0x00028000,
    SET_CONTEXT_REG_end            = 0x00029000,
    SET_ALU_CONST_offset           = 0x00030000,
    SET_ALU_CONST_end              = 0x00032000,
    SET_RESOURCE_offset            = 0x00038000,
    SET_RESOURCE_end               = 0x0003c000,
    SET_SAMPLER_offset             = 0x0003c000,
    SET_SAMPLER_end                = 0x0003cff0,
    SET_CTL_CONST_offset           = 0x0003cff0,
    SET_CTL_CONST_end              = 0x0003e200,
    SET_LOOP_CONST_offset          = 0x0003e200,
    SET_LOOP_CONST_end             = 0x0003e380,
    SET_BOOL_CONST_offset          = 0x0003e380,
    SET_BOOL_CONST_end             = 0x0003e38c
};

/* packet3 IT_SURFACE_BASE_UPDATE bits */
enum {
	DEPTH_BASE    = (1 << 0),
	COLOR0_BASE   = (1 << 1),
	COLOR1_BASE   = (1 << 2),
	COLOR2_BASE   = (1 << 3),
	COLOR3_BASE   = (1 << 4),
	COLOR4_BASE   = (1 << 5),
	COLOR5_BASE   = (1 << 6),
	COLOR6_BASE   = (1 << 7),
	COLOR7_BASE   = (1 << 8),
	STRMOUT_BASE0 = (1 << 9),
	STRMOUT_BASE1 = (1 << 10),
	STRMOUT_BASE2 = (1 << 11),
	STRMOUT_BASE3 = (1 << 12),
	COHER_BASE0   = (1 << 13),
	COHER_BASE1   = (1 << 14)
};

/* packet3 IT_WAIT_REG_MEM operation encoding */
enum {
	WAIT_ALWAYS = (0<<0),
	WAIT_LT     = (1<<0),
	WAIT_LE     = (2<<0),
	WAIT_EQ     = (3<<0),
	WAIT_NE     = (4<<0),
	WAIT_GE     = (5<<0),
	WAIT_GT     = (6<<0),

	WAIT_REG    = (0<<4),
	WAIT_MEM    = (1<<4)
};

/* Packet3 commands */
enum {
    IT_NOP                               = 0x10,
    IT_INDIRECT_BUFFER_END               = 0x17,
    IT_SET_PREDICATION                   = 0x20,
    IT_REG_RMW                           = 0x21,
    IT_COND_EXEC                         = 0x22,
    IT_PRED_EXEC                         = 0x23,
    IT_START_3D_CMDBUF                   = 0x24,
    IT_DRAW_INDEX_2                      = 0x27,
    IT_CONTEXT_CONTROL                   = 0x28,
    IT_DRAW_INDEX_IMMD_BE                = 0x29,
    IT_INDEX_TYPE                        = 0x2A,
    IT_DRAW_INDEX                        = 0x2B,
    IT_DRAW_INDEX_AUTO                   = 0x2D,
    IT_DRAW_INDEX_IMMD                   = 0x2E,
    IT_NUM_INSTANCES                     = 0x2F,
    IT_STRMOUT_BUFFER_UPDATE             = 0x34,
    IT_INDIRECT_BUFFER_MP                = 0x38,
    IT_MEM_SEMAPHORE                     = 0x39,
    IT_MPEG_INDEX                        = 0x3A,
    IT_WAIT_REG_MEM                      = 0x3C,
    IT_MEM_WRITE                         = 0x3D,
    IT_INDIRECT_BUFFER                   = 0x32,
    IT_CP_INTERRUPT                      = 0x40,
    IT_SURFACE_SYNC                      = 0x43,
    IT_ME_INITIALIZE                     = 0x44,
    IT_COND_WRITE                        = 0x45,
    IT_EVENT_WRITE                       = 0x46,
    IT_EVENT_WRITE_EOP                   = 0x47,
    IT_ONE_REG_WRITE                     = 0x57,
    IT_SET_CONFIG_REG                    = 0x68,
    IT_SET_CONTEXT_REG                   = 0x69,
    IT_SET_ALU_CONST                     = 0x6A,
    IT_SET_BOOL_CONST                    = 0x6B,
    IT_SET_LOOP_CONST                    = 0x6C,
    IT_SET_RESOURCE                      = 0x6D,
    IT_SET_SAMPLER                       = 0x6E,
    IT_SET_CTL_CONST                     = 0x6F,
    IT_SURFACE_BASE_UPDATE               = 0x73
};

#endif
