
/******************************************************************************/
/*                                                                            */
/* Broadcom BCM4400 Linux Network Driver, Copyright (c) 2002 Broadcom         */
/* Corporation.                                                               */
/* All rights reserved.                                                       */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation, located in the file LICENSE.                 */
/*                                                                            */
/* History:                                                                   */
/*                                                                            */
/******************************************************************************/

#ifndef B44_H
#define B44_H

#include "b44lm.h"



/******************************************************************************/
/* Constants. */
/******************************************************************************/

/* Maxim number of packet descriptors used for sending packets. */
#define MAX_TX_PACKET_DESC_COUNT            512
#define DEFAULT_TX_PACKET_DESC_COUNT        64

#define MAX_RX_PACKET_DESC_COUNT            512
#define DEFAULT_RX_PACKET_DESC_COUNT        64

#define BIT_0                               0x01
#define BIT_1                               0x02
#define BIT_2                               0x04
#define BIT_3                               0x08
#define BIT_4                               0x10
#define BIT_5                               0x20
#define BIT_6                               0x40
#define BIT_7                               0x80
#define BIT_8                               0x100
#define BIT_9                               0x200
#define BIT_10                              0x400
#define BIT_11                              0x800
#define BIT_12                              0x1000
#define BIT_13                              0x2000
#define BIT_14                              0x4000
#define BIT_15                              0x8000
#define BIT_16                              0x10000
#define BIT_17                              0x20000
#define BIT_18                              0x40000
#define BIT_19                              0x80000
#define BIT_20                              0x100000
#define BIT_21                              0x200000
#define BIT_22                              0x400000
#define BIT_23                              0x800000
#define BIT_24                              0x1000000
#define BIT_25                              0x2000000
#define BIT_26                              0x4000000
#define BIT_27                              0x8000000
#define BIT_28                              0x10000000
#define BIT_29                              0x20000000
#define BIT_30                              0x40000000
#define BIT_31                              0x80000000

#define ROUNDUP(x, y)        ((((LM_UINT32)(x)+((y)-1))/(y))*(y))

/******************************************************************************/
/* MII registers. */
/******************************************************************************/

/* Control register. */
#define PHY_CTRL_REG                                0x00

#define PHY_CTRL_SPEED_MASK                         (BIT_6 | BIT_13)
#define PHY_CTRL_SPEED_SELECT_10MBPS                BIT_NONE
#define PHY_CTRL_SPEED_SELECT_100MBPS               BIT_13
#define PHY_CTRL_SPEED_SELECT_1000MBPS              BIT_6
#define PHY_CTRL_COLLISION_TEST_ENABLE              BIT_7
#define PHY_CTRL_FULL_DUPLEX_MODE                   BIT_8
#define PHY_CTRL_RESTART_AUTO_NEG                   BIT_9
#define PHY_CTRL_ISOLATE_PHY                        BIT_10
#define PHY_CTRL_LOWER_POWER_MODE                   BIT_11
#define PHY_CTRL_AUTO_NEG_ENABLE                    BIT_12
#define PHY_CTRL_LOOPBACK_MODE                      BIT_14
#define PHY_CTRL_PHY_RESET                          BIT_15


/* Status register. */
#define PHY_STATUS_REG                              0x01

#define PHY_STATUS_LINK_PASS                        BIT_2
#define PHY_STATUS_AUTO_NEG_COMPLETE                BIT_5


/* Phy Id registers. */
#define PHY_ID1_REG                                 0x02
#define PHY_ID1_OUI_MASK                            0xffff

#define PHY_ID2_REG                                 0x03
#define PHY_ID2_REV_MASK                            0x000f
#define PHY_ID2_MODEL_MASK                          0x03f0
#define PHY_ID2_OUI_MASK                            0xfc00


/* Auto-negotiation advertisement register. */
#define PHY_AN_AD_REG                               0x04

#define PHY_AN_AD_ASYM_PAUSE                        BIT_11
#define PHY_AN_AD_PAUSE_CAPABLE                     BIT_10
#define PHY_AN_AD_10BASET_HALF                      BIT_5
#define PHY_AN_AD_10BASET_FULL                      BIT_6
#define PHY_AN_AD_100BASETX_HALF                    BIT_7
#define PHY_AN_AD_100BASETX_FULL                    BIT_8
#define PHY_AN_AD_PROTOCOL_802_3_CSMA_CD            0x01

#define PHY_AN_AD_ALL_SPEEDS (PHY_AN_AD_10BASET_HALF | \
    PHY_AN_AD_10BASET_FULL | PHY_AN_AD_100BASETX_HALF | \
    PHY_AN_AD_100BASETX_FULL)

/* Auto-negotiation Link Partner Ability register. */
#define PHY_LINK_PARTNER_ABILITY_REG                0x05

#define PHY_LINK_PARTNER_ASYM_PAUSE                 BIT_11
#define PHY_LINK_PARTNER_PAUSE_CAPABLE              BIT_10


#define	STAT_REMFAULT	(1 << 4)	/* remote fault */
#define	STAT_LINK	(1 << 2)	/* link status */
#define	STAT_JAB	(1 << 1)	/* jabber detected */
#define	AUX_FORCED	(1 << 2)	/* forced 10/100 */
#define	AUX_SPEED	(1 << 1)	/* speed 0=10mbps 1=100mbps */
#define	AUX_DUPLEX	(1 << 0)	/* duplex 0=half 1=full */


/******************************************************************************/
/* Register definitions. */
/******************************************************************************/

typedef volatile LM_UINT8 T3_8BIT_REGISTER, *PT3_8BIT_REGISTER;
typedef volatile LM_UINT16 T3_16BIT_REGISTER, *PT3_16BIT_REGISTER;
typedef volatile LM_UINT32 T3_32BIT_REGISTER, *PT3_32BIT_REGISTER;

/*
 * Each DMA processor consists of a transmit channel and a receive channel.
 */
typedef volatile struct {
	/* transmit channel */
	LM_UINT32	xmtcontrol;			/* enable, et al */
	LM_UINT32	xmtaddr;			/* descriptor ring base address (4K aligned) */
	LM_UINT32	xmtptr;				/* last descriptor posted to chip */
	LM_UINT32	xmtstatus;			/* current active descriptor, et al */

	/* receive channel */
	LM_UINT32	rcvcontrol;			/* enable, et al */
	LM_UINT32	rcvaddr;			/* descriptor ring base address (4K aligned) */
	LM_UINT32	rcvptr;				/* last descriptor posted to chip */
	LM_UINT32	rcvstatus;			/* current active descriptor, et al */

	/* diag access */
	LM_UINT32	fifoaddr;			/* diag address */
	LM_UINT32	fifodatalow;			/* low 32bits of data */
	LM_UINT32	fifodatahigh;			/* high 32bits of data */
	LM_UINT32	pad;				/* reserved */
} dmaregs_t;

