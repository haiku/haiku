/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __CX23882_REGS_H
#define __CX23882_REGS_H

#define PCI_PCICMD_IOS			0x01
#define PCI_PCICMD_MSE			0x02
#define PCI_PCICMD_BME			0x04

#define PCI_VENDOR_SIS			0x1039
#define PCI_VENDOR_VIA			0x1106

#define REG_PDMA_STHRSH			0x200000
#define REG_PDMA_DTHRSH			0x200010

#define PDMA_ISBTHRSH_1			0x0100
#define PDMA_ISBTHRSH_2			0x0200
#define PDMA_ISBTHRSH_3			0x0300
#define PDMA_ISBTHRSH_4			0x0400
#define PDMA_ISBTHRSH_5			0x0500
#define PDMA_ISBTHRSH_6			0x0600
#define PDMA_ISBTHRSH_7			0x0700

#define PDMA_PCITHRSH_1			0x0001
#define PDMA_PCITHRSH_2			0x0002
#define PDMA_PCITHRSH_3			0x0003
#define PDMA_PCITHRSH_4			0x0004
#define PDMA_PCITHRSH_5			0x0005
#define PDMA_PCITHRSH_6			0x0006
#define PDMA_PCITHRSH_7			0x0007

#define REG_DEV_CNTRL2			0x200034

#define DEV_CNTRL2_RUN_RISC		0x20

#define REG_PCI_INT_MSK			0x200040
#define REG_PCI_INT_STAT		0x200044
#define REG_PCI_INT_MSTAT		0x200048

#define PCI_INT_STAT_VID_INT	0x01
#define PCI_INT_STAT_AUD_INT	0x02
#define PCI_INT_STAT_TS_INT		0x04
#define PCI_INT_STAT_VIP_INT	0x08
#define PCI_INT_STAT_HST_INT	0x10

#define REG_VID_INT_MSK			0x200050
#define REG_VID_INT_STAT		0x200054
#define REG_VID_INT_MSTAT		0x200058

#define REG_AUD_INT_MSK			0x200060
#define REG_AUD_INT_STAT		0x200064
#define REG_AUD_INT_MSTAT		0x200068

#define REG_TS_INT_MSK			0x200070
#define REG_TS_INT_STAT			0x200074
#define REG_TS_INT_MSTAT		0x200078
	
#define TS_INT_STAT_TS_RISC1	0x000001
#define TS_INT_STAT_TS_RISC2	0x000010
#define TS_INT_STAT_OPC_ERR		0x010000

#define REG_VIP_INT_MSK			0x200080
#define REG_VIP_INT_STAT		0x200084
#define REG_VIP_INT_MSTAT		0x200088

#define REG_HST_INT_MSK			0x200090
#define REG_HST_INT_STAT		0x200094
#define REG_HST_INT_MSTAT		0x200098

#define REG_F2_DEV_CNTRL1		0x2f0240
#define F2_DEV_CNTRL1_EN_VSFX	0x8

#define REG_DMA28_PTR1			0x30009c
#define REG_DMA28_PTR2			0x3000dc
#define REG_DMA28_CNT1			0x30011c
#define REG_DMA28_CNT2			0x30015c

#define REG_TS_GP_CNT_CNTRL		0x33c030
#define REG_TS_DMA_CNTRL		0x33c040

#define TS_DMA_CNTRL_TS_FIFO_EN	0x01
#define TS_DMA_CNTRL_TS_RISC_EN	0x10

#define REG_TS_LNGTH			0x33c048
#define REG_HW_SOP_CONTROL		0x33c04c
#define REG_TS_GEN_CONTROL		0x33c050

#define TS_GEN_CONTROL_IPB_SMODE 0x08

#define REG_TS_BD_PKT_STATUS	0x33c054
#define REG_TS_SOP_STATUS		0x33c058

#define REG_VIP_STREAM_EN		0x34c040

// these 3 are not in my spec, taken form Linux
#define REG_DMA_RISC_INT_MSK 	0x35C060
#define REG_DMA_RISC_INT_STAT	0x35C064
#define REG_DMA_RISC_INT_MSTAT	0x35C068

#define REG_I2C_CONTROL			0x368000
#define I2C_SDA					0x01
#define I2C_SCL					0x02
#define I2C_HW_MODE				0x80

#define REG_HST_STREAM_EN		0x38c040


// RISC instructions
#define RISC_RESYNC				0x80008000
#define RISC_WRITE				0x10000000
#define RISC_SKIP				0x20000000
#define RISC_JUMP				0x70000000
#define RISC_WRITECR			0xd0000000

#define RISC_IMM				0x00000001
#define RISC_SOL				0x08000000
#define RISC_EOL				0x04000000
#define RISC_IRQ2				0x02000000
#define RISC_IRQ1				0x01000000
#define RISC_SRP				0x00000001

#endif
