/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 *
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "arch_smp.h"

#include <boot/stage2.h>


//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


extern "C" void long_smp_trampoline(void);
extern "C" void long_smp_trampoline_args(void);
extern "C" void long_smp_trampoline_end(void);
extern "C" uint64 gLongGDTR;

struct gdtr {
    uint16 limit;
    uint32 base;
    unsigned char null[8];
    unsigned char code[8];
    unsigned char data[8];
    unsigned char code64[8];
} __attribute__((packed));

// Arguments passed to the SMP trampoline.
struct long_trampoline_args {
	uint32 trampoline;        // Trampoline address
	uint32 gdt32;             // 32-bit GDTR
	uint32 pml4;              // 64-bit PML4
	uint32 padding;           // Padding
	uint64 gdt64;             // 64-bit GDTR
	uint64 kernel_entry;      // Kernel entry point
	uint64 kernel_args;       // Kernel arguments
	uint64 current_cpu;       // CPU number
	uint64 stack_top;         // Kernel stack
	volatile uint64 sentinel; // Sentinel, AP sets to 0 when finished

	// smp_boot_other_cpus puts the GDTR here.
	struct gdtr gdtr;
};


void
copy_trampoline_code(uint64 trampolineCode, uint64 trampolineStack)
{
	TRACE("copying the trampoline code to %p from %p\n", (char*)trampolineCode, (const void*)&long_smp_trampoline);
	TRACE("size of trampoline code = %" PRIu64 " bytes\n", (uint64)&long_smp_trampoline_end - (uint64)&long_smp_trampoline);
	memcpy((char *)trampolineCode, (const void*)&long_smp_trampoline,
		(uint64)&long_smp_trampoline_end - (uint64)&long_smp_trampoline);
}


void
prepare_trampoline_args(uint64 trampolineCode, uint64 trampolineStack,
	uint32 pagedir, uint64 kernelEntry, addr_t virtKernelArgs,
	uint32 currentCpu)
{
	long_trampoline_args* args = (long_trampoline_args *)trampolineStack;
	args->trampoline = trampolineCode;

	args->gdt32 = (uint64) &args->gdtr;
	args->gdtr.limit = 8 * 4 - 1;
	args->gdtr.base = (uint32)(uint64)args->gdtr.null;
	#define COPY_ARRAY(A, X0, X1, X2, X3, X4, X5, X6, X7) \
		{ A[0] = X0; A[1] = X1; A[2] = X2; A[3] = X3; A[4] = X4; A[5] = X5; A[6] = X6; A[7] = X7; }
	COPY_ARRAY(args->gdtr.null, 0, 0, 0, 0, 0, 0, 0, 0);
	COPY_ARRAY(args->gdtr.code, 0xff, 0xff, 0, 0, 0, 0x9a, 0xcf, 0);
	COPY_ARRAY(args->gdtr.data, 0xff, 0xff, 0, 0, 0, 0x92, 0xcf, 0);
	COPY_ARRAY(args->gdtr.code64, 0xff, 0xff, 0, 0, 0, 0x9a, 0xaf, 0);
	#undef COPY_ARRAY
	args->gdt64 = (uint64_t)&gLongGDTR;

	args->pml4 = pagedir;
	args->kernel_entry = kernelEntry;
	args->kernel_args = virtKernelArgs;
	args->current_cpu = currentCpu;
	args->stack_top = gKernelArgs.cpu_kstack[currentCpu].start
		+ gKernelArgs.cpu_kstack[currentCpu].size;
	args->sentinel = 1;

	// put the args in the right place
	uint32 * args_ptr =
		(uint32 *)(trampolineCode + (uint64)long_smp_trampoline_args - (uint64)long_smp_trampoline);
	*args_ptr = (uint32)trampolineStack;
}


uint32
get_sentinel(uint64 trampolineStack)
{
	long_trampoline_args* args = (long_trampoline_args *)trampolineStack;
	return args->sentinel;
}
