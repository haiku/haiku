/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <arch/sh4/vcpu.h>
#include <arch/sh4/vcpu_struct.h>
#include <boot/stage2.h>
#include "serial.h"
#include <string.h>
#include <arch/cpu.h>
#include <arch/sh4/sh4.h>

#define SAVE_CPU_STATE 0

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

#define CHATTY_TLB 0

extern vcpu_struct kernel_struct;
#if SAVE_CPU_STATE
extern unsigned int last_ex_cpu_state[];
#endif
unsigned int next_utlb_ent = 0;

unsigned int vector_base();
unsigned int boot_stack[256] = { 0, };

void vcpu_clear_all_itlb_entries();
void vcpu_clear_all_utlb_entries();
void vcpu_dump_utlb_entry(int ent);
void vcpu_dump_all_itlb_entries();
void vcpu_dump_all_utlb_entries();

unsigned int get_sr();
void set_sr(unsigned int sr);
unsigned int get_vbr();
void set_vbr(unsigned int vbr);
unsigned int get_sgr();
unsigned int get_ssr();
unsigned int get_spc();
asm(
".globl _get_sr,_set_sr\n"
".globl _get_vbr,_set_vbr\n"
".globl _get_sgr\n"

"_get_sr:\n"
"        stc     sr,r0\n"
"        rts\n"
"        nop\n"

"_set_sr:\n"
"        ldc     r4,sr\n"
"        rts\n"
"        nop\n"

"_get_vbr:\n"
"        stc     vbr,r0\n"
"        rts\n"
"        nop\n"

"_set_vbr:\n"
"        ldc     r4,vbr\n"
"        rts\n"
"        nop\n"

"_get_sgr:\n"
"	stc	sgr,r0\n"
"	rts\n"
"	nop\n"

"_get_ssr:\n"
"	stc	ssr,r0\n"
"	rts\n"
"	nop\n"

"_get_spc:\n"
"	stc spc,r0\n"
"	rts\n"
"	nop\n"
);

#if SAVE_CPU_STATE
static void dump_cpu_state()
{
	int i;
	unsigned int *stack;
	unsigned int *stack_top;

	dprintf("dump_cpu_state entry\n");

	dprintf("registers:\n");
	for(i=0; i<16; i++) {
		dprintf("r%d 0x%x\n", i, last_ex_cpu_state[i]);
	}
	dprintf("gbr 0x%x\n", last_ex_cpu_state[16]);
	dprintf("mach 0x%x\n", last_ex_cpu_state[17]);
	dprintf("macl 0x%x\n", last_ex_cpu_state[18]);
	dprintf("pr 0x%x\n", last_ex_cpu_state[19]);
	dprintf("spc 0x%x\n", last_ex_cpu_state[20]);
	dprintf("sgr 0x%x\n", last_ex_cpu_state[21]);
	dprintf("ssr 0x%x\n", last_ex_cpu_state[22]);
	dprintf("fpul 0x%x\n", last_ex_cpu_state[23]);
	dprintf("fpscr 0x%x\n", last_ex_cpu_state[24]);

	stack = (unsigned int *)(get_sgr() - 0x200);
	stack_top = (unsigned int *)ROUNDUP((unsigned int)stack, PAGE_SIZE);
	dprintf("stack at 0x%x to 0x%x\n", stack, stack_top);
	// dump the stack
	for(;stack < stack_top; stack++)
		dprintf("0x%x\n", *stack);

	dprintf("utlb entries:\n");
	vcpu_dump_all_utlb_entries();
	dprintf("itlb entries:\n");
	vcpu_dump_all_itlb_entries();


	for(;;);
}
#endif
int reentrant_fault()
{
	dprintf("bad reentrancy fault\n");
	dprintf("spinning forever\n");
	for(;;);
	return 0;
}

