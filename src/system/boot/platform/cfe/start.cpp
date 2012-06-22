/*
 * Copyright 2011, François Revol, revol@free.fr.
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Alexander von Gluck, kallisti5@unixzen.com
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <OS.h>

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/heap.h>
#include <boot/platform/cfe/cfe.h>
#include <platform_arch.h>

#include "console.h"
#include "real_time_clock.h"


#define HEAP_SIZE 65536


extern "C" uint32 _start(uint64 handle, uint64 entry, uint32 _unused, 
	uint32 signature);
extern "C" void start(uint64 cfeHandle, uint64 cfeEntry);

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

#if 0//OF
uint32 gMachine;
#endif
static uint32 sBootOptions;


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &_end - &__bss_start);
}


extern "C" void
platform_start_kernel(void)
{
	preloaded_elf32_image* image = static_cast<preloaded_elf32_image*>(
		gKernelArgs.kernel_image);

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

	panic("Kernel returned! Return value: %ld\n", error);
}


extern "C" void
platform_exit(void)
{
	cfe_exit(CFE_FLG_WARMSTART, 0);
	panic("cfe_exit() failed.");
}


extern "C" uint32
platform_boot_options(void)
{
	return sBootOptions;
}


extern "C" uint32
_start(uint64 handle, uint64 entry, uint32 _unused, uint32 signature)
{

	if (signature != CFE_EPTSEAL)
		return 123;//XXX:something?

	clear_bss();
	call_ctors();
		// call C++ constructors before doing anything else
	//return 456;

	start(handle, entry);
}


extern "C" void
start(uint64 cfeHandle, uint64 cfeEntry)
{
	char bootargs[512];

	// stage2 args - might be set via the command line one day
	stage2_args args;
	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	cfe_init(cfeHandle, cfeEntry);

	// check for arguments
#if 0//OF
	if (of_getprop(gChosen, "bootargs", bootargs, sizeof(bootargs))
			!= OF_FAILED) {
		static const char *sArgs[] = { NULL, NULL };
		sArgs[0] = (const char *)bootargs;
		args.arguments = sArgs;
	}
#endif

#if 0//OF
	determine_machine();
#endif
	console_init();

	// XXX:FIXME: doesn't even land here.
	dprintf("testing...\n");
	while (true);

#if 0//OF
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
		cfe_exit(CFE_FLG_WARMSTART, 1);

#if 0//OF FIXME
	if (init_real_time_clock() != B_OK)
		cfe_exit(CFE_FLG_WARMSTART, 1);
#endif

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

	gKernelArgs.platform_args.cfe_entry = cfeEntry;

	main(&args);
		// if everything goes fine, main() never returns

	cfe_exit(CFE_FLG_WARMSTART, 1);
}
