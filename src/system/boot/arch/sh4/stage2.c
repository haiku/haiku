/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/bootdir.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch/sh4/sh4.h>
#include <arch/sh4/vcpu.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <newos/elf32.h>
#include "serial.h"
#include "mmu.h"

#define BOOTDIR 0x8c001000
#define P2_AREA 0x8c000000
#define PHYS_ADDR_START 0x0c000000

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))

void test_interrupt();
void switch_stacks_and_call(unsigned int stack, unsigned int call_addr, unsigned int call_arg, unsigned int call_arg2);

void load_elf_image(void *data, unsigned int *next_paddr, addr_range *ar0, addr_range *ar1, unsigned int *start_addr, addr_range *dynamic_section);

int _start()
{
	unsigned int i;
	boot_entry *bootdir = (boot_entry *)BOOTDIR;
	unsigned int bootdir_len;
	unsigned int next_vaddr;
	unsigned int next_paddr;
	unsigned int kernel_entry;
	kernel_args *ka;

	serial_init();

	serial_puts("Stage 2 loader entry\n");

	// look at the bootdir
	bootdir_len = 1; // account for bootdir directory
	for(i=0; i<64; i++) {
		if(bootdir[i].be_type == BE_TYPE_NONE)
			break;
		bootdir_len += bootdir[i].be_size;
	}

	dprintf("bootdir is %d pages long\n", bootdir_len);
	next_paddr = PHYS_ADDR_START + bootdir_len * PAGE_SIZE;

	// find a location for the kernel args
	ka = (kernel_args *)(P2_AREA + bootdir_len * PAGE_SIZE);
	memset(ka, 0, sizeof(kernel_args));
	next_paddr += PAGE_SIZE;

	// initialize the vcpu
	vcpu_init(ka);
	mmu_init(ka, &next_paddr);

	// map the kernel text & data
	load_elf_image((void *)(bootdir[2].be_offset * PAGE_SIZE + BOOTDIR), &next_paddr,
			&ka->kernel_seg0_addr, &ka->kernel_seg1_addr, &kernel_entry, &ka->kernel_dynamic_section_addr);
	dprintf("mapped kernel from 0x%x to 0x%x\n", ka->kernel_seg0_addr.start, ka->kernel_seg1_addr.start + ka->kernel_seg1_addr.size);
	dprintf("kernel entry @ 0x%x\n", kernel_entry);

#if 0
	dprintf("diffing the mapped memory\n");
	dprintf("memcmp = %d\n", memcmp((void *)KERNEL_LOAD_ADDR, (void *)BOOTDIR + bootdir[2].be_offset * PAGE_SIZE, PAGE_SIZE));
	dprintf("done diffing the memory\n");
#endif

	next_vaddr = ROUNDUP(ka->kernel_seg1_addr.start + ka->kernel_seg1_addr.size, PAGE_SIZE);

	// map in a kernel stack
	ka->cpu_kstack[0].start = next_vaddr;
	for(i=0; i<2; i++) {
		mmu_map_page(next_vaddr, next_paddr);
		next_vaddr += PAGE_SIZE;
		next_paddr += PAGE_SIZE;
	}
	ka->cpu_kstack[0].size = next_vaddr - ka->cpu_kstack[0].start;

	// record this first region of allocation space
	ka->phys_alloc_range[0].start = PHYS_ADDR_START;
	ka->phys_alloc_range[0].size = next_paddr - PHYS_ADDR_START;
	ka->num_phys_alloc_ranges = 1;
	ka->virt_alloc_range[0].start = ka->kernel_seg0_addr.start;
	ka->virt_alloc_range[0].size  = next_vaddr - ka->virt_alloc_range[0].start;
	ka->virt_alloc_range[1].start = ka->kernel_seg1_addr.start;
	ka->virt_alloc_range[1].size  = next_vaddr - ka->virt_alloc_range[1].start;
	ka->num_virt_alloc_ranges = 2;

	ka->fb.enabled = 1;
	ka->fb.x_size = 640;
	ka->fb.y_size = 480;
	ka->fb.bit_depth = 16;
	ka->fb.mapping.start = 0xa5000000;
	ka->fb.mapping.size = ka->fb.x_size * ka->fb.y_size * 2;
	ka->fb.already_mapped = 1;

	ka->cons_line = 0;
	ka->str = 0;
	ka->bootdir_addr.start = P1_TO_PHYS_ADDR(BOOTDIR);
	ka->bootdir_addr.size = bootdir_len * PAGE_SIZE;
	ka->phys_mem_range[0].start = PHYS_ADDR_START;
	ka->phys_mem_range[0].size  = 16*1024*1024;
	ka->num_phys_mem_ranges = 1;
	ka->num_cpus = 1;

	for(i=0; i<ka->num_phys_alloc_ranges; i++) {
		dprintf("prange %d start = 0x%x, size = 0x%x\n",
			i, ka->phys_alloc_range[i].start, ka->phys_alloc_range[i].size);
	}

	for(i=0; i<ka->num_virt_alloc_ranges; i++) {
		dprintf("vrange %d start = 0x%x, size = 0x%x\n",
			i, ka->virt_alloc_range[i].start, ka->virt_alloc_range[i].size);
	}

	dprintf("switching stack to 0x%x and calling 0x%x\n",
		ka->cpu_kstack[0].start + ka->cpu_kstack[0].size - 4, kernel_entry);
	switch_stacks_and_call(ka->cpu_kstack[0].start + ka->cpu_kstack[0].size - 4,
		kernel_entry,
		(unsigned int)ka,
		0);
	return 0;
}

