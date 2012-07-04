/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <int.h>

#include <string.h>
#include <stdio.h>

#include <boot/kernel_args.h>
#include <cpu.h>

#include <arch/x86/apic.h>
#include <arch/x86/descriptors.h>
#include <arch/x86/pic.h>


typedef void interrupt_handler_function(iframe* frame);


static const char* kInterruptNames[] = {
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

static const uint32 kInterruptHandlerTableSize = 256;

static interrupt_descriptor* sIDT;
interrupt_handler_function* gInterruptHandlerTable[kInterruptHandlerTableSize];

extern uint8 isr_array[kInterruptHandlerTableSize][16];
extern void hardware_interrupt(struct iframe* frame);


static const char*
exception_name(uint64 number, char* buffer, size_t bufferSize)
{
	if (number >= 0
			&& number < (sizeof(kInterruptNames) / sizeof(kInterruptNames[0])))
		return kInterruptNames[number];

	snprintf(buffer, bufferSize, "exception %lu", number);
	return buffer;
}


static void
invalid_exception(iframe* frame)
{
	char name[32];
	panic("unhandled trap %#lx (%s) at ip %#lx\n",
		frame->vector, exception_name(frame->vector, name, sizeof(name)),
		frame->rip);
}


static void
fatal_exception(iframe* frame)
{
	char name[32];
	panic("fatal exception %#lx (%s) at ip %#lx, error code %#lx\n",
		frame->vector, exception_name(frame->vector, name, sizeof(name)),
		frame->rip, frame->error_code);
}


static void
unexpected_exception(iframe* frame)
{
	char name[32];
	panic("fatal exception %#lx (%s) at ip %#lx, error code %#lx\n",
		frame->vector, exception_name(frame->vector, name, sizeof(name)),
		frame->rip, frame->error_code);
}


static void
page_fault_exception(iframe* frame)
{
	addr_t cr2 = x86_read_cr2();

	panic("page fault exception at ip %#lx on %#lx, error code %#lx\n",
		frame->rip, cr2, frame->error_code);
}


/*!	Returns the virtual IDT address for CPU \a cpu. */
void*
x86_get_idt(int32 cpu)
{
	// We use a single IDT for all CPUs on x86_64.
	return sIDT;
}


// #pragma mark -


status_t
arch_int_init(kernel_args* args)
{
	// The loader allocates an empty IDT for us.
	sIDT = (interrupt_descriptor*)args->arch_args.vir_idt;

	// Fill out the IDT, pointing each entry to the corresponding entry in the
	// ISR array created in arch_interrupts.S (see there to see how this works).
	for(uint32 i = 0; i < kInterruptHandlerTableSize; i++) {
		// x86_64 removes task gates, therefore we cannot use a separate TSS
		// for the double fault exception. However, instead it adds a new stack
		// switching mechanism, the IST. The IST is a table of stack addresses
		// in the TSS. If the IST field of an interrupt descriptor is non-zero,
		// the CPU will switch to the stack specified by that IST entry when
		// handling that interrupt. So, we use IST entry 1 to store the double
		// fault stack address (this is set up in arch_cpu.cpp).
		uint32 ist = (i == 8) ? 1 : 0;

		set_interrupt_descriptor(&sIDT[i], (addr_t)&isr_array[i],
			GATE_INTERRUPT, KERNEL_CODE_SEG, DPL_KERNEL, ist);
	}

	interrupt_handler_function** table = gInterruptHandlerTable;

	// Initialize the interrupt handler table.
	for (uint32 i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = invalid_exception;
	for (uint32 i = ARCH_INTERRUPT_BASE; i < kInterruptHandlerTableSize; i++)
		table[i] = hardware_interrupt;

	table[0]  = unexpected_exception;	// Divide Error Exception (#DE)
	//table[1]  = x86_handle_debug_exception; // Debug Exception (#DB)
	table[1]  = unexpected_exception;
	table[2]  = fatal_exception;		// NMI Interrupt
	//table[3]  = x86_handle_breakpoint_exception; // Breakpoint Exception (#BP)
	table[3]  = unexpected_exception;
	table[4]  = unexpected_exception;	// Overflow Exception (#OF)
	table[5]  = unexpected_exception;	// BOUND Range Exceeded Exception (#BR)
	table[6]  = unexpected_exception;	// Invalid Opcode Exception (#UD)
	table[7]  = fatal_exception;		// Device Not Available Exception (#NM)
	table[8]  = fatal_exception;		// Double Fault Exception (#DF)
	table[9]  = fatal_exception;		// Coprocessor Segment Overrun
	table[10] = fatal_exception;		// Invalid TSS Exception (#TS)
	table[11] = fatal_exception;		// Segment Not Present (#NP)
	table[12] = fatal_exception;		// Stack Fault Exception (#SS)
	table[13] = unexpected_exception;	// General Protection Exception (#GP)
	table[14] = page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = unexpected_exception;	// SIMD Floating-Point Exception (#XF)

	// Load the IDT.
	gdt_idt_descr idtr = {
		256 * sizeof(interrupt_descriptor) - 1,
		(addr_t)sIDT
	};
	asm volatile("lidt %0" :: "m"(idtr));

	// Set up the legacy PIC.
	pic_init();

	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args* args)
{
	// Always init the local apic as it can be used for timers even if we
	// don't end up using the io apic
	//apic_init(args);

	// TODO: create area for IDT.

	return B_OK;
}
