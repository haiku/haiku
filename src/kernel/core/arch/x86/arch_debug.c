/* 
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <Errors.h>
#include <debug.h>
#include <thread.h>
#include <elf.h>
#include <arch/debug.h>

#include <arch_cpu.h>


#define NUM_PREVIOUS_LOCATIONS 16


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
	struct thread *t;
	uint32 ebp;
	int32 i, num = 0, last = 0;

	if (argc < 2)
		t = thread_get_current_thread();
	else {
		dprintf("not supported\n");
		return 0;
	}

	for (i = 0; i < t->arch_info.iframe_ptr; i++) {
		char *temp = (char *)t->arch_info.iframes[i];
		dprintf("iframe %p %p %p\n", temp, temp + sizeof(struct iframe), temp + sizeof(struct iframe) - 8);
	}

	dprintf("stack trace for thread 0x%lx \"%s\"\n", t->id, t->name);

	dprintf("    kernel stack: %p to %p\n", 
		(void *)t->kernel_stack_base, (void *)(t->kernel_stack_base + KSTACK_SIZE));
	if (t->user_stack_base != 0) {
		dprintf("      user stack: %p to %p\n", (void *)t->user_stack_base,
			(void *)(t->user_stack_base + t->user_stack_size));
	}

	dprintf("frame            caller     <image>:function + offset\n");

	read_ebp(ebp);
	for (;;) {
		bool is_iframe = false;
		// see if the ebp matches the iframe
		for (i = 0; i < t->arch_info.iframe_ptr; i++) {
			if (ebp == ((uint32)t->arch_info.iframes[i] - 8)) {
				// it's an iframe
				is_iframe = true;
			}
		}

		if (is_iframe) {
			struct iframe *frame = (struct iframe *)(ebp + 8);

			dprintf("iframe at %p\n", frame);
			dprintf(" eax 0x%-9x    ebx 0x%-9x     ecx 0x%-9x  edx 0x%x\n", frame->eax, frame->ebx, frame->ecx, frame->edx);
			dprintf(" esi 0x%-9x    edi 0x%-9x     ebp 0x%-9x  esp 0x%x\n", frame->esi, frame->edi, frame->ebp, frame->esp);
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
arch_dbg_init(kernel_args *ka)
{
	// at this stage, the debugger command system is alive

	add_debugger_command("where", &dbg_stack_trace, "Stack crawl for current thread");
	add_debugger_command("sc", &dbg_stack_trace, NULL);

	return B_NO_ERROR;
}