/* transmit channel control */
#define	XC_XE		((LM_UINT32)1 << 0)	/* transmit enable */
#define	XC_SE		((LM_UINT32)1 << 1)	/* transmit suspend request */
#define	XC_LE		((LM_UINT32)1 << 2)	/* loopback enable */
#define	XC_FPRI		((LM_UINT32)1 << 3)	/* fair priority */
#define	XC_FL		((LM_UINT32)1 << 4)	/* flush request */

/* transmit descriptor table pointer */
#define	XP_LD_MASK	0xfff			/* last valid descriptor */

/* transmit channel status */
#define	XS_CD_MASK	0x0fff			/* current descriptor pointer */
#define	XS_XS_MASK	0xf000			/* transmit state */
#define	XS_XS_SHIFT	12
#define	XS_XS_DISABLED	0x0000			/* disabled */
#define	XS_XS_ACTIVE	0x1000			/* active */
#define	XS_XS_IDLE	0x2000			/* idle wait */
#define	XS_XS_STOPPED	0x3000			/* stopped */
#define	XS_XS_SUSP	0x4000			/* suspend pending */
#define	XS_XE_MASK	0xf0000			/* transmit errors */
#define	XS_XE_SHIFT	16
#define	XS_XE_NOERR	0x00000			/* no error */
#define	XS_XE_DPE	0x10000			/* descriptor protocol error */
#define	XS_XE_DFU	0x20000			/* data fifo underrun */
#define	XS_XE_BEBR	0x30000			/* bus error on buffer read */
#define	XS_XE_BEDA	0x40000			/* bus error on descriptor access */
#define	XS_FL		((LM_UINT32)1 << 20)	/* flushed */

/* receive channel control */
#define	RC_RE		((LM_UINT32)1 << 0)	/* receive enable */
#define	RC_RO_MASK	0xfe			/* receive frame offset */
#define	RC_RO_SHIFT	1

/* receive descriptor table pointer */
#define	RP_LD_MASK	0xfff			/* last valid descriptor */

/* receive channel status */
#define	RS_CD_MASK	0x0fff			/* current descriptor pointer */
#define	RS_RS_MASK	0xf000			/* receive state */
#define	RS_RS_SHIFT	12
#define	RS_RS_DISABLED	0x0000			/* disabled */
#define	RS_RS_ACTIVE	0x1000			/* active */
#define	RS_RS_IDLE	0x2000			/* idle wait */
#define	RS_RS_STOPPED	0x3000			/* reserved */
#define	RS_RE_MASK	0xf0000			/* receive errors */
#define	RS_RE_SHIFT	16
#define	RS_RE_NOERR	0x00000			/* no error */
#define	RS_RE_DPE	0x10000			/* descriptor protocol error */
#define	RS_RE_DFO	0x20000			/* data fifo overflow */
#define	RS_RE_BEBW	0x30000			/* bus error on buffer write */
#define	RS_RE_BEDA	0x40000			/* bus error on descriptor access */

/* fifoaddr */
#define	FA_OFF_MASK	0xffff			/* offset */
#define	FA_SEL_MASK	0xf0000			/* select */
#define	FA_SEL_SHIFT	16
#define	FA_SEL_XDD	0x00000			/* transmit dma data */
#define	FA_SEL_XDP	0x10000			/* transmit dma pointers */
#define	FA_SEL_RDD	0x40000			/* receive dma data */
#define	FA_SEL_RDP	0x50000			/* receive dma pointers */
#define	FA_SEL_XFD	0x80000			/* transmit fifo data */
#define	FA_SEL_XFP	0x90000			/* transmit fifo pointers */
#define	FA_SEL_RFD	0xc0000			/* receive fifo data */
#define	FA_SEL_RFP	0xd0000			/* receive fifo pointers */

/*
 * DMA Descriptor
 * Descriptors are only read by the hardware, never written back.
 */
typedef volatile struct {
	LM_UINT32	ctrl;		/* misc control bits & bufcount */
	LM_UINT32	addr;		/* data buffer address */
} dmadd_t;

/*
 * Each descriptor ring must be 4096byte aligned
 * and fit within a single 4096byte page.
 */
#define	DMAMAXRINGSZ	4096
#define	DMARINGALIGN	4096

/* control flags */
#define	CTRL_BC_MASK	0x1fff			/* buffer byte count */
#define	CTRL_EOT	((LM_UINT32)1 << 28)	/* end of descriptor table */
#define	CTRL_IOC	((LM_UINT32)1 << 29)	/* interrupt on completion */
#define	CTRL_EOF	((LM_UINT32)1 << 30)	/* end of frame */
#define	CTRL_SOF	((LM_UINT32)1 << 31)	/* start of frame */

/* control flags in the range [27:20] are core-specific and not defined here */
#define	CTRL_CORE_MASK	0x0ff00000


#define	BCMENET_NFILTERS	64		/* # ethernet address filter entries */
#define	BCMENET_MCHASHBASE	0x200		/* multicast hash filter base address */
#define	BCMENET_MCHASHSIZE	256		/* multicast hash filter size in bytes */
#define	BCMENET_MAX_DMA		4096		/* chip has 12 bits of DMA addressing */

/* power management event wakeup pattern constants */
#define	BCMENET_NPMP		4		/* chip supports 4 wakeup patterns */
#define	BCMENET_PMPBASE		0x400		/* wakeup pattern base address */
#define	BCMENET_PMPSIZE		0x80		/* 128bytes each pattern */
#define	BCMENET_PMMBASE		0x600		/* wakeup mask base address */
#define	BCMENET_PMMSIZE		0x10		/* 128bits each mask */

/* PCI config space "back door" access registers */
#define BCMENET_BACK_DOOR_ADDR	0xa0
#define BCMENET_BACK_DOOR_DATA	0xa4

#define BCMENET_PMC             0x42
#define BCMENET_PMCSR           0x44
#define ENABLE_PCICONFIG_PME    0x8100

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

/*
 * EMAC MIB Registers
 */
