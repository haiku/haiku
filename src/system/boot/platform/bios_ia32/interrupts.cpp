/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "interrupts.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>

#include <KernelExport.h>

#include <boot/platform.h>
#include <boot/platform/generic/text_console.h>

#include <arch_cpu.h>
#include <arch/x86/descriptors.h>

#include "debug.h"
#include "keyboard.h"


//#define TRACE_INTERRUPTS
#ifdef TRACE_INTERRUPTS
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// interrupt function prototypes
#define INTERRUPT_FUNCTION(vector) \
	extern "C" void interrupt_function##vector();
INTERRUPT_FUNCTIONS()
#undef INTERRUPT_FUNCTION

#define INTERRUPT_FUNCTION(vector) \
	extern "C" void exception_interrupt_function##vector();
INTERRUPT_FUNCTIONS()
#undef INTERRUPT_FUNCTION


static const char *kInterruptNames[DEBUG_IDT_SLOT_COUNT] = {
	/*  0 */ "Divide Error Exception",
	/*  1 */ "Debug Exception",
	/*  2 */ "NMI Interrupt",
	/*  3 */ "Breakpoint Exception",
	/*  4 */ "Overflow Exception",
	/*  5 */ "BOUND Range Exceeded Exception",
	/*  6 */ "Invalid Opcode Exception",
	/*  7 */ "Device Not Available Exception",
	/*  8 */ "Double Fault Exception",
	/*  9 */ "Coprocessor Segment Overrun",
	/* 10 */ "Invalid TSS Exception",
	/* 11 */ "Segment Not Present",
	/* 12 */ "Stack Fault Exception",
	/* 13 */ "General Protection Exception",
	/* 14 */ "Page-Fault Exception",
	/* 15 */ "-",
	/* 16 */ "x87 FPU Floating-Point Error",
	/* 17 */ "Alignment Check Exception",
	/* 18 */ "Machine-Check Exception",
	/* 19 */ "SIMD Floating-Point Exception",
};


struct interrupt_frame {
    uint32 vector;
    uint32 edi;
    uint32 esi;
    uint32 ebp;
    uint32 esp;
    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;
	uint32 error_code;
	uint32 eip;
	uint32 cs;
	uint32 eflags;
};


static interrupt_descriptor sDebugIDT[DEBUG_IDT_SLOT_COUNT];

static gdt_idt_descr sBIOSIDTDescriptor;
static gdt_idt_descr sDebugIDTDescriptor = { sizeof(sDebugIDT) - 1, sDebugIDT };


static void
set_interrupt_gate(int n, void (*function)())
{
	if (n >= DEBUG_IDT_SLOT_COUNT)
		return;

	sDebugIDT[n].a = (KERNEL_CODE_SEG << 16) | (0x0000ffff & (addr_t)function);
	sDebugIDT[n].b = (0xffff0000 & (addr_t)function) | 0x8e00;
}


static void
set_idt(const gdt_idt_descr& idtDescriptor)
{
	asm("lidt	%0;" : : "m" (idtDescriptor));
}


extern "C" void
handle_exception_exception()
{
	while (true);
}


extern "C" void
handle_exception(interrupt_frame frame)
{
	// Before doing anything else, set the interrupt gates so that
	// handle_exception_exception() is called. This way we may avoid
	// triple-faulting, if e.g. the stack is messed up and the user has at
	// least the already printed output.
	#define INTERRUPT_FUNCTION(vector) \
		set_interrupt_gate(vector, &exception_interrupt_function##vector);
	INTERRUPT_FUNCTIONS()
	#undef INTERRUPT_FUNCTION

	// If the console is already/still initialized, clear the screen and prepare
	// for output.
	if (stdout != NULL) {
		console_set_color(RED, BLACK);
		console_clear_screen();
		console_set_cursor(0, 0);
		console_hide_cursor();
	}

	// print exception name
	if (frame.vector < DEBUG_IDT_SLOT_COUNT)
		kprintf("%s", kInterruptNames[frame.vector]);
	else
		kprintf("Unknown exception %" B_PRIu32, frame.vector);

	// additional info for page fault
	if (frame.vector == 14) {
		uint32 cr2;
		asm("movl %%cr2, %0" : "=r"(cr2));
		kprintf(": %s fault at address: %#" B_PRIx32,
			(frame.error_code & 0x2) != 0 ? "write" : "read", cr2);
	}

	kprintf("\n");

	// print greeting and registers
	kprintf("Welcome to Boot Loader Death Land!\n");
	kprintf("\n");

	#define REG(name)	" " #name " %-#10" B_PRIx32

	kprintf(REG(eax) "   " REG(ebx) "    " REG(ecx) " " REG(edx) "\n",
		frame.eax, frame.ebx, frame.ecx, frame.edx);
	kprintf(REG(esi) "   " REG(edi) "    " REG(ebp) " " REG(esp) "\n",
		frame.esi, frame.edi, frame.ebp, frame.esp);
	kprintf(REG(eip) REG(eflags) "\n", frame.eip, frame.eflags);

	#undef REG

	// print stack trace (max 10 frames)
	kprintf("\n       frame    return address\n");

	struct x86_stack_frame {
		x86_stack_frame*	next;
		void*				return_address;
	};

	x86_stack_frame* stackFrame = (x86_stack_frame*)frame.ebp;
	void* instructionPointer = (void*)frame.eip;

	for (int32 i = 0; i < 10 && stackFrame != NULL; i++) {
		kprintf("  %p        %p\n", stackFrame, instructionPointer);

		instructionPointer = stackFrame->return_address;
		stackFrame = stackFrame->next;
	}

	if (stdout != NULL) {
		kprintf("\nPress a key to reboot.\n");
		clear_key_buffer();
		wait_for_key();
		platform_exit();
	}

	while (true);
}


void
interrupts_init()
{
	// get the IDT
	asm("sidt	%0;" : : "m" (sBIOSIDTDescriptor));

	TRACE("IDT: base: %p, limit: %u\n", sBIOSIDTDescriptor.base,
		sBIOSIDTDescriptor.limit);

	// set interrupt gates
	#define INTERRUPT_FUNCTION(vector) \
		set_interrupt_gate(vector, &interrupt_function##vector);
	INTERRUPT_FUNCTIONS()
	#undef INTERRUPT_FUNCTION

	// set the debug IDT
	set_debug_idt();
}


void
restore_bios_idt()
{
	set_idt(sBIOSIDTDescriptor);
}


void
set_debug_idt()
{
	set_idt(sDebugIDTDescriptor);
}


void
interrupts_init_kernel_idt(void* idt, size_t idtSize)
{
	// clear it but copy the descriptors we've set up for the exceptions
	memset(idt, 0, idtSize);
	memcpy(idt, sDebugIDT, sizeof(sDebugIDT));

	// load the idt
	gdt_idt_descr idtDescriptor;
	idtDescriptor.limit = idtSize - 1;
	idtDescriptor.base = idt;
	set_idt(idtDescriptor);
}