static int default_vector(void *_frame)
{
	struct iframe *frame = (struct iframe *)_frame;

	dprintf("default_vector: ex_code 0x%x, pc 0x%x\n", frame->excode, frame->spc);
	dprintf("sgr = 0x%x\n", frame->sgr);
	dprintf("spinning forever\n");
	for(;;);
	return 0;
}

int vcpu_init(kernel_args *ka)
{
	int i;
	unsigned int sr;
	unsigned int vbr;

	dprintf("vcpu_init: entry\n");

	memset(&kernel_struct, 0, sizeof(kernel_struct));
	for(i=0; i<256; i++) {
		kernel_struct.vt[i].func = &default_vector;
	}
	kernel_struct.kstack = (unsigned int *)((int)boot_stack + sizeof(boot_stack) - 4);

	// set the vbr
	vbr = (unsigned int)&vector_base;
	set_vbr(vbr);
	dprintf("vbr = 0x%x\n", get_vbr());

	// disable exceptions
	sr = get_sr();
	sr |= 0x10000000;
	set_sr(sr);

	if((sr & 0x20000000) != 0) {
		// we're using register bank 1 now
		dprintf("using bank 1, switching register banks\n");
		// this switches in the bottom 8 registers.
		// dont have to do anything more, since the bottom 8 are
		// not saved in the call.
		set_sr(sr & 0xdfffffff);
	}

	// enable exceptions
	sr = get_sr();
	sr &= 0xefffffff;
	set_sr(sr);

	ka->arch_args.vcpu = &kernel_struct;

	// enable the mmu
	vcpu_clear_all_itlb_entries();
	vcpu_clear_all_utlb_entries();
	*(int *)PTEH = 0;
	*(int *)MMUCR = 0x00000105;

	return 0;
}

static struct ptent *get_ptent(struct pdent *pd, unsigned int fault_address)
{
	struct ptent *pt;

#if CHATTY_TLB
	dprintf("get_ptent: fault_address 0x%x\n", fault_address);
#endif

	if((unsigned int)pd < P1_PHYS_MEM_START || (unsigned int)pd >= P1_PHYS_MEM_END) {
#if CHATTY_TLB
		dprintf("get_ptent: bad pdent 0x%x\n", pd);
#endif
		return 0;
	}

	if(pd[fault_address >> 22].v == 0) {
		return 0;
	}
	pt = (struct ptent *)PHYS_ADDR_TO_P1(pd[fault_address >> 22].ppn << 12);
#if CHATTY_TLB
	dprintf("get_ptent: found ptent 0x%x\n", pt);
#endif

	return &pt[(fault_address >> 12) & 0x000003ff];
}

static void tlb_map(unsigned int vpn, struct ptent *ptent, unsigned int tlb_ent, unsigned int asid)
{
	union {
		struct utlb_data data;
		unsigned int n[3];
	} u;

	ptent->tlb_ent = tlb_ent;

	u.n[0] = 0;
	u.data.a.asid = asid;
	u.data.a.vpn = vpn << 2;
	u.data.a.dirty = ptent->d;
	u.data.a.valid = 1;

	u.n[1] = 0;
	u.data.da1.ppn = ptent->ppn << 2;
	u.data.da1.valid = 1;
	u.data.da1.psize1 = (ptent->sz & 0x2) ? 1 : 0;
	u.data.da1.prot_key = ptent->pr;
	u.data.da1.psize0 = ptent->sz & 0x1;
	u.data.da1.cacheability = ptent->c;
	u.data.da1.dirty = ptent->d;
	u.data.da1.sh = ptent->sh;
	u.data.da1.wt = ptent->wt;

	u.n[2] = 0;

	*((unsigned int *)(UTLB | (next_utlb_ent << UTLB_ADDR_SHIFT))) = u.n[0];
	*((unsigned int *)(UTLB1 | (next_utlb_ent << UTLB_ADDR_SHIFT))) = u.n[1];
	*((unsigned int *)(UTLB2 | (next_utlb_ent << UTLB_ADDR_SHIFT))) = u.n[2];
}

