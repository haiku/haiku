/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Alexander von Gluck, kallisti5@unixzen.com
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include "start.h"

#include <string.h>

#include <OS.h>

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/heap.h>
#include <platform/openfirmware/openfirmware.h>
#include <platform_arch.h>

#include "console.h"
#include "machine.h"


#define HEAP_SIZE 65536


// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);

uint32 gMachine;
static uint32 sBootOptions;


void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}


extern "C" void
platform_start_kernel(void)
{
	preloaded_elf32_image* image = static_cast<preloaded_elf32_image*>(
		gKernelArgs.kernel_image.Pointer());

	addr_t kernelEntry = image->elf_header.e_entry;
	addr_t stackTop = gKernelArgs.cpu_kstack[0].start
		+ gKernelArgs.cpu_kstack[0].size;

	printf("kernel entry at %p\n", (void*)kernelEntry);
	printf("kernel stack top: %p\n", (void*)stackTop);

	/* TODO: ?
	mmu_init_for_kernel();
	smp_boot_other_cpus();
	*/

	status_t error = arch_start_kernel(&gKernelArgs, kernelEntry, stackTop);

	panic("Kernel returned! Return value: %" B_PRId32 "\n", error);
}


extern "C" void
platform_exit(void)
{
	of_interpret("reset-all", 0, 0);
}


extern "C" uint32
platform_boot_options(void)
{
	return sBootOptions;
}


extern "C" void
start(void *openFirmwareEntry)
{
	char bootargs[512];

	// stage2 args - might be set via the command line one day
	stage2_args args;
	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	if (of_init((intptr_t (*)(void*))openFirmwareEntry) != B_OK)
		return;

	// check for arguments
	if (of_getprop(gChosen, "bootargs", bootargs, sizeof(bootargs))
			!= OF_FAILED) {
		static const char *sArgs[] = { NULL, NULL };
		sArgs[0] = (const char *)bootargs;
		args.arguments = sArgs;
		args.arguments_count = 1;
	}

	determine_machine();
	if (console_init() != B_OK)
		return;

#ifdef __powerpc__
	if ((gMachine & MACHINE_QEMU) != 0)
		dprintf("OpenBIOS (QEMU?) OpenFirmware machine detected\n");
	else if ((gMachine & MACHINE_PEGASOS) != 0)
		dprintf("Pegasos PowerPC machine detected\n");
	else
		dprintf("Apple PowerPC machine assumed\n");
#endif

	// Initialize and take over MMU and set the OpenFirmware callbacks - it
	// will ask us for memory after that instead of maintaining it itself
	// (the kernel will need to adjust the callback later on as well)
	arch_mmu_init();

	if (boot_arch_cpu_init() != B_OK)
		of_exit();

	// check for key presses once
	sBootOptions = 0;
	int key = console_check_for_key();
	if (key == 32) {
		// space bar: option menu
		sBootOptions |= BOOT_OPTION_MENU;
	} else if (key == 27) {
		// ESC: debug output
		sBootOptions |= BOOT_OPTION_DEBUG_OUTPUT;
	}

	gKernelArgs.platform_args.openfirmware_entry = openFirmwareEntry;

	main(&args);
		// if everything goes fine, main() never returns

	of_exit();
}
