/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include "stage2_priv.h"
#include <boot/bootdir.h>
#include <boot/stage2.h>
#include <arch/cpu.h>

static kernel_args ka = { 0 };

#define BOOTDIR_ADDR 0x4381000
static const boot_entry *bootdir = (boot_entry*)BOOTDIR_ADDR;

int _start(char *boot_args, char *monitor);
int _start(char *boot_args, char *monitor)
{
	unsigned int bootdir_pages;

	init_nextmon(monitor);
	dprintf("\nNewOS stage2: args '%s', monitor %p\n", boot_args, monitor);

	probe_memory(&ka);

	dprintf("tc 0x%x\n", get_tc());
	dprintf("urp 0x%x\n", get_urp());
	dprintf("srp 0x%x\n", get_srp());

	// calculate how big the bootdir is
	{
		int entry;
		bootdir_pages = 0;
		for (entry = 0; entry < BOOTDIR_MAX_ENTRIES; entry++) {
			if (bootdir[entry].be_type == BE_TYPE_NONE)
				break;

			bootdir_pages += bootdir[entry].be_size;
		}
		ka.bootdir_addr.start = (unsigned long)bootdir;
		ka.bootdir_addr.size = bootdir_pages * PAGE_SIZE;
		dprintf("bootdir: start %p, size 0x%x\n", (char *)ka.bootdir_addr.start, ka.bootdir_addr.size);
	}

	// begin to set up the physical page allocation range data structures
	ka.num_phys_alloc_ranges = 1;
	ka.phys_alloc_range[0].start = ka.bootdir_addr.start;
	ka.phys_alloc_range[0].size = ka.bootdir_addr.size;

	// allocate a stack for the kernel when we jump into it
	ka.cpu_kstack[0].start = allocate_page(&ka);
	ka.cpu_kstack[0].size = PAGE_SIZE;

	ka.num_cpus = 0;

	return 0;
}

unsigned long allocate_page(kernel_args *ka)
{
	unsigned long page;

	page = ka->phys_alloc_range[0].start + ka->phys_alloc_range[0].size;
	ka->phys_alloc_range[0].size += PAGE_SIZE;

	return page;
}

