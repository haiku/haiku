/*
 * Copyright 2003-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 */


#include <arch/debug.h>

#include <arch_cpu.h>
#include <debug.h>
#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <thread.h>


struct stack_frame {
	struct stack_frame	*previous;
	addr_t				return_address;
};

#define NUM_PREVIOUS_LOCATIONS 32

extern struct iframe_stack gBootFrameStack;


static bool
already_visited(uint32 *visited, int32 *_last, int32 *_num, uint32 framePointer)
{
	int32 last = *_last;
	int32 num = *_num;
	int32 i;

	for (i = 0; i < num; i++) {
		if (visited[(NUM_PREVIOUS_LOCATIONS + last - i)
				% NUM_PREVIOUS_LOCATIONS] == framePointer) {
			return true;
		}
	}

	*_last = last = (last + 1) % NUM_PREVIOUS_LOCATIONS;
	visited[last] = framePointer;

	if (num < NUM_PREVIOUS_LOCATIONS)
		*_num = num + 1;

	return false;
}


static inline stack_frame *
get_current_stack_frame()
{
	stack_frame *frame;
#warning M68K: a6 or a7 ?
	asm volatile("move.l %%a6,%0" : "=r"(frame));
	return frame;
}


static status_t
get_next_frame(addr_t framePointer, addr_t *next, addr_t *ip)
{
	struct thread *thread = thread_get_current_thread();
	addr_t oldFaultHandler = thread->fault_handler;

	// set fault handler, so that we can safely access user stacks
	if (thread) {
		if (m68k_set_fault_handler(&thread->fault_handler, (addr_t)&&error))
			goto error;
	}

	*ip = ((struct stack_frame *)framePointer)->return_address;
	*next = (addr_t)((struct stack_frame *)framePointer)->previous;

	if (thread)
		thread->fault_handler = oldFaultHandler;
	return B_OK;

error:
	thread->fault_handler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


static void
print_stack_frame(struct thread *thread, addr_t ip, addr_t framePointer,
	addr_t nextFramePointer)
{
	addr_t diff = nextFramePointer - framePointer;

	// kernel space/user space switch
	if (diff & 0x80000000)
		diff = 0;

	// lookup symbol
	const char *symbol, *image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status = elf_debug_lookup_symbol_address(ip, &baseAddress, &symbol,
		&image, &exactMatch);
	if (status != B_OK && !IS_KERNEL_ADDRESS(ip) && thread) {
		// try to locate the image in the images loaded into user space
		status = image_debug_lookup_user_symbol_address(thread->team, ip,
			&baseAddress, &symbol, &image, &exactMatch);
	}
	if (status == B_OK) {
		if (symbol != NULL) {
			kprintf("%08lx (+%4ld) %08lx   <%s>:%s + 0x%04lx%s\n", framePointer,
				diff, ip, image, symbol, ip - baseAddress,
				(exactMatch ? "" : " (nearest)"));
		} else {
			kprintf("%08lx (+%4ld) %08lx   <%s@%p>:unknown + 0x%04lx\n",
				framePointer, diff, ip, image, (void *)baseAddress,
				ip - baseAddress);
		}
	} else
		kprintf("%08lx (+%4ld) %08lx\n", framePointer, diff, ip);
}


static int
stack_trace(int argc, char **argv)
{
	uint32 previousLocations[NUM_PREVIOUS_LOCATIONS];
	struct iframe_stack *frameStack;
	struct thread *thread;
	addr_t framePointer;
	int32 i, num = 0, last = 0;

	if (argc < 2) {
		thread = thread_get_current_thread();
		framePointer = (addr_t)get_current_stack_frame();
	} else {
// TODO: Add support for stack traces of other threads.
/*		thread_id id = strtoul(argv[1], NULL, 0);
		thread = thread_get_thread_struct_locked(id);
		if (thread == NULL) {
			kprintf("could not find thread %ld\n", id);
			return 0;
		}

		// read %ebp from the thread's stack stored by a pushad
		ebp = thread->arch_info.current_stack.esp[2];

		if (id != thread_get_current_thread_id()) {
			// switch to the page directory of the new thread to be
			// able to follow the stack trace into userland
			addr_t newPageDirectory = (addr_t)x86_next_page_directory(
				thread_get_current_thread(), thread);

			if (newPageDirectory != 0) {
				read_cr3(oldPageDirectory);
				write_cr3(newPageDirectory);
			}
		}
*/
kprintf("Stack traces of other threads not supported yet!\n");
return 0;
	}

	// We don't have a thread pointer early in the boot process
	if (thread != NULL)
		frameStack = &thread->arch_info.iframes;
	else
		frameStack = &gBootFrameStack;

	for (i = 0; i < frameStack->index; i++) {
		kprintf("iframe %p (end = %p)\n",
			frameStack->frames[i], frameStack->frames[i] + 1);
	}

	if (thread != NULL) {
		kprintf("stack trace for thread 0x%lx \"%s\"\n", thread->id,
			thread->name);

		kprintf("    kernel stack: %p to %p\n",
			(void *)thread->kernel_stack_base,
			(void *)(thread->kernel_stack_base + KERNEL_STACK_SIZE));
		if (thread->user_stack_base != 0) {
			kprintf("      user stack: %p to %p\n",
				(void *)thread->user_stack_base,
				(void *)(thread->user_stack_base + thread->user_stack_size));
		}
	}

	kprintf("frame            caller     <image>:function + offset\n");

	for (;;) {
		// see if the frame pointer matches the iframe
		struct iframe *frame = NULL;
		for (i = 0; i < frameStack->index; i++) {
			if (framePointer == (((addr_t)frameStack->frames[i] - 8) & ~0xf)) {
				// it's an iframe
				frame = frameStack->frames[i];
				break;
			}
		}

		if (frame) {
			kprintf("iframe at %p\n", frame);
			kprintf("   d0 0x%08lx    d1 0x%08lx    d2 0x%08lx    d3 0x%08lx\n",
				frame->d0, frame->d1, frame->d2, frame->d3);
			kprintf("   d4 0x%08lx    d5 0x%08lx    d6 0x%08lx    d7 0x%08lx\n",
				frame->d4, frame->d5, frame->d6, frame->d7);
			kprintf("   a0 0x%08lx    a1 0x%08lx    a2 0x%08lx    a3 0x%08lx\n",
				frame->a0, frame->a1, frame->a2, frame->a3);
			kprintf("   a4 0x%08lx    a5 0x%08lx    a6 0x%08lx    a7 0x%08lx (sp)\n",
				frame->a4, frame->a5, frame->a6, frame->a7);

			/*kprintf("   pc 0x%08lx   ccr 0x%02x\n",
			  frame->pc, frame->ccr);*/
			kprintf("   pc 0x%08lx        sr 0x%04x\n",
				frame->pc, frame->sr);
#warning M68K: missing regs

			print_stack_frame(thread, frame->pc, framePointer, frame->a6);
 			framePointer = frame->a6;
		} else {
			addr_t ip, nextFramePointer;

			if (get_next_frame(framePointer, &nextFramePointer, &ip) != B_OK) {
				kprintf("%08lx -- read fault\n", framePointer);
				break;
			}

			if (ip == 0 || framePointer == 0)
				break;

			print_stack_frame(thread, ip, framePointer, nextFramePointer);
			framePointer = nextFramePointer;
		}

		if (already_visited(previousLocations, &last, &num, framePointer)) {
			kprintf("circular stack frame: %p!\n", (void *)framePointer);
			break;
		}
		if (framePointer == 0)
			break;
	}

/*	if (oldPageDirectory != 0) {
		// switch back to the previous page directory to no cause any troubles
		write_cr3(oldPageDirectory);
	}
*/

	return 0;
}



// #pragma mark -


void
arch_debug_save_registers(int *regs)
{
}


bool
arch_debug_contains_call(struct thread *thread, const char *symbol,
	addr_t start, addr_t end)
{
	return false;
}


void *
arch_debug_get_caller(void)
{
	// TODO: implement me
	return (void *)&arch_debug_get_caller;
}


status_t
arch_debug_init(kernel_args *args)
{
	add_debugger_command("where", &stack_trace, "Same as \"sc\"");
	add_debugger_command("bt", &stack_trace, "Same as \"sc\" (as in gdb)");
	add_debugger_command("sc", &stack_trace, "Stack crawl for current thread");

	return B_NO_ERROR;
}

