/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <boot/stage2.h>
#include <boot/heap.h>
#include <platform_arch.h>

#include <string.h>

#include "openfirmware.h"
#include "console.h"


#define HEAP_SIZE 32768


void _start(uint32 _unused1, uint32 _unused2, void *openFirmwareEntry);
void start(void *openFirmwareEntry);

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;


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
	console_init();
	arch_mmu_init();
	printf("testing mmu allocation: %p\n", arch_mmu_alloc(128 * 1024, B_READ_AREA));

	main(&args);
		// if everything wents fine, main() never returns

	of_exit();
}

