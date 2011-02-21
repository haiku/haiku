/*
 * Copyright (c) 2003-2004 Stefano Ceccherini (stefano.ceccherini@gmail.com)
 * Copyright (c) 1997, 1998
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef __WB840_H
#define __WB840_H

#include <PCI.h>
#include <KernelExport.h>
#include "ether_driver.h"

/*
 * Winbond register definitions.
 */
enum registers {
	WB_BUSCTL 			= 0x00,	/* bus control */
	WB_TXSTART 			= 0x04,	/* tx start demand */
	WB_RXSTART 			= 0x08,	/* rx start demand */
	WB_RXADDR 			= 0x0C,	/* rx descriptor list start addr */
	WB_TXADDR 			= 0x10,	/* tx descriptor list start addr */
	WB_ISR 				= 0x14,	/* interrupt status register */
	WB_NETCFG 			= 0x18,	/* network config register */
	WB_IMR 				= 0x1C,	/* interrupt mask */
	WB_FRAMESDISCARDED	= 0x20,	/* # of discarded frames */
	WB_SIO 				= 0x24,	/* MII and ROM/EEPROM access */
	WB_BOOTROMADDR 		= 0x28,
	WB_TIMER 			= 0x2C,	/* general timer */
	WB_CURRXCTL 		= 0x30,	/* current RX descriptor */
	WB_CURRXBUF 		= 0x34,	/* current RX buffer */
	WB_MAR0 			= 0x38,	/* multicast filter 0 */
	WB_MAR1 			= 0x3C,	/* multicast filter 1 */
	WB_NODE0 			= 0x40,	/* physical address 0 */
	WB_NODE1 			= 0x44,	/* physical address 1 */
	WB_BOOTROMSIZE 		= 0x48,	/* boot ROM size */
	WB_CURTXCTL	 		= 0x4C,	/* current TX descriptor */
	WB_CURTXBUF 		= 0x50,	/* current TX buffer */
};

/*
 * Bus control bits.
 */
enum busControlBits {
	WB_BUSCTL_RESET = 0x00000001,
	WB_BUSCTL_ARBITRATION = 0x00000002,
	WB_BUSCTL_SKIPLEN = 0x0000007C,
	WB_BUSCTL_BUF_BIGENDIAN = 0x00000080,
	WB_BUSCTL_BURSTLEN = 0x00003F00,
	WB_BUSCTL_CACHEALIGN = 0x0000C000,
	WB_BUSCTL_DES_BIGENDIAN = 0x00100000,
	WB_BUSCTL_WAIT = 0x00200000,
	WB_BUSCTL_MUSTBEONE = 0x00400000,

	WB_SKIPLEN_1LONG = 0x00000004,
	WB_SKIPLEN_2LONG = 0x00000008,
	WB_SKIPLEN_3LONG = 0x00000010,
	WB_SKIPLEN_4LONG = 0x00000020,
	WB_SKIPLEN_5LONG = 0x00000040,

	WB_CACHEALIGN_NONE = 0x00000000,
	WB_CACHEALIGN_8LONG = 0x00004000,
	WB_CACHEALIGN_16LONG = 0x00008000,
	WB_CACHEALIGN_32LONG = 0x0000C000,

	WB_BURSTLEN_USECA = 0x00000000,
	WB_BURSTLEN_1LONG = 0x00000100,
	WB_BURSTLEN_2LONG = 0x00000200,
	WB_BURSTLEN_4LONG = 0x00000400,
	WB_BURSTLEN_8LONG = 0x00000800,
	WB_BURSTLEN_16LONG = 0x00001000,
	WB_BURSTLEN_32LONG = 0x00002000,

};

#define WB_BUSCTL_CONFIG (WB_CACHEALIGN_8LONG|WB_SKIPLEN_4LONG|WB_BURSTLEN_8LONG)

/*
 * Interrupt status bits.
 */
