/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/heap.h>
#include <platform_arch.h>

#include <string.h>

#include "openfirmware.h"
#include "console.h"
#include "machine.h"


#define HEAP_SIZE 32768


void _start(uint32 _unused1, uint32 _unused2, void *openFirmwareEntry);
void start(void *openFirmwareEntry);

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

uint32 gMachine;


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


static void 
determine_machine(void)
{
	int root = of_finddevice("/");
	char buffer[64];
	int length;
	if ((length = of_getprop(root, "device_type", buffer, sizeof(buffer) - 1)) == OF_FAILED)
		return;
	buffer[length] = '\0';

	// ToDo: add more, and be as generic as possible

	gMachine = MACHINE_UNKNOWN;

	if (!strcasecmp("chrp", buffer))
		gMachine = MACHINE_CHRP;
	else if (!strcasecmp("bootrom", buffer))
		gMachine = MACHINE_MAC;

	if ((length = of_getprop(root, "model", buffer, sizeof(buffer) - 1)) == OF_FAILED)
		return;
	buffer[length] = '\0';

	if (!strcasecmp("pegasos", buffer))
		gMachine |= MACHINE_PEGASOS;
}


void
platform_start_kernel(void)
{
	printf("kernel entry at %p\n", (void *)gKernelArgs.kernel_image.elf_header.e_entry);
	of_exit();
}


void
_start(uint32 _unused1, uint32 _unused3, void *openFirmwareEntry)
{
	// According to the PowerPC bindings, OpenFirmware should have created
	// a stack of 32kB or higher for us at this point

	clear_bss();
	call_ctors();
		// call C++ constructors before doing anything else

	start(openFirmwareEntry);
}


void
start(void *openFirmwareEntry)
{
	// stage2 args - might be set via the command line one day
	stage2_args args;
	args.heap_size = HEAP_SIZE;

	of_init(openFirmwareEntry);

	determine_machine();
	console_init();

	// Initialize and take over MMU and set the OpenFirmware callbacks - it 
	// will ask us for memory after that instead of maintaining it itself
	// (the kernel will need to adjust the callback later on as well)
	arch_mmu_init();
	arch_set_callback();

	main(&args);
		// if everything wents fine, main() never returns

	of_exit();
}

