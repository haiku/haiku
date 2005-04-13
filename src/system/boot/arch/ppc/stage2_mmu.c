/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include <kernel/kernel.h>
#include <arch/cpu.h>
#include <libc/string.h>
#include "stage2_priv.h"

static unsigned int primary_hash(unsigned int vsid, unsigned int vaddr);
static unsigned int secondary_hash(unsigned int primary_hash);

static struct ppc_pteg *ptable = 0;
static int ptable_size = 0;
static unsigned int ptable_hash_mask = 0;


static unsigned long total_ram_size = 0;

static void print_pte(struct ppc_pte *e);

static bool does_intersect(unsigned long base1, unsigned long len1, unsigned long base2, unsigned long len2)
{
	unsigned long end1 = base1 + len1;
	unsigned long end2 = base2 + len2;

	if(base2 >= base1 && base2 <= end1)
		return true; // base2 is inside first range
	if(end2 >= base1 && end2 <= end1)
		return true; // end of second range inside first range
	if(base1 >= base2 && base1 <= end2)
		return true; // base1 is inside second range
	if(end1 >= base2 && end1 <= end2)
		return true; // end of first range inside second range

	return false;
}

static void find_phys_memory_map(kernel_args *ka)
{
	int handle;
	unsigned int i;
	struct mem_region {
		unsigned long pa;
		int len;
	} mem_regions[33];
	unsigned int mem_regions_len = 0;

	// get the physical memory map of the system
	handle = of_finddevice("/memory");
	memset(mem_regions, 0, sizeof(mem_regions));
	mem_regions_len = of_getprop(handle, "reg", mem_regions, sizeof(mem_regions[0]) * 32);
	mem_regions_len /= sizeof(struct mem_region);

	// copy these regions over to the kernel args structure
	ka->num_phys_mem_ranges = 0;
	for(i=0; i<mem_regions_len; i++) {
		if(mem_regions[i].len > 0) {
			total_ram_size += mem_regions[i].len;
			if(i > 0) {
				if(mem_regions[i].pa == ka->phys_mem_range[ka->num_phys_mem_ranges-1].start + ka->phys_mem_range[ka->num_phys_mem_ranges-1].size) {
					// this range just extends the old one
					ka->phys_mem_range[ka->num_phys_mem_ranges-1].size += mem_regions[i].len;
					break;
				}
			}
			ka->phys_mem_range[ka->num_phys_mem_ranges].start = mem_regions[i].pa;
			ka->phys_mem_range[ka->num_phys_mem_ranges].size = mem_regions[i].len;
			ka->num_phys_mem_ranges++;
			if(ka->num_phys_mem_ranges == MAX_PHYS_MEM_ADDR_RANGE) {
				printf("too many physical memory maps, increase MAX_PHYS_MEM_ADDR_RANGE\n");
				for(;;);
			}
		}
	}
	for(i=0; i<ka->num_phys_mem_ranges; i++) {
		printf("phys map %d: pa 0x%lx, len 0x%lx\n", i, ka->phys_mem_range[i].start, ka->phys_mem_range[i].size);
	}
}

static bool is_in_phys_mem(kernel_args *ka, unsigned long addr)
{
	unsigned int i;

	for(i = 0; i < ka->num_phys_mem_ranges; i++) {
		if(does_intersect(ka->phys_mem_range[i].start, ka->phys_mem_range[i].size, addr, 0))
			return true;
	}
	return false;
}

