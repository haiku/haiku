/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _SH4_H
#define _SH4_H

#define P4_AREA 0xe0000000
#define P4_AREA_LEN 0x20000000
#define P3_AREA 0xc0000000
#define P3_AREA_LEN (P4_AREA - P3_AREA)
#define P2_AREA 0xa0000000
#define P2_AREA_LEN (P3_AREA - P2_AREA)
#define P1_AREA 0x80000000
#define P1_AREA_LEN (P2_AREA - P1_AREA)
#define P0_AREA 0
#define P0_AREA_LEN (P1_AREA - P0_AREA)
#define U0_AREA P0_AREA
#define U0_AREA_LEN P0_AREA_LEN

#define PHYS_MEM_START    0x0c000000
#define PHYS_MEM_LEN      0x01000000
#define PHYS_MEM_END      (PHYS_MEM_START + PHYS_MEM_LEN)
#define P1_PHYS_MEM_START (P1_AREA + PHYS_MEM_START)
#define P1_PHYS_MEM_LEN   PHYS_MEM_LEN
#define P1_PHYS_MEM_END   (P1_PHYS_MEM_START + P1_PHYS_MEM_LEN)

#define PHYS_ADDR_TO_P1(x) (((unsigned int)(x)) + P1_AREA)
#define PHYS_ADDR_TO_P2(x) (((unsigned int)(x)) + P2_AREA)
#define P1_TO_PHYS_ADDR(x) (((unsigned int)(x)) - P1_AREA)

#define PHYS_ADDR_SIZE     (0x1fffffff+1)

//#define PAGE_SIZE	4096
#define PAGE_SIZE_1K	0
#define PAGE_SIZE_4K	1
#define PAGE_SIZE_64K	2
#define PAGE_SIZE_1M	3

#define PTEH 	0xff000000
#define PTEL 	0xff000004
#define PTEA	0xff000034
#define TTB  	0xff000008
#define TEA  	0xff00000c
#define MMUCR 	0xff000010

#define UTLB    0xf6000000
#define UTLB1   0xf7000000
#define UTLB2   0xf3800000
#define UTLB_ADDR_SHIFT 0x8
#define UTLB_COUNT 64

struct utlb_addr_array {
	unsigned int asid: 8;
	unsigned int valid: 1;
	unsigned int dirty: 1;
	unsigned int vpn: 22;
};

struct utlb_data_array_1 {
	unsigned int wt: 1;
	unsigned int sh: 1;
	unsigned int dirty: 1;
	unsigned int cacheability: 1;
	unsigned int psize0: 1;
	unsigned int prot_key: 2;
	unsigned int psize1: 1;
	unsigned int valid: 1;
	unsigned int unused: 1;
	unsigned int ppn: 19;
	unsigned int unused2: 3;
};

struct utlb_data_array_2 {
	unsigned int sa: 2;
	unsigned int tc: 1;
	unsigned int unused: 29;
};

struct utlb_data {
	struct utlb_addr_array a;
	struct utlb_data_array_1 da1;
	struct utlb_data_array_2 da2;
};

#define	ITLB	0xf2000000
#define	ITLB1	0xf3000000
#define ITLB2	0xf3800000
#define ITLB_ADDR_SHIFT 0x8
#define ITLB_COUNT 4

struct itlb_addr_array {
	unsigned int asid: 8;
	unsigned int valid: 1;
	unsigned int unused: 1;
	unsigned int vpn: 22;
};

struct itlb_data_array_1 {
	unsigned int unused1: 1;
	unsigned int sh: 1;
	unsigned int unused2: 1;
	unsigned int cacheability: 1;
	unsigned int psize0: 1;
	unsigned int unused3: 1;
	unsigned int prot_key: 1;
	unsigned int psize1: 1;
	unsigned int valid: 1;
	unsigned int unused4: 1;
	unsigned int ppn: 19;
	unsigned int unused5: 3;
};

struct itlb_data_array_2 {
	unsigned int sa: 2;
	unsigned int tc: 1;
	unsigned int unused: 29;
};

struct itlb_data {
	struct itlb_addr_array a;
	struct itlb_data_array_1 da1;
	struct itlb_data_array_2 da2;
};

// timer stuff
#define	TOCR	0xffd80000
#define TSTR	0xffd80004
#define	TCOR0	0xffd80008
#define TCNT0	0xffd8000c
#define TCR0	0xffd80010
#define TCOR1	0xffd80014
#define TCNT1	0xffd80018
#define TCR1	0xffd8001c
#define TCOR2	0xffd80020
#define TCNT2	0xffd80024
#define TCR2	0xffd80028
#define TCPR2	0xffd8002c

// interrupt controller stuff
#define ICR		0xffd00000
#define IPRA	0xffd00004
#define IPRB	0xffd00008
#define IPRC	0xffd0000c

// cache stuff
#define CCR		0xff00001c
#define QACR0	0xff000038
#define QACR1	0xff00003c

#endif