unsigned int tlb_miss(unsigned int excode, unsigned int pc)
{
	struct pdent *pd;
	struct ptent *ent;
	unsigned int fault_addr = *(unsigned int *)TEA;
	unsigned int shifted_fault_addr;
	unsigned int asid;

#if CHATTY_TLB
	dprintf("tlb_miss: excode 0x%x, pc 0x%x, sgr 0x%x, fault_address 0x%x\n", excode, pc, get_sgr(), fault_addr);
#endif

//	if(fault_addr == 0 || pc == 0 || get_sgr() == 0)
//		dump_cpu_state();

	if(fault_addr >= P1_AREA) {
		pd = (struct pdent *)kernel_struct.kernel_pgdir;
		asid = kernel_struct.kernel_asid;
		shifted_fault_addr = fault_addr & 0x7fffffff;
	} else {
		pd = (struct pdent *)kernel_struct.user_pgdir;
		asid = kernel_struct.user_asid;
		shifted_fault_addr = fault_addr;
	}

	ent = get_ptent(pd, shifted_fault_addr);
	if(ent == NULL || ent->v == 0) {
		if(excode == 0x2)
			return EXCEPTION_PAGE_FAULT_READ;
		else
			return EXCEPTION_PAGE_FAULT_WRITE;
	}

#if CHATTY_TLB
	dprintf("found entry. vaddr 0x%x maps to paddr 0x%x\n",
		fault_addr, ent->ppn << 12);
#endif


	if(excode == 0x3) {
		// this is a tlb miss because of a write, so
		// go ahead and mark it dirty
		ent->d = 1;
	}

#if 0
	// XXX hack!
	if(fault_addr == 0x7ffffff8) {
		dprintf("sr = 0x%x\n", get_sr());
		dprintf("ssr = 0x%x sgr = 0x%x spc = 0x%x\n", get_ssr(), get_sgr(), get_spc());
		dprintf("kernel_struct: kpgdir 0x%x upgdir 0x%x kasid 0x%x uasid 0x%x kstack 0x%x\n",
			kernel_struct.kernel_pgdir, kernel_struct.user_pgdir, kernel_struct.kernel_asid,
			kernel_struct.user_asid, kernel_struct.kstack);
	}
#endif
#if 0
	{
		static int clear_all = 0;
		if(fault_addr == 0x7ffffff8)
			clear_all = 1;
		if(clear_all)
			vcpu_clear_all_utlb_entries();
	}
#endif

	tlb_map(fault_addr >> 12, ent, next_utlb_ent, asid);
#if CHATTY_TLB
	vcpu_dump_utlb_entry(next_utlb_ent);
#endif
	next_utlb_ent++;
	if(next_utlb_ent >= UTLB_COUNT)
		next_utlb_ent = 0;

#if CHATTY_TLB
	dprintf("tlb_miss exit\n");
#endif
	return excode;
}