static void mark_used_phys_mem_range(kernel_args *ka, unsigned long base, unsigned long len)
{
	unsigned int i;
	unsigned long start;

	base = ROUNDOWN(base, PAGE_SIZE);
	len = ROUNDUP(len, PAGE_SIZE);
	start = base;

	while(start < base + len){
		// cycle through the list of physical runs of used pages,
		// seeing if start will intersect one of them
		for(i = 0; i < ka->num_phys_alloc_ranges; i++) {
			if(start == ka->phys_alloc_range[i].start + ka->phys_alloc_range[i].size) {
				// it will extend it
				ka->phys_alloc_range[i].size += PAGE_SIZE;
				goto next_page;
			}
			if(start + PAGE_SIZE == ka->phys_alloc_range[i].start) {
				// it will prepend it
				ka->phys_alloc_range[i].start = start;
				ka->phys_alloc_range[i].size += PAGE_SIZE;
				goto next_page;
			}
			if(does_intersect(ka->phys_alloc_range[i].start, ka->phys_alloc_range[i].size, start, PAGE_SIZE)) {
				// it's off in the middle of this range, skip it
				goto next_page;
			}
		}

		// didn't find it in one of the existing ranges, must need to start a new one
		if(ka->num_phys_alloc_ranges >= MAX_PHYS_ALLOC_ADDR_RANGE) {
			printf("mark_used_phys_mem_range: MAX_PHYS_ALLOC_ADDR_RANGE (%d) too small\n", MAX_PHYS_ALLOC_ADDR_RANGE);
			for(;;);
		}

		// create a new allocated range
		ka->phys_alloc_range[ka->num_phys_alloc_ranges].start = start;
		ka->phys_alloc_range[ka->num_phys_alloc_ranges].size = PAGE_SIZE;
		ka->num_phys_alloc_ranges++;

next_page:
		start += PAGE_SIZE;
	}
}

static void find_used_phys_memory_map(kernel_args *ka)
{
	int handle;
	unsigned int i;
	struct translation_map {
		unsigned long va;
		int len;
		unsigned long pa;
		int mode;
	} memmap[64];
	unsigned int translation_map_len = 0;

	ka->num_phys_alloc_ranges = 0;

	// get the current translation map of the system,
	// to find how much memory was mapped to load the stage1 and bootdir
	handle = of_finddevice("/chosen");
	of_getprop(handle, "mmu", &handle, sizeof(handle));
	handle = of_instance_to_package(handle);
	memset(&memmap, 0, sizeof(memmap));
	translation_map_len = of_getprop(handle, "translations", memmap, sizeof(memmap));
	translation_map_len /= sizeof(struct translation_map);
	for(i=0; i<translation_map_len; i++) {
		if(is_in_phys_mem(ka, memmap[i].va)) {
			printf("package loaded at pa 0x%lx va 0x%lx, len 0x%x\n", memmap[i].pa, memmap[i].va, memmap[i].len);
			// we found the translation that covers the loaded package. Save this.
			mark_used_phys_mem_range(ka, memmap[i].pa, memmap[i].len);
		}
	}
	for(i=0; i<ka->num_phys_alloc_ranges; i++) {
		printf("phys alloc map %d: pa 0x%lx, len 0x%lx\n", i, ka->phys_alloc_range[i].start, ka->phys_alloc_range[i].size);
	}
}

static void mark_used_virt_mem_range(kernel_args *ka, unsigned long base, unsigned long len)
{
	unsigned int i;
	unsigned long start;

	base = ROUNDOWN(base, PAGE_SIZE);
	len = ROUNDUP(len, PAGE_SIZE);
	start = base;

	while(start < base + len) {
		// cycle through the list of virtual runs of used pages,
		// seeing if start will intersect one of them
		for(i = 0; i < ka->num_virt_alloc_ranges; i++) {
			if(start == ka->virt_alloc_range[i].start + ka->virt_alloc_range[i].size) {
				// it will extend it
				ka->virt_alloc_range[i].size += PAGE_SIZE;
				goto next_page;
			}
			if(start + PAGE_SIZE == ka->virt_alloc_range[i].start) {
				// it will prepend it
				ka->virt_alloc_range[i].start = start;
				ka->virt_alloc_range[i].size += PAGE_SIZE;
				goto next_page;
			}
			if(does_intersect(ka->virt_alloc_range[i].start, ka->virt_alloc_range[i].size, start, PAGE_SIZE)) {
				// it's off in the middle of this range, skip it
				goto next_page;
			}
		}

		// didn't find it in one of the existing ranges, must need to start a new one
		if(ka->num_virt_alloc_ranges >= MAX_VIRT_ALLOC_ADDR_RANGE) {
			printf("mark_used_virt_mem_range: MAX_VIRT_ALLOC_ADDR_RANGE (%d) too small\n", MAX_VIRT_ALLOC_ADDR_RANGE);
			for(;;);
		}

		// create a new allocated range
		ka->virt_alloc_range[ka->num_virt_alloc_ranges].start = start;
		ka->virt_alloc_range[ka->num_virt_alloc_ranges].size = PAGE_SIZE;
		ka->num_virt_alloc_ranges++;

next_page:
		start += PAGE_SIZE;
	}
}

