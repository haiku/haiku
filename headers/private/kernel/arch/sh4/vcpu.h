/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _VCPU_H
#define _VCPU_H

#include <boot/stage2.h>

// layout of the iframe is defined in boot/sh4/vcpu.S
struct iframe {
	unsigned int page_fault_addr;
	unsigned int excode;
	unsigned int spc;
	unsigned int ssr;
	unsigned int sgr;
	unsigned int pr;
	unsigned int macl;
	unsigned int mach;
	unsigned int gbr;
	unsigned int fpscr;
	unsigned int fpul;

	float	fr15_1;
	float	fr14_1;
	float	fr13_1;
	float	fr12_1;
	float	fr11_1;
	float	fr10_1;
	float	fr9_1;
	float	fr8_1;
	float	fr7_1;
	float	fr6_1;
	float	fr5_1;
	float	fr4_1;
	float	fr3_1;
	float	fr2_1;
	float	fr1_1;
	float	fr0_1;

	float	fr15_0;
	float	fr14_0;
	float	fr13_0;
	float	fr12_0;
	float	fr11_0;
	float	fr10_0;
	float	fr9_0;
	float	fr8_0;
	float	fr7_0;
	float	fr6_0;
	float	fr5_0;
	float	fr4_0;
	float	fr3_0;
	float	fr2_0;
	float	fr1_0;
	float	fr0_0;

	unsigned int r7;	
	unsigned int r6;	
	unsigned int r5;	
	unsigned int r4;	
	unsigned int r3;	
	unsigned int r2;	
	unsigned int r1;	
	unsigned int r0;	
	unsigned int r14;	
	unsigned int r13;	
	unsigned int r12;	
	unsigned int r11;	
	unsigned int r10;	
	unsigned int r9;	
	unsigned int r8;	
};

// page table structures
struct pdent {
	unsigned int v:1;
	unsigned int unused0:11;
	unsigned int ppn:17;
	unsigned int unused1:3;
};

struct ptent {
	unsigned int v:1;
	unsigned int d:1;
	unsigned int sh:1;
	unsigned int sz:2;
	unsigned int c:1;
	unsigned int tlb_ent:6;
	unsigned int ppn:17;
	unsigned int pr:2;
	unsigned int wt:1;
};

// soft faults
#define EXCEPTION_PAGE_FAULT_READ 0xfe
#define EXCEPTION_PAGE_FAULT_WRITE 0xff

// can only be used in stage2
int vcpu_init(kernel_args *ka);

#endif

