/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <vm.h>
#include <debug.h>
#include <console.h>
#include <int.h>
#include <thread.h>
#include <smp.h>
#include <syscalls.h>
#include <Errors.h>
#include <vm_priv.h>
#include <ksyscalls.h>

#include <arch/cpu.h>
#include <arch/int.h>
#include <arch/faults.h>
#include <arch/vm.h>
#include <arch/smp.h>

#include <arch/x86/interrupts.h>
#include <arch/x86/faults.h>

#include <stage2.h>

#include <string.h>

#define MAX_ARGS 16

struct int_frame {
	unsigned int gs;
	unsigned int fs;
	unsigned int es;
	unsigned int ds;
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int esp;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
	unsigned int vector;
	unsigned int error_code;
	unsigned int eip;
	unsigned int cs;
	unsigned int flags;
	unsigned int user_esp;
	unsigned int user_ss;
};

static desc_table *idt = NULL;

static void interrupt_ack(int n)
{
	if(n >= 0x20 && n < 0x30) {
		// 8239 controlled interrupt
		if(n > 0x27)
			out8(0x20, 0xa0);	// EOI to pic 2
		out8(0x20, 0x20);	// EOI to pic 1
	}
}

static void _set_gate(desc_table *gate_addr, unsigned int addr, int type, int dpl)
{
	unsigned int gate1; // first byte of gate desc
	unsigned int gate2; // second byte of gate desc

	gate1 = (KERNEL_CODE_SEG << 16) | (0x0000ffff & addr);
	gate2 = (0xffff0000 & addr) | 0x8000 | (dpl << 13) | (type << 8);

	gate_addr->a = gate1;
	gate_addr->b = gate2;
}

void arch_int_enable_io_interrupt(int irq)
{
	if(irq < 0x20 || irq >= 0x30) return;
	irq -= 0x20;
	// if this is a external interrupt via 8239, enable it here
	if (irq < 8)
		out8(in8(0x21) & ~(1 << irq), 0x21);
	else
		out8(in8(0xa1) & ~(1 << (irq - 8)), 0xa1);
}

void arch_int_disable_io_interrupt(int irq)
{
	if(irq < 0x20 || irq >= 0x30) return;
	irq -= 0x20;
	// if this is a external interrupt via 8239, disable it here
	if (irq < 8)
		out8(in8(0x21) | (1 << irq), 0x21);
	else
		out8(in8(0xa1) | (1 << (irq - 8)), 0xa1);
}

static void set_intr_gate(int n, void *addr)
{
	_set_gate(&idt[n], (unsigned int)addr, 14, 0);
}

/* XXX - currently unused and static...
static void set_trap_gate(int n, void *addr)
{
	_set_gate(&idt[n], (unsigned int)addr, 15, 0);
}
*/

static void set_system_gate(int n, void *addr)
{
	_set_gate(&idt[n], (unsigned int)addr, 15, 3);
}

void arch_int_enable_interrupts(void)
{
	asm("sti");
}

