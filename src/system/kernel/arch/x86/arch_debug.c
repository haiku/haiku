/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <debug.h>
#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <thread.h>
#include <vm.h>

#include <arch/debug.h>
#include <arch_cpu.h>

#include <stdlib.h>


struct stack_frame {
	struct stack_frame	*previous;
	addr_t				return_address;
};

#define NUM_PREVIOUS_LOCATIONS 32

extern struct iframe_stack gBootFrameStack;


static bool
already_visited(uint32 *visited, int32 *_last, int32 *_num, uint32 ebp)
{
	int32 last = *_last;
	int32 num = *_num;
	int32 i;

	for (i = 0; i < num; i++) {
		if (visited[(NUM_PREVIOUS_LOCATIONS + last - i) % NUM_PREVIOUS_LOCATIONS] == ebp)
			return true;
	}

	*_last = last = (last + 1) % NUM_PREVIOUS_LOCATIONS;
	visited[last] = ebp;

	if (num < NUM_PREVIOUS_LOCATIONS)
		*_num = num + 1;

	return false;
}


static status_t
get_next_frame(addr_t ebp, addr_t *_next, addr_t *_eip)
{
	// set fault handler, so that we can safely access user stacks
	thread_get_current_thread()->fault_handler = (addr_t)&&error;

	*_eip = ((struct stack_frame *)ebp)->return_address;
	*_next = (addr_t)((struct stack_frame *)ebp)->previous;

	thread_get_current_thread()->fault_handler = NULL;
	return B_OK;

error:
	thread_get_current_thread()->fault_handler = NULL;
	return B_BAD_ADDRESS;
}


static void
print_stack_frame(struct thread *thread, addr_t eip, addr_t ebp, addr_t nextEbp)
{
	const char *symbol, *image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status;
	addr_t diff;

	diff = nextEbp - ebp;

	// kernel space/user space switch
	if (diff & 0x80000000)
		diff = 0;

	status = elf_debug_lookup_symbol_address(eip, &baseAddress, &symbol,
		&image, &exactMatch);
	if (status != B_OK) {
		// try to locate the image in the images loaded into user space
		status = image_debug_lookup_user_symbol_address(thread->team, eip,
			&baseAddress, &symbol, &image, &exactMatch);
	}
	
	kprintf("%08lx (+%4ld) %08lx", ebp, diff, eip);

	if (status == B_OK) {
		if (symbol != NULL) {
			kprintf("   <%s>:%s + 0x%04lx%s\n", image, symbol,
				eip - baseAddress, exactMatch ? "" : " (nearest)");
		} else {
			kprintf("   <%s@%p>:unknown + 0x%04lx\n", image,
				(void *)baseAddress, eip - baseAddress);
		}
	} else {
		vm_area *area = NULL;
		if (thread->team->address_space != NULL)
			area = vm_area_lookup(thread->team->address_space, eip);
		if (area != NULL) {
			kprintf("   %ld:%s@%p + %#lx\n", area->id, area->name, (void *)area->base,
				eip - area->base);
		} else
			kprintf("\n");
	}
}


static void
print_iframe(struct iframe *frame)
{
	kprintf("iframe at %p (end = %p)\n", frame, frame + 1);

	kprintf(" eax 0x%-9lx    ebx 0x%-9lx     ecx 0x%-9lx  edx 0x%lx\n",
		frame->eax, frame->ebx, frame->ecx, frame->edx);
	kprintf(" esi 0x%-9lx    edi 0x%-9lx     ebp 0x%-9lx  esp 0x%lx\n",
		frame->esi, frame->edi, frame->ebp, frame->esp);
	kprintf(" eip 0x%-9lx eflags 0x%-9lx", frame->eip, frame->flags);
	if ((frame->error_code & 0x4) != 0) {
		// from user space
		kprintf("user esp 0x%lx", frame->user_esp);
	}
	kprintf("\n");
	kprintf(" vector: 0x%lx, error code: 0x%lx\n", frame->vector, frame->error_code);
}


