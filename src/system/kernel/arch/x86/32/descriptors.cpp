/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2008-2011, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/x86/descriptors.h>

#include <stdio.h>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <int.h>
#include <tls.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/int.h>
#include <arch/user_debugger.h>

#include "interrupts.h"


#define IDT_GATES_COUNT	256


typedef interrupt_descriptor interrupt_descriptor_table[IDT_GATES_COUNT];

global_descriptor_table gGDTs[SMP_MAX_CPUS];
static interrupt_descriptor_table sIDTs[SMP_MAX_CPUS];

// table with functions handling respective interrupts
typedef void interrupt_handler_function(struct iframe* frame);

static const uint32 kInterruptHandlerTableSize = IDT_GATES_COUNT;
interrupt_handler_function* gInterruptHandlerTable[kInterruptHandlerTableSize];


/*!	Initializes a descriptor in an IDT.
*/
static void
set_gate(interrupt_descriptor *gate_addr, addr_t addr, int type, int dpl)
{
	unsigned int gate1; // first byte of gate desc
	unsigned int gate2; // second byte of gate desc

	gate1 = (KERNEL_CODE_SELECTOR << 16) | (0x0000ffff & addr);
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
static void
set_task_gate(int32 cpu, int32 n, int32 segment)
{
	sIDTs[cpu][n].a = (segment << 16);
	sIDTs[cpu][n].b = 0x8000 | (0 << 13) | (0x5 << 8); // present, dpl 0, type 5
}


static inline void
load_tss()
{
	uint16 segment = (TSS_SEGMENT << 3) | DPL_KERNEL;
	asm("ltr %w0" : : "r" (segment));
}


static inline void
load_gdt(int cpu)
{
	struct {
		uint16	limit;
		void*	address;
	} _PACKED gdtDescriptor = {
		GDT_SEGMENT_COUNT * sizeof(segment_descriptor) - 1,
		gGDTs[cpu]
	};

	asm volatile("lgdt %0" : : "m" (gdtDescriptor));
}


static inline void
load_idt(int cpu)
{
	struct {
		uint16	limit;
		void*	address;
	} _PACKED idtDescriptor = {
		IDT_GATES_COUNT * sizeof(interrupt_descriptor) - 1,
		&sIDTs[cpu]
	};

	asm volatile("lidt %0" : : "m" (idtDescriptor));
}


//	#pragma mark - Double fault handling


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


static void
init_double_fault(int cpuNum)
{
	// set up the double fault TSS
	struct tss* tss = &gCPU[cpuNum].arch.double_fault_tss;

	memset(tss, 0, sizeof(struct tss));
	size_t stackSize = 0;
	//tss->sp0 = (addr_t)x86_get_double_fault_stack(cpuNum, &stackSize);
	tss->sp0 += stackSize;
	tss->ss0 = KERNEL_DATA_SELECTOR;
	tss->cr3 = x86_read_cr3();
		// copy the current cr3 to the double fault cr3
	tss->eip = (uint32)&double_fault;
	tss->es = KERNEL_DATA_SELECTOR;
	tss->cs = KERNEL_CODE_SELECTOR;
	tss->ss = KERNEL_DATA_SELECTOR;
	tss->esp = tss->sp0;
	tss->ds = KERNEL_DATA_SELECTOR;
	tss->fs = KERNEL_DATA_SELECTOR;
	tss->gs = KERNEL_DATA_SELECTOR;
	tss->ldt_seg_selector = 0;
	tss->io_map_base = sizeof(struct tss);

	// add TSS descriptor for this new TSS
	set_tss_descriptor(&gGDTs[cpuNum][DOUBLE_FAULT_TSS_SEGMENT], (addr_t)tss,
		sizeof(struct tss));

	set_task_gate(cpuNum, 8, DOUBLE_FAULT_TSS_SEGMENT << 3);
}


static void
init_gdt_percpu(kernel_args* args, int cpu)
{
	STATIC_ASSERT(GDT_SEGMENT_COUNT <= 8192);

	segment_descriptor* gdt = get_gdt(cpu);

	clear_segment_descriptor(&gdt[0]);

	set_segment_descriptor(&gdt[KERNEL_CODE_SEGMENT], 0, addr_t(-1),
		DT_CODE_READABLE, DPL_KERNEL);
	set_segment_descriptor(&gdt[KERNEL_DATA_SEGMENT], 0, addr_t(-1),
		DT_DATA_WRITEABLE, DPL_KERNEL);

	set_segment_descriptor(&gdt[USER_CODE_SEGMENT], 0, addr_t(-1),
		DT_CODE_READABLE, DPL_USER);
	set_segment_descriptor(&gdt[USER_DATA_SEGMENT], 0, addr_t(-1),
		DT_DATA_WRITEABLE, DPL_USER);

	// initialize the regular and double fault tss stored in the per-cpu
	// structure
	memset(&gCPU[cpu].arch.tss, 0, sizeof(struct tss));
	gCPU[cpu].arch.tss.ss0 = (KERNEL_DATA_SEGMENT << 3) | DPL_KERNEL;
	gCPU[cpu].arch.tss.io_map_base = sizeof(struct tss);

	// add TSS descriptor for this new TSS
	set_tss_descriptor(&gdt[TSS_SEGMENT], (addr_t)&gCPU[cpu].arch.tss,
		sizeof(struct tss));

	// initialize the double fault tss
	init_double_fault(cpu);

	set_segment_descriptor(&gdt[KERNEL_TLS_SEGMENT],
		(addr_t)&gCPU[cpu].arch.kernel_tls, sizeof(void*), DT_DATA_WRITEABLE,
		DPL_KERNEL);
	set_segment_descriptor(&gdt[USER_TLS_SEGMENT], 0, TLS_SIZE,
		DT_DATA_WRITEABLE, DPL_USER);

	load_gdt(cpu);

	load_tss();

	// set kernel TLS segment
	asm volatile("movw %w0, %%gs" : : "r" (KERNEL_TLS_SEGMENT << 3));
}


static void
init_idt_percpu(kernel_args* args, int cpu)
{
	set_interrupt_gate(cpu, 0, &trap0);
	set_interrupt_gate(cpu, 1, &trap1);
	set_interrupt_gate(cpu, 2, &trap2);
	set_trap_gate(cpu, 3, &trap3);
	set_interrupt_gate(cpu, 4, &trap4);
	set_interrupt_gate(cpu, 5, &trap5);
	set_interrupt_gate(cpu, 6, &trap6);
	set_interrupt_gate(cpu, 7, &trap7);
	// trap8 (double fault) is set in init_double_fault().
	set_interrupt_gate(cpu, 9, &trap9);
	set_interrupt_gate(cpu, 10, &trap10);
	set_interrupt_gate(cpu, 11, &trap11);
	set_interrupt_gate(cpu, 12, &trap12);
	set_interrupt_gate(cpu, 13, &trap13);
	set_interrupt_gate(cpu, 14, &trap14);
	//set_interrupt_gate(cpu, 15, &trap15);
	set_interrupt_gate(cpu, 16, &trap16);
	set_interrupt_gate(cpu, 17, &trap17);
	set_interrupt_gate(cpu, 18, &trap18);
	set_interrupt_gate(cpu, 19, &trap19);

	// legacy or ioapic interrupts
	set_interrupt_gate(cpu, 32, &trap32);
	set_interrupt_gate(cpu, 33, &trap33);
	set_interrupt_gate(cpu, 34, &trap34);
	set_interrupt_gate(cpu, 35, &trap35);
	set_interrupt_gate(cpu, 36, &trap36);
	set_interrupt_gate(cpu, 37, &trap37);
	set_interrupt_gate(cpu, 38, &trap38);
	set_interrupt_gate(cpu, 39, &trap39);
	set_interrupt_gate(cpu, 40, &trap40);
	set_interrupt_gate(cpu, 41, &trap41);
	set_interrupt_gate(cpu, 42, &trap42);
	set_interrupt_gate(cpu, 43, &trap43);
	set_interrupt_gate(cpu, 44, &trap44);
	set_interrupt_gate(cpu, 45, &trap45);
	set_interrupt_gate(cpu, 46, &trap46);
	set_interrupt_gate(cpu, 47, &trap47);

	// additional ioapic interrupts
	set_interrupt_gate(cpu, 48, &trap48);
	set_interrupt_gate(cpu, 49, &trap49);
	set_interrupt_gate(cpu, 50, &trap50);
	set_interrupt_gate(cpu, 51, &trap51);
	set_interrupt_gate(cpu, 52, &trap52);
	set_interrupt_gate(cpu, 53, &trap53);
	set_interrupt_gate(cpu, 54, &trap54);
	set_interrupt_gate(cpu, 55, &trap55);

	// configurable msi or msi-x interrupts
	set_interrupt_gate(cpu, 56, &trap56);
	set_interrupt_gate(cpu, 57, &trap57);
	set_interrupt_gate(cpu, 58, &trap58);
	set_interrupt_gate(cpu, 59, &trap59);
	set_interrupt_gate(cpu, 60, &trap60);
	set_interrupt_gate(cpu, 61, &trap61);
	set_interrupt_gate(cpu, 62, &trap62);
	set_interrupt_gate(cpu, 63, &trap63);
	set_interrupt_gate(cpu, 64, &trap64);
	set_interrupt_gate(cpu, 65, &trap65);
	set_interrupt_gate(cpu, 66, &trap66);
	set_interrupt_gate(cpu, 67, &trap67);
	set_interrupt_gate(cpu, 68, &trap68);
	set_interrupt_gate(cpu, 69, &trap69);
	set_interrupt_gate(cpu, 70, &trap70);
	set_interrupt_gate(cpu, 71, &trap71);
	set_interrupt_gate(cpu, 72, &trap72);
	set_interrupt_gate(cpu, 73, &trap73);
	set_interrupt_gate(cpu, 74, &trap74);
	set_interrupt_gate(cpu, 75, &trap75);
	set_interrupt_gate(cpu, 76, &trap76);
	set_interrupt_gate(cpu, 77, &trap77);
	set_interrupt_gate(cpu, 78, &trap78);
	set_interrupt_gate(cpu, 79, &trap79);
	set_interrupt_gate(cpu, 80, &trap80);
	set_interrupt_gate(cpu, 81, &trap81);
	set_interrupt_gate(cpu, 82, &trap82);
	set_interrupt_gate(cpu, 83, &trap83);
	set_interrupt_gate(cpu, 84, &trap84);
	set_interrupt_gate(cpu, 85, &trap85);
	set_interrupt_gate(cpu, 86, &trap86);
	set_interrupt_gate(cpu, 87, &trap87);
	set_interrupt_gate(cpu, 88, &trap88);
	set_interrupt_gate(cpu, 89, &trap89);
	set_interrupt_gate(cpu, 90, &trap90);
	set_interrupt_gate(cpu, 91, &trap91);
	set_interrupt_gate(cpu, 92, &trap92);
	set_interrupt_gate(cpu, 93, &trap93);
	set_interrupt_gate(cpu, 94, &trap94);
	set_interrupt_gate(cpu, 95, &trap95);
	set_interrupt_gate(cpu, 96, &trap96);
	set_interrupt_gate(cpu, 97, &trap97);

	set_trap_gate(cpu, 98, &trap98);	// for performance testing only
	set_trap_gate(cpu, 99, &trap99);	// syscall interrupt

	// configurable msi or msi-x interrupts
	set_interrupt_gate(cpu, 100, &trap100);
	set_interrupt_gate(cpu, 101, &trap101);
	set_interrupt_gate(cpu, 102, &trap102);
	set_interrupt_gate(cpu, 103, &trap103);
	set_interrupt_gate(cpu, 104, &trap104);
	set_interrupt_gate(cpu, 105, &trap105);
	set_interrupt_gate(cpu, 106, &trap106);
	set_interrupt_gate(cpu, 107, &trap107);
	set_interrupt_gate(cpu, 108, &trap108);
	set_interrupt_gate(cpu, 109, &trap109);
	set_interrupt_gate(cpu, 110, &trap110);
	set_interrupt_gate(cpu, 111, &trap111);
	set_interrupt_gate(cpu, 112, &trap112);
	set_interrupt_gate(cpu, 113, &trap113);
	set_interrupt_gate(cpu, 114, &trap114);
	set_interrupt_gate(cpu, 115, &trap115);
	set_interrupt_gate(cpu, 116, &trap116);
	set_interrupt_gate(cpu, 117, &trap117);
	set_interrupt_gate(cpu, 118, &trap118);
	set_interrupt_gate(cpu, 119, &trap119);
	set_interrupt_gate(cpu, 120, &trap120);
	set_interrupt_gate(cpu, 121, &trap121);
	set_interrupt_gate(cpu, 122, &trap122);
	set_interrupt_gate(cpu, 123, &trap123);
	set_interrupt_gate(cpu, 124, &trap124);
	set_interrupt_gate(cpu, 125, &trap125);
	set_interrupt_gate(cpu, 126, &trap126);
	set_interrupt_gate(cpu, 127, &trap127);
	set_interrupt_gate(cpu, 128, &trap128);
	set_interrupt_gate(cpu, 129, &trap129);
	set_interrupt_gate(cpu, 130, &trap130);
	set_interrupt_gate(cpu, 131, &trap131);
	set_interrupt_gate(cpu, 132, &trap132);
	set_interrupt_gate(cpu, 133, &trap133);
	set_interrupt_gate(cpu, 134, &trap134);
	set_interrupt_gate(cpu, 135, &trap135);
	set_interrupt_gate(cpu, 136, &trap136);
	set_interrupt_gate(cpu, 137, &trap137);
	set_interrupt_gate(cpu, 138, &trap138);
	set_interrupt_gate(cpu, 139, &trap139);
	set_interrupt_gate(cpu, 140, &trap140);
	set_interrupt_gate(cpu, 141, &trap141);
	set_interrupt_gate(cpu, 142, &trap142);
	set_interrupt_gate(cpu, 143, &trap143);
	set_interrupt_gate(cpu, 144, &trap144);
	set_interrupt_gate(cpu, 145, &trap145);
	set_interrupt_gate(cpu, 146, &trap146);
	set_interrupt_gate(cpu, 147, &trap147);
	set_interrupt_gate(cpu, 148, &trap148);
	set_interrupt_gate(cpu, 149, &trap149);
	set_interrupt_gate(cpu, 150, &trap150);
	set_interrupt_gate(cpu, 151, &trap151);
	set_interrupt_gate(cpu, 152, &trap152);
	set_interrupt_gate(cpu, 153, &trap153);
	set_interrupt_gate(cpu, 154, &trap154);
	set_interrupt_gate(cpu, 155, &trap155);
	set_interrupt_gate(cpu, 156, &trap156);
	set_interrupt_gate(cpu, 157, &trap157);
	set_interrupt_gate(cpu, 158, &trap158);
	set_interrupt_gate(cpu, 159, &trap159);
	set_interrupt_gate(cpu, 160, &trap160);
	set_interrupt_gate(cpu, 161, &trap161);
	set_interrupt_gate(cpu, 162, &trap162);
	set_interrupt_gate(cpu, 163, &trap163);
	set_interrupt_gate(cpu, 164, &trap164);
	set_interrupt_gate(cpu, 165, &trap165);
	set_interrupt_gate(cpu, 166, &trap166);
	set_interrupt_gate(cpu, 167, &trap167);
	set_interrupt_gate(cpu, 168, &trap168);
	set_interrupt_gate(cpu, 169, &trap169);
	set_interrupt_gate(cpu, 170, &trap170);
	set_interrupt_gate(cpu, 171, &trap171);
	set_interrupt_gate(cpu, 172, &trap172);
	set_interrupt_gate(cpu, 173, &trap173);
	set_interrupt_gate(cpu, 174, &trap174);
	set_interrupt_gate(cpu, 175, &trap175);
	set_interrupt_gate(cpu, 176, &trap176);
	set_interrupt_gate(cpu, 177, &trap177);
	set_interrupt_gate(cpu, 178, &trap178);
	set_interrupt_gate(cpu, 179, &trap179);
	set_interrupt_gate(cpu, 180, &trap180);
	set_interrupt_gate(cpu, 181, &trap181);
	set_interrupt_gate(cpu, 182, &trap182);
	set_interrupt_gate(cpu, 183, &trap183);
	set_interrupt_gate(cpu, 184, &trap184);
	set_interrupt_gate(cpu, 185, &trap185);
	set_interrupt_gate(cpu, 186, &trap186);
	set_interrupt_gate(cpu, 187, &trap187);
	set_interrupt_gate(cpu, 188, &trap188);
	set_interrupt_gate(cpu, 189, &trap189);
	set_interrupt_gate(cpu, 190, &trap190);
	set_interrupt_gate(cpu, 191, &trap191);
	set_interrupt_gate(cpu, 192, &trap192);
	set_interrupt_gate(cpu, 193, &trap193);
	set_interrupt_gate(cpu, 194, &trap194);
	set_interrupt_gate(cpu, 195, &trap195);
	set_interrupt_gate(cpu, 196, &trap196);
	set_interrupt_gate(cpu, 197, &trap197);
	set_interrupt_gate(cpu, 198, &trap198);
	set_interrupt_gate(cpu, 199, &trap199);
	set_interrupt_gate(cpu, 200, &trap200);
	set_interrupt_gate(cpu, 201, &trap201);
	set_interrupt_gate(cpu, 202, &trap202);
	set_interrupt_gate(cpu, 203, &trap203);
	set_interrupt_gate(cpu, 204, &trap204);
	set_interrupt_gate(cpu, 205, &trap205);
	set_interrupt_gate(cpu, 206, &trap206);
	set_interrupt_gate(cpu, 207, &trap207);
	set_interrupt_gate(cpu, 208, &trap208);
	set_interrupt_gate(cpu, 209, &trap209);
	set_interrupt_gate(cpu, 210, &trap210);
	set_interrupt_gate(cpu, 211, &trap211);
	set_interrupt_gate(cpu, 212, &trap212);
	set_interrupt_gate(cpu, 213, &trap213);
	set_interrupt_gate(cpu, 214, &trap214);
	set_interrupt_gate(cpu, 215, &trap215);
	set_interrupt_gate(cpu, 216, &trap216);
	set_interrupt_gate(cpu, 217, &trap217);
	set_interrupt_gate(cpu, 218, &trap218);
	set_interrupt_gate(cpu, 219, &trap219);
	set_interrupt_gate(cpu, 220, &trap220);
	set_interrupt_gate(cpu, 221, &trap221);
	set_interrupt_gate(cpu, 222, &trap222);
	set_interrupt_gate(cpu, 223, &trap223);
	set_interrupt_gate(cpu, 224, &trap224);
	set_interrupt_gate(cpu, 225, &trap225);
	set_interrupt_gate(cpu, 226, &trap226);
	set_interrupt_gate(cpu, 227, &trap227);
	set_interrupt_gate(cpu, 228, &trap228);
	set_interrupt_gate(cpu, 229, &trap229);
	set_interrupt_gate(cpu, 230, &trap230);
	set_interrupt_gate(cpu, 231, &trap231);
	set_interrupt_gate(cpu, 232, &trap232);
	set_interrupt_gate(cpu, 233, &trap233);
	set_interrupt_gate(cpu, 234, &trap234);
	set_interrupt_gate(cpu, 235, &trap235);
	set_interrupt_gate(cpu, 236, &trap236);
	set_interrupt_gate(cpu, 237, &trap237);
	set_interrupt_gate(cpu, 238, &trap238);
	set_interrupt_gate(cpu, 239, &trap239);
	set_interrupt_gate(cpu, 240, &trap240);
	set_interrupt_gate(cpu, 241, &trap241);
	set_interrupt_gate(cpu, 242, &trap242);
	set_interrupt_gate(cpu, 243, &trap243);
	set_interrupt_gate(cpu, 244, &trap244);
	set_interrupt_gate(cpu, 245, &trap245);
	set_interrupt_gate(cpu, 246, &trap246);
	set_interrupt_gate(cpu, 247, &trap247);
	set_interrupt_gate(cpu, 248, &trap248);
	set_interrupt_gate(cpu, 249, &trap249);
	set_interrupt_gate(cpu, 250, &trap250);

	// smp / apic local interrupts
	set_interrupt_gate(cpu, 251, &trap251);
	set_interrupt_gate(cpu, 252, &trap252);
	set_interrupt_gate(cpu, 253, &trap253);
	set_interrupt_gate(cpu, 254, &trap254);
	set_interrupt_gate(cpu, 255, &trap255);

	load_idt(cpu);
}


//	#pragma mark -


void
x86_descriptors_preboot_init_percpu(kernel_args* args, int cpu)
{
	init_idt_percpu(args, cpu);
	init_gdt_percpu(args, cpu);
}


void
x86_descriptors_init(kernel_args* args)
{
	reserve_io_interrupt_vectors(2, 98, INTERRUPT_TYPE_SYSCALL);

	// init interrupt handler table
	interrupt_handler_function** table = gInterruptHandlerTable;

	// defaults
	uint32 i;
	for (i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = x86_invalid_exception;
	for (i = ARCH_INTERRUPT_BASE; i < kInterruptHandlerTableSize; i++)
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
}

