/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "console.h"
#include "mmu.h"

#include <SupportDefs.h>
#include <boot/platform.h>
#include <boot/heap.h>

#include <string.h>


#define HEAP_SIZE 32768

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

extern int main(stage2_args *args);
void _start(void);


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


bool
platform_user_menu_requested(void)
{
	return false;
}


status_t
platform_allocate_region(void **_address, size_t size, uint8 protection)
{
	puts(__FUNCTION__);
	return B_ERROR;
}


status_t
platform_free_region(void *address, size_t size)
{
	puts(__FUNCTION__);
	return B_ERROR;
}


void
platform_release_heap(struct stage2_args *args, void *base)
{
	mmu_free(base, args->heap_size);
}


status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	void *heap = mmu_allocate(NULL, args->heap_size);
	if (heap == NULL)
		return B_NO_MEMORY;

	*_base = heap;
	*_top = (void *)((int8 *)heap + args->heap_size);
	return B_OK;
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
	mmu_init();

	main(&args);
}

