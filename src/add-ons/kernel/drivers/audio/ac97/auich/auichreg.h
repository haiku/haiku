/*
 * Auich BeOS Driver for Intel Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 
 * Original code : BeOS Driver for Intel ICH AC'97 Link interface
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
 
#ifndef _AUICHREG_H_
#define _AUICHREG_H_

/* Native Audio Bus Master Control Registers */
enum AUICH_GLOBAL_REGISTER 
{
	AUICH_REG_GLOB_CNT	= 0x2C,
	AUICH_REG_GLOB_STA	= 0x30,
	AUICH_REG_ACC_SEMA	= 0x34,
	AUICH_REG_SDM			= 0x80
};

enum AUICH_X_REGISTER_BASE /* base addresses for the following offsets */
{
	AUICH_REG_PI_BASE		= 0x00,
	AUICH_REG_PO_BASE		= 0x10,
	AUICH_REG_MC_BASE		= 0x20
};

enum AUICH_X_REGISTER_OFFSETS /* add base address to get the PI, PO or MC reg */
{
	AUICH_REG_X_BDBAR		= 0x00,
	AUICH_REG_X_CIV		= 0x04,
	AUICH_REG_X_LVI		= 0x05,
	AUICH_REG_X_SR		= 0x06, 
	AUICH_REG_X_PICB	= 0x08, 
	AUICH_REG_X_PIV		= 0x0A,
	AUICH_REG_X_CR		= 0x0B
};

/* AUICH_REG_X_SR (Status Register) Bits */
enum REG_X_SR_BITS 
{
	SR_DCH				= 0x0001,
	SR_CELV				= 0x0002,
	SR_LVBCI			= 0x0004,
	SR_BCIS				= 0x0008,
	SR_FIFOE			= 0x0010,
	SR_MASK				= SR_FIFOE | SR_BCIS | SR_LVBCI | SR_CELV | SR_DCH //| 0x200 | 0x80 | 0x2
};

/* AUICH_REG_X_CR (Control Register) Bits */
enum REG_X_CR_BITS
{
	CR_RPBM				= 0x01,
	CR_RR				= 0x02,
	CR_LVBIE			= 0x04,
	CR_FEIE				= 0x08,
	CR_IOCE				= 0x10
};

/* AUICH_REG_GLOB_CNT (Global Control Register) Bits */
enum REG_GLOB_CNT_BITS
{
	CNT_GIE				= 0x01,
	CNT_COLD			= 0x02,
	CNT_WARM			= 0x04,
	CNT_SHUT			= 0x08,
	CNT_PRIE			= 0x10,
	CNT_SRIE			= 0x20
};

/* AUICH_REG_GLOB_STA (Global Status Register) Bits */
enum REG_GLOB_STA_BITS
{
	STA_GSCI			= 0x00000001, /* GPI Status Change Interrupt */
	STA_MIINT			= 0x00000002, /* Modem In Interrupt */
	STA_MOINT			= 0x00000004, /* Modem Out Interrupt */
	STA_PIINT			= 0x00000020, /* PCM In Interrupt */
	STA_POINT			= 0x00000040, /* PCM Out Interrupt */
	STA_MINT			= 0x00000080, /* Mic In Interrupt */
	STA_S0CR			= 0x00000100, /* AC_SDIN0 Codec Ready */
	STA_S1CR			= 0x00000200, /* AC_SDIN1 Codec Ready */
	STA_S0RI			= 0x00000400, /* AC_SDIN0 Resume Interrupt */
	STA_S1RI			= 0x00000800, /* AC_SDIN1 Resume Interrupt */
	STA_RCS				= 0x00008000, /* Read Completition Status */
	STA_AD3				= 0x00010000,
	STA_MD3				= 0x00020000,
	STA_SAMPLE_CAP		= 0x00c00000, /* sampling precision capability */
	STA_POM20			= 0x00400000, /* PCM out precision 20bit */
	STA_CHAN_CAP		= 0x00300000, /* multi-channel capability */
	STA_PCM4			= 0x00100000, /* 4ch output */
	STA_PCM6			= 0x00200000, /* 6ch output */
	STA_M2INT			= 0x01000000,	/* Microphone 2 In Interrupt */
	STA_P2INT			= 0x02000000,	/* PCM In 2 Interrupt */
	STA_SPINT			= 0x04000000,	/* S/PDIF Interrupt */
	STA_BCS				= 0x08000000,	/* Bit Clock Stopped */
	STA_S2CR			= 0x10000000,	/* AC_SDIN2 Codec Ready */
	STA_S2RI			= 0x20000000,	/* AC_SDIN2 Resume Interrupt */
	STA_INTMASK			= (STA_MIINT | STA_MOINT | STA_PIINT | STA_POINT | STA_MINT | STA_S0RI | STA_S1RI | STA_M2INT | STA_P2INT | STA_SPINT | STA_S2RI)
};

#define ICH4_MMBAR_SIZE	512
#define ICH4_MBBAR_SIZE	256

#endif /* _AUICHREG_H_ */
