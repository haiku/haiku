/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __HARDWARE_H
#define __HARDWARE_H

#define PCI_PCICMD_IOS	0x01
#define PCI_PCICMD_MSE	0x02
#define PCI_PCICMD_BME	0x04

typedef struct {
	uint32 stat_len;
	uint32 vlan;
	uint32 buf_low;		// must be 8 byte aligned
	uint32 buf_high;
} _PACKED buf_desc;

enum {
	TX_DESC_OWN			= 0x80000000,
	TX_DESC_EOR			= 0x40000000,
	TX_DESC_FS			= 0x20000000,
	TX_DESC_LS			= 0x10000000,
	TX_DESC_LEN_MASK	= 0x00003fff,
};

enum {
	RX_DESC_OWN			= 0x80000000,
	RX_DESC_EOR			= 0x40000000,
	RX_DESC_FS			= 0x20000000,
	RX_DESC_LS			= 0x10000000,
	RX_DESC_MAR			= 0x08000000,
	RX_DESC_PAM			= 0x04000000,
	RX_DESC_BAR			= 0x02000000,
	RX_DESC_BOVF		= 0x01000000,
	RX_DESC_FOVF		= 0x00800000,
	RX_DESC_RWT			= 0x00400000,
	RX_DESC_RES			= 0x00200000,
	RX_DESC_RUNT		= 0x00100000,
	RX_DESC_CRC			= 0x00080000,
	RX_DESC_LEN_MASK	= 0x00003fff,
};

enum {
	REG_TNPDS_LOW		= 0x20,
	REG_TNPDS_HIGH		= 0x24,
	REG_THPDS_LOW		= 0x28,
	REG_THPDS_HIGH		= 0x2c,
	REG_RDSAR_LOW		= 0xe4,
	REG_RDSAR_HIGH		= 0xe8,

	REG_CR				= 0x37,	// 8 bit
	REG_TPPOLL			= 0x38,	// 8 bit
	
	REG_INT_MASK		= 0x3c, // 16 bit
	REG_INT_STAT		= 0x3e, // 16 bit
	
	REG_TX_CONFIG		= 0x40,
	REG_RX_CONFIG		= 0x44,

	REG_9346CR			= 0x50,	// 8 bit
	REG_CONFIG0			= 0x51,	// 8 bit
	REG_CONFIG1			= 0x52,	// 8 bit
	REG_CONFIG2			= 0x53,	// 8 bit
	REG_CONFIG3			= 0x54,	// 8 bit
	REG_CONFIG4			= 0x55,	// 8 bit
	REG_CONFIG5			= 0x56,	// 8 bit


	REG_PHYAR			= 0x60,
	REG_TBICSR			= 0x64,
	REG_PHY_STAT		= 0x6c,	// 8bit
	
};

// for REG_CR
enum {
	CR_RST	= 0x10,
	CR_RE	= 0x08,
	CR_TE	= 0x04,
};

// for REG_TPPOLL
enum {
	TPPOLL_HPQ 		= 0x80,
	TPPOLL_NPQ		= 0x40,
	TPPOLL_FSWINT	= 0x01,
};

// for REG_INT_MASK and REG_INT_STAT
enum {
	INT_SERR	= 0x8000,
	INT_TimeOut	= 0x4000,
	INT_SwInt	= 0x0100,
	INT_TDU		= 0x0080,
	INT_FOVW	= 0x0040,
	INT_PUN		= 0x0020,
	INT_RDU		= 0x0010,
	INT_TER		= 0x0008,
	INT_TOK		= 0x0004,
	INT_RER		= 0x0002,
	INT_ROK		= 0x0001,
};

// for REG_PHY_STAT
enum {
	PHY_STAT_EnTBI		= 0x80,
	PHY_STAT_TxFlow		= 0x40,
	PHY_STAT_RxFlow		= 0x20,
	PHY_STAT_1000MF		= 0x10,
	PHY_STAT_100M		= 0x08,
	PHY_STAT_10M		= 0x04,
	PHY_STAT_LinkSts	= 0x02,
	PHY_STAT_FullDup	= 0x01,
};

// for REG_TBICSR
enum {
	TBICSR_ResetTBI		= 0x80000000,
	TBICSR_TBILoopBack	= 0x40000000,
	TBICSR_TBINWEn		= 0x20000000,
	TBICSR_TBIReNW		= 0x10000000,
	TBICSR_TBILinkOk	= 0x02000000,
	TBICSR_NWComplete	= 0x01000000,
};

// for REG_RX_CONFIG
enum {
	RX_CONFIG_MulERINT		= 0x01000000,
	RX_CONFIG_RER8			= 0x00010000,
	RX_CONFIG_AcceptErr		= 0x00000020,
	RX_CONFIG_AcceptRunt	= 0x00000010,
	RX_CONFIG_AcceptBroad 	= 0x00000008,
	RX_CONFIG_AcceptMulti 	= 0x00000004,
	RX_CONFIG_AcceptMyPhys	= 0x00000002,
	RX_CONFIG_AcceptAllPhys	= 0x00000001,
	RX_CONFIG_MASK			= 0xfe7e1880, // XXX should be 0xfefe1880 but Realtek driver clears the undocumented bit 23
	RC_CONFIG_RXFTH_Shift	= 13,
	RC_CONFIG_MAXDMA_Shift	= 8,
};

#endif