typedef volatile struct {
	LM_UINT32 tx_good_octets;
	LM_UINT32 tx_good_pkts;
	LM_UINT32 tx_octets;
	LM_UINT32 tx_pkts;
	LM_UINT32 tx_broadcast_pkts;
	LM_UINT32 tx_multicast_pkts;
	LM_UINT32 tx_len_64;
	LM_UINT32 tx_len_65_to_127;
	LM_UINT32 tx_len_128_to_255;
	LM_UINT32 tx_len_256_to_511;
	LM_UINT32 tx_len_512_to_1023;
	LM_UINT32 tx_len_1024_to_max;
	LM_UINT32 tx_jabber_pkts;
	LM_UINT32 tx_oversize_pkts;
	LM_UINT32 tx_fragment_pkts;
	LM_UINT32 tx_underruns;
	LM_UINT32 tx_total_cols;
	LM_UINT32 tx_single_cols;
	LM_UINT32 tx_multiple_cols;
	LM_UINT32 tx_excessive_cols;
	LM_UINT32 tx_late_cols;
	LM_UINT32 tx_defered;
	LM_UINT32 tx_carrier_lost;
	LM_UINT32 tx_pause_pkts;
	LM_UINT32 PAD[8];

	LM_UINT32 rx_good_octets;
	LM_UINT32 rx_good_pkts;
	LM_UINT32 rx_octets;
	LM_UINT32 rx_pkts;
	LM_UINT32 rx_broadcast_pkts;
	LM_UINT32 rx_multicast_pkts;
	LM_UINT32 rx_len_64;
	LM_UINT32 rx_len_65_to_127;
	LM_UINT32 rx_len_128_to_255;
	LM_UINT32 rx_len_256_to_511;
	LM_UINT32 rx_len_512_to_1023;
	LM_UINT32 rx_len_1024_to_max;
	LM_UINT32 rx_jabber_pkts;
	LM_UINT32 rx_oversize_pkts;
	LM_UINT32 rx_fragment_pkts;
	LM_UINT32 rx_missed_pkts;
	LM_UINT32 rx_crc_align_errs;
	LM_UINT32 rx_undersize;
	LM_UINT32 rx_crc_errs;
	LM_UINT32 rx_align_errs;
	LM_UINT32 rx_symbol_errs;
	LM_UINT32 rx_pause_pkts;
	LM_UINT32 rx_nonpause_pkts;
} bcmenetmib_t;

#define SB_ENUM_BASE    0x18000000
#define SB_CORE_SIZE    0x1000
#define	SBCONFIGOFF	0xf00			/* core register space offset in bytes */
#define	SBCONFIGSIZE	256			/* size in bytes */

/*
 * Sonics Configuration Space Registers.
 */
typedef volatile struct _sbconfig {
	LM_UINT32	PAD[2];
	LM_UINT32	sbipsflag;		/* initiator port ocp slave flag */
	LM_UINT32	PAD[3];
	LM_UINT32	sbtpsflag;		/* target port ocp slave flag */
	LM_UINT32	PAD[17];
	LM_UINT32	sbadmatch3;		/* address match3 */
	LM_UINT32	PAD;
	LM_UINT32	sbadmatch2;		/* address match2 */
	LM_UINT32	PAD;
	LM_UINT32	sbadmatch1;		/* address match1 */
	LM_UINT32	PAD[7];
	LM_UINT32	sbimstate;		/* initiator agent state */
	LM_UINT32	sbintvec;		/* interrupt mask */
	LM_UINT32	sbtmstatelow;		/* target state */
	LM_UINT32	sbtmstatehigh;		/* target state */
	LM_UINT32	sbbwa0;			/* bandwidth allocation table0 */
	LM_UINT32	PAD;
	LM_UINT32	sbimconfiglow;		/* initiator configuration */
	LM_UINT32	sbimconfighigh;		/* initiator configuration */
	LM_UINT32	sbadmatch0;		/* address match0 */
	LM_UINT32	PAD;
	LM_UINT32	sbtmconfiglow;		/* target configuration */
	LM_UINT32	sbtmconfighigh;		/* target configuration */
	LM_UINT32	sbbconfig;		/* broadcast configuration */
	LM_UINT32	PAD;
	LM_UINT32	sbbstate;		/* broadcast state */
	LM_UINT32	PAD[3];
	LM_UINT32	sbactcnfg;		/* activate configuration */
	LM_UINT32	PAD[3];
	LM_UINT32	sbflagst;		/* current sbflags */
	LM_UINT32	PAD[3];
	LM_UINT32	sbidlow;		/* identification */
	LM_UINT32	sbidhigh;		/* identification */
} sbconfig_t;

/* XXX 4710-specific and should be deleted since they can be read from sbtpsflag */
/* interrupt sbFlags */
#define	SBFLAG_PCI	0
#define	SBFLAG_ENET0	1
#define	SBFLAG_ILINE20	2
#define	SBFLAG_CODEC	3
#define	SBFLAG_USB	4
#define	SBFLAG_EXTIF	5
#define	SBFLAG_ENET1	6

/* sbipsflag */
#define SBIPSFLAG	0x08		/* offset */
#define	SBIPS_INT1_MASK	0x3f		/* which sbflags get routed to mips interrupt 1 */
#define	SBIPS_INT1_SHIFT	0
#define	SBIPS_INT2_MASK	0x3f00		/* which sbflags get routed to mips interrupt 2 */
#define	SBIPS_INT2_SHIFT	8
#define	SBIPS_INT3_MASK	0x3f0000	/* which sbflags get routed to mips interrupt 3 */
#define	SBIPS_INT3_SHIFT	16
#define	SBIPS_INT4_MASK	0x3f000000	/* which sbflags get routed to mips interrupt 4 */
#define	SBIPS_INT4_SHIFT	24

/* sbtpsflag */
#define SBTPSFLAG	0x18		/* offset */
#define	SBTPS_NUM0_MASK	0x3f		/* interrupt sbFlag # generated by this core */
#define	SBTPS_F0EN0	0x40		/* interrupt is always sent on the backplane */

/* sbadmatch */
#define SBADMATCH3	0x60		/* offset */
#define SBADMATCH2	0x68		/* offset */
#define SBADMATCH1	0x70		/* offset */

/* sbimstate */
#define SBIMSTATE	0x90		/* offset */
#define	SBIM_PC		0xf		/* pipecount */
#define	SBIM_AP_MASK	0x30		/* arbitration policy */
#define	SBIM_AP_BOTH	0x00		/* use both timeslaces and token */
#define	SBIM_AP_TS	0x10		/* use timesliaces only */
#define	SBIM_AP_TK	0x20		/* use token only */
#define	SBIM_AP_RSV	0x30		/* reserved */
#define	SBIM_IBE	0x20000		/* inbanderror */
#define	SBIM_TO		0x40000		/* timeout */

