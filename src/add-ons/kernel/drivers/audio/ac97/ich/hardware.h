/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _HARDWARE_H_
#define _HARDWARE_H_

/* Native Audio Bus Master Control Registers */
enum ICH_GLOBAL_REGISTER 
{
	ICH_REG_GLOB_CNT	= 0x2C,
	ICH_REG_GLOB_STA	= 0x30,
	ICH_REG_ACC_SEMA	= 0x34
};

enum ICH_X_REGISTER_BASE /* base addresses for the following offsets */
{
	ICH_REG_PI_BASE		= 0x00,
	ICH_REG_PO_BASE		= 0x10,
	ICH_REG_MC_BASE		= 0x20
};

enum ICH_X_REGISTER_OFFSETS /* add base address to get the PI, PO or MC reg */
{
	ICH_REG_X_BDBAR		= 0x00,
	ICH_REG_X_CIV		= 0x04,
	ICH_REG_X_LVI		= 0x05,
	ICH_REG_X_SR		= 0x06,
	ICH_REG_X_PICB		= 0x08,
	ICH_REG_X_PIV		= 0x0A,
	ICH_REG_X_CR		= 0x0B
};

/* ICH_REG_X_SR (Status Register) Bits */
enum REG_X_SR_BITS 
{
	SR_DCH				= 0x0001,
	SR_CELV				= 0x0002,
	SR_LVBCI			= 0x0004,
	SR_BCIS				= 0x0008,
	SR_FIFOE			= 0x0010
};

/* ICH_REG_X_CR (Control Register) Bits */
enum REG_X_CR_BITS
{
	CR_RPBM				= 0x01,
	CR_RR				= 0x02,
	CR_LVBIE			= 0x04,
	CR_FEIE				= 0x08,
	CR_IOCE				= 0x10
};

/* ICH_REG_GLOB_CNT (Global Control Register) Bits */
enum REG_GLOB_CNT_BITS
{
	CNT_GIE				= 0x01,
	CNT_COLD			= 0x02,
	CNT_WARM			= 0x04,
	CNT_SHUT			= 0x08,
	CNT_PRIE			= 0x10,
	CNT_SRIE			= 0x20
};

/* ICH_REG_GLOB_STA (Global Status Register) Bits */
enum REG_GLOB_STA_BITS
{
	STA_GSCI			= 0x00000001,
	STA_MIINT			= 0x00000002,
	STA_MOINT			= 0x00000004,
	STA_PIINT			= 0x00000020,
	STA_POINT			= 0x00000040,
	STA_MINT			= 0x00000080,
	STA_PCR				= 0x00000100,
	STA_SCR				= 0x00000200,
	STA_PRI				= 0x00000400,
	STA_SRI				= 0x00000800,
	STA_RCS				= 0x00008000,
	STA_AD3				= 0x00010000,
	STA_MD3				= 0x00020000,
	STA_INTMASK			= (STA_MIINT | STA_MOINT | STA_PIINT | STA_POINT | STA_MINT | STA_PRI | STA_SRI)
};

#define ICH_BD_COUNT	32
/* The Hardware Buffer Descriptor */
typedef struct {
	volatile uint32 buffer;
	volatile uint16 length;
	volatile uint16 flags;
	#define ICH_BDC_FLAG_IOC 0x8000
} ich_bd;

/* PCI Configuration Space */

#define PCI_PCICMD		0x04
#define PCI_PCICMD_IOS	0x01
#define PCI_PCICMD_MSE	0x02
#define PCI_PCICMD_BME	0x04

#define PCI_CFG			0x41
#define PCI_CFG_IOSE	0x01

#endif
