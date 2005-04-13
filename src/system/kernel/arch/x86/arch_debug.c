/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <kernel.h>
#include <debug.h>
#include <thread.h>
#include <elf.h>

#include <arch/debug.h>
#include <arch_cpu.h>


#define NUM_PREVIOUS_LOCATIONS 16

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


static int
dbg_stack_trace(int argc, char **argv)
{
	uint32 previousLocations[NUM_PREVIOUS_LOCATIONS];
	struct iframe_stack *frameStack;
	struct thread *thread;
	uint32 ebp;
	int32 i, num = 0, last = 0;

	if (argc < 2) {
		thread = thread_get_current_thread();
		if (thread != NULL)
			frameStack = &thread->arch_info.iframes;
		else
			frameStack = &gBootFrameStack;
	} else {
		dprintf("not supported\n");
		return 0;
	}

	for (i = 0; i < frameStack->index; i++) {
		dprintf("iframe %p (end = %p)\n",
			frameStack->frames[i], frameStack->frames[i] + 1);
	}

	// We don't have a thread pointer early in the boot process
	if (thread != NULL) {
		dprintf("stack trace for thread 0x%lx \"%s\"\n", thread->id, thread->name);

		dprintf("    kernel stack: %p to %p\n", 
			(void *)thread->kernel_stack_base, (void *)(thread->kernel_stack_base + KERNEL_STACK_SIZE));
		if (thread->user_stack_base != 0) {
			dprintf("      user stack: %p to %p\n", (void *)thread->user_stack_base,
				(void *)(thread->user_stack_base + thread->user_stack_size));
		}
	}

	dprintf("frame            caller     <image>:function + offset\n");

	read_ebp(ebp);
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

			dprintf("iframe at %p\n", frame);
			dprintf(" eax 0x%-9x    ebx 0x%-9x     ecx 0x%-9x  edx 0x%x\n",
				frame->eax, frame->ebx, frame->ecx, frame->edx);
			dprintf(" esi 0x%-9x    edi 0x%-9x     ebp 0x%-9x  esp 0x%x\n",
				frame->esi, frame->edi, frame->ebp, frame->esp);
			dprintf(" eip 0x%-9x eflags 0x%-9x", frame->eip, frame->flags);
			if ((frame->error_code & 0x4) != 0) {
				// from user space
				dprintf("user esp 0x%x", frame->user_esp);
			}
			dprintf("\n");
			dprintf(" vector: 0x%x, error code: 0x%x\n", frame->vector, frame->error_code);
 			ebp = frame->ebp;
		} else {
			uint32 eip = *((uint32 *)ebp + 1);
			const char *symbol, *image;
			uint32 nextEbp = *(uint32 *)ebp;
			addr_t baseAddress;
			bool exactMatch;
			uint32 diff = nextEbp - ebp;

			if (eip == 0 || ebp == 0)
				break;

			if (elf_lookup_symbol_address(eip, &baseAddress, &symbol,
					&image, &exactMatch) == B_OK) {
				if (symbol != NULL) {
					dprintf("%08lx (+%4ld) %08lx   <%s>:%s + 0x%04lx%s\n", ebp, diff, eip,
						image, symbol, eip - baseAddress, exactMatch ? "" : " (nearest)");
				} else {
					dprintf("%08lx (+%4ld) %08lx   <%s@%p>:unknown + 0x%04lx\n", ebp, diff, eip,
						image, (void *)baseAddress, eip - baseAddress);
				}
			} else
				dprintf("%08lx   %08lx\n", ebp, eip);

			ebp = nextEbp;
		}

		if (already_visited(previousLocations, &last, &num, ebp)) {
			dprintf("circular stack frame: %p!\n", (void *)ebp);
			break;
		}
		if (ebp == 0)
			break;
	}

	return 0;
}


int
arch_dbg_init(kernel_args *args)
{
	// at this stage, the debugger command system is alive

	add_debugger_command("where", &dbg_stack_trace, "Stack crawl for current thread");
	add_debugger_command("sc", &dbg_stack_trace, NULL);

	return B_NO_ERROR;
}

