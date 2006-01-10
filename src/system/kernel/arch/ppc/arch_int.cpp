/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <int.h>

#include <boot/kernel_args.h>

#include <arch/smp.h>
#include <kscheduler.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <vm.h>
#include <vm_address_space.h>
#include <vm_priv.h>

#include <string.h>

// defined in arch_exceptions.S
extern int __irqvec_start;
extern int __irqvec_end;

extern"C" void ppc_exception_tail(void);


// the exception contexts for all CPUs
static ppc_cpu_exception_context sCPUExceptionContexts[SMP_MAX_CPUS];


// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;


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


extern "C" void ppc_exception_entry(int vector, struct iframe *iframe);
void 
ppc_exception_entry(int vector, struct iframe *iframe)
{
	int ret = B_HANDLED_INTERRUPT;

	if (vector != 0x900) {
		dprintf("ppc_exception_entry: time %lld vector 0x%x, iframe %p, "
			"srr0: %p\n", system_time(), vector, iframe, (void*)iframe->srr0);
	}

	struct thread *thread = thread_get_current_thread();

	// push iframe
	if (thread)
		ppc_push_iframe(&thread->arch_info.iframes, iframe);
	else
		ppc_push_iframe(&gBootFrameStack, iframe);

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

	// pop iframe
	if (thread)
		ppc_pop_iframe(&thread->arch_info.iframes);
	else
		ppc_pop_iframe(&gBootFrameStack);
}


status_t 
arch_int_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	void *handlers = (void *)args->arch_args.exception_handlers.start;

	// We may need to remap the exception handler area into the kernel address
	// space.
	if (!IS_KERNEL_ADDRESS(handlers)) {
		addr_t address = (addr_t)handlers;
		status_t error = ppc_remap_address_range(&address,
			args->arch_args.exception_handlers.size, true);
		if (error != B_OK) {
			panic("arch_int_init_post_vm(): Failed to remap the exception "
				"handler area!");
			return error;
		}
		handlers = (void*)(address);
	}

	// create a region to map the irq vector code into (physical address 0x0)
	area_id exceptionArea = create_area("exception_handlers",
		&handlers, B_EXACT_ADDRESS, args->arch_args.exception_handlers.size, 
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (exceptionArea < B_OK)
		panic("arch_int_init2: could not create exception handler region\n");

	dprintf("exception handlers at %p\n", handlers);

	// copy the handlers into this area
	memcpy(handlers, &__irqvec_start, args->arch_args.exception_handlers.size);
	arch_cpu_sync_icache(handlers, args->arch_args.exception_handlers.size);

	// init the CPU exception contexts
	int cpuCount = smp_get_num_cpus();
	for (int i = 0; i < cpuCount; i++) {
		ppc_cpu_exception_context *context = ppc_get_cpu_exception_context(i);
		context->kernel_handle_exception = (void*)&ppc_exception_tail;
		context->exception_context = context;
		// kernel_stack is set when the current thread changes. At this point
		// we don't have threads yet.
	}

	// set the exception context for this CPU
	ppc_set_current_cpu_exception_context(ppc_get_cpu_exception_context(0));

	return B_OK;
}


// #pragma mark -

struct ppc_cpu_exception_context *
ppc_get_cpu_exception_context(int cpu)
{
	return sCPUExceptionContexts + cpu;
}


void
ppc_set_current_cpu_exception_context(struct ppc_cpu_exception_context *context)
{
	// translate to physical address
	addr_t physicalPage;
	addr_t inPageOffset = (addr_t)context & (B_PAGE_SIZE - 1);
	status_t error = vm_get_page_mapping(vm_kernel_address_space_id(),
		(addr_t)context - inPageOffset, &physicalPage);
	if (error != B_OK) {
		panic("ppc_set_current_cpu_exception_context(): Failed to get physical "
			"address!");
		return;
	}
	
	asm volatile("mtsprg0 %0" : : "r"(physicalPage + inPageOffset));
}

