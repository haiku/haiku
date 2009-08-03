/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "serial.h"
#include "console.h"
#include "cpu.h"
#include "smp.h"
#include "keyboard.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <arch/cpu.h>


#include <string.h>


#define HEAP_SIZE (128 * 1024)

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

extern int main(stage2_args *args);
void _start(void);


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




void
platform_start_kernel(void)
{
	static struct kernel_args *args = &gKernelArgs;
		// something goes wrong when we pass &gKernelArgs directly
		// to the assembler inline below - might be a bug in GCC
		// or I don't see something important...
	addr_t stackTop
		= gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;

//	smp_init_other_cpus();
	serial_cleanup();
//	mmu_init_for_kernel();
//	smp_boot_other_cpus();

	dprintf("kernel entry at %lx\n",
		gKernelArgs.kernel_image.elf_header.e_entry);

/*	asm("movl	%0, %%eax;	"			// move stack out of way
		"movl	%%eax, %%esp; "
		: : "m" (stackTop));
	asm("pushl  $0x0; "					// we're the BSP cpu (0)
		"pushl 	%0;	"					// kernel args
		"pushl 	$0x0;"					// dummy retval for call to main
		"pushl 	%1;	"					// this is the start address
		"ret;		"					// jump.
		: : "g" (args), "g" (gKernelArgs.kernel_image.elf_header.e_entry));
*/
	panic("kernel returned!\n");
}


void
platform_exit(void)
{
}


void
start2(void)
{
	stage2_args args;

	clear_bss();
		// call C++ constructors before doing anything else
	call_ctors();
	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;


	serial_init();
	console_init();
	cpu_init();
//	mmu_init();

	// wait a bit to give the user the opportunity to press a key
//	spin(750000);

	// reading the keyboard doesn't seem to work in graphics mode
	// (maybe a bochs problem)
//	sBootOptions = check_for_boot_keys();
	//if (sBootOptions & BOOT_OPTION_DEBUG_OUTPUT)
		serial_enable();

	main(&args);
}

uint32
platform_boot_options(void)
{
	return sBootOptions;
}