/* sbintvec */
#define SBINTVEC	0x94		/* offset */
#define	SBIV_PCI	0x1		/* enable interrupts for pci */
#define	SBIV_ENET0	0x2		/* enable interrupts for enet 0 */
#define	SBIV_ILINE20	0x4		/* enable interrupts for iline20 */
#define	SBIV_CODEC	0x8		/* enable interrupts for v90 codec */
#define	SBIV_USB	0x10		/* enable interrupts for usb */
#define	SBIV_EXTIF	0x20		/* enable interrupts for external i/f */
#define	SBIV_ENET1	0x40		/* enable interrupts for enet 1 */

/* sbtmstatelow */
#define SBTMSTATELOW	0x98		/* offset */
#define	SBTML_RESET	0x1		/* reset */
#define	SBTML_REJ	0x2		/* reject */
#define	SBTML_CLK	0x10000		/* clock enable */
#define	SBTML_FGC	0x20000		/* force gated clocks on */
#define	SBTML_PE	0x40000000	/* pme enable */
#define	SBTML_BE	0x80000000	/* bist enable */

/* sbtmstatehigh */
#define SBTMSTATEHIGH	0x9C		/* offset */
#define	SBTMH_SERR	0x1		/* serror */
#define	SBTMH_INT	0x2		/* interrupt */
#define	SBTMH_BUSY	0x4		/* busy */
#define	SBTMH_GCR	0x20000000	/* gated clock request */
#define	SBTMH_BISTF	0x40000000	/* bist failed */
#define	SBTMH_BISTD	0x80000000	/* bist done */

/* sbbwa0 */
#define SBBWA0		0xA0		/* offset */
#define	SBBWA_TAB0_MASK	0xffff		/* lookup table 0 */
#define	SBBWA_TAB1_MASK	0xffff		/* lookup table 1 */
#define	SBBWA_TAB1_SHIFT	16

/* sbimconfiglow */
#define SBIMCONFIGLOW	0xA8		/* offset */
#define	SBIMCL_STO_MASK	0x3		/* service timeout */
#define	SBIMCL_RTO_MASK	0x30		/* request timeout */
#define	SBIMCL_RTO_SHIFT	4
#define	SBIMCL_CID_MASK	0xff0000	/* connection id */
#define	SBIMCL_CID_SHIFT	16

/* sbimconfighigh */
#define SBIMCONFIGHIGH	0xAC		/* offset */
#define	SBIMCH_IEM_MASK	0xc		/* inband error mode */
#define	SBIMCH_TEM_MASK	0x30		/* timeout error mode */
#define	SBIMCH_TEM_SHIFT	4
#define	SBIMCH_BEM_MASK	0xc0		/* bus error mode */
#define	SBIMCH_BEM_SHIFT	6

/* sbadmatch0 */
#define SBADMATCH0	0xB0		/* offset */
#define	SBAM_TYPE_MASK	0x3		/* address type */
#define	SBAM_AD64	0x4		/* reserved */
#define	SBAM_ADINT0_MASK	0xf8	/* type0 size */
#define	SBAM_ADINT0_SHIFT	3
#define	SBAM_ADINT1_MASK	0x1f8	/* type1 size */
#define	SBAM_ADINT1_SHIFT	3
#define	SBAM_ADINT2_MASK	0x1f8	/* type2 size */
#define	SBAM_ADINT2_SHIFT	3
#define	SBAM_ADEN		0x400	/* enable */
#define	SBAM_ADNEG		0x800	/* negative decode */
#define	SBAM_BASE0_MASK	0xffffff00	/* type0 base address */
#define	SBAM_BASE0_SHIFT	8
#define	SBAM_BASE1_MASK	0xfffff000	/* type1 base address for the core */
#define	SBAM_BASE1_SHIFT	12
#define	SBAM_BASE2_MASK	0xffff0000	/* type2 base address for the core */
#define	SBAM_BASE2_SHIFT	16

/* sbtmconfiglow */
#define SBTMCONFIGLOW	0xB8		/* offset */
#define	SBTMCL_CD_MASK	0xff		/* clock divide */
#define	SBTMCL_CO_MASK	0xf800		/* clock offset */
#define	SBTMCL_CO_SHIFT	11
#define	SBTMCL_IF_MASK	0xfc0000	/* interrupt flags */
#define	SBTMCL_IF_SHIFT	18
#define	SBTMCL_IM_MASK	0x3000000	/* interrupt mode */
#define	SBTMCL_IM_SHIFT	24

/* sbtmconfighigh */
#define SBTMCONFIGHIGH	0xBC		/* offset */
#define	SBTMCH_BM_MASK	0x3		/* busy mode */
#define	SBTMCH_RM_MASK	0x3		/* retry mode */
#define	SBTMCH_RM_SHIFT	2
#define	SBTMCH_SM_MASK	0x30		/* stop mode */
#define	SBTMCH_SM_SHIFT	4
#define	SBTMCH_EM_MASK	0x300		/* sb error mode */
#define	SBTMCH_EM_SHIFT	8
#define	SBTMCH_IM_MASK	0xc00		/* int mode */
#define	SBTMCH_IM_SHIFT	10

/* sbbconfig */
#define SBBCONFIG	0xC0		/* offset */
#define	SBBC_LAT_MASK	0x3		/* sb latency */
#define	SBBC_MAX0_MASK	0xf0000		/* maxccntr0 */
#define	SBBC_MAX0_SHIFT	16
#define	SBBC_MAX1_MASK	0xf00000	/* maxccntr1 */
#define	SBBC_MAX1_SHIFT	20

/* sbbstate */
#define SBBSTATE	0xC8		/* offset */
#define	SBBS_SRD	0x1		/* st reg disable */
#define	SBBS_HRD	0x2		/* hold reg disable */

/* sbactcnfg */
#define SBACTCNFG	0xD8		/* offset */

/* sbflagst */
#define	SBFLAGST	0xE8		/* offset */

/* sbidlow */
#define SBIDLOW		0xF8		/* offset */
#define	SBIDL_CS_MASK	0x3		/* config space */
#define	SBIDL_AR_MASK	0x38		/* # address ranges supported */
#define	SBIDL_AR_SHIFT	3
#define	SBIDL_SYNCH	0x40		/* sync */
#define	SBIDL_INIT	0x80		/* initiator */
#define	SBIDL_MINLAT_MASK	0xf00	/* minimum backplane latency */
#define	SBIDL_MINLAT_SHIFT	8
#define	SBIDL_MAXLAT	0xf000		/* maximum backplane latency */
#define	SBIDL_MAXLAT_SHIFT	12
#define	SBIDL_FIRST	0x10000		/* this initiator is first */
#define	SBIDL_CW_MASK	0xc0000		/* cycle counter width */
#define	SBIDL_CW_SHIFT	18
#define	SBIDL_TP_MASK	0xf00000	/* target ports */
#define	SBIDL_TP_SHIFT	20
#define	SBIDL_IP_MASK	0xf000000	/* initiator ports */
#define	SBIDL_IP_SHIFT	24