enum InterruptStatusBits {
	WB_ISR_TX_OK = 0x00000001,
	WB_ISR_TX_IDLE = 0x00000002,
	WB_ISR_TX_NOBUF = 0x00000004,
	WB_ISR_RX_EARLY = 0x00000008,
	WB_ISR_RX_ERR = 0x00000010,
	WB_ISR_TX_UNDERRUN = 0x00000020,
	WB_ISR_RX_OK = 0x00000040,
	WB_ISR_RX_NOBUF = 0x00000080,
	WB_ISR_RX_IDLE = 0x00000100,
	WB_ISR_TX_EARLY = 0x00000400,
	WB_ISR_TIMER_EXPIRED = 0x00000800,
	WB_ISR_BUS_ERR = 0x00002000,
	WB_ISR_ABNORMAL = 0x00008000,
	WB_ISR_NORMAL = 0x00010000,
	WB_ISR_RX_STATE	= 0x000E0000,
	WB_ISR_TX_STATE = 0x00700000,
	WB_ISR_BUSERRTYPE = 0x03800000,
};

/*
 * The RX_STATE and TX_STATE fields are not described anywhere in the
 * Winbond datasheet, however it appears that the Winbond chip is an
 * attempt at a DEC 'tulip' clone, hence the ISR register is identical
 * to that of the tulip chip and we can steal the bit definitions from
 * the tulip documentation.
 */
enum rxState {
	WB_RXSTATE_STOPPED 	= 0x00000000, 	/* 000 - Stopped */
	WB_RXSTATE_FETCH 	= 0x00020000,	/* 001 - Fetching descriptor */
	WB_RXSTATE_ENDCHECK	= 0x00040000,	/* 010 - check for rx end */
	WB_RXSTATE_WAIT 	= 0x00060000,	/* 011 - waiting for packet */
	WB_RXSTATE_SUSPEND 	= 0x00080000,	/* 100 - suspend rx */
	WB_RXSTATE_CLOSE 	= 0x000A0000,	/* 101 - close tx desc */
	WB_RXSTATE_FLUSH 	= 0x000C0000,	/* 110 - flush from FIFO */
	WB_RXSTATE_DEQUEUE 	= 0x000E0000,	/* 111 - dequeue from FIFO */
};

enum txState {
	 WB_TXSTATE_RESET 	= 0x00000000,	/* 000 - reset */
	 WB_TXSTATE_FETCH 	= 0x00100000,	/* 001 - fetching descriptor */
	 WB_TXSTATE_WAITEND = 0x00200000,	/* 010 - wait for tx end */
	 WB_TXSTATE_READING = 0x00300000,	/* 011 - read and enqueue */
	 WB_TXSTATE_RSVD 	= 0x00400000,	/* 100 - reserved */
	 WB_TXSTATE_SETUP 	= 0x00500000,	/* 101 - setup packet */
	 WB_TXSTATE_SUSPEND = 0x00600000,	/* 110 - suspend tx */
	 WB_TXSTATE_CLOSE 	= 0x00700000,	/* 111 - close tx desc */
};
/*
 * Network config bits.
 */
enum networkConfigBits {
	WB_NETCFG_RX_ON 		= 0x00000002,
	WB_NETCFG_RX_ALLPHYS 	= 0x00000008,
	WB_NETCFG_RX_MULTI 		= 0x00000010,
	WB_NETCFG_RX_BROAD 		= 0x00000020,
	WB_NETCFG_RX_RUNT 		= 0x00000040,
	WB_NETCFG_RX_ERR 		= 0x00000080,
	WB_NETCFG_FULLDUPLEX 	= 0x00000200,
	WB_NETCFG_LOOPBACK 		= 0x00000C00,
	WB_NETCFG_TX_ON 		= 0x00002000,
	WB_NETCFG_TX_THRESH		= 0x001FC000,
	WB_NETCFG_RX_EARLYTHRSH	= 0x1FE00000,
	WB_NETCFG_100MBPS 		= 0x20000000,
	WB_NETCFG_TX_EARLY_ON 	= 0x40000000,
	WB_NETCFG_RX_EARLY_ON 	= 0x80000000,
};
/*
 * The tx threshold can be adjusted in increments of 32 bytes.
 */
#define WB_TXTHRESH(x)		((x >> 5) << 14)
#define WB_TXTHRESH_CHUNK	32
#define WB_TXTHRESH_INIT	0 /*72*/
 
/*
 * Interrupt mask bits.
 */
