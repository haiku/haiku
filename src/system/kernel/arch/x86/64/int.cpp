/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <int.h>

#include <stdio.h>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/x86/apic.h>
#include <arch/x86/descriptors.h>
#include <arch/x86/pic.h>


typedef void interrupt_handler_function(iframe* frame);


static const uint32 kInterruptHandlerTableSize = 256;

static interrupt_descriptor* sIDT;
interrupt_handler_function* gInterruptHandlerTable[kInterruptHandlerTableSize];

extern uint8 isr_array[kInterruptHandlerTableSize][16];


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

		// Breakpoint exception can be raised from userland.
		uint32 dpl = (i == 3) ? DPL_USER : DPL_KERNEL;

		set_interrupt_descriptor(&sIDT[i], (addr_t)&isr_array[i],
			GATE_INTERRUPT, KERNEL_CODE_SEG, dpl, ist);
	}

	interrupt_handler_function** table = gInterruptHandlerTable;

	// Initialize the interrupt handler table.
	for (uint32 i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = x86_invalid_exception;
	for (uint32 i = ARCH_INTERRUPT_BASE; i < kInterruptHandlerTableSize; i++)
		table[i] = x86_hardware_interrupt;

	table[0]  = x86_unexpected_exception;	// Divide Error Exception (#DE)
	table[1]  = x86_handle_debug_exception; // Debug Exception (#DB)
	table[2]  = x86_fatal_exception;		// NMI Interrupt
	table[3]  = x86_handle_breakpoint_exception; // Breakpoint Exception (#BP)
	table[4]  = x86_unexpected_exception;	// Overflow Exception (#OF)
	table[5]  = x86_unexpected_exception;	// BOUND Range Exceeded Exception (#BR)
	table[6]  = x86_unexpected_exception;	// Invalid Opcode Exception (#UD)
	table[7]  = x86_fatal_exception;		// Device Not Available Exception (#NM)
	table[8]  = x86_fatal_exception;		// Double Fault Exception (#DF)
	table[9]  = x86_fatal_exception;		// Coprocessor Segment Overrun
	table[10] = x86_fatal_exception;		// Invalid TSS Exception (#TS)
	table[11] = x86_fatal_exception;		// Segment Not Present (#NP)
	table[12] = x86_fatal_exception;		// Stack Fault Exception (#SS)
	table[13] = x86_unexpected_exception;	// General Protection Exception (#GP)
	table[14] = x86_page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = x86_unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = x86_unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = x86_fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = x86_unexpected_exception;	// SIMD Floating-Point Exception (#XF)

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
	apic_init(args);

	// Create an area for the IDT.
	area_id area = create_area("idt", (void**)&sIDT, B_EXACT_ADDRESS,
		B_PAGE_SIZE, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < 0)
		return area;

	return B_OK;
}
