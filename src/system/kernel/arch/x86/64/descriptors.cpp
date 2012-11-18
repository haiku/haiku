/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/x86/descriptors.h>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/int.h>
#include <arch/user_debugger.h>


typedef void interrupt_handler_function(iframe* frame);


static segment_descriptor* sGDT;
static interrupt_descriptor* sIDT;

static const uint32 kInterruptHandlerTableSize = 256;
interrupt_handler_function* gInterruptHandlerTable[kInterruptHandlerTableSize];

extern uint8 isr_array[kInterruptHandlerTableSize][16];


static void
load_tss(int cpu)
{
	uint16 seg = (TSS_SEGMENT(cpu) << 3) | DPL_KERNEL;
	asm volatile("ltr %%ax" :: "a" (seg));
}


//	#pragma mark - Exception handlers


static void
x86_64_general_protection_fault(iframe* frame)
{
	if (debug_debugger_running()) {
		// Handle GPFs if there is a debugger fault handler installed, for
		// non-canonical address accesses.
		cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
		if (cpu->fault_handler != 0) {
			debug_set_page_fault_info(0, frame->ip, DEBUG_PAGE_FAULT_NO_INFO);
			frame->ip = cpu->fault_handler;
			frame->bp = cpu->fault_handler_stack_pointer;
			return;
		}
	}

	x86_unexpected_exception(frame);
}


// #pragma mark -


void
x86_descriptors_init(kernel_args* args)
{
	// The boot loader sets up a GDT and allocates an empty IDT for us.
	sGDT = (segment_descriptor*)args->arch_args.vir_gdt;
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
		// fault stack address (set up in x86_descriptors_init_post_vm()).
		uint32 ist = (i == 8) ? 1 : 0;

		// Breakpoint exception can be raised from userland.
		uint32 dpl = (i == 3) ? DPL_USER : DPL_KERNEL;

		set_interrupt_descriptor(&sIDT[i], (addr_t)&isr_array[i],
			GATE_INTERRUPT, KERNEL_CODE_SEG, dpl, ist);
	}

	// Initialize the interrupt handler table.
	interrupt_handler_function** table = gInterruptHandlerTable;
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
	table[13] = x86_64_general_protection_fault; // General Protection Exception (#GP)
	table[14] = x86_page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = x86_unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = x86_unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = x86_fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = x86_unexpected_exception;	// SIMD Floating-Point Exception (#XF)
}


void
x86_descriptors_init_percpu(kernel_args* args, int cpu)
{
	// Load the IDT.
	gdt_idt_descr idtr = {
		256 * sizeof(interrupt_descriptor) - 1,
		(addr_t)sIDT
	};
	asm volatile("lidt %0" :: "m" (idtr));

	// Load the TSS for non-boot CPUs (boot CPU gets done below).
	if (cpu != 0) {
		load_tss(cpu);
	}
}


status_t
x86_descriptors_init_post_vm(kernel_args* args)
{
	area_id area;

	// Create an area for the GDT.
	area = create_area("gdt", (void**)&sGDT, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	// Same for the IDT.
	area = create_area("idt", (void**)&sIDT, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	for (uint32 i = 0; i < args->num_cpus; i++) {
		// Set up the task state segment.
		memset(&gCPU[i].arch.tss, 0, sizeof(struct tss));
		gCPU[i].arch.tss.io_map_base = sizeof(struct tss);

		// Set up the descriptor for this TSS.
		set_tss_descriptor(&sGDT[TSS_SEGMENT(i)], (addr_t)&gCPU[i].arch.tss,
			sizeof(struct tss));

		// Set up the double fault IST entry (see x86_descriptors_init()).
		struct tss* tss = &gCPU[i].arch.tss;
		size_t stackSize;
		tss->ist1 = (addr_t)x86_get_double_fault_stack(i, &stackSize);
		tss->ist1 += stackSize;
	}

	// Load the TSS for the boot CPU.
	load_tss(0);

	return B_OK;
}
