/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>

#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch/sh4/vcpu.h>
#include <arch/sh4/sh4.h>
#include "serial.h"
#include "mmu.h"

#define KERNEL_LOAD_ADDR 0xc0000000

struct pdent *pd = 0;

void mmu_map_page(unsigned int vaddr, unsigned int paddr)
{
	struct ptent *pt;
	int index;

	vaddr &= 0xfffff000;
	paddr &= 0xfffff000;
	dprintf("mmu_map_page: mapping 0x%x to 0x%x\n", paddr, vaddr);

	if(vaddr < P1_AREA) {
		dprintf("mmu_map_page: cannot map user space now!\n");
		for(;;);
	}

	vaddr &= 0x7fffffff;

	index = vaddr >> 22;
	if(pd[index].v == 0) {
		dprintf("mmu_map_page: no page dir exists for this address\n");
		for(;;);
	}

	pt = (struct ptent *)PHYS_ADDR_TO_P1(pd[index].ppn << 12);

	index = (vaddr >> 12) & 0x00000fff;
	pt[index].wt = 0;
	pt[index].pr = 1;	// rw, supervisor only
	pt[index].ppn = paddr >> 12;
	pt[index].tlb_ent = 0;
	pt[index].c = 1;
	pt[index].sz = 1;	// 4k page
	pt[index].sh = 0;
	pt[index].d = 0;
	pt[index].v = 1;
}

void mmu_init(kernel_args *ka, unsigned int *next_paddr)
{
	struct ptent *pt;
	int index;

	dprintf("mmu_init: entry\n");

	// allocate a kernel pgdir
	ka->arch_args.vcpu->kernel_pgdir = (unsigned int *)PHYS_ADDR_TO_P1(*next_paddr);
	(*next_paddr) += PAGE_SIZE;

	pd = (struct pdent *)ka->arch_args.vcpu->kernel_pgdir;
	memset(pd, 0, sizeof(struct pdent) * 512);

	dprintf("kernel_pgdir = 0x%x\n", ka->arch_args.vcpu->kernel_pgdir);

	// allocate an initial page table
	pt = (struct ptent *)PHYS_ADDR_TO_P1(*next_paddr);
	memset(pt, 0, sizeof(struct ptent) * 1024);
	(*next_paddr) += PAGE_SIZE;

	dprintf("intial page table = 0x%x\n", pt);

	index = (KERNEL_LOAD_ADDR & 0x7fffffff) >> 22;
	pd[index].ppn = (unsigned int)pt >> 12;
	pd[index].v = 1;
}

