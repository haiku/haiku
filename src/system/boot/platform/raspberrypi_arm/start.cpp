/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2003-2008 Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "arch_cpu.h"
#include "gpio.h"
#include "keyboard.h"
#include "arch_mmu.h"
#include "serial.h"

#include <KernelExport.h>

#include <arch/cpu.h>

#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>

#include <platform_arch.h>

#include <string.h>


#define HEAP_SIZE 65536


// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern void (*__dtor_list)(void);
extern void (*__dtor_end)(void);
extern uint8 __bss_start;
extern uint8 __bss_end;
extern uint8 __stack_start;
extern uint8 __stack_end;

extern int main(stage2_args *args);

// Adjusted during mmu_init
addr_t gPeripheralBase = DEVICE_BASE;


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &__bss_end - &__bss_start);
}


static void
fill_stack(void)
{
	memset(&__stack_start, 0xBEBEBEBE, &__stack_end - &__stack_start);
}


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}


//	#pragma mark -


void
abort(void)
{
	panic("abort");
}


uint32
platform_boot_options(void)
{
	#warning IMPLEMENT platform_boot_options
	return 0;
}


void
platform_start_kernel(void)
{
	preloaded_elf32_image *image = static_cast<preloaded_elf32_image *>(
		gKernelArgs.kernel_image.Pointer());

	addr_t kernelEntry = image->elf_header.e_entry;
	addr_t stackTop
		= gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;

	serial_cleanup();
	mmu_init_for_kernel();

	dprintf("kernel entry at %lx\n", kernelEntry);

	status_t error = arch_start_kernel(&gKernelArgs, kernelEntry,
		stackTop);

	panic("kernel returned!\n");
}


void
platform_exit(void)
{
	serial_cleanup();
	// Wait for keypress on serial
	serial_getc(true);
	serial_disable();
}


extern "C" void
_start(void)
{
	stage2_args args;

	clear_bss();
	fill_stack();
	call_ctors();

	cpu_init();
	gpio_init();

	// Flick on "OK" led, use pre-mmu firmware base
	gpio_write(gPeripheralBase + GPIO_BASE, 16, 0);

	// To debug mmu, enable serial_init above me!
	mmu_init();

	// Map in the boot archive loaded into memory by the firmware.
	args.platform.boot_tgz_size = BOOT_ARCHIVE_SIZE;
	args.platform.boot_tgz_data = (void*)mmu_map_physical_memory(
		BOOT_ARCHIVE_BASE, args.platform.boot_tgz_size, kDefaultPageFlags);

	serial_init();
	console_init();

	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	main(&args);
}