/* sbidhigh */
#define SBIDHIGH	0xFC		/* offset */
#define	SBIDH_RC_MASK	0xf		/* revision code*/
#define	SBIDH_CC_MASK	0xfff0		/* core code */
#define	SBIDH_CC_SHIFT	4
#define	SBIDH_VC_MASK	0xffff0000	/* vendor code */
#define	SBIDH_VC_SHIFT	16

/* core codes */
#define	SB_ILINE20	0x801		/* iline20 core */
#define	SB_SDRAM	0x803		/* sdram core */
#define	SB_PCI		0x804		/* pci core */
#define	SB_MIPS		0x805		/* mips core */
#define	SB_ENET		0x806		/* enet mac core */
#define	SB_CODEC	0x807		/* v90 codec core */
#define	SB_USB		0x808		/* usb core */
#define	SB_ILINE100	0x80a		/* iline100 core */
#define	SB_EXTIF	0x811		/* external interface core */

struct sbmap {
	LM_UINT32 id;
	LM_UINT32 coreunit;
	LM_UINT32 sbaddr;
};

#define SBID_SDRAM		0
#define SBID_PCI_MEM		1
#define SBID_PCI_CFG		2
#define SBID_PCI_DMA		3
#define	SBID_SDRAM_SWAPPED	4
#define SBID_ENUM		5
#define SBID_REG_SDRAM		6
#define SBID_REG_ILINE20	7
#define SBID_REG_EMAC		8
#define SBID_REG_CODEC		9
#define SBID_REG_USB		10
#define SBID_REG_PCI		11
#define SBID_REG_MIPS		12
#define SBID_REG_EXTIF		13
#define	SBID_EXTIF		14
#define	SBID_EJTAG		15
#define	SBID_MAX		16

/*
 * Host Interface Registers
 */
typedef volatile struct _bcmenettregs {
	/* Device and Power Control */
	LM_UINT32	devcontrol;
	LM_UINT32	PAD[2];
	LM_UINT32	biststatus;
	LM_UINT32	wakeuplength;
#define DISABLE_32_PATMATCH     0x80800000
#define DISABLE_3210_PATMATCH   0x80808080
	LM_UINT32	PAD[3];
	
	/* Interrupt Control */
	LM_UINT32	intstatus;
	LM_UINT32	intmask;
	LM_UINT32	gptimer;
	LM_UINT32	PAD[23];

	/* Ethernet MAC Address Filtering Control */
	LM_UINT32	enetaddrlo;	/* added in B0 */
	LM_UINT32	enetaddrhi;	/* added in B0 */
	LM_UINT32	enetftaddr;
	LM_UINT32	enetftdata;
	LM_UINT32	PAD[2];

	/* Ethernet MAC Control */
	LM_UINT32	emactxmaxburstlen;
	LM_UINT32	emacrxmaxburstlen;
	LM_UINT32	emaccontrol;
	LM_UINT32	emacflowcontrol;

	LM_UINT32	PAD[20];

	/* DMA Lazy Interrupt Control */
	LM_UINT32	intrecvlazy;
	LM_UINT32	PAD[63];

	/* DMA engine */
	dmaregs_t	dmaregs;
	LM_UINT32	PAD[116];

	/* EMAC Registers */
	LM_UINT32 rxconfig;
	LM_UINT32 rxmaxlength;
	LM_UINT32 txmaxlength;
	LM_UINT32 PAD;
	LM_UINT32 mdiocontrol;
	LM_UINT32 mdiodata;
	LM_UINT32 emacintmask;
	LM_UINT32 emacintstatus;
	LM_UINT32 camdatalo;
	LM_UINT32 camdatahi;
	LM_UINT32 camcontrol;
	LM_UINT32 enetcontrol;
	LM_UINT32 txcontrol;
	LM_UINT32 txwatermark;
	LM_UINT32 mibcontrol;
	LM_UINT32 PAD[49];

	/* EMAC MIB counters */
	bcmenetmib_t	mib;

	LM_UINT32	PAD[585];

	/* Sonics SiliconBackplane config registers */
	sbconfig_t	sbconfig;
} bcmenetregs_t;

/* device control */
#define	DC_MPM		((LM_UINT32)1 << 6)	/* Magic Packet PME enable(B0)*/
#define	DC_PM		((LM_UINT32)1 << 7)	/* pattern filtering enable */
#define	DC_IP		((LM_UINT32)1 << 10)	/* internal ephy present (rev >= 1) */
#define	DC_ER		((LM_UINT32)1 << 15)	/* ephy reset */
#define	DC_MP		((LM_UINT32)1 << 16)	/* mii phy mode enable */
#define	DC_CO		((LM_UINT32)1 << 17)	/* mii phy mode: enable clocks */
#define	DC_PA_MASK	0x7c0000		/* mii phy mode: mdc/mdio phy address */
#define	DC_PA_SHIFT	18

/* wakeup length */
#define	WL_P0_MASK	0x7f			/* pattern 0 */
#define	WL_D0		((LM_UINT32)1 << 7)
#define	WL_P1_MASK	0x7f00			/* pattern 1 */
#define	WL_P1_SHIFT	8
#define	WL_D1		((LM_UINT32)1 << 15)
#define	WL_P2_MASK	0x7f0000		/* pattern 2 */
#define	WL_P2_SHIFT	16
#define	WL_D2		((LM_UINT32)1 << 23)
#define	WL_P3_MASK	0x7f000000		/* pattern 3 */
#define	WL_P3_SHIFT	24
#define	WL_D3		((LM_UINT32)1 << 31)

/* intstatus and intmask */
#define I_LS		((LM_UINT32)1 << 5)	/* link change (new in B0) */
#define	I_PME		((LM_UINT32)1 << 6)	/* power management event */
#define	I_TO		((LM_UINT32)1 << 7)	/* general purpose timeout */
#define	I_PC		((LM_UINT32)1 << 10)	/* descriptor error */
#define	I_PD		((LM_UINT32)1 << 11)	/* data error */
#define	I_DE		((LM_UINT32)1 << 12)	/* descriptor protocol error */
#define	I_RU		((LM_UINT32)1 << 13)	/* receive descriptor underflow */
#define	I_RO		((LM_UINT32)1 << 14)	/* receive fifo overflow */
#define	I_XU		((LM_UINT32)1 << 15)	/* transmit fifo underflow */
#define	I_RI		((LM_UINT32)1 << 16)	/* receive interrupt */
#define	I_XI		((LM_UINT32)1 << 24)	/* transmit interrupt */
#define	I_EM		((LM_UINT32)1 << 26)	/* emac interrupt */
#define	I_MW		((LM_UINT32)1 << 27)	/* mii write */
#define	I_MR		((LM_UINT32)1 << 28)	/* mii read */

