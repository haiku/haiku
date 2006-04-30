/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <int.h>
#include <kscheduler.h>
#include <ksyscalls.h>
#include <smp.h>
#include <team.h>
#include <thread.h>
#include <vm.h>
#include <vm_priv.h>

#include <arch/cpu.h>
#include <arch/int.h>
#include <arch/smp.h>
#include <arch/user_debugger.h>
#include <arch/vm.h>

#include <arch/x86/descriptors.h>

#include "interrupts.h"

#include <string.h>
#include <stdio.h>


//#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// Definitions for the PIC 8259 controller
// (this is not a complete list, only what we're actually using)

#define PIC_MASTER_CONTROL		0x20
#define PIC_MASTER_MASK			0x21
#define PIC_SLAVE_CONTROL		0xa0
#define PIC_SLAVE_MASK			0xa1
#define PIC_MASTER_INIT1		PIC_MASTER_CONTROL
#define PIC_MASTER_INIT2		PIC_MASTER_MASK
#define PIC_MASTER_INIT3		PIC_MASTER_MASK
#define PIC_MASTER_INIT4		PIC_MASTER_MASK
#define PIC_SLAVE_INIT1			PIC_SLAVE_CONTROL
#define PIC_SLAVE_INIT2			PIC_SLAVE_MASK
#define PIC_SLAVE_INIT3			PIC_SLAVE_MASK
#define PIC_SLAVE_INIT4			PIC_SLAVE_MASK

// the edge/level trigger control registers
#define PIC_MASTER_TRIGGER_MODE	0x4d0
#define PIC_SLAVE_TRIGGER_MODE	0x4d1

#define PIC_INIT1				0x10
#define PIC_INIT1_SEND_INIT4	0x01
#define PIC_INIT3_IR2_IS_SLAVE	0x04
#define PIC_INIT3_SLAVE_ID2		0x02
#define PIC_INIT4_x86_MODE		0x01

#define PIC_CONTROL3			0x08
#define PIC_CONTROL3_READ_ISR	0x03
#define PIC_CONTROL3_READ_IRR	0x02

#define PIC_NON_SPECIFIC_EOI	0x20

#define PIC_INT_BASE			ARCH_INTERRUPT_BASE
#define PIC_SLAVE_INT_BASE		(ARCH_INTERRUPT_BASE + 8)
#define PIC_NUM_INTS			0x0f

