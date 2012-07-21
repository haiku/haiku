/*
 * Copyright 2008-2011, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <int.h>

#include <stdio.h>

#include <cpu.h>
#include <smp.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/cpu.h>
#include <arch/int.h>
#include <arch/smp.h>
#include <arch/user_debugger.h>
#include <arch/vm.h>

#include <arch/x86/apic.h>
#include <arch/x86/descriptors.h>
#include <arch/x86/pic.h>
#include <arch/x86/vm86.h>

#include "interrupts.h"


static interrupt_descriptor* sIDTs[B_MAX_CPU_COUNT];

// table with functions handling respective interrupts
typedef void interrupt_handler_function(struct iframe* frame);

#define INTERRUPT_HANDLER_TABLE_SIZE 256
interrupt_handler_function* gInterruptHandlerTable[
	INTERRUPT_HANDLER_TABLE_SIZE];


/*!	Initializes a descriptor in an IDT.
*/
static void
set_gate(interrupt_descriptor *gate_addr, addr_t addr, int type, int dpl)
{
	unsigned int gate1; // first byte of gate desc
	unsigned int gate2; // second byte of gate desc

	gate1 = (KERNEL_CODE_SEG << 16) | (0x0000ffff & addr);
	gate2 = (0xffff0000 & addr) | 0x8000 | (dpl << 13) | (type << 8);

	gate_addr->a = gate1;
	gate_addr->b = gate2;
}


/*!	Initializes the descriptor for interrupt vector \a n in the IDT of the
	specified CPU to an interrupt-gate descriptor with the given procedure
	address.
	For CPUs other than the boot CPU it must not be called before
	arch_int_init_post_vm().
*/
static void
set_interrupt_gate(int32 cpu, int n, void (*addr)())
{
	set_gate(&sIDTs[cpu][n], (addr_t)addr, 14, DPL_KERNEL);
}


/*!	Initializes the descriptor for interrupt vector \a n in the IDT of the
	specified CPU to an trap-gate descriptor with the given procedure address.
	For CPUs other than the boot CPU it must not be called before
	arch_int_init_post_vm().
*/
static void
set_trap_gate(int32 cpu, int n, void (*addr)())
{
	set_gate(&sIDTs[cpu][n], (unsigned int)addr, 15, DPL_USER);
}


/*!	Initializes the descriptor for interrupt vector \a n in the IDT of CPU
	\a cpu to a task-gate descripter referring to the TSS segment identified
	by TSS segment selector \a segment.
	For CPUs other than the boot CPU it must not be called before
	arch_int_init_post_vm() (arch_cpu_init_post_vm() is fine).
*/
void
x86_set_task_gate(int32 cpu, int32 n, int32 segment)
{
	sIDTs[cpu][n].a = (segment << 16);
	sIDTs[cpu][n].b = 0x8000 | (0 << 13) | (0x5 << 8); // present, dpl 0, type 5
}


/*!	Returns the virtual IDT address for CPU \a cpu. */
void*
x86_get_idt(int32 cpu)
{
	return sIDTs[cpu];
}


// #pragma mark -


void
x86_double_fault_exception(struct iframe* frame)
{
	int cpu = x86_double_fault_get_cpu();

	// The double fault iframe contains no useful information (as
	// per Intel's architecture spec). Thus we simply save the
	// information from the (unhandlable) exception which caused the
	// double in our iframe. This will result even in useful stack
	// traces. Only problem is that we trust that at least the
	// TSS is still accessible.
	struct tss *tss = &gCPU[cpu].arch.tss;

	frame->cs = tss->cs;
	frame->es = tss->es;
	frame->ds = tss->ds;
	frame->fs = tss->fs;
	frame->gs = tss->gs;
	frame->ip = tss->eip;
	frame->bp = tss->ebp;
	frame->sp = tss->esp;
	frame->ax = tss->eax;
	frame->bx = tss->ebx;
	frame->cx = tss->ecx;
	frame->dx = tss->edx;
	frame->si = tss->esi;
	frame->di = tss->edi;
	frame->flags = tss->eflags;

	// Use a special handler for page faults which avoids the triple fault
	// pitfalls.
	set_interrupt_gate(cpu, 14, &trap14_double_fault);

	debug_double_fault(cpu);
}


