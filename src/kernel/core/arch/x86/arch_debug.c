/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <debug.h>
#include <arch/debug.h>
#include <Errors.h>

#include <kernel.h>
#include <thread.h>
#include <debug.h>
#include <elf.h>
#include <arch/debug.h>
#include <arch/x86/arch_cpu.h>


static int
dbg_stack_trace(int argc, char **argv)
{
	struct thread *t;
	uint32 ebp;
	int i;

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

	dprintf("stack trace for thread 0x%lx '%s'\n", t->id, t->name);
	dprintf("frame      caller     <image>:function + offset\n");

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
			dprintf(" eax\t0x%x\tebx\t0x%x\tecx\t0x%x\tedx\t0x%x\n", frame->eax, frame->ebx, frame->ecx, frame->edx);
			dprintf(" esi\t0x%x\tedi\t0x%x\tebp\t0x%x\tesp\t0x%x\n", frame->esi, frame->edi, frame->ebp, frame->esp);
			dprintf(" eip\t0x%x\teflags\t0x%x", frame->eip, frame->flags);
			if ((frame->error_code & 0x4) != 0) {
				// from user space
				dprintf("\tuser esp\t0x%x", frame->user_esp);
			}
			dprintf("\n");
			dprintf(" vector\t0x%x\terror code\t0x%x\n", frame->vector, frame->error_code);
 			ebp = frame->ebp;
		} else {
			uint32 eip = *((uint32 *)ebp + 1);
			const char *symbol, *image;
			addr baseAddress;
			bool exactMatch;

			if (eip == 0 || ebp == 0)
				break;

			if (elf_lookup_symbol_address(eip, &baseAddress, &symbol,
					&image, &exactMatch) == B_OK) {
				if (symbol != NULL) {
					dprintf("%08lx   %08lx   <%s>:%s + 0x%04lx%s\n", ebp, eip,
						image, symbol, eip - baseAddress, exactMatch ? "" : " (nearest)");
				} else {
					dprintf("%08lx   %08lx   <%s@%p>:unknown + 0x%04lx\n", ebp, eip,
						image, (void *)baseAddress, eip - baseAddress);
				}
			} else
				dprintf("%08lx   %08lx\n", ebp, eip);

			ebp = *(uint32 *)ebp;
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

