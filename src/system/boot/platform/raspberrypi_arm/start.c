/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2003-2008 Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "cpu.h"
#include "gpio.h"
#include "keyboard.h"
#include "mmu.h"
#include "serial.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <arch/cpu.h>

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
void _start(void);


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
	#warning IMPLEMENT platform_start_kernel
	panic("kernel returned!\n");
}


void
platform_exit(void)
{
	serial_cleanup();
}


void
pi_start(void)
{
	stage2_args args;

	clear_bss();
	fill_stack();
	call_ctors();

	cpu_init();
	gpio_init();

	// Flick on "OK" led
	gpio_write(GPIO_BASE, 16, 0);

	mmu_init();
	serial_init();
	console_init();

	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	main(&args);
}