unsigned long mmu_allocate_page(kernel_args *ka)
{
	unsigned long page;

	if(ka->num_phys_alloc_ranges == 0) {
		// no physical allocated ranges, start one
		page = ka->phys_mem_range[0].start;
		mark_used_phys_mem_range(ka, page, PAGE_SIZE);
		return page;
	}

	// allocate from the first allocated physical range
	page = ka->phys_alloc_range[0].start + ka->phys_alloc_range[0].size;
	ka->phys_alloc_range[0].size += PAGE_SIZE;

	// XXX check for validity better

	return page;
}


static void tlbia()
{
	unsigned long i;

	asm volatile("sync");
	for(i=0; i< 0x40000; i += 0x1000) {
		asm volatile("tlbie %0" :: "r" (i));
		asm volatile("eieio");
		asm volatile("sync");
	}
	asm volatile("tlbsync");
	asm volatile("sync");
}

#define CACHELINE 32

void syncicache(void *address, int len)
{
	int l, off;
	char *p;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;

	l = len;
	p = (char *)address - off;
	do {
		asm volatile ("dcbst 0,%0" :: "r"(p));
		p += CACHELINE;
	} while((l -= CACHELINE) > 0);
	asm volatile ("sync");
	p = (char *)address - off;
	do {
		asm volatile ("icbi 0,%0" :: "r"(p));
		p += CACHELINE;
	} while((len -= CACHELINE) > 0);
	asm volatile ("sync");
	asm volatile ("isync");
}