int arch_int_disable_interrupts(void)
{
	int flags;

	asm("pushfl;\n"
		"popl %0;\n"
		"cli" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}

void arch_int_restore_interrupts(int oldstate)
{
	int flags = oldstate ? 0x200 : 0;

	asm (
		"pushfl;\n"
		"popl	%1;\n"
		"andl	$0xfffffdff,%1;\n"
		"orl	%0,%1;\n"
		"pushl	%1;\n"
		"popfl\n"
		: : "r" (flags), "r" (0));
}

bool arch_int_is_interrupts_enabled(void)
{
	int flags;

	asm("pushfl;\n"
		"popl %0;\n" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}

void i386_handle_trap(struct int_frame frame); /* keep the compiler happy, this function must be called only from assembly */
void i386_handle_trap(struct int_frame frame)
{
	int ret = INT_NO_RESCHEDULE;

//	if(frame.vector != 0x20)
//		dprintf("i386_handle_trap: vector 0x%x, ip 0x%x, cpu %d\n", frame.vector, frame.eip, smp_get_current_cpu());
	switch(frame.vector) {
		case 8:
			ret = i386_double_fault(frame.error_code);
			break;
		case 13:
			ret = i386_general_protection_fault(frame.error_code);
			break;
		case 14: {
			unsigned int cr2;
			addr newip;

			asm ("movl %%cr2, %0" : "=r" (cr2) );

			// get the old interrupt enable/disable state and restore to that
			if(frame.flags & 0x200) {
//				dprintf("page_fault: enabling interrupts\n");
				int_enable_interrupts();
			}
			ret = vm_page_fault(cr2, frame.eip,
				(frame.error_code & 0x2) != 0,
				(frame.error_code & 0x4) != 0,
				&newip);
			if(newip != 0) {
				// the page fault handler wants us to modify the iframe to set the
				// IP the cpu will return to to be this ip
				frame.eip = newip;
			}
			break;
		}
		case 99: {
			uint64 retcode;
			unsigned int args[MAX_ARGS];
			int rc;

			thread_atkernel_entry();
#if 0
{
			int i;
			dprintf("i386_handle_trap: syscall %d, count %d, ptr 0x%x\n", frame.eax, frame.ecx, frame.edx);
			dprintf(" call stack:\n");
			for(i=0; i<frame.ecx; i++)
				dprintf("\t0x%x\n", ((unsigned int *)frame.edx)[i]);
}
#endif
			/*
			** syscall interface works as such:
			** eax has syscall #
			** ecx has number of args (0-16)
			** edx has pointer to buffer containing args from first to last
			** each is verified to make sure someone doesn't try to clobber it
			*/
			if(frame.ecx <= MAX_ARGS) {
				if((addr)frame.edx >= KERNEL_BASE && (addr)frame.edx <= KERNEL_TOP) {
					retcode =  ERR_VM_BAD_USER_MEMORY;
				} else {
					rc = user_memcpy(args, (void *)frame.edx, frame.ecx * sizeof(unsigned int));
					if(rc < 0)
						retcode = ERR_VM_BAD_USER_MEMORY;
					else
						ret = syscall_dispatcher(frame.eax, (void *)args, &retcode);
				}
			} else {
				// want to pass too many args into the system
				retcode = ERR_INVALID_ARGS;
			}
			frame.eax = retcode & 0xffffffff;
			frame.edx = retcode >> 32;
			break;
		}
		default:
			if(frame.vector >= 0x20) {
				interrupt_ack(frame.vector); // ack the 8239 (if applicable)
				ret = int_io_interrupt_handler(frame.vector);
			} else {
				panic("i386_handle_trap: unhandled cpu trap 0x%x at ip 0x%x!\n", frame.vector, frame.eip);
				ret = INT_NO_RESCHEDULE;
			}
			break;
	}

	if(ret == INT_RESCHEDULE) {
		int state = int_disable_interrupts();
		GRAB_THREAD_LOCK();
		thread_resched();
		RELEASE_THREAD_LOCK();
		int_restore_interrupts(state);
	}

	if(frame.cs == USER_CODE_SEG || frame.vector == 99) {
		thread_atkernel_exit();
	}
//	dprintf("0x%x cpu %d!\n", thread_get_current_thread_id(), smp_get_current_cpu());
}

int arch_int_init(kernel_args *ka)
{
	idt = (desc_table *)ka->arch_args.vir_idt;

	// setup the interrupt controller
	out8(0x11, 0x20);	// Start initialization sequence for #1.
	out8(0x11, 0xa0);	// ...and #2.
	out8(0x20, 0x21);	// Set start of interrupts for #1 (0x20).
	out8(0x28, 0xa1);	// Set start of interrupts for #2 (0x28).
	out8(0x04, 0x21);	// Set #1 to be the master.
	out8(0x02, 0xa1);	// Set #2 to be the slave.
	out8(0x01, 0x21);	// Set both to operate in 8086 mode.
	out8(0x01, 0xa1);
	out8(0xfb, 0x21);	// Mask off all interrupts (except slave pic line).
	out8(0xff, 0xa1); 	// Mask off interrupts on the slave.

	set_intr_gate(0,  &trap0);
	set_intr_gate(1,  &trap1);
	set_intr_gate(2,  &trap2);
	set_intr_gate(3,  &trap3);
	set_intr_gate(4,  &trap4);
	set_intr_gate(5,  &trap5);
	set_intr_gate(6,  &trap6);
	set_intr_gate(7,  &trap7);
	set_intr_gate(8,  &trap8);
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

	return 0;
}

int arch_int_init2(kernel_args *ka)
{
	idt = (desc_table *)ka->arch_args.vir_idt;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "idt", (void *)&idt,
		REGION_ADDR_EXACT_ADDRESS, PAGE_SIZE, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);
	return 0;
}