#define	I_ERRORS	(I_PC | I_PD | I_DE | I_RU | I_RO | I_XU)
#define	DEF_INTMASK	(I_TO | I_XI | I_RI | I_ERRORS)

/* emaccontrol */
#define	EMC_CG		((LM_UINT32)1 << 0)	/* crc32 generation enable */
#define	EMC_EP		((LM_UINT32)1 << 2)	/* onchip ephy: powerdown (rev >= 1) */
#define	EMC_ED		((LM_UINT32)1 << 3)	/* onchip ephy: energy detected (rev >= 1) */
#define	EMC_LC_MASK	0xe0			/* onchip ephy: led control (rev >= 1) */
#define	EMC_LC_SHIFT	5

/* emacflowcontrol */
#define	EMF_RFH_MASK	0xff			/* rx fifo hi water mark */
#define	EMF_PG		((LM_UINT32)1 << 15)	/* enable pause frame generation */

/* interrupt receive lazy */
#define	IRL_TO_MASK	0x00ffffff		/* timeout */
#define	IRL_FC_MASK	0xff000000		/* frame count */
#define	IRL_FC_SHIFT	24			/* frame count */

/* emac receive config */
#define	ERC_DB		((LM_UINT32)1 << 0)	/* disable broadcast */
#define	ERC_AM		((LM_UINT32)1 << 1)	/* accept all multicast */
#define	ERC_RDT		((LM_UINT32)1 << 2)	/* receive disable while transmitting */
#define	ERC_PE		((LM_UINT32)1 << 3)	/* promiscuous enable */
#define	ERC_LE		((LM_UINT32)1 << 4)	/* loopback enable */
#define	ERC_EF		((LM_UINT32)1 << 5)	/* enable flow control */
#define	ERC_UF		((LM_UINT32)1 << 6)	/* accept unicast flow control frame */
#define	ERC_RF		((LM_UINT32)1 << 7)	/* reject filter */

/* emac mdio control */
#define	MC_MF_MASK	0x7f			/* mdc frequency */
#define	MC_PE		((LM_UINT32)1 << 7)	/* mii preamble enable */

/* emac mdio data */
#define	MD_DATA_MASK	0xffffL			/* r/w data */
#define	MD_TA_MASK	0x30000L		/* turnaround value */
#define	MD_TA_SHIFT	16
#define	MD_TA_VALID	(2L << MD_TA_SHIFT)	/* valid ta */
#define	MD_RA_MASK	0x7c0000L		/* register address */
#define	MD_RA_SHIFT	18
#define	MD_PMD_MASK	0xf800000L		/* physical media device */
#define	MD_PMD_SHIFT	23
#define	MD_OP_MASK	0x30000000L		/* opcode */
#define	MD_OP_SHIFT	28
#define	MD_OP_WRITE	(1L << MD_OP_SHIFT)	/* write op */
#define	MD_OP_READ	(2L << MD_OP_SHIFT)	/* read op */
#define	MD_SB_MASK	0xc0000000L		/* start bits */
#define	MD_SB_SHIFT	30
#define	MD_SB_START	(0x1L << MD_SB_SHIFT)	/* start of frame */

/* emac intstatus and intmask */
#define	EI_MII		((LM_UINT32)1 << 0)	/* mii mdio interrupt */
#define	EI_MIB		((LM_UINT32)1 << 1)	/* mib interrupt */
#define	EI_FLOW		((LM_UINT32)1 << 2)	/* flow control interrupt */

/* emac cam data high */
#define	CD_V		((LM_UINT32)1 << 16)	/* valid bit */

/* emac cam control */
#define	CC_CE		((LM_UINT32)1 << 0)	/* cam enable */
#define	CC_MS		((LM_UINT32)1 << 1)	/* mask select */
#define	CC_RD		((LM_UINT32)1 << 2)	/* read */
#define	CC_WR		((LM_UINT32)1 << 3)	/* write */
#define	CC_INDEX_MASK	0x3f0000		/* index */
#define	CC_INDEX_SHIFT	16
#define	CC_CB		((LM_UINT32)1 << 31)	/* cam busy */

/* emac ethernet control */
#define	EC_EE		((LM_UINT32)1 << 0)	/* emac enable */
#define	EC_ED		((LM_UINT32)1 << 1)	/* emac disable */
#define	EC_ES		((LM_UINT32)1 << 2)	/* emac soft reset */
#define	EC_EP		((LM_UINT32)1 << 3)	/* external phy select */

/* emac transmit control */
#define	EXC_FD		((LM_UINT32)1 << 0)	/* full duplex */
#define	EXC_FM		((LM_UINT32)1 << 1)	/* flowmode */
#define	EXC_SB		((LM_UINT32)1 << 2)	/* single backoff enable */
#define	EXC_SS		((LM_UINT32)1 << 3)	/* small slottime */

/* emac mib control */
#define	EMC_RZ		((LM_UINT32)1 << 0)	/* autoclear on read */

/*
 * The Ethernet MAC core returns an 8-byte Receive Frame Data Header
 * with every frame consisting of
 * 16bits of frame length, followed by
 * 16bits of EMAC rx descriptor info, followed by 32bits of undefined.
 */
typedef volatile struct {
	LM_UINT16	len;
	LM_UINT16	flags;
	LM_UINT16	pad[12];
} bcmenetrxh_t;

#define	RXHDR_LEN	28

#define	RXF_L		((LM_UINT16)1 << 11)	/* last buffer in a frame */
#define	RXF_MISS	((LM_UINT16)1 << 7)	/* received due to promisc mode */
#define	RXF_BRDCAST	((LM_UINT16)1 << 6)	/* dest is broadcast address */
#define	RXF_MULT	((LM_UINT16)1 << 5)	/* dest is multicast address */
#define	RXF_LG		((LM_UINT16)1 << 4)	/* frame length > rxmaxlength */
#define	RXF_NO		((LM_UINT16)1 << 3)	/* odd number of nibbles */
#define	RXF_RXER	((LM_UINT16)1 << 2)	/* receive symbol error */
#define	RXF_CRC		((LM_UINT16)1 << 1)	/* crc error */
#define	RXF_OV		((LM_UINT16)1 << 0)	/* fifo overflow */

