/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "console.h"
#include "cpu.h"
#include "mmu.h"
#include "keyboard.h"
#include "bios.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>

#include <string.h>


#define HEAP_SIZE 32768

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


uint32
platform_boot_options(void)
{
#if 0
	if (!gKernelArgs.fb.enabled)
		sBootOptions |= check_for_boot_keys();
#endif
	return sBootOptions;
}


void
platform_start_kernel(void)
{
	static struct kernel_args *args = &gKernelArgs;
		// something goes wrong when we pass &gKernelArgs directly
		// to the assembler inline below - might be a bug in GCC
		// or I don't see something important...
	addr_t stackTop = gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;

	mmu_init_for_kernel();
	cpu_boot_other_cpus();

	printf("kernel entry at %lx\n", gKernelArgs.kernel_image.elf_header.e_entry);

	asm("movl	%0, %%eax;	"			// move stack out of way
		"movl	%%eax, %%esp; "
		: : "m" (stackTop));
	asm("pushl  $0x0; "					// we're the BSP cpu (0)
		"pushl 	%0;	"					// kernel args
		"pushl 	$0x0;"					// dummy retval for call to main
		"pushl 	%1;	"					// this is the start address
		"ret;		"					// jump.
		: : "g" (args), "g" (gKernelArgs.kernel_image.elf_header.e_entry));

	panic("kernel returned!\n");
}


void
platform_exit(void)
{
	struct bios_regs regs;
	regs.eax = 0x0;
	call_bios(0x19, &regs);
		// this should reboot the system (but doesn't seem to work...)
}


void
_start(void)
{
	stage2_args args;

	asm("cld");			// Ain't nothing but a GCC thang.
	asm("fninit");		// initialize floating point unit

	clear_bss();
	call_ctors();
		// call C++ constructors before doing anything else

	args.heap_size = HEAP_SIZE;

	console_init();
	cpu_init();
	mmu_init();

	spin(50000);
		// wait a bit to give the user the opportunity to press a key

	// reading the keyboard doesn't seem to work in graphics mode (maybe a bochs problem)
	sBootOptions = check_for_boot_keys();
	if (sBootOptions & BOOT_OPTION_DEBUG_OUTPUT)
		serial_enable();

	main(&args);
}