int s2_mmu_init(kernel_args *ka)
{
	unsigned int ibats[8];
	unsigned int dbats[8];
	unsigned long top_ram = 0;
	int i;

	ka->num_virt_alloc_ranges = 0;

	// figure out where physical memory is and what is being used
	find_phys_memory_map(ka);
	find_used_phys_memory_map(ka);


#if 0
	// find the largest address of physical memory, but with a max of 256 MB,
	// so it'll be within our 256 MB BAT window
	for(i=0; i<ka->num_phys_mem_ranges; i++) {
		if(ka->phys_mem_range[i].start + ka->phys_mem_range[i].size > top_ram) {
			if(ka->phys_mem_range[i].start + ka->phys_mem_range[i].size > 256*1024*1024) {
				if(ka->phys_mem_range[i].start < 256*1024*1024) {
					top_ram = 256*1024*1024;
					break;
				}
			}
			top_ram = ka->phys_mem_range[i].start + ka->phys_mem_range[i].size;
		}
	}
	printf("top of ram (but under 256MB) is 0x%x\n", top_ram);
#endif

	// figure the size of the new pagetable, as recommended by Motorola
	if(total_ram_size <= 8*1024*1024) {
		ptable_size = 64*1024;
	} else if(total_ram_size <= 16*1024*1024) {
		ptable_size = 128*1024;
	} else if(total_ram_size <= 32*1024*1024) {
		ptable_size = 256*1024;
	} else if(total_ram_size <= 64*1024*1024) {
		ptable_size = 512*1024;
	} else if(total_ram_size <= 128*1024*1024) {
		ptable_size = 1024*1024;
	} else if(total_ram_size <= 256*1024*1024) {
		ptable_size = 2*1024*1024;
	} else if(total_ram_size <= 512*1024*1024) {
		ptable_size = 4*1024*1024;
	} else if(total_ram_size <= 1024*1024*1024) {
		ptable_size = 8*1024*1024;
	} else if(total_ram_size <= 2*1024*1024*1024UL) {
		ptable_size = 16*1024*1024;
	} else {
		ptable_size = 32*1024*1024;
	}

	// figure out where to put the page table
	printf("allocating a page table using claim\n");
	ptable_hash_mask = (ptable_size >> 6) - 1;
	ptable = (struct ppc_pteg *)of_claim(0, ptable_size, ptable_size);
	printf("ptable at pa 0x%x, size 0x%x\n", ptable, ptable_size);
	printf("mask = 0x%x\n", ptable_hash_mask);

	// mark it used
	mark_used_phys_mem_range(ka, (unsigned long)ptable, ptable_size);

	// save it's new location in the kernel args
	ka->arch_args.page_table.start = (unsigned long)ptable;
	ka->arch_args.page_table.size = ptable_size;
	ka->arch_args.page_table_mask = ptable_hash_mask;

#if 0
	{
		struct ppc_pteg *old_ptable;
		int j;

		printf("sdr1 = 0x%x\n", getsdr1());

		old_ptable = (struct ppc_pteg *)((unsigned int)getsdr1() & 0xffff0000);
		printf("old_ptable %p\n", old_ptable);
		for(i=0; i< (64*1024) >> 6 ; i++) {
			for(j=0; j< 8; j++)
				if(old_ptable[i].pte[j].v && old_ptable[i].pte[j].vsid == 0)
					print_pte(&old_ptable[i].pte[j]);
		}
	}
#endif

	unsigned int sp;
	asm volatile("mr	%0,1" : "=r"(sp));
	printf("sp = 0x%x\n", sp);

	/* set up the new BATs */
	getibats(ibats);
	getdbats(dbats);

	for(i=0; i<8; i++) {
		ibats[i] = 0;
		dbats[i] = 0;
	}
	// identity map the first 256MB of RAM
	dbats[0] = ibats[0] = BATU_LEN_256M | BATU_VS;
	dbats[1] = ibats[1] = BATL_MC | BATL_PP_RW;

	// map the framebuffer using a BAT to 256MB
	{
		unsigned int framebuffer_phys = ka->fb.mapping.start & ~((16*1024*1024) - 1);

		dbats[2] = ibats[2] = 0x10000000 | BATU_LEN_16M | BATU_VS;
		dbats[3] = ibats[3] = framebuffer_phys | BATL_CI | BATL_PP_RW;
		printf("remapping framebuffer at pa 0x%x to va 0x%x using BAT\n",
			ka->fb.mapping.start, 0x10000000 + ka->fb.mapping.start - framebuffer_phys);
		s2_change_framebuffer_addr(ka, 0x10000000 + ka->fb.mapping.start - framebuffer_phys);
	}
	setibats(ibats);
	setdbats(dbats);

	tlbia();

	printf("unsetting the old page table\n");
	setsdr1(0);
	tlbia();

	printf("memsetting new pagetable\n");
	memset(ptable, 0, ptable_size);

	printf("done\n");

	printf("setting up the 16 segment registers\n");
	// set up the segment registers
	for(i=0; i<16; i++) {
		setsr(i * 0x10000000, i);
	}

	printf("done, setting sdr1\n");
	setsdr1(((unsigned int)ptable & 0xffff0000) | (ptable_hash_mask >> 10));
	tlbia();
	printf("sdr1 = 0x%x\n", getsdr1());

#if 0
	mmu_map_page(0x96008000, 0x96008000);
	mmu_map_page(0x96009000, 0x96009000);
	mmu_map_page(0x9600a000, 0x9600a000);
	mmu_map_page(0x96008000, 0x30000000);

	printf("testing...\n");
	printf("hello\n");
	printf("%d\n", *(int *)0x30000000);
	printf("%d\n", *(int *)0x96008000);

	*(int *)0x30000000 = 0x99;
	printf("%d\n", *(int *)0x30000000);
	printf("%d\n", *(int *)0x96008000);

	printf("hello2\n");
#endif

	printf("done\n");
	return 0;
}

