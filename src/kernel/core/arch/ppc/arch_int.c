/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <boot/kernel_args.h>

#include <int.h>
#include <vm.h>
#include <vm_priv.h>
#include <timer.h>
#include <thread.h>

#include <string.h>

// defined in arch_exceptions.S
extern int __irqvec_start;
extern int __irqvec_end;


void 
arch_int_enable_io_interrupt(int irq)
{
	return;
}


void 
arch_int_disable_io_interrupt(int irq)
{
	return;
}


/* arch_int_*_interrupts() and friends are in arch_asm.S */


static void 
print_iframe(struct iframe *frame)
{
	dprintf("iframe at %p:\n", frame);
	dprintf("r0-r3:   0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r0, frame->r1, frame->r2, frame->r3);
	dprintf("r4-r7:   0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r4, frame->r5, frame->r6, frame->r7);
	dprintf("r8-r11:  0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r8, frame->r9, frame->r10, frame->r11);
	dprintf("r12-r15: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r12, frame->r13, frame->r14, frame->r15);
	dprintf("r16-r19: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r16, frame->r17, frame->r18, frame->r19);
	dprintf("r20-r23: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r20, frame->r21, frame->r22, frame->r23);
	dprintf("r24-r27: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r24, frame->r25, frame->r26, frame->r27);
	dprintf("r28-r31: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r28, frame->r29, frame->r30, frame->r31);
	dprintf("     ctr 0x%08lx        xer 0x%08lx\n", frame->ctr, frame->xer);
	dprintf("      cr 0x%08lx         lr 0x%08lx\n", frame->cr, frame->lr);
	dprintf("   dsisr 0x%08lx        dar 0x%08lx\n", frame->dsisr, frame->dar);
	dprintf("    srr1 0x%08lx       srr0 0x%08lx\n", frame->srr1, frame->srr0);
}


void ppc_exception_entry(int vector, struct iframe *iframe);
void 
ppc_exception_entry(int vector, struct iframe *iframe)
{
	int ret = B_HANDLED_INTERRUPT;

	if (vector != 0x900)
		dprintf("ppc_exception_entry: time %Ld vector 0x%x, iframe %p\n", system_time(), vector, iframe);

	switch (vector) {
		case 0x100: // system reset
			panic("system reset exception\n");
			break;
		case 0x200: // machine check
			panic("machine check exception\n");
			break;
		case 0x300: // DSI
		case 0x400: // ISI
		{
			addr_t newip;

			ret = vm_page_fault(iframe->dar, iframe->srr0,
				iframe->dsisr & (1 << 25), // store or load
				iframe->srr1 & (1 << 14), // was the system in user or supervisor
				&newip);
			if (newip != 0) {
				// the page fault handler wants us to modify the iframe to set the
				// IP the cpu will return to to be this ip
				iframe->srr0 = newip;
			}
 			break;
		}
		case 0x500: // external interrupt
			panic("external interrrupt exception: unimplemented\n");
			break;
		case 0x600: // alignment exception
			panic("alignment exception: unimplemented\n");
			break;
		case 0x700: // program exception
			panic("program exception: unimplemented\n");
			break;
		case 0x800: // FP unavailable exception
			panic("FP unavailable exception: unimplemented\n");
			break;
		case 0x900: // decrementer exception
			ret = timer_interrupt();
			break;
		case 0xc00: // system call
			panic("system call exception: unimplemented\n");
			break;
		case 0xd00: // trace exception
			panic("trace exception: unimplemented\n");
			break;
		case 0xe00: // FP assist exception
			panic("FP assist exception: unimplemented\n");
			break;
		case 0xf00: // performance monitor exception
			panic("performance monitor exception: unimplemented\n");
			break;
		case 0xf20: // alitivec unavailable exception
			panic("alitivec unavailable exception: unimplemented\n");
			break;
		case 0x1000:
		case 0x1100:
		case 0x1200:
			panic("TLB miss exception: unimplemented\n");
			break;
		case 0x1300: // instruction address exception
			panic("instruction address exception: unimplemented\n");
			break;
		case 0x1400: // system management exception
			panic("system management exception: unimplemented\n");
			break;
		case 0x1600: // altivec assist exception
			panic("altivec assist exception: unimplemented\n");
			break;
		case 0x1700: // thermal management exception
			panic("thermal management exception: unimplemented\n");
			break;
		default:
			dprintf("unhandled exception type 0x%x\n", vector);
			print_iframe(iframe);
			panic("unhandled exception type\n");
	}

	if (ret == B_INVOKE_SCHEDULER) {
		int state = disable_interrupts();
		GRAB_THREAD_LOCK();
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}
}


int 
arch_int_init(kernel_args *ka)
{
	return 0;
}


int 
arch_int_init2(kernel_args *ka)
{
	region_id exception_region;
	void *handlers;

	// create a region to map the irq vector code into (physical address 0x0)
	handlers = (void *)ka->arch_args.exception_handlers.start;
	exception_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "exception_handlers",
		&handlers, REGION_ADDR_EXACT_ADDRESS,
		ka->arch_args.exception_handlers.size, 
		REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);
	if (exception_region < 0)
		panic("arch_int_init2: could not create exception handler region\n");

	dprintf("exception handlers at %p\n", handlers);

	// copy the handlers into this area
	memcpy(handlers, &__irqvec_start, ka->arch_args.exception_handlers.size);
	arch_cpu_sync_icache(0, 0x1000);

	return 0;
}

