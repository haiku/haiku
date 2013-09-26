/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "mmu.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch/ppc/arch_mmu_amcc440.h>
#include <arch_kernel.h>
#include <platform/openfirmware/openfirmware.h>
#ifdef __ARM__
#include <arm_mmu.h>
#endif
#include <kernel.h>

#include <board_config.h>

#include <OS.h>

#include <string.h>

int32 of_address_cells(int package);
int32 of_size_cells(int package);

#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*!	Computes the recommended minimal page table size as
	described in table 7-22 of the PowerPC "Programming
	Environment for 32-Bit Microprocessors".
	The page table size ranges from 64 kB (for 8 MB RAM)
	to 32 MB (for 4 GB RAM).
	FIXME: account for larger TLB descriptors for Book-E
*/
static size_t
suggested_page_table_size(phys_addr_t total)
{
	uint32 max = 23;
		// 2^23 == 8 MB

	while (max < 32) {
		if (total <= (1UL << max))
			break;

		max++;
	}

	return 1UL << (max - 7);
		// 2^(23 - 7) == 64 kB
}


static void
read_TLB(int i, uint32 tlb[3], uint8 &pid)
{
	//FIXME:read pid too
	asm volatile(
		"tlbre %0,%3,0\n"
		"\ttlbre %1,%3,1\n"
		"\ttlbre %2,%3,2"
		:   "=r"(tlb[0]),
			"=r"(tlb[1]),
			"=r"(tlb[2])
		:	"r"(i)
	);
}


static void
write_TLB(int i, uint32 tlb[3], uint8 pid)
{
	//FIXME:write pid too
	asm volatile(
		"tlbwe %0,%3,0\n"
		"\ttlbwe %1,%3,1\n"
		"\ttlbwe %2,%3,2"
		: : "r"(tlb[0]),
			"r"(tlb[1]),
			"r"(tlb[2]),
			"r"(i)
	);
}


static void
dump_TLBs(void)
{
	int i;
	for (i = 0; i < TLB_COUNT; i++) {
		uint32 tlb[3];// = { 0, 0, 0 };
		uint8 pid;
		read_TLB(i, tlb, pid);
		dprintf("TLB[%02d]: %08lx %08lx %08lx %02x\n",
			i, tlb[0], tlb[1], tlb[2], pid);
	}
}


status_t
arch_mmu_setup_pinned_tlb_amcc440(phys_addr_t totalRam, size_t &tableSize,
	size_t &tlbSize)
{
	dump_TLBs();
	tlb_length tlbLength = TLB_LENGTH_16MB;
//XXX:totalRam = 4LL*1024*1024*1024;

	size_t suggestedTableSize = suggested_page_table_size(totalRam);
	dprintf("suggested page table size = %" B_PRIuSIZE "\n",
		suggestedTableSize);

	tableSize = suggestedTableSize;

	// add 4MB for kernel and some more for modules...
	tlbSize = tableSize + 8 * 1024 * 1024;

	// round up to realistic TLB lengths, either 16MB or 256MB
	// the unused space will be filled with SLAB areas
	if (tlbSize < 16 * 1024 * 1024)
		tlbSize = 16 * 1024 * 1024;
	else {
		tlbSize = 256 * 1024 * 1024;
		tlbLength = TLB_LENGTH_256MB;
	}

	uint32 tlb[3];
	uint8 pid;
	int i;

	// Make sure the last TLB is free, else we are in trouble
	// XXX: allow using a different TLB entry?
	read_TLB(TLB_COUNT - 1, tlb, pid);
	if ((tlb[0] & TLB_V) != 0) {
		panic("Last TLB already in use. FIXME.");
		return B_ERROR;
	}

	// TODO: remove existing mapping from U-Boot at KERNEL_BASE !!!
	// (on Sam460ex it's pci mem)
	// for now we just move it to AS1 which we don't use, until calling
	// the kernel.
	// we could probably swap it with our own KERNEL_BASE TLB to call U-Boot
	// if required, but it'd be quite ugly.
	for (i = 0; i < TLB_COUNT; i++) {
		read_TLB(i, tlb, pid);
		//dprintf("tlb[%d][0] = %08lx\n", i, tlb[0]);
		// TODO: make the test more complete and correct
		if ((tlb[0] & 0xfffffc00) == KERNEL_BASE) {
			tlb[0] |= 0x100; // AS1
			write_TLB(i, tlb, pid);
			dprintf("Moved existing translation in TLB[%d] to AS1\n", i);
		}
	}

	// pin the last TLB
	//XXX:also maybe skip the FDT + initrd + loader ?
	phys_addr_t physBase = gKernelArgs.physical_memory_range[0].start;
	//TODO:make sure 1st range is large enough?
	i = TLB_COUNT - 1; // last one
	pid = 0; // the kernel's PID
	tlb[0] = (KERNEL_BASE | tlbLength << 4 | TLB_V);
	tlb[1] = ((physBase & 0xfffffc00) | (physBase >> 32));
	tlb[2] = (0x0000003f); // user:RWX kernel:RWX
	write_TLB(i, tlb, pid);

	return B_OK;
}

