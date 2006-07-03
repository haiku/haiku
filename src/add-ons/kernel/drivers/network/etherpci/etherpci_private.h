/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/*
 * etherpci_private.h
 * Copyright (c) 1998 Be, Inc.	All Rights Reserved 
 *
 * Definitions private to ne2000 PCI ethernet driver
 *
 * Modification History (most recent first):
 *
 * 18 May 98	malyn	new today
 */

#ifndef _ETHERPCI_PRIVATE_H
#define _ETHERPCI_PRIVATE_H

#ifndef _KERNEL_EXPORT_H
#include <KernelExport.h>
#endif

/*
 * Wait this long before giving up on an ethernet transmit
 */
#define ETHER_TRANSMIT_TIMEOUT ((bigtime_t)1000000)	/* one second */

#define NIRQS 128	//x hmm...

#define MAX_MULTI 14

#define ETHER_BUF_START_NE2000	0x4000
#define ETHER_BUF_SIZE_NE2000	0x4000

#define ETHER_BUF_SIZE_MAX		ETHER_BUF_SIZE_NE2000

#define ETHER_MTU 1500
#define ETHER_MIN_SIZE 60
#define ETHER_MAX_SIZE 1514
#define ETHER_IRQ2 0x10
#define ETHER_IRQ3 0x20
#define ETHER_IRQ4 0x40
#define ETHER_IRQ5 0x80


#define ECNTRL_RESET    0x01         
#define ECNTRL_ONBOARD	0x02         
#define ECNTRL_SAPROM   0x04         

#define EGACFR_NORM		0x49
#define EGACFR_IRQOFF	0xc9

#define EC_PAGE_SIZE 256
#define EC_PAGE_SHIFT 8

#define EN_CCMD	   0x0

#define EN0_STARTPG	0x01	
#define EN0_STOPPG	0x02	
#define EN0_BOUNDARY 0x03
#define EN0_TPSR	0x04	
#define EN0_TCNTLO	0x05
#define EN0_TCNTHI	0x06
#define EN0_ISR		0x07	
#define EN0_RADDRLO	0x08
#define EN0_RADDRHI	0x09
#define EN0_RCNTLO	0x0a
#define EN0_RCNTHI	0x0b

#define NE_DATA		0x10
#define NE_RESET	0x1f

#define ISR_RECEIVE 0x01
#define ISR_TRANSMIT 0x02
#define ISR_RECEIVE_ERROR 0x04
#define ISR_TRANSMIT_ERROR 0x08
#define ISR_OVERWRITE   0x10
#define ISR_COUNTER	0x20
#define ISR_DMADONE 0x40
#define ISR_RESET   0x80

#define EN0_RXCR	0xc	
#define EN0_TXCR	0xd	
#define EN0_DCFG	0xe
#define EN0_IMR		0xf	

#define EN0_CNTR0	0x0d
#define EN0_CNTR1	0x0e
#define EN0_CNTR2	0x0f

#define DCFG_BM8	0x48
#define DCFG_BM16	0x49

#define EN1_PHYS	0x1
#define EN1_CURPAG	0x7	
#define EN1_MULT	0x8

#define ENRXCR_MON	0x20	
#define ENRXCR_MCST	0x08
#define ENRXCR_BCST	0x04

#define TXCR_LOOPBACK	0x02

#define ENC_PAGE0	0x00
#define ENC_STOP	0x01
#define ENC_START   0x02
#define ENC_TRANS	0x04
#define ENC_DMAREAD 0x08
#define ENC_DMAWRITE 0x10
#define ENC_NODMA	0x20
#define ENC_PAGE1	0x40


#define RSR_INTACT	0x01

#define TSR_ABORTED		0x08
#define TSR_UNDERRUN	0x20
#define TSR_HEARTBEAT	0x40

#define ETHER_ADDR_LEN 6

/*
 * Maximum iterations to poll before assuming error
 */
#define MAXWAIT 10000


/*
 * Swap the bytes in a short, but not on a little-endian machine
 */
static const union { long l; char b[4]; } ENDIAN_TEST = { 1 };
#define LITTLE_ENDIAN ENDIAN_TEST.b[0]
#define SWAPSHORT(x) (((x & 0xff) << 8) | ((x >> 8) & 0xff))
#define swapshort(x) (LITTLE_ENDIAN ? (x) : SWAPSHORT(x))


/*
 * NS8390 ring header structure
 */
typedef struct ring_header	{
	unsigned char status;
	unsigned char next_packet;
	unsigned short count;
} ring_header;

#endif