#define RXF_ERRORS (RXF_NO | RXF_CRC | RXF_OV)

/* Sonics side: PCI core and host control registers */
typedef struct sbpciregs {
	LM_UINT32 control;		/* PCI control */
	LM_UINT32 PAD[3];
	LM_UINT32 arbcontrol;	/* PCI arbiter control */
	LM_UINT32 PAD[3];
	LM_UINT32 intstatus;	/* Interrupt status */
	LM_UINT32 intmask;		/* Interrupt mask */
	LM_UINT32 sbtopcimailbox;	/* Sonics to PCI mailbox */
	LM_UINT32 PAD[9];
	LM_UINT32 bcastaddr;	/* Sonics broadcast address */
	LM_UINT32 bcastdata;	/* Sonics broadcast data */
	LM_UINT32 PAD[42];
	LM_UINT32 sbtopci0;	/* Sonics to PCI translation 0 */
	LM_UINT32 sbtopci1;	/* Sonics to PCI translation 1 */
	LM_UINT32 sbtopci2;	/* Sonics to PCI translation 2 */
	LM_UINT32 PAD[445];
	LM_UINT16 sprom[36];	/* SPROM shadow Area */
	LM_UINT32 PAD[46];
} sbpciregs_t;

/* PCI control */
#define PCI_RST_OE	0x01	/* When set, drives PCI_RESET out to pin */
#define PCI_RST		0x02	/* Value driven out to pin */
#define PCI_CLK_OE	0x04	/* When set, drives clock as gated by PCI_CLK out to pin */
#define PCI_CLK		0x08	/* Gate for clock driven out to pin */	

/* PCI arbiter control */
#define PCI_INT_ARB	0x01	/* When set, use an internal arbiter */
#define PCI_EXT_ARB	0x02	/* When set, use an external arbiter */
#define PCI_PARKID_MASK	0x06	/* Selects which agent is parked on an idle bus */
#define PCI_PARKID_SHIFT   1
#define PCI_PARKID_LAST	   0	/* Last requestor */
#define PCI_PARKID_4710	   1	/* 4710 */
#define PCI_PARKID_EXTREQ0 2	/* External requestor 0 */
#define PCI_PARKID_EXTREQ1 3	/* External requestor 1 */

/* Interrupt status/mask */
#define PCI_INTA	0x01	/* PCI INTA# is asserted */
#define PCI_INTB	0x02	/* PCI INTB# is asserted */
#define PCI_SERR	0x04	/* PCI SERR# has been asserted (write one to clear) */
#define PCI_PERR	0x08	/* PCI PERR# has been asserted (write one to clear) */
#define PCI_PME		0x10	/* PCI PME# is asserted */

/* (General) PCI/SB mailbox interrupts, two bits per pci function */
#define	MAILBOX_F0_0	0x100	/* function 0, int 0 */
#define	MAILBOX_F0_1	0x200	/* function 0, int 1 */
#define	MAILBOX_F1_0	0x400	/* function 1, int 0 */
#define	MAILBOX_F1_1	0x800	/* function 1, int 1 */
#define	MAILBOX_F2_0	0x1000	/* function 2, int 0 */
#define	MAILBOX_F2_1	0x2000	/* function 2, int 1 */
#define	MAILBOX_F3_0	0x4000	/* function 3, int 0 */
#define	MAILBOX_F3_1	0x8000	/* function 3, int 1 */

/* Sonics broadcast address */
#define BCAST_ADDR_MASK	0xff	/* Broadcast register address */

/* Sonics to PCI translation types */
#define SBTOPCI0_MASK	0xfc000000
#define SBTOPCI1_MASK	0xfc000000
#define SBTOPCI2_MASK	0xc0000000
#define SBTOPCI_MEM	0
#define SBTOPCI_IO	1
#define SBTOPCI_CFG0	2
#define SBTOPCI_CFG1	3
#define	SBTOPCI_PREF	0x4	/* prefetch enable */
#define	SBTOPCI_BURST	0x8	/* burst enable */

/* PCI side: Reserved PCI configuration registers (see pcicfg.h) */
#define cap_list	rsvd_a[0]
#define bar0_window	dev_dep[0x80 - 0x40]
#define bar1_window	dev_dep[0x84 - 0x40]
#define sprom_control	dev_dep[0x88 - 0x40]

#define	PCI_BAR0_WIN		0x80
#define	PCI_BAR1_WIN		0x84
#define	PCI_SPROM_CONTROL	0x88
#define	PCI_BAR1_CONTROL	0x8c

#define	PCI_BAR0_SPROM_OFFSET	4096	/* top 4K of bar0 accesses external sprom */

/* PCI clock must be active and stable to read SPROM */
#ifdef MIPSEL
#define pci_host(sprom) ((sprom[0] == 1) && (sprom[2] == 0x4710) && (sprom[8] == 0xf))
#else
#define pci_host(sprom) ((sprom[1] == 1) && (sprom[3] == 0x4710) && (sprom[9] == 0xf))
#endif

DECLARE_QUEUE_TYPE(LM_RX_PACKET_Q, MAX_RX_PACKET_DESC_COUNT);
DECLARE_QUEUE_TYPE(LM_TX_PACKET_Q, MAX_TX_PACKET_DESC_COUNT);

typedef struct _LM_PACKET {
    /* Set in LM. */
    LM_STATUS PacketStatus;

    /* Set in LM for Rx, in UM for Tx. */
    LM_UINT32 PacketSize;

    union {
        /* Send info. */
        struct {
            /* Set up by UM. */
            LM_UINT32 FragCount;

        } Tx;

        /* Receive info. */
        struct {
            /* Receive buffer size */
            LM_UINT32 RxBufferSize;

            /* Virtual and physical address of the receive buffer. */
	    LM_UINT8 *pRxBufferVirt;
	    LM_PHYSICAL_ADDRESS RxBufferPhy;
            
        } Rx;
    } u;
} LM_PACKET;

#ifdef BCM_WOL

#define BCMENET_PMPSIZE            0x80
#define BCMENET_PMMSIZE            0x10

#endif

