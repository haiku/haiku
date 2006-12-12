/*
 * Copyright 2006, Hideyuki Abe. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VLANCE_H
#define VLANCE_H


#include <SupportDefs.h>


/* PCnet/PCI definitions */

/* I/O mapping */
#define	PCNET_APROM_OFFSET	(0x00UL)
#define	PCNET_RDP_OFFSET	(0x10UL)
#define	PCNET_RAP_OFFSET	(0x14UL)
#define	PCNET_RST_OFFSET	(0x18UL)
#define	PCNET_BDP_OFFSET	(0x1cUL)

/* CSR register numbers */
#define	PCNET_CSR_STATUS	(  0)
#define	PCNET_CSR_IADDR0	(  1)
#define	PCNET_CSR_IADDR1	(  2)
#define	PCNET_CSR_IMSK		(  3)
#define	PCNET_CSR_FTR		(  4)
#define	PCNET_CSR_EXTCNT	(  5)
#define	PCNET_CSR_LADRF0	(  8)
#define	PCNET_CSR_LADRF1	(  9)
#define	PCNET_CSR_LADRF2	( 10)
#define	PCNET_CSR_LADRF3	( 11)
#define	PCNET_CSR_PADR0		( 12)
#define	PCNET_CSR_PADR1		( 13)
#define	PCNET_CSR_PADR2		( 14)
#define	PCNET_CSR_MODE		( 15)
#define	PCNET_CSR_BADRL		( 24)
#define	PCNET_CSR_BADRH		( 25)
#define	PCNET_CSR_BADXL		( 30)
#define	PCNET_CSR_BADXH		( 31)
#define	PCNET_CSR_POLLINT	( 47)
#define	PCNET_CSR_RCVRL		( 76)
#define	PCNET_CSR_XMTRL		( 78)
#define	PCNET_CSR_DMABAT	( 82)
#define	PCNET_CSR_MERRTO	(100)
#define	PCNET_CSR_MFC		(112)
#define	PCNET_CSR_RCC		(114)
#define	PCNET_CSR_RCVALGN	(122)

/* BCR register numbers */
#define	PCNET_BCR_MC		(  2)
#define	PCNET_BCR_LNKST		(  4)
#define	PCNET_BCR_LED1		(  5)
#define	PCNET_BCR_LED2		(  6)
#define	PCNET_BCR_LED3		(  7)
#define	PCNET_BCR_FDC		(  9)
#define	PCNET_BCR_BSBC		( 18)
#define	PCNET_BCR_SWS		( 20)

/* Receive Descriptor (Little-Endian ordering) */
typedef union recv_desc {
	uint32 rmd[4];
	struct {
		uint32 rbadr;
		int16 bcnt;
		uint16 status;
		uint32 mcnt;
		uint32 rsvd;
	} s;
} recv_desc_t;

/* Transmit Descriptor (Little-Endian ordering) */
typedef union trns_desc {
	uint32 tmd[4];
	struct {
		uint32 tbadr;
		int16 bcnt;
		uint16 status;
		uint32 misc;
		uint32 rsvd;
	} s;
} trns_desc_t;

/* Initialization Block (Little-Endian ordering) */
typedef union init_block {
	uint32 iblk[7];
	struct {
		uint32 mode;
		uint8 padr[8];
		uint32 ladr[2];
		uint32 rdra;
		uint32 tdra;
	} s;
} init_block_t;

#endif	/* VLANCE_H */