unsigned int tlb_initial_page_write(unsigned int excode, unsigned int pc)
{
	struct pdent *pd;
	struct ptent *ent;
	unsigned int fault_addr = *(unsigned int *)TEA;
	unsigned int shifted_fault_addr;
	unsigned int asid;

#if CHATTY_TLB
	dprintf("tlb_initial_page_write: excode 0x%x, pc 0x%x, fault_address 0x%x\n",
		excode, pc, fault_addr);
#endif

	if(fault_addr >= P1_AREA) {
		pd = (struct pdent *)kernel_struct.kernel_pgdir;
		asid = kernel_struct.kernel_asid;
		shifted_fault_addr = fault_addr & 0x7fffffff;
	} else {
		pd = (struct pdent *)kernel_struct.user_pgdir;
		asid = kernel_struct.user_asid;
		shifted_fault_addr = fault_addr;
	}

	ent = get_ptent(pd, shifted_fault_addr);
	if(ent == NULL || ent->v == 0) {
		// if we're here, the page table is
		// out of sync with the tlb cache.
		// time to die.
		dprintf("tlb_ipw exception called but no page table ent exists!\n");
		for(;;);
	}

	{
		struct utlb_addr_array   *a;
		struct utlb_data_array_1 *da1;

		a = (struct utlb_addr_array *)(UTLB | (ent->tlb_ent << UTLB_ADDR_SHIFT));
		da1 = (struct utlb_data_array_1 *)(UTLB1 | (ent->tlb_ent << UTLB_ADDR_SHIFT));

		// inspect this tlb entry to make sure it's the right one
		if(asid != a->asid || (ent->ppn << 2) != da1->ppn || ((fault_addr >> 12) << 2) != a->vpn) {
			dprintf("tlb_ipw exception found that the page table out of sync with tlb\n");
			dprintf("page_table entry: 0x%x\n", *(unsigned int *)ent);
			vcpu_dump_utlb_entry(ent->tlb_ent);
			for(;;);
		}
	 	a->dirty = 1;
		ent->d = 1;
	}

	return excode;
}

void vcpu_dump_itlb_entry(int ent)
{
	struct itlb_data data;

	*(int *)&data.a = *((int *)(ITLB | (ent << ITLB_ADDR_SHIFT)));
	*(int *)&data.da1 = *((int *)(ITLB1 | (ent << ITLB_ADDR_SHIFT)));
	*(int *)&data.da2 = *((int *)(ITLB2 | (ent << ITLB_ADDR_SHIFT)));

	dprintf("itlb[%d] = \n", ent);
	dprintf(" asid = %d\n", data.a.asid);
	dprintf(" valid = %d\n", data.a.valid);
	dprintf(" vpn = 0x%x\n", data.a.vpn << 10);
	dprintf(" ppn = 0x%x\n", data.da1.ppn << 10);
}

void vcpu_clear_all_itlb_entries()
{
	int i;
	for(i=0; i<4; i++) {
		*((int *)(ITLB | (i << ITLB_ADDR_SHIFT))) = 0;
		*((int *)(ITLB1 | (i << ITLB_ADDR_SHIFT))) = 0;
		*((int *)(ITLB2 | (i << ITLB_ADDR_SHIFT))) = 0;
	}
}

void vcpu_dump_all_itlb_entries()
{
	int i;

	for(i=0; i<4; i++) {
		vcpu_dump_itlb_entry(i);
	}
}

void vcpu_dump_utlb_entry(int ent)
{
	struct utlb_data data;

	*(int *)&data.a = *((int *)(UTLB | (ent << UTLB_ADDR_SHIFT)));
	*(int *)&data.da1 = *((int *)(UTLB1 | (ent << UTLB_ADDR_SHIFT)));
	*(int *)&data.da2 = *((int *)(UTLB2 | (ent << UTLB_ADDR_SHIFT)));

	dprintf("utlb[%d] = \n", ent);
	dprintf(" asid = %d\n", data.a.asid);
	dprintf(" valid = %d\n", data.a.valid);
	dprintf(" dirty = %d\n", data.a.dirty);
	dprintf(" vpn = 0x%x\n", data.a.vpn << 10);
	dprintf(" ppn = 0x%x\n", data.da1.ppn << 10);
}

void vcpu_clear_all_utlb_entries()
{
	int i;
	for(i=0; i<64; i++) {
		*((int *)(UTLB | (i << UTLB_ADDR_SHIFT))) = 0;
		*((int *)(UTLB1 | (i << UTLB_ADDR_SHIFT))) = 0;
		*((int *)(UTLB2 | (i << UTLB_ADDR_SHIFT))) = 0;
	}
}

void vcpu_dump_all_utlb_entries()
{
	int i;

	for(i=0; i<64; i++) {
		vcpu_dump_utlb_entry(i);
	}
}