enum interruptMaskBits {
	WB_IMR_TX_OK 			= 0x00000001,
	WB_IMR_TX_IDLE 			= 0x00000002,
	WB_IMR_TX_NOBUF 		= 0x00000004,
	WB_IMR_TX_UNDERRUN 		= 0x00000020,
	WB_IMR_TX_EARLY 		= 0x00000400,
	WB_IMR_RX_EARLY 		= 0x00000008,
	WB_IMR_RX_ERR 			= 0x00000010,
	WB_IMR_RX_OK 			= 0x00000040,
	WB_IMR_RX_NOBUF			= 0x00000080,
	WB_IMR_RX_IDLE			= 0x00000100,
	WB_IMR_TIMER_EXPIRED	= 0x00000800,
	WB_IMR_BUS_ERR 			= 0x00002000,
	WB_IMR_ABNORMAL 		= 0x00008000,
	WB_IMR_NORMAL 			= 0x00010000,
};

#define WB_INTRS	\
	(WB_IMR_RX_OK|WB_IMR_RX_IDLE|WB_IMR_RX_ERR|WB_IMR_RX_NOBUF \
	|WB_IMR_RX_EARLY|WB_IMR_TX_OK|WB_IMR_TX_EARLY|WB_IMR_TX_NOBUF \
	|WB_IMR_TX_UNDERRUN|WB_IMR_TX_IDLE|WB_IMR_BUS_ERR \
	|WB_IMR_ABNORMAL|WB_IMR_NORMAL|WB_IMR_TIMER_EXPIRED)

/*
 * Serial I/O (EEPROM/ROM) bits.
 */
enum EEpromBits {
	WB_SIO_EE_CS 		= 0x00000001,			/* EEPROM chip select */
	WB_SIO_EE_CLK 		= 0x00000002,			/* EEPROM clock */
	WB_SIO_EE_DATAIN 	= 0x00000004,		/* EEPROM data output */
	WB_SIO_EE_DATAOUT 	= 0x00000008,		/* EEPROM data input */
	WB_SIO_ROMDATA4 	= 0x00000010,
	WB_SIO_ROMDATA5 	= 0x00000020,
	WB_SIO_ROMDATA6 	= 0x00000040,
	WB_SIO_ROMDATA7		= 0x00000080,
	WB_SIO_ROMCTL_WRITE	= 0x00000200,
	WB_SIO_ROMCTL_READ 	= 0x00000400,
	WB_SIO_EESEL 		= 0x00000800,
	WB_SIO_MII_CLK 		= 0x00010000,		/* MDIO clock */
	WB_SIO_MII_DATAIN 	= 0x00020000,		/* MDIO data out */
	WB_SIO_MII_DIR 		= 0x00040000,		/* MDIO dir */
	WB_SIO_MII_DATAOUT 	= 0x00080000,	/* MDIO data in */
};

enum EEpromCmd {
	WB_EECMD_WRITE = 0x140,
	WB_EECMD_READ = 0x180,
	WB_EECMD_ERASE = 0x1c0
};

/*
 * Winbond TX/RX descriptor structure.
 */

typedef struct wb_desc wb_desc;
struct wb_desc {
	uint32		wb_status;
	uint32		wb_ctl;
	uint32		wb_data;
	uint32		wb_next;
};

enum rxStatusBits {
	WB_RXSTAT_CRCERR  	= 0x00000002,
	WB_RXSTAT_DRIBBLE 	= 0x00000004,
 	WB_RXSTAT_MIIERR  	= 0x00000008,
 	WB_RXSTAT_LATEEVENT = 0x00000040,
 	WB_RXSTAT_GIANT 	= 0x00000080,
 	WB_RXSTAT_LASTFRAG 	= 0x00000100,
 	WB_RXSTAT_FIRSTFRAG = 0x00000200,
 	WB_RXSTAT_MULTICAST = 0x00000400,
 	WB_RXSTAT_RUNT 		= 0x00000800,
 	WB_RXSTAT_RXTYPE 	= 0x00003000,
 	WB_RXSTAT_RXERR 	= 0x00008000,
 	WB_RXSTAT_RXLEN		= 0x3FFF0000,
	WB_RXSTAT_RXCMP		= 0x40000000,
	WB_RXSTAT_OWN		= 0x80000000
};

#define WB_RXBYTES(x)		((x & WB_RXSTAT_RXLEN) >> 16)
#define WB_RXSTAT (WB_RXSTAT_FIRSTFRAG|WB_RXSTAT_LASTFRAG|WB_RXSTAT_OWN)

enum rxControlBits {
	WB_RXCTL_BUFLEN1	= 0x00000FFF,
 	WB_RXCTL_BUFLEN2	= 0x00FFF000,
 	WB_RXCTL_RLINK 		= 0x01000000,
 	WB_RXCTL_RLAST		= 0x02000000
};