asm(".text\n"
".align 2\n"
"_switch_stacks_and_call:\n"
"	mov	r4,r15\n"
"	mov	r5,r1\n"
"	mov	r6,r4\n"
"	jsr	@r1\n"
"	mov	r7,r5");

void load_elf_image(void *data, unsigned int *next_paddr, addr_range *ar0, addr_range *ar1, unsigned int *start_addr, addr_range *dynamic_section)
{
	struct Elf32_Ehdr *imageHeader = (struct Elf32_Ehdr*) data;
	struct Elf32_Phdr *segments = (struct Elf32_Phdr*)(imageHeader->e_phoff + (unsigned) imageHeader);
	int segmentIndex;
	int foundSegmentIndex = 0;

	ar0->size = 0;
	ar1->size = 0;
	dynamic_section->size = 0;

	for (segmentIndex = 0; segmentIndex < imageHeader->e_phnum; segmentIndex++) {
		struct Elf32_Phdr *segment = &segments[segmentIndex];
		unsigned segmentOffset;

		switch(segment->p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				dynamic_section->start = segment->p_vaddr;
				dynamic_section->size = segment->p_memsz;
			default:
				continue;
		}

		dprintf("segment %d\n", segmentIndex);
		dprintf("p_offset 0x%x p_vaddr 0x%x p_paddr 0x%x p_filesz 0x%x p_memsz 0x%x\n",
			segment->p_offset, segment->p_vaddr, segment->p_paddr, segment->p_filesz, segment->p_memsz);

		/* Map initialized portion */
		for (segmentOffset = 0;
			segmentOffset < ROUNDUP(segment->p_filesz, PAGE_SIZE);
			segmentOffset += PAGE_SIZE) {

			mmu_map_page(segment->p_vaddr + segmentOffset, *next_paddr);
			memcpy((void *)ROUNDOWN(segment->p_vaddr + segmentOffset, PAGE_SIZE),
				(void *)ROUNDOWN((unsigned)data + segment->p_offset + segmentOffset, PAGE_SIZE), PAGE_SIZE);
			(*next_paddr) += PAGE_SIZE;
		}

		/* Clean out the leftover part of the last page */
		if(((segment->p_vaddr + segment->p_filesz) % PAGE_SIZE) > 0) {
			dprintf("memsetting 0 to va 0x%x, size %d\n", (void*)((unsigned)segment->p_vaddr + segment->p_filesz),
				PAGE_SIZE - ((segment->p_vaddr + segment->p_filesz) % PAGE_SIZE));
			memset((void*)((unsigned)segment->p_vaddr + segment->p_filesz), 0,
				PAGE_SIZE - ((segment->p_vaddr + segment->p_filesz) % PAGE_SIZE));
		}

		/* Map uninitialized portion */
		for (; segmentOffset < ROUNDUP(segment->p_memsz, PAGE_SIZE); segmentOffset += PAGE_SIZE) {
			dprintf("mapping zero page at va 0x%x\n", segment->p_vaddr + segmentOffset);
			mmu_map_page(segment->p_vaddr + segmentOffset, *next_paddr);
			memset((void *)(segment->p_vaddr + segmentOffset), 0, PAGE_SIZE);
			(*next_paddr) += PAGE_SIZE;
		}
		switch(foundSegmentIndex) {
			case 0:
				ar0->start = segment->p_vaddr;
				ar0->size = segment->p_memsz;
				break;
			case 1:
				ar1->start = segment->p_vaddr;
				ar1->size = segment->p_memsz;
				break;
			default:
				;
		}
		foundSegmentIndex++;
	}
	*start_addr = imageHeader->e_entry;
}