static const char *kInterruptNames[] = {
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
static const int kInterruptNameCount = 20;

#define MAX_ARGS 16

struct iframe_stack gBootFrameStack;

typedef struct {
	uint32 a, b;
} desc_table;
static desc_table *sIDT = NULL;


static void
set_gate(desc_table *gate_addr, addr_t addr, int type, int dpl)
{
	unsigned int gate1; // first byte of gate desc
	unsigned int gate2; // second byte of gate desc

	gate1 = (KERNEL_CODE_SEG << 16) | (0x0000ffff & addr);
	gate2 = (0xffff0000 & addr) | 0x8000 | (dpl << 13) | (type << 8);

	gate_addr->a = gate1;
	gate_addr->b = gate2;
}


static void
set_intr_gate(int n, void *addr)
{
	set_gate(&sIDT[n], (addr_t)addr, 14, DPL_KERNEL);
}


static void
set_system_gate(int n, void *addr)
{
	set_gate(&sIDT[n], (unsigned int)addr, 15, DPL_USER);
}


void
x86_set_task_gate(int32 n, int32 segment)
{
	sIDT[n].a = (segment << 16);
	sIDT[n].b = 0x8000 | (0 << 13) | (0x5 << 8); // present, dpl 0, type 5
}


/**	Tests if the interrupt in-service register of the responsible
 *	PIC is set for interrupts 7 and 15, and if that's not the case, 
 *	it must assume it's a spurious interrupt.
 */

static inline bool
pic_is_spurious_interrupt(int32 num)
{
	int32 isr;

	if (num != 7)
		return false;

	// Note, detecting spurious interrupts on line 15 obviously doesn't
	// work correctly - and since those are extremely rare, anyway, we
	// just ignore them

	out8(PIC_CONTROL3 | PIC_CONTROL3_READ_ISR, PIC_MASTER_CONTROL);
	isr = in8(PIC_MASTER_CONTROL);
	out8(PIC_CONTROL3 | PIC_CONTROL3_READ_IRR, PIC_MASTER_CONTROL);

	return (isr & 0x80) == 0;
}


/**	Sends a non-specified EOI (end of interrupt) notice to the PIC in
 *	question (or both of them).
 *	This clears the PIC interrupt in-service bit.
 */

static void
pic_end_of_interrupt(int32 num)
{
	if (num >= PIC_INT_BASE && num <= PIC_INT_BASE + PIC_NUM_INTS) {
		// PIC 8259 controlled interrupt
		if (num >= PIC_SLAVE_INT_BASE)
			out8(PIC_NON_SPECIFIC_EOI, PIC_SLAVE_CONTROL);

		// we always need to acknowledge the master PIC
		out8(PIC_NON_SPECIFIC_EOI, PIC_MASTER_CONTROL);
	}
}


static void
pic_init(void)
{
	// Start initialization sequence for the master and slave PICs
	out8(PIC_INIT1 | PIC_INIT1_SEND_INIT4, PIC_MASTER_INIT1);
	out8(PIC_INIT1 | PIC_INIT1_SEND_INIT4, PIC_SLAVE_INIT1);

	// Set start of interrupts to 0x20 for master, 0x28 for slave
	out8(PIC_INT_BASE, PIC_MASTER_INIT2);
	out8(PIC_SLAVE_INT_BASE, PIC_SLAVE_INIT2);

	// Specify cascading through interrupt 2
	out8(PIC_INIT3_IR2_IS_SLAVE, PIC_MASTER_INIT3);
	out8(PIC_INIT3_SLAVE_ID2, PIC_SLAVE_INIT3);

	// Set both to operate in 8086 mode
	out8(PIC_INIT4_x86_MODE, PIC_MASTER_INIT4);
	out8(PIC_INIT4_x86_MODE, PIC_SLAVE_INIT4);

	out8(0xfb, PIC_MASTER_MASK);	// Mask off all interrupts (except slave pic line IRQ 2).
	out8(0xff, PIC_SLAVE_MASK); 	// Mask off interrupts on the slave.

	// dump which interrupts are level or edge triggered

	TRACE(("PIC level trigger mode: %04x\n", in8(PIC_MASTER_TRIGGER_MODE)
		| (in8(PIC_SLAVE_TRIGGER_MODE) << 8)));
}


void
arch_int_enable_io_interrupt(int irq)
{
	// interrupt is specified "normalized"
	if (irq < 0 || irq > PIC_NUM_INTS)
		return;

	// enable PIC 8259 controlled interrupt

	TRACE(("arch_int_enable_io_interrupt: irq %d\n", irq));

	if (irq < 8)
		out8(in8(PIC_MASTER_MASK) & ~(1 << irq), PIC_MASTER_MASK);
	else
		out8(in8(PIC_SLAVE_MASK) & ~(1 << (irq - 8)), PIC_SLAVE_MASK);
}


void
arch_int_disable_io_interrupt(int irq)
{
	// interrupt is specified "normalized"
	// never disable slave pic line IRQ 2
	if (irq < 0 || irq > PIC_NUM_INTS || irq == 2)
		return;

	// disable PIC 8259 controlled interrupt

	if (irq < 8)
		out8(in8(PIC_MASTER_MASK) | (1 << irq), PIC_MASTER_MASK);
	else
		out8(in8(PIC_SLAVE_MASK) | (1 << (irq - 8)), PIC_SLAVE_MASK);
}


void
arch_int_enable_interrupts(void)
{
	asm("sti");
}


int
arch_int_disable_interrupts(void)
{
	int flags;

	asm("pushfl;\n"
		"popl %0;\n"
		"cli" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}


void
arch_int_restore_interrupts(int oldstate)
{
	int flags = oldstate ? 0x200 : 0;

	asm("pushfl;\n"
		"popl	%1;\n"
		"andl	$0xfffffdff,%1;\n"
		"orl	%0,%1;\n"
		"pushl	%1;\n"
		"popfl\n"
		: : "r" (flags), "r" (0));
}


bool
arch_int_are_interrupts_enabled(void)
{
	int flags;

	asm("pushfl;\n"
		"popl %0;\n" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}


static const char *
exception_name(int number, char *buffer, int32 bufferSize)
{
	if (number >= 0 && number < kInterruptNameCount)
		return kInterruptNames[number];

	snprintf(buffer, bufferSize, "exception %d", number);
	return buffer;
}


static void
fatal_exception(struct iframe *frame)
{
	char name[32];
	panic("Fatal exception \"%s\" occurred! Error code: 0x%x\n",
		exception_name(frame->vector, name, sizeof(name)), frame->error_code);
}


static void
unexpected_exception(struct iframe *frame, debug_exception_type type,
	int signal)
{
	if (frame->cs == USER_CODE_SEG) {
		enable_interrupts();

		if (user_debug_exception_occurred(type, signal))
			send_signal(team_get_current_team_id(), signal);
	} else {
		char name[32];
		panic("Unexpected exception \"%s\" occurred in kernel mode! "
			"Error code: 0x%x\n",
			exception_name(frame->vector, name, sizeof(name)),
			frame->error_code);
	}
}


/* keep the compiler happy, this function must be called only from assembly */
void i386_handle_trap(struct iframe frame);

void
i386_handle_trap(struct iframe frame)
{
	struct thread *thread = thread_get_current_thread();
	int ret = B_HANDLED_INTERRUPT;

	// all exceptions besides 3 (breakpoint), and 99 (syscall) enter this
	// function with interrupts disabled

	if (thread)
		x86_push_iframe(&thread->arch_info.iframes, &frame);
	else
		x86_push_iframe(&gBootFrameStack, &frame);

	if (frame.cs == USER_CODE_SEG) {
		i386_exit_user_debug_at_kernel_entry();
		thread_at_kernel_entry();
	}

//	if(frame.vector != 0x20)
//		dprintf("i386_handle_trap: vector 0x%x, ip 0x%x, cpu %d\n", frame.vector, frame.eip, smp_get_current_cpu());

	switch (frame.vector) {
		// fatal exceptions

		case 2:		// NMI Interrupt			
		case 9:		// Coprocessor Segment Overrun
		case 7:		// Device Not Available Exception (#NM)
		case 10:	// Invalid TSS Exception (#TS)
		case 11: 	// Segment Not Present (#NP)
		case 12: 	// Stack Fault Exception (#SS)
		case 18: 	// Machine-Check Exception (#MC)
			fatal_exception(&frame);
			break;

		case 8:		// Double Fault Exception (#DF)
		{
			struct tss *tss = x86_get_main_tss();
			
			frame.cs = tss->cs;
			frame.eip = tss->eip;
			frame.esp = tss->esp;
			frame.flags = tss->eflags;
			
			panic("double fault! errorcode = 0x%x\n", frame.error_code);

			break;
		}

		// exceptions we can handle
		// (most of them only when occurring in userland)

		case 0:		// Divide Error Exception (#DE)
			unexpected_exception(&frame, B_DIVIDE_ERROR, SIGFPE);
			break;

		case 1:		// Debug Exception (#DB)
			ret = i386_handle_debug_exception(&frame);
			break;

		case 3:		// Breakpoint Exception (#BP)
			ret = i386_handle_breakpoint_exception(&frame);
			break;

		case 4:		// Overflow Exception (#OF)
			unexpected_exception(&frame, B_OVERFLOW_EXCEPTION, SIGTRAP);
			break;

		case 5:		// BOUND Range Exceeded Exception (#BR)
			unexpected_exception(&frame, B_BOUNDS_CHECK_EXCEPTION, SIGTRAP);
			break;

		case 6:		// Invalid Opcode Exception (#UD)
			unexpected_exception(&frame, B_INVALID_OPCODE_EXCEPTION, SIGILL);
			break;

		case 13: 	// General Protection Exception (#GP)
			unexpected_exception(&frame, B_GENERAL_PROTECTION_FAULT, SIGKILL);
			break;

		case 14: 	// Page-Fault Exception (#PF)
		{
			bool kernelDebugger = debug_debugger_running();
			unsigned int cr2;
			addr_t newip;

			asm("movl %%cr2, %0" : "=r" (cr2));

			if (kernelDebugger) {
				// if this thread has a fault handler, we're allowed to be here
				struct thread *thread = thread_get_current_thread();
				if (thread && thread->fault_handler != NULL) {
					frame.eip = thread->fault_handler;
					break;
				}

				// otherwise, not really
				panic("page fault in debugger without fault handler! Touching "
					"address %p from eip %p\n", (void *)cr2, (void *)frame.eip);
				break;
			} else if ((frame.flags & 0x200) == 0) {
				// if the interrupts were disabled, and we are not running the kernel startup
				// the page fault was not allowed to happen and we must panic
				panic("page fault, but interrupts were disabled. Touching address "
					"%p from eip %p\n", (void *)cr2, (void *)frame.eip);
				break;
			} else if (thread != NULL && thread->page_faults_allowed < 1) {
				panic("page fault not allowed at this place. Touching address "
					"%p from eip %p\n", (void *)cr2, (void *)frame.eip);
			}

			enable_interrupts();

			ret = vm_page_fault(cr2, frame.eip,
				(frame.error_code & 0x2) != 0,	// write access
				(frame.error_code & 0x4) != 0,	// userland
				&newip);
			if (newip != 0) {
				// the page fault handler wants us to modify the iframe to set the
				// IP the cpu will return to to be this ip
				frame.eip = newip;
			}
			break;
		}

		case 16: 	// x87 FPU Floating-Point Error (#MF)
			unexpected_exception(&frame, B_FLOATING_POINT_EXCEPTION, SIGFPE);
			break;

		case 17: 	// Alignment Check Exception (#AC)
			unexpected_exception(&frame, B_ALIGNMENT_EXCEPTION, SIGTRAP);
			break;

		case 19: 	// SIMD Floating-Point Exception (#XF)
			unexpected_exception(&frame, B_FLOATING_POINT_EXCEPTION, SIGFPE);
			break;

		case 99:	// syscall
		{
			uint64 retcode;
			unsigned int args[MAX_ARGS];

#if 0
{
			int i;
			dprintf("i386_handle_trap: syscall %d, count %d, ptr 0x%x\n", frame.eax, frame.ecx, frame.edx);
			dprintf(" call stack:\n");
			for(i=0; i<frame.ecx; i++)
				dprintf("\t0x%x\n", ((unsigned int *)frame.edx)[i]);
}
#endif
			/* syscall interface works as such:
			 *  %eax has syscall #
			 *  %esp + 4 points to the syscall parameters
			 */
			if (frame.eax >= 0 && frame.eax < kSyscallCount) {
				void *params = (void*)(frame.user_esp + 4);
				int paramSize = kSyscallInfos[frame.eax].parameter_size;
				if (IS_KERNEL_ADDRESS((addr_t)params)
					|| user_memcpy(args, params, paramSize) < B_OK) {
					retcode =  B_BAD_ADDRESS;
				} else
					ret = syscall_dispatcher(frame.eax, (void *)args, &retcode);
			} else {
				// invalid syscall number
				retcode = EINVAL;
			}
			frame.eax = retcode & 0xffffffff;
			frame.edx = retcode >> 32;
			break;
		}

		default:
			if (frame.vector >= ARCH_INTERRUPT_BASE) {
				// This is a workaround for spurious assertions of interrupts 7/15
				// which seems to be an often seen problem on the PC platform
				if (pic_is_spurious_interrupt(frame.vector - ARCH_INTERRUPT_BASE)) {
					TRACE(("got spurious interrupt at vector %ld\n", frame.vector));
					break;
				}

				pic_end_of_interrupt(frame.vector);

				ret = int_io_interrupt_handler(frame.vector - ARCH_INTERRUPT_BASE);
			} else {
				char name[32];
				panic("i386_handle_trap: unhandled trap 0x%x (%s) at ip 0x%x, "
					"thread 0x%x!\n", frame.vector,
					exception_name(frame.vector, name, sizeof(name)), frame.eip,
					thread ? thread->id : -1);
				ret = B_HANDLED_INTERRUPT;
			}
			break;
	}

	if (ret == B_INVOKE_SCHEDULER) {
		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		scheduler_reschedule();

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	if (frame.cs == USER_CODE_SEG) {
		enable_interrupts();
			// interrupts are not enabled at this point if we came from
			// a hardware interrupt
		thread_at_kernel_exit();
		i386_init_user_debug_at_kernel_exit(&frame);
	}

//	dprintf("0x%x cpu %d!\n", thread_get_current_thread_id(), smp_get_current_cpu());

	if (thread)
		x86_pop_iframe(&thread->arch_info.iframes);
	else
		x86_pop_iframe(&gBootFrameStack);
}


status_t
arch_int_init(kernel_args *args)
{
	// set the global sIDT variable
	sIDT = (desc_table *)args->arch_args.vir_idt;

	// setup the interrupt controller
	pic_init();

	set_intr_gate(0,  &trap0);
	set_intr_gate(1,  &trap1);
	set_intr_gate(2,  &trap2);
	set_system_gate(3,  &trap3);
	set_intr_gate(4,  &trap4);
	set_intr_gate(5,  &trap5);
	set_intr_gate(6,  &trap6);
	set_intr_gate(7,  &trap7);
	// trap8 (double fault) is set in arch_cpu.c
	set_intr_gate(9,  &trap9);
	set_intr_gate(10,  &trap10);
	set_intr_gate(11,  &trap11);
	set_intr_gate(12,  &trap12);
	set_intr_gate(13,  &trap13);
	set_intr_gate(14,  &trap14);
//	set_intr_gate(15,  &trap15);
	set_intr_gate(16,  &trap16);
	set_intr_gate(17,  &trap17);
	set_intr_gate(18,  &trap18);
	set_intr_gate(19,  &trap19);

	set_intr_gate(32,  &trap32);
	set_intr_gate(33,  &trap33);
	set_intr_gate(34,  &trap34);
	set_intr_gate(35,  &trap35);
	set_intr_gate(36,  &trap36);
	set_intr_gate(37,  &trap37);
	set_intr_gate(38,  &trap38);
	set_intr_gate(39,  &trap39);
	set_intr_gate(40,  &trap40);
	set_intr_gate(41,  &trap41);
	set_intr_gate(42,  &trap42);
	set_intr_gate(43,  &trap43);
	set_intr_gate(44,  &trap44);
	set_intr_gate(45,  &trap45);
	set_intr_gate(46,  &trap46);
	set_intr_gate(47,  &trap47);

	set_system_gate(99, &trap99);

	set_intr_gate(251, &trap251);
	set_intr_gate(252, &trap252);
	set_intr_gate(253, &trap253);
	set_intr_gate(254, &trap254);
	set_intr_gate(255, &trap255);

	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	area_id area;

	sIDT = (desc_table *)args->arch_args.vir_idt;
	area = create_area("idt", (void *)&sIDT, B_EXACT_ADDRESS, B_PAGE_SIZE, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	return area >= B_OK ? B_OK : area;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_OK;
}