enum txStatusBits {
	WB_TXSTAT_DEFER		= 0x00000001,
 	WB_TXSTAT_UNDERRUN	= 0x00000002,
 	WB_TXSTAT_COLLCNT	= 0x00000078,
 	WB_TXSTAT_SQE		= 0x00000080,
 	WB_TXSTAT_ABORT		= 0x00000100,
 	WB_TXSTAT_LATECOLL	= 0x00000200,
 	WB_TXSTAT_NOCARRIER	= 0x00000400,
 	WB_TXSTAT_CARRLOST	= 0x00000800,
 	WB_TXSTAT_TXERR		= 0x00001000,
 	WB_TXSTAT_OWN		= 0x80000000
};

enum txControlBits {
	WB_TXCTL_BUFLEN1 	= 0x000007FF,
	WB_TXCTL_BUFLEN2 	= 0x003FF800,
	WB_TXCTL_PAD		= 0x00800000,
	WB_TXCTL_TLINK		= 0x01000000,
	WB_TXCTL_TLAST		= 0x02000000,
	WB_TXCTL_NOCRC		= 0x08000000,
	WB_TXCTL_FIRSTFRAG	= 0x20000000,
	WB_TXCTL_LASTFRAG	= 0x40000000,
	WB_TXCTL_FINT		= 0x80000000
};

#define WB_MAXFRAGS			16
#define WB_RX_LIST_CNT		64
#define WB_TX_LIST_CNT		64
#define WB_RX_CNT_MASK		(WB_RX_LIST_CNT - 1)
#define WB_TX_CNT_MASK		(WB_TX_LIST_CNT - 1)
#define WB_MIN_FRAMELEN		60
#define WB_MAX_FRAMELEN		1536

#define WB_UNSENT	0x1234
#define WB_BUFBYTES 2048

/* Ethernet defines */
#define CRC_SIZE 4
#define ETHER_TRANSMIT_TIMEOUT ((bigtime_t)5000000)  /* five seconds */
#define WB_TIMEOUT		1000

typedef struct wb_mii_frame wb_mii_frame;
struct wb_mii_frame {
	uint8		mii_stdelim;
	uint8		mii_opcode;
	uint8		mii_phyaddr;
	uint8		mii_regaddr;
	uint8		mii_turnaround;
	uint16		mii_data;
};

/*
 * MII constants
 */
#define WB_MII_STARTDELIM	0x01
#define WB_MII_READOP		0x02
#define WB_MII_WRITEOP		0x01
#define WB_MII_TURNAROUND	0x02

typedef struct wb_device wb_device;
struct wb_device {
	timer		timer;
	int32		devId;
	pci_info*	pciInfo;
	uint16		irq;		/* IRQ line */
	volatile uint32 reg_base;	/* hardware register base address */
	
	// rx data
	volatile wb_desc rxDescriptor[WB_RX_LIST_CNT];
	volatile void* rxBuffer[WB_RX_LIST_CNT];
	int32 rxLock;
	sem_id rxSem;
	spinlock rxSpinlock;
	area_id rxArea; 	
	int16 rxCurrent;
	int16 rxInterruptIndex;
	int16 rxFree;
	
	//tx data
	volatile wb_desc txDescriptor[WB_TX_LIST_CNT];
	volatile char* txBuffer[WB_TX_LIST_CNT];
	int32 txLock;
	sem_id txSem;
	spinlock txSpinlock;
	area_id txArea;		
	int16 txCurrent;
	int16 txInterruptIndex;
	int16 txSent;
	
	struct mii_phy* firstPHY;
	struct mii_phy* currentPHY;
	uint16 phy;
	bool autoNegotiationComplete;
	bool link;
	bool full_duplex;
	uint16 speed;
	uint16 fixedMode;
	
	volatile int32 blockFlag;
	ether_address_t MAC_Address;
	
	spinlock intLock;
	const char* deviceName;
	uint8 wb_type;
	uint16 wb_txthresh;
	int wb_cachesize;
};


/* MII Interface */
struct mii_phy {
	struct mii_phy *next;
	uint16	id0, id1;
	uint16	address;
	uint8	types;
};