static int
stack_trace(int argc, char **argv)
{
	uint32 previousLocations[NUM_PREVIOUS_LOCATIONS];
	struct iframe_stack *frameStack;
	struct thread *thread = NULL;
	addr_t oldPageDirectory = 0;
	uint32 ebp = 0;
	int32 i, num = 0, last = 0;

	if (argc == 2) {
		thread_id id = strtoul(argv[1], NULL, 0);
		thread = thread_get_thread_struct_locked(id);
		if (thread == NULL) {
			kprintf("could not find thread %ld\n", id);
			return 0;
		}

		if (id != thread_get_current_thread_id()) {
			// switch to the page directory of the new thread to be
			// able to follow the stack trace into userland
			addr_t newPageDirectory = (addr_t)x86_next_page_directory(
				thread_get_current_thread(), thread);

			if (newPageDirectory != 0) {
				read_cr3(oldPageDirectory);
				write_cr3(newPageDirectory);
			}

			// read %ebp from the thread's stack stored by a pushad
			ebp = thread->arch_info.current_stack.esp[2];
		} else
			thread == NULL;
	} else if (argc > 2) {
		kprintf("usage: %s [thread id]\n", argv[0]);
		return 0;
	}

	if (thread == NULL) {
		// if we don't have a thread yet, we want the current one
		thread = thread_get_current_thread();
		ebp = x86_read_ebp();
	}

	// We don't have a thread pointer early in the boot process
	if (thread != NULL)
		frameStack = &thread->arch_info.iframes;
	else
		frameStack = &gBootFrameStack;

	if (thread != NULL) {
		kprintf("stack trace for thread 0x%lx \"%s\"\n", thread->id, thread->name);

		kprintf("    kernel stack: %p to %p\n", 
			(void *)thread->kernel_stack_base, (void *)(thread->kernel_stack_base + KERNEL_STACK_SIZE));
		if (thread->user_stack_base != 0) {
			kprintf("      user stack: %p to %p\n", (void *)thread->user_stack_base,
				(void *)(thread->user_stack_base + thread->user_stack_size));
		}
	}

	kprintf("frame            caller     <image>:function + offset\n");

	for (;;) {
		bool isIFrame = false;
		// see if the ebp matches the iframe
		for (i = 0; i < frameStack->index; i++) {
			if (ebp == ((uint32)frameStack->frames[i] - 8)) {
				// it's an iframe
				isIFrame = true;
			}
		}

		if (isIFrame) {
			struct iframe *frame = (struct iframe *)(ebp + 8);

			print_iframe(frame);
			print_stack_frame(thread, frame->eip, ebp, frame->ebp);

 			ebp = frame->ebp;
		} else {
			addr_t eip, nextEbp;

			if (get_next_frame(ebp, &nextEbp, &eip) != B_OK) {
				kprintf("%08lx -- read fault\n", ebp);
				break;
			}

			if (eip == 0 || ebp == 0)
				break;

			print_stack_frame(thread, eip, ebp, nextEbp);
			ebp = nextEbp;
		}

		if (already_visited(previousLocations, &last, &num, ebp)) {
			kprintf("circular stack frame: %p!\n", (void *)ebp);
			break;
		}
		if (ebp == 0)
			break;
	}

	if (oldPageDirectory != 0) {
		// switch back to the previous page directory to no cause any troubles
		write_cr3(oldPageDirectory);
	}

	return 0;
}


static int
dump_iframes(int argc, char **argv)
{
	struct iframe_stack *frameStack;
	struct thread *thread = NULL;
	int32 i;

	if (argc < 2) {
		thread = thread_get_current_thread();
	} else if (argc == 2) {
		thread_id id = strtoul(argv[1], NULL, 0);
		thread = thread_get_thread_struct_locked(id);
		if (thread == NULL) {
			kprintf("could not find thread %ld\n", id);
			return 0;
		}
	} else if (argc > 2) {
		kprintf("usage: %s [thread id]\n", argv[0]);
		return 0;
	}

	// We don't have a thread pointer early in the boot process
	if (thread != NULL)
		frameStack = &thread->arch_info.iframes;
	else
		frameStack = &gBootFrameStack;

	if (thread != NULL)
		kprintf("iframes for thread 0x%lx \"%s\"\n", thread->id, thread->name);

	for (i = 0; i < frameStack->index; i++) {
		print_iframe(frameStack->frames[i]);
	}

	return 0;
}


//	#pragma mark -


void *
arch_debug_get_caller(void)
{
	// It looks like you would get the wrong stack frame here, but
	// since read_ebp() is an assembler inline macro, GCC seems to 
	// be smart enough to save its original value.
	struct stack_frame *frame = (struct stack_frame *)x86_read_ebp();

	return (void *)frame->return_address;
}


status_t
arch_debug_init(kernel_args *args)
{
	// at this stage, the debugger command system is alive

	add_debugger_command("where", &stack_trace, "Same as \"sc\"");
	add_debugger_command("bt", &stack_trace, "Same as \"sc\" (as in gdb)");
	add_debugger_command("sc", &stack_trace, "Stack crawl for current thread (or any other)");
	add_debugger_command("iframe", &dump_iframes, "Dump iframes for the specified thread");

	return B_NO_ERROR;
}

