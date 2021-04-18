/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <arch/cpu.h>

#include <string.h>

#include "console.h"
#include "cpu.h"
#include "mmu.h"
#include "fdt.h"
#include "Htif.h"
#include "virtio.h"
#include "graphics.h"


#define HEAP_SIZE (1024*1024)

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

extern "C" int main(stage2_args* args);
extern "C" void _start(int hartId, void* fdt);
extern "C" status_t arch_enter_kernel(struct kernel_args* kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop);;


static uint32 sBootOptions;
static bool sBootOptionsValid = false;


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &_end - &__bss_start);
}


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++)
		(**f)();
}


static uint32
check_for_boot_keys()
{
	uint32 options = 0;
	bigtime_t t0 = system_time();
	while (system_time() - t0 < 100000) {
		int key = virtio_input_get_key();
		switch(key) {
			case 94 /* space */:
				options |= BOOT_OPTION_MENU;
				break;
		}
	}
	return options;
}


extern "C" uint32
platform_boot_options(void)
{
	if (!sBootOptionsValid) {
		sBootOptions = check_for_boot_keys();
		sBootOptionsValid = true;
/*
		if (sBootOptions & BOOT_OPTION_DEBUG_OUTPUT)
			serial_enable();
*/
	}

	return sBootOptions;
}


extern "C" void
platform_start_kernel(void)
{
	static struct kernel_args* args = &gKernelArgs;
	preloaded_elf64_image* image = static_cast<preloaded_elf64_image*>(
		gKernelArgs.kernel_image.Pointer());

	//smp_init_other_cpus();
	//serial_cleanup();
	//mmu_init_for_kernel();
	//smp_boot_other_cpus();

	// Avoid interrupts from virtio devices before kernel driver takes control.
	virtio_fini();

	fdt_set_kernel_args();
	mmu_init_for_kernel();

	dprintf("kernel entry at %lx\n", image->elf_header.e_entry);

	addr_t stackTop
		= gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;
	arch_enter_kernel(args, image->elf_header.e_entry, stackTop);

	panic("kernel returned!\n");

}


extern "C" void
platform_exit(void)
{
	HtifShutdown();
}


extern "C" void
_start(int hartId, void* fdt)
{
	HtifOutString("haiku_loader entry point\n");

	clear_bss();
	fdt_init(fdt);
	call_ctors();

	stage2_args args;
	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	// console_init();
	// virtio_init();
	cpu_init();
	mmu_init();
	//apm_init();
	//smp_init();

	main(&args);
}