// taken from Axel's Sis900 driver
enum MII_address {
	// standard registers
	MII_CONTROL		= 0x00,
	MII_STATUS		= 0x01,
	MII_PHY_ID0		= 0x02,
	MII_PHY_ID1		= 0x03,
	MII_AUTONEG_ADV				= 0x04,
	MII_AUTONEG_LINK_PARTNER	= 0x05,
	MII_AUTONEG_EXT				= 0x06
};

enum MII_control {
	MII_CONTROL_RESET			= 0x8000,
	MII_CONTROL_RESET_AUTONEG	= 0x0200,
	MII_CONTROL_AUTO			= 0x1000,
	MII_CONTROL_FULL_DUPLEX		= 0x0100,
	MII_CONTROL_ISOLATE			= 0x0400
};

enum MII_commands {
	MII_CMD_READ		= 0x6000,
	MII_CMD_WRITE		= 0x5002,

	MII_PHY_SHIFT		= 7,
	MII_REG_SHIFT		= 2,
};

enum MII_status_bits {
	MII_STATUS_EXT			= 0x0001,
	MII_STATUS_JAB			= 0x0002,
	MII_STATUS_LINK			= 0x0004,
	MII_STATUS_CAN_AUTO		= 0x0008,
	MII_STATUS_FAULT		= 0x0010,
	MII_STATUS_AUTO_DONE	= 0x0020,
	MII_STATUS_CAN_T		= 0x0800,
	MII_STATUS_CAN_T_FDX	= 0x1000,
	MII_STATUS_CAN_TX		= 0x2000,
	MII_STATUS_CAN_TX_FDX	= 0x4000,
	MII_STATUS_CAN_T4		= 0x8000
};

enum MII_auto_negotiation {
	MII_NWAY_NODE_SEL	= 0x001f,
	MII_NWAY_CSMA_CD	= 0x0001,
	MII_NWAY_T			= 0x0020,
	MII_NWAY_T_FDX		= 0x0040,
	MII_NWAY_TX			= 0x0080,
	MII_NWAY_TX_FDX		= 0x0100,
	MII_NWAY_T4			= 0x0200,
	MII_NWAY_PAUSE		= 0x0400,
	MII_NWAY_RF			= 0x2000,
	MII_NWAY_ACK		= 0x4000,
	MII_NWAY_NP			= 0x8000
};


enum MII_link_status {
	MII_LINK_FAIL			= 0x4000,
	MII_LINK_100_MBIT		= 0x0080,
	MII_LINK_FULL_DUPLEX	= 0x0040
};

enum link_modes {
	LINK_HALF_DUPLEX	= 0x0100,
	LINK_FULL_DUPLEX	= 0x0200,
	LINK_DUPLEX_MASK	= 0xff00,

	LINK_SPEED_HOME		= 1,
	LINK_SPEED_10_MBIT	= 10,
	LINK_SPEED_100_MBIT	= 100,
	LINK_SPEED_DEFAULT	= LINK_SPEED_100_MBIT,
	LINK_SPEED_MASK		= 0x00ff
};

/*
 * Vendor and Card IDs
 *
 * Winbond
 */
#define	WB_VENDORID			0x1050
#define	WB_DEVICEID_840F	0x0840

/*
 * Compex
 */
#define CP_VENDORID			0x11F6
#define CP_DEVICEID_RL100	0x2011

/*
 * Utility Macros
 */
#define WB_SETBIT(reg, x) write32(reg, read32(reg) | x)
#define WB_CLRBIT(reg, x) write32(reg, read32(reg) & ~x)

// Prototypes
extern int32		wb_interrupt(void* arg);

extern status_t		wb_create_semaphores(wb_device* device);
extern void		wb_delete_semaphores(wb_device* device);

extern status_t		wb_create_rings(wb_device* device);
extern void		wb_delete_rings(wb_device* device);

extern void		wb_init(wb_device* device);
extern void		wb_reset(wb_device* device);
extern status_t		wb_stop(wb_device* device);

extern status_t		wb_initPHYs(wb_device* device);

extern void		wb_disable_interrupts(wb_device* device);
extern void		wb_enable_interrupts(wb_device* device);

extern void		wb_set_mode(wb_device* device, int mode);
extern int32		wb_read_mode(wb_device* device);

extern void		wb_set_rx_filter(wb_device* device);

extern int32		wb_tick(timer* arg);
extern void		wb_put_rx_descriptor(volatile wb_desc* desc);

extern void		print_address(ether_address_t* addr);

#endif //__WB840_H