int s2_mmu_remap_pagetable(kernel_args *ka)
{
	unsigned long i;
	unsigned long new_ptable;

	// find a new spot to allocate the page table
	// XXX make better
	new_ptable = ka->virt_alloc_range[0].start + ka->virt_alloc_range[0].size;

	for(i = 0; i < ptable_size; i += PAGE_SIZE) {
		mmu_map_page(ka, ka->arch_args.page_table.start + i, new_ptable + i, true);
	}

	ka->arch_args.page_table_virt.start = new_ptable;
	ka->arch_args.page_table_virt.size = ka->arch_args.page_table.size;
}

int s2_mmu_remove_fb_bat_entries(kernel_args *ka)
{
	unsigned int ibat[8];
	unsigned int dbat[8];

	// zero out the 2nd bat entry, used to map the framebuffer
	getibats(ibat);
	getdbats(dbat);
	ibat[2] = ibat[3] = dbat[2] = dbat[3] = 0;
	setibats(ibat);
	setdbats(dbat);

	return NO_ERROR;
}


static void print_pte(struct ppc_pte *e)
{
	printf("entry %p: ", e);
	printf("v %d ", e->v);
	if(e->v) {
		printf("vsid 0x%x ", e->vsid);
		printf("hash %d ", e->hash);
		printf("api 0x%x ", e->api);

		printf("ppn 0x%x ", e->ppn);
		printf("r %d ", e->r);
		printf("c %d ", e->c);
		printf("wimg 0x%x ", e->wimg);
		printf("pp 0x%x ", e->pp);
	}
	printf("\n");
}

void mmu_map_page(kernel_args *ka, unsigned long pa, unsigned long va, bool cached)
{
	unsigned int hash;
	struct ppc_pteg *pteg;
	int i;
	unsigned int vsid;

	// mark it used if this is in the kernel area
	if(va >= KERNEL_BASE) {
		mark_used_virt_mem_range(ka, va, PAGE_SIZE);
	}

	// lookup the vsid based off the va
	vsid = getsr(va) & 0xffffff;

//	printf("mmu_map_page: vsid %d, pa 0x%x, va 0x%x\n", vsid, pa, va);

	hash = primary_hash(vsid, va);
//	printf("hash = 0x%x\n", hash);

	pteg = &ptable[hash];
//	printf("pteg @ 0x%x\n", pteg);

	// search for the first free slot for this pte
	for(i=0; i<8; i++) {
//		printf("trying pteg[%i]\n", i);
		if(pteg->pte[i].v == 0) {
			// upper word
			pteg->pte[i].ppn = pa / PAGE_SIZE;
			pteg->pte[i].unused = 0;
			pteg->pte[i].r = 0;
			pteg->pte[i].c = 0;
			pteg->pte[i].wimg = cached ? 0 : (1 << 3);
			pteg->pte[i].unused1 = 0;
			pteg->pte[i].pp = 0x2; // RW
			asm volatile("eieio");
			// lower word
			pteg->pte[i].vsid = vsid;
			pteg->pte[i].hash = 0; // primary
			pteg->pte[i].api = (va >> 22) & 0x3f;
			pteg->pte[i].v = 1;
			tlbia();
//			printf("set pteg to ");
//			print_pte(&pteg->pte[i]);
// 			printf("set pteg to 0x%x 0x%x\n", *((int *)&pteg->pte[i]), *(((int *)&pteg->pte[i])+1));
			return;
		}
	}
}

static unsigned int primary_hash(unsigned int vsid, unsigned int vaddr)
{
	unsigned int page_index;

	vsid &= 0x7ffff;
	page_index = (vaddr >> 12) & 0xffff;

	return (vsid ^ page_index) & ptable_hash_mask;
}

static unsigned int secondary_hash(unsigned int primary_hash)
{
	return ~primary_hash;
}