void
x86_page_fault_exception_double_fault(struct iframe* frame)
{
	addr_t cr2 = x86_read_cr2();

	// Only if this CPU has a fault handler, we're allowed to be here.
	cpu_ent& cpu = gCPU[x86_double_fault_get_cpu()];
	addr_t faultHandler = cpu.fault_handler;
	if (faultHandler != 0) {
		debug_set_page_fault_info(cr2, frame->ip,
			(frame->error_code & 0x2) != 0 ? DEBUG_PAGE_FAULT_WRITE : 0);
		frame->ip = faultHandler;
		frame->bp = cpu.fault_handler_stack_pointer;
		return;
	}

	// No fault handler. This is bad. Since we originally came from a double
	// fault, we don't try to reenter the kernel debugger. Instead we just
	// print the info we've got and enter an infinite loop.
	kprintf("Page fault in double fault debugger without fault handler! "
		"Touching address %p from eip %p. Entering infinite loop...\n",
		(void*)cr2, (void*)frame->ip);

	while (true);
}


status_t
arch_int_init(struct kernel_args *args)
{
	int i;
	interrupt_handler_function** table;

	// set the global sIDT variable
	sIDTs[0] = (interrupt_descriptor *)(addr_t)args->arch_args.vir_idt;

	// setup the standard programmable interrupt controller
	pic_init();

	set_interrupt_gate(0, 0, &trap0);
	set_interrupt_gate(0, 1, &trap1);
	set_interrupt_gate(0, 2, &trap2);
	set_trap_gate(0, 3, &trap3);
	set_interrupt_gate(0, 4, &trap4);
	set_interrupt_gate(0, 5, &trap5);
	set_interrupt_gate(0, 6, &trap6);
	set_interrupt_gate(0, 7, &trap7);
	// trap8 (double fault) is set in arch_cpu.c
	set_interrupt_gate(0, 9, &trap9);
	set_interrupt_gate(0, 10, &trap10);
	set_interrupt_gate(0, 11, &trap11);
	set_interrupt_gate(0, 12, &trap12);
	set_interrupt_gate(0, 13, &trap13);
	set_interrupt_gate(0, 14, &trap14);
//	set_interrupt_gate(0, 15, &trap15);
	set_interrupt_gate(0, 16, &trap16);
	set_interrupt_gate(0, 17, &trap17);
	set_interrupt_gate(0, 18, &trap18);
	set_interrupt_gate(0, 19, &trap19);

	// legacy or ioapic interrupts
	set_interrupt_gate(0, 32, &trap32);
	set_interrupt_gate(0, 33, &trap33);
	set_interrupt_gate(0, 34, &trap34);
	set_interrupt_gate(0, 35, &trap35);
	set_interrupt_gate(0, 36, &trap36);
	set_interrupt_gate(0, 37, &trap37);
	set_interrupt_gate(0, 38, &trap38);
	set_interrupt_gate(0, 39, &trap39);
	set_interrupt_gate(0, 40, &trap40);
	set_interrupt_gate(0, 41, &trap41);
	set_interrupt_gate(0, 42, &trap42);
	set_interrupt_gate(0, 43, &trap43);
	set_interrupt_gate(0, 44, &trap44);
	set_interrupt_gate(0, 45, &trap45);
	set_interrupt_gate(0, 46, &trap46);
	set_interrupt_gate(0, 47, &trap47);

	// additional ioapic interrupts
	set_interrupt_gate(0, 48, &trap48);
	set_interrupt_gate(0, 49, &trap49);
	set_interrupt_gate(0, 50, &trap50);
	set_interrupt_gate(0, 51, &trap51);
	set_interrupt_gate(0, 52, &trap52);
	set_interrupt_gate(0, 53, &trap53);
	set_interrupt_gate(0, 54, &trap54);
	set_interrupt_gate(0, 55, &trap55);

	// configurable msi or msi-x interrupts
	set_interrupt_gate(0, 56, &trap56);
	set_interrupt_gate(0, 57, &trap57);
	set_interrupt_gate(0, 58, &trap58);
	set_interrupt_gate(0, 59, &trap59);
	set_interrupt_gate(0, 60, &trap60);
	set_interrupt_gate(0, 61, &trap61);
	set_interrupt_gate(0, 62, &trap62);
	set_interrupt_gate(0, 63, &trap63);
	set_interrupt_gate(0, 64, &trap64);
	set_interrupt_gate(0, 65, &trap65);
	set_interrupt_gate(0, 66, &trap66);
	set_interrupt_gate(0, 67, &trap67);
	set_interrupt_gate(0, 68, &trap68);
	set_interrupt_gate(0, 69, &trap69);
	set_interrupt_gate(0, 70, &trap70);
	set_interrupt_gate(0, 71, &trap71);
	set_interrupt_gate(0, 72, &trap72);
	set_interrupt_gate(0, 73, &trap73);
	set_interrupt_gate(0, 74, &trap74);
	set_interrupt_gate(0, 75, &trap75);
	set_interrupt_gate(0, 76, &trap76);
	set_interrupt_gate(0, 77, &trap77);
	set_interrupt_gate(0, 78, &trap78);
	set_interrupt_gate(0, 79, &trap79);
	set_interrupt_gate(0, 80, &trap80);
	set_interrupt_gate(0, 81, &trap81);
	set_interrupt_gate(0, 82, &trap82);
	set_interrupt_gate(0, 83, &trap83);
	set_interrupt_gate(0, 84, &trap84);
	set_interrupt_gate(0, 85, &trap85);
	set_interrupt_gate(0, 86, &trap86);
	set_interrupt_gate(0, 87, &trap87);
	set_interrupt_gate(0, 88, &trap88);
	set_interrupt_gate(0, 89, &trap89);
	set_interrupt_gate(0, 90, &trap90);
	set_interrupt_gate(0, 91, &trap91);
	set_interrupt_gate(0, 92, &trap92);
	set_interrupt_gate(0, 93, &trap93);
	set_interrupt_gate(0, 94, &trap94);
	set_interrupt_gate(0, 95, &trap95);
	set_interrupt_gate(0, 96, &trap96);
	set_interrupt_gate(0, 97, &trap97);

	set_trap_gate(0, 98, &trap98);	// for performance testing only
	set_trap_gate(0, 99, &trap99);	// syscall interrupt

	reserve_io_interrupt_vectors(2, 98);

	// configurable msi or msi-x interrupts
	set_interrupt_gate(0, 100, &trap100);
	set_interrupt_gate(0, 101, &trap101);
	set_interrupt_gate(0, 102, &trap102);
	set_interrupt_gate(0, 103, &trap103);
	set_interrupt_gate(0, 104, &trap104);
	set_interrupt_gate(0, 105, &trap105);
	set_interrupt_gate(0, 106, &trap106);
	set_interrupt_gate(0, 107, &trap107);
	set_interrupt_gate(0, 108, &trap108);
	set_interrupt_gate(0, 109, &trap109);
	set_interrupt_gate(0, 110, &trap110);
	set_interrupt_gate(0, 111, &trap111);
	set_interrupt_gate(0, 112, &trap112);
	set_interrupt_gate(0, 113, &trap113);
	set_interrupt_gate(0, 114, &trap114);
	set_interrupt_gate(0, 115, &trap115);
	set_interrupt_gate(0, 116, &trap116);
	set_interrupt_gate(0, 117, &trap117);
	set_interrupt_gate(0, 118, &trap118);
	set_interrupt_gate(0, 119, &trap119);
	set_interrupt_gate(0, 120, &trap120);
	set_interrupt_gate(0, 121, &trap121);
	set_interrupt_gate(0, 122, &trap122);
	set_interrupt_gate(0, 123, &trap123);
	set_interrupt_gate(0, 124, &trap124);
	set_interrupt_gate(0, 125, &trap125);
	set_interrupt_gate(0, 126, &trap126);
	set_interrupt_gate(0, 127, &trap127);
	set_interrupt_gate(0, 128, &trap128);
	set_interrupt_gate(0, 129, &trap129);
	set_interrupt_gate(0, 130, &trap130);
	set_interrupt_gate(0, 131, &trap131);
	set_interrupt_gate(0, 132, &trap132);
	set_interrupt_gate(0, 133, &trap133);
	set_interrupt_gate(0, 134, &trap134);
	set_interrupt_gate(0, 135, &trap135);
	set_interrupt_gate(0, 136, &trap136);
	set_interrupt_gate(0, 137, &trap137);
	set_interrupt_gate(0, 138, &trap138);
	set_interrupt_gate(0, 139, &trap139);
	set_interrupt_gate(0, 140, &trap140);
	set_interrupt_gate(0, 141, &trap141);
	set_interrupt_gate(0, 142, &trap142);
	set_interrupt_gate(0, 143, &trap143);
	set_interrupt_gate(0, 144, &trap144);
	set_interrupt_gate(0, 145, &trap145);
	set_interrupt_gate(0, 146, &trap146);
	set_interrupt_gate(0, 147, &trap147);
	set_interrupt_gate(0, 148, &trap148);
	set_interrupt_gate(0, 149, &trap149);
	set_interrupt_gate(0, 150, &trap150);
	set_interrupt_gate(0, 151, &trap151);
	set_interrupt_gate(0, 152, &trap152);
	set_interrupt_gate(0, 153, &trap153);
	set_interrupt_gate(0, 154, &trap154);
	set_interrupt_gate(0, 155, &trap155);
	set_interrupt_gate(0, 156, &trap156);
	set_interrupt_gate(0, 157, &trap157);
	set_interrupt_gate(0, 158, &trap158);
	set_interrupt_gate(0, 159, &trap159);
	set_interrupt_gate(0, 160, &trap160);
	set_interrupt_gate(0, 161, &trap161);
	set_interrupt_gate(0, 162, &trap162);
	set_interrupt_gate(0, 163, &trap163);
	set_interrupt_gate(0, 164, &trap164);
	set_interrupt_gate(0, 165, &trap165);
	set_interrupt_gate(0, 166, &trap166);
	set_interrupt_gate(0, 167, &trap167);
	set_interrupt_gate(0, 168, &trap168);
	set_interrupt_gate(0, 169, &trap169);
	set_interrupt_gate(0, 170, &trap170);
	set_interrupt_gate(0, 171, &trap171);
	set_interrupt_gate(0, 172, &trap172);
	set_interrupt_gate(0, 173, &trap173);
	set_interrupt_gate(0, 174, &trap174);
	set_interrupt_gate(0, 175, &trap175);
	set_interrupt_gate(0, 176, &trap176);
	set_interrupt_gate(0, 177, &trap177);
	set_interrupt_gate(0, 178, &trap178);
	set_interrupt_gate(0, 179, &trap179);
	set_interrupt_gate(0, 180, &trap180);
	set_interrupt_gate(0, 181, &trap181);
	set_interrupt_gate(0, 182, &trap182);
	set_interrupt_gate(0, 183, &trap183);
	set_interrupt_gate(0, 184, &trap184);
	set_interrupt_gate(0, 185, &trap185);
	set_interrupt_gate(0, 186, &trap186);
	set_interrupt_gate(0, 187, &trap187);
	set_interrupt_gate(0, 188, &trap188);
	set_interrupt_gate(0, 189, &trap189);
	set_interrupt_gate(0, 190, &trap190);
	set_interrupt_gate(0, 191, &trap191);
	set_interrupt_gate(0, 192, &trap192);
	set_interrupt_gate(0, 193, &trap193);
	set_interrupt_gate(0, 194, &trap194);
	set_interrupt_gate(0, 195, &trap195);
	set_interrupt_gate(0, 196, &trap196);
	set_interrupt_gate(0, 197, &trap197);
	set_interrupt_gate(0, 198, &trap198);
	set_interrupt_gate(0, 199, &trap199);
	set_interrupt_gate(0, 200, &trap200);
	set_interrupt_gate(0, 201, &trap201);
	set_interrupt_gate(0, 202, &trap202);
	set_interrupt_gate(0, 203, &trap203);
	set_interrupt_gate(0, 204, &trap204);
	set_interrupt_gate(0, 205, &trap205);
	set_interrupt_gate(0, 206, &trap206);
	set_interrupt_gate(0, 207, &trap207);
	set_interrupt_gate(0, 208, &trap208);
	set_interrupt_gate(0, 209, &trap209);
	set_interrupt_gate(0, 210, &trap210);
	set_interrupt_gate(0, 211, &trap211);
	set_interrupt_gate(0, 212, &trap212);
	set_interrupt_gate(0, 213, &trap213);
	set_interrupt_gate(0, 214, &trap214);
	set_interrupt_gate(0, 215, &trap215);
	set_interrupt_gate(0, 216, &trap216);
	set_interrupt_gate(0, 217, &trap217);
	set_interrupt_gate(0, 218, &trap218);
	set_interrupt_gate(0, 219, &trap219);
	set_interrupt_gate(0, 220, &trap220);
	set_interrupt_gate(0, 221, &trap221);
	set_interrupt_gate(0, 222, &trap222);
	set_interrupt_gate(0, 223, &trap223);
	set_interrupt_gate(0, 224, &trap224);
	set_interrupt_gate(0, 225, &trap225);
	set_interrupt_gate(0, 226, &trap226);
	set_interrupt_gate(0, 227, &trap227);
	set_interrupt_gate(0, 228, &trap228);
	set_interrupt_gate(0, 229, &trap229);
	set_interrupt_gate(0, 230, &trap230);
	set_interrupt_gate(0, 231, &trap231);
	set_interrupt_gate(0, 232, &trap232);
	set_interrupt_gate(0, 233, &trap233);
	set_interrupt_gate(0, 234, &trap234);
	set_interrupt_gate(0, 235, &trap235);
	set_interrupt_gate(0, 236, &trap236);
	set_interrupt_gate(0, 237, &trap237);
	set_interrupt_gate(0, 238, &trap238);
	set_interrupt_gate(0, 239, &trap239);
	set_interrupt_gate(0, 240, &trap240);
	set_interrupt_gate(0, 241, &trap241);
	set_interrupt_gate(0, 242, &trap242);
	set_interrupt_gate(0, 243, &trap243);
	set_interrupt_gate(0, 244, &trap244);
	set_interrupt_gate(0, 245, &trap245);
	set_interrupt_gate(0, 246, &trap246);
	set_interrupt_gate(0, 247, &trap247);
	set_interrupt_gate(0, 248, &trap248);
	set_interrupt_gate(0, 249, &trap249);
	set_interrupt_gate(0, 250, &trap250);

	// smp / apic local interrupts
	set_interrupt_gate(0, 251, &trap251);
	set_interrupt_gate(0, 252, &trap252);
	set_interrupt_gate(0, 253, &trap253);
	set_interrupt_gate(0, 254, &trap254);
	set_interrupt_gate(0, 255, &trap255);

	// init interrupt handler table
	table = gInterruptHandlerTable;

	// defaults
	for (i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = x86_invalid_exception;
	for (i = ARCH_INTERRUPT_BASE; i < INTERRUPT_HANDLER_TABLE_SIZE; i++)
		table[i] = x86_hardware_interrupt;

	table[0] = x86_unexpected_exception;	// Divide Error Exception (#DE)
	table[1] = x86_handle_debug_exception;	// Debug Exception (#DB)
	table[2] = x86_fatal_exception;			// NMI Interrupt
	table[3] = x86_handle_breakpoint_exception; // Breakpoint Exception (#BP)
	table[4] = x86_unexpected_exception;	// Overflow Exception (#OF)
	table[5] = x86_unexpected_exception;	// BOUND Range Exceeded Exception (#BR)
	table[6] = x86_unexpected_exception;	// Invalid Opcode Exception (#UD)
	table[7] = x86_fatal_exception;			// Device Not Available Exception (#NM)
	table[8] = x86_double_fault_exception;	// Double Fault Exception (#DF)
	table[9] = x86_fatal_exception;			// Coprocessor Segment Overrun
	table[10] = x86_fatal_exception;		// Invalid TSS Exception (#TS)
	table[11] = x86_fatal_exception;		// Segment Not Present (#NP)
	table[12] = x86_fatal_exception;		// Stack Fault Exception (#SS)
	table[13] = x86_unexpected_exception;	// General Protection Exception (#GP)
	table[14] = x86_page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = x86_unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = x86_unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = x86_fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = x86_unexpected_exception;	// SIMD Floating-Point Exception (#XF)

	return B_OK;
}


status_t
arch_int_init_post_vm(struct kernel_args *args)
{
	// Always init the local apic as it can be used for timers even if we
	// don't end up using the io apic
	apic_init(args);

	// create IDT area for the boot CPU
	area_id area = create_area("idt", (void**)&sIDTs[0], B_EXACT_ADDRESS,
		B_PAGE_SIZE, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < 0)
		return area;

	// create IDTs for the off-boot CPU
	size_t idtSize = 256 * 8;
		// 256 8 bytes-sized descriptors
	int32 cpuCount = smp_get_num_cpus();
	if (cpuCount > 0) {
		size_t areaSize = ROUNDUP(cpuCount * idtSize, B_PAGE_SIZE);
		interrupt_descriptor* idt;
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		area = create_area_etc(B_SYSTEM_TEAM, "idt", areaSize, B_CONTIGUOUS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_WAIT,
			&virtualRestrictions, &physicalRestrictions, (void**)&idt);
		if (area < 0)
			return area;

		for (int32 i = 1; i < cpuCount; i++) {
			sIDTs[i] = idt;
			memcpy(idt, sIDTs[0], idtSize);
			idt += 256;
			// The CPU's IDTR will be set in arch_cpu_init_percpu().
		}
	}

	return area >= B_OK ? B_OK : area;
}
