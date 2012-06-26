/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>

#include "acpi.h"
#include "apm.h"
#include "bios.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "hpet.h"
#include "interrupts.h"
#include "keyboard.h"
#include "long.h"
#include "mmu.h"
#include "multiboot.h"
#include "serial.h"
#include "smp.h"


#define HEAP_SIZE (128 * 1024)

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

extern "C" int main(stage2_args *args);
extern "C" void _start(void);


uint32 sBootOptions;


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &_end - &__bss_start);
}


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}


extern "C" uint32
platform_boot_options(void)
{
#if 0
	if (!gKernelArgs.fb.enabled)
		sBootOptions |= check_for_boot_keys();
#endif
	return sBootOptions;
}


extern "C" void
platform_start_kernel(void)
{
	// 64-bit kernel entry is all handled in long.cpp
	if (gKernelArgs.kernel_image->elf_class == ELFCLASS64) {
		long_start_kernel();
		return;
	}

	static struct kernel_args *args = &gKernelArgs;
		// something goes wrong when we pass &gKernelArgs directly
		// to the assembler inline below - might be a bug in GCC
		// or I don't see something important...
	addr_t stackTop
		= gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;

	preloaded_elf32_image *image = static_cast<preloaded_elf32_image *>(
		gKernelArgs.kernel_image.Pointer());

	smp_init_other_cpus();
	debug_cleanup();
	mmu_init_for_kernel();

	// We're about to enter the kernel -- disable console output.
	stdout = NULL;

	smp_boot_other_cpus();

	dprintf("kernel entry at %lx\n", image->elf_header.e_entry);

	asm("movl	%0, %%eax;	"			// move stack out of way
		"movl	%%eax, %%esp; "
		: : "m" (stackTop));
	asm("pushl  $0x0; "					// we're the BSP cpu (0)
		"pushl 	%0;	"					// kernel args
		"pushl 	$0x0;"					// dummy retval for call to main
		"pushl 	%1;	"					// this is the start address
		"ret;		"					// jump.
		: : "g" (args), "g" (image->elf_header.e_entry));

	panic("kernel returned!\n");
}


extern "C" void
platform_exit(void)
{
	// reset the system using the keyboard controller
	out8(0xfe, 0x64);
}


extern "C" void
_start(void)
{
	stage2_args args;

	asm("cld");			// Ain't nothing but a GCC thang.
	asm("fninit");		// initialize floating point unit

	clear_bss();
	call_ctors();
		// call C++ constructors before doing anything else

	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	serial_init();
	serial_enable();
	interrupts_init();
	console_init();
	cpu_init();
	mmu_init();
	debug_init_post_mmu();
	parse_multiboot_commandline(&args);

	// reading the keyboard doesn't seem to work in graphics mode
	// (maybe a bochs problem)
	sBootOptions = check_for_boot_keys();
//	if (sBootOptions & BOOT_OPTION_DEBUG_OUTPUT)
//		serial_enable();

	apm_init();
	acpi_init();
	smp_init();
	hpet_init();
	dump_multiboot_info();
	main(&args);
}