typedef struct _LM_DEVICE_BLOCK 
{
    /* Memory view. */
    bcmenetregs_t *pMemView;
    LM_UINT8 *pMappedMemBase;

    PLM_VOID pPacketDescBase;

    LM_UINT32 RxPacketDescCnt;
    LM_UINT32 MaxRxPacketDescCnt;
    LM_UINT32 TxPacketDescCnt;
    LM_UINT32 MaxTxPacketDescCnt;

    LM_RX_PACKET_Q RxPacketFreeQ;
    LM_RX_PACKET_Q RxPacketReceivedQ;
    LM_TX_PACKET_Q TxPacketFreeQ;
    LM_TX_PACKET_Q TxPacketXmittedQ;

    LM_PACKET *RxPacketArr[DMAMAXRINGSZ / sizeof(dmadd_t)];
    LM_PACKET *TxPacketArr[DMAMAXRINGSZ / sizeof(dmadd_t)];

    MM_ATOMIC_T SendDescLeft;

    /* Current node address. */
    LM_UINT8 NodeAddress[6];

    /* The adapter's node address. */
    LM_UINT8 PermanentNodeAddress[6];

    /* Multicast address list. */
    LM_UINT32 McEntryCount;
    LM_UINT8 McTable[LM_MAX_MC_TABLE_SIZE][LM_MC_ENTRY_SIZE];

    LM_UINT16 PciVendorId;
    LM_UINT16 PciDeviceId;
    LM_UINT16 PciSubvendorId;
    LM_UINT16 PciSubsystemId;
    LM_UINT8  PciRevId;
    LM_UINT8  Reserved1[3];

    LM_UINT32 corerev;
    LM_UINT32 coreunit;
    LM_UINT32 pcirev;

    LM_UINT32 PhyAddr;
    LM_UINT32 MdcPort;

    dmadd_t *pRxDesc;
    LM_PHYSICAL_ADDRESS RxDescPhy;

    dmadd_t *pTxDesc;
    LM_PHYSICAL_ADDRESS TxDescPhy;

    LM_UINT32 ddoffset;
    LM_UINT32 dataoffset;
    LM_UINT32 rxoffset;

    LM_UINT32 rxin;
    LM_UINT32 rxout;

    LM_UINT32 txin;
    LM_UINT32 txout;

    LM_UINT32 lazyrxfc;
    LM_UINT32 lazyrxmult;
    LM_UINT32 lazytxfc;
    LM_UINT32 lazytxmult;

    struct sbmap *sbmap;

    LM_LINE_SPEED RequestedLineSpeed;
    LM_DUPLEX_MODE RequestedDuplexMode;

    LM_LINE_SPEED LineSpeed;
    LM_DUPLEX_MODE DuplexMode;

    LM_FLOW_CONTROL FlowControlCap;
    LM_FLOW_CONTROL FlowControl;

    LM_UINT32 Advertising;

    LM_UINT32 DisableAutoNeg;

    LM_STATUS LinkStatus;

    LM_UINT32 ReceiveMask;

    LM_BOOL InitDone;

    LM_BOOL InReset;

    LM_BOOL mibgood;

    LM_BOOL ShuttingDown;

    LM_BOOL QueueRxPackets;

    LM_UINT32 intstatus;
    LM_UINT32 intmask;

    LM_COUNTER tx_good_octets;
    LM_COUNTER tx_good_pkts;
    LM_COUNTER tx_octets;
    LM_COUNTER tx_pkts;
    LM_COUNTER tx_broadcast_pkts;
    LM_COUNTER tx_multicast_pkts;
    LM_COUNTER tx_len_64;
    LM_COUNTER tx_len_65_to_127;
    LM_COUNTER tx_len_128_to_255;
    LM_COUNTER tx_len_256_to_511;
    LM_COUNTER tx_len_512_to_1023;
    LM_COUNTER tx_len_1024_to_max;
    LM_COUNTER tx_jabber_pkts;
    LM_COUNTER tx_oversize_pkts;
    LM_COUNTER tx_fragment_pkts;
    LM_COUNTER tx_underruns;
    LM_COUNTER tx_total_cols;
    LM_COUNTER tx_single_cols;
    LM_COUNTER tx_multiple_cols;
    LM_COUNTER tx_excessive_cols;
    LM_COUNTER tx_late_cols;
    LM_COUNTER tx_defered;
    LM_COUNTER tx_carrier_lost;
    LM_COUNTER tx_pause_pkts;

    LM_COUNTER rx_good_octets;
    LM_COUNTER rx_good_pkts;
    LM_COUNTER rx_octets;
    LM_COUNTER rx_pkts;
    LM_COUNTER rx_broadcast_pkts;
    LM_COUNTER rx_multicast_pkts;
    LM_COUNTER rx_len_64;
    LM_COUNTER rx_len_65_to_127;
    LM_COUNTER rx_len_128_to_255;
    LM_COUNTER rx_len_256_to_511;
    LM_COUNTER rx_len_512_to_1023;
    LM_COUNTER rx_len_1024_to_max;
    LM_COUNTER rx_jabber_pkts;
    LM_COUNTER rx_oversize_pkts;
    LM_COUNTER rx_fragment_pkts;
    LM_COUNTER rx_missed_pkts;
    LM_COUNTER rx_crc_align_errs;
    LM_COUNTER rx_undersize;
    LM_COUNTER rx_crc_errs;
    LM_COUNTER rx_align_errs;
    LM_COUNTER rx_symbol_errs;
    LM_COUNTER rx_pause_pkts;
    LM_COUNTER rx_nonpause_pkts;

#ifdef BCM_WOL
    LM_WAKE_UP_MODE WakeUpMode;
#endif
#ifdef BCM_NAPI_RXPOLL
    LM_UINT32 RxPoll;
#endif
} LM_DEVICE_BLOCK;

/******************************************************************************/
/* NIC register read/write macros. */
/******************************************************************************/

#define REG_RD(pDevice, OffsetName)                                         \
    MM_MEMREADL(&((pDevice)->pMemView->OffsetName))

#define REG_WR(pDevice, OffsetName, Value32)                                \
    (void) MM_MEMWRITEL(&((pDevice)->pMemView->OffsetName), Value32)

#define REG_RD_OFFSET(pDevice, Offset)                                      \
    MM_MEMREADL(((LM_UINT8 *) (pDevice)->pMemView + Offset))

#define REG_WR_OFFSET(pDevice, Offset, Value32)                             \
    MM_MEMWRITEL(((LM_UINT8 *) (pDevice)->pMemView + Offset), Value32)

#define REG_OR(pDevice, OffsetName, Value32)                                \
    REG_WR(pDevice, OffsetName, REG_RD(pDevice, OffsetName) | Value32)

#define REG_AND(pDevice, OffsetName, Value32)                                \
    REG_WR(pDevice, OffsetName, REG_RD(pDevice, OffsetName) & Value32)

#define SPINWAIT(exp, us) { \
	LM_UINT32 countdown = (us) + 9; \
	while ((exp) && (countdown >= 10)) {\
		b44_MM_Wait(10); \
		countdown -= 10; \
	} \
}

#endif /* B44_H */

