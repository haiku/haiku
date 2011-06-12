/*
 * Copyright 2003-2011, Haiku Inc. All rights reserved.
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

#include <arch/smp.h>
#include <boot/kernel_args.h>
#include <device_manager.h>
#include <kscheduler.h>
#include <interrupt_controller.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>

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

// interrupt controller interface (initialized
// in arch_int_init_post_device_manager())
static struct interrupt_controller_module_info *sPIC;
static void *sPICCookie;


void
arch_int_enable_io_interrupt(int irq)
{
	if (!sPIC)
		return;

	// TODO: I have no idea, what IRQ type is appropriate.
	sPIC->enable_io_interrupt(sPICCookie, irq, IRQ_TYPE_LEVEL);
}


void
arch_int_disable_io_interrupt(int irq)
{
	if (!sPIC)
		return;

	sPIC->disable_io_interrupt(sPICCookie, irq);
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
	if (vector != 0x900) {
		dprintf("ppc_exception_entry: time %lld vector 0x%x, iframe %p, "
			"srr0: %p\n", system_time(), vector, iframe, (void*)iframe->srr0);
	}

	Thread *thread = thread_get_current_thread();

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
			bool kernelDebugger = debug_debugger_running();

			if (kernelDebugger) {
				// if this CPU or this thread has a fault handler,
				// we're allowed to be here
				cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
				if (cpu->fault_handler != 0) {
					iframe->srr0 = cpu->fault_handler;
					iframe->r1 = cpu->fault_handler_stack_pointer;
					break;
				}
				Thread *thread = thread_get_current_thread();
				if (thread && thread->fault_handler != 0) {
					iframe->srr0 = thread->fault_handler;
					break;
				}

				// otherwise, not really
				panic("page fault in debugger without fault handler! Touching "
					"address %p from ip %p\n", (void *)iframe->dar,
					(void *)iframe->srr0);
				break;
			} else if ((iframe->srr1 & MSR_EXCEPTIONS_ENABLED) == 0) {
				// if the interrupts were disabled, and we are not running the
				// kernel startup the page fault was not allowed to happen and
				// we must panic
				panic("page fault, but interrupts were disabled. Touching "
					"address %p from ip %p\n", (void *)iframe->dar,
					(void *)iframe->srr0);
				break;
			} else if (thread != NULL && thread->page_faults_allowed < 1) {
				panic("page fault not allowed at this place. Touching address "
					"%p from ip %p\n", (void *)iframe->dar,
					(void *)iframe->srr0);
			}

			enable_interrupts();

			addr_t newip;

			vm_page_fault(iframe->dar, iframe->srr0,
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
		{
			if (!sPIC) {
				panic("ppc_exception_entry(): external interrupt although we "
					"don't have a PIC driver!");
				break;
			}

dprintf("handling I/O interrupts...\n");
			int irq;
			while ((irq = sPIC->acknowledge_io_interrupt(sPICCookie)) >= 0) {
// TODO: correctly pass level-triggered vs. edge-triggered to the handler!
				int_io_interrupt_handler(irq, true);
			}
dprintf("handling I/O interrupts done\n");
			break;
		}

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
			timer_interrupt();
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
		case 0xf20: // altivec unavailable exception
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

	cpu_status state = disable_interrupts();
	if (thread->cpu->invoke_scheduler) {
		SpinLocker schedulerLocker(gSchedulerLock);
		scheduler_reschedule();
		schedulerLocker.Unlock();
		restore_interrupts(state);
	} else if (thread->post_interrupt_callback != NULL) {
		void (*callback)(void*) = thread->post_interrupt_callback;
		void* data = thread->post_interrupt_data;

		thread->post_interrupt_callback = NULL;
		thread->post_interrupt_data = NULL;

		restore_interrupts(state);

		callback(data);
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


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


template<typename ModuleInfo>
struct Module : DoublyLinkedListLinkImpl<Module<ModuleInfo> > {
	Module(ModuleInfo *module)
		: module(module)
	{
	}

	~Module()
	{
		if (module)
			put_module(((module_info*)module)->name);
	}

	ModuleInfo	*module;
};

typedef Module<interrupt_controller_module_info> PICModule;

struct PICModuleList : DoublyLinkedList<PICModule> {
	~PICModuleList()
	{
		while (PICModule *module = First()) {
			Remove(module);
			delete module;
		}
	}
};


class DeviceTreeIterator {
public:
	DeviceTreeIterator(device_manager_info *deviceManager)
		: fDeviceManager(deviceManager),
		  fNode(NULL),
		  fParent(NULL)
	{
		Rewind();
	}

	~DeviceTreeIterator()
	{
		if (fParent != NULL)
			fDeviceManager->put_node(fParent);
		if (fNode != NULL)
			fDeviceManager->put_node(fNode);
	}

	void Rewind()
	{
		fNode = fDeviceManager->get_root_node();
	}

	bool HasNext() const
	{
		return (fNode != NULL);
	}

	device_node *Next()
	{
		if (fNode == NULL)
			return NULL;

		device_node *foundNode = fNode;

		// get first child
		device_node *child = NULL;
		if (fDeviceManager->get_next_child_node(fNode, NULL, &child)
				== B_OK) {
			// move to the child node
			if (fParent != NULL)
				fDeviceManager->put_node(fParent);
			fParent = fNode;
			fNode = child;

		// no more children; backtrack to find the next sibling
		} else {
			while (fParent != NULL) {
				if (fDeviceManager->get_next_child_node(fParent, NULL, &fNode)
						== B_OK) {
						// get_next_child_node() always puts the node
					break;
				}
				fNode = fParent;
				fParent = fDeviceManager->get_parent_node(fNode);
			}

			// if we hit the root node again, we're done
			if (fParent == NULL) {
				fDeviceManager->put_node(fNode);
				fNode = NULL;
			}
		}

		return foundNode;
	}

private:
	device_manager_info *fDeviceManager;
	device_node	*fNode;
	device_node	*fParent;
};


static void
get_interrupt_controller_modules(PICModuleList &list)
{
	const char *namePrefix = "interrupt_controllers/";
	size_t namePrefixLen = strlen(namePrefix);

	char name[B_PATH_NAME_LENGTH];
	size_t length;
	uint32 cookie = 0;
	while (get_next_loaded_module_name(&cookie, name, &(length = sizeof(name)))
			== B_OK) {
		// an interrupt controller module?
		if (length <= namePrefixLen
			|| strncmp(name, namePrefix, namePrefixLen) != 0) {
			continue;
		}

		// get the module
		interrupt_controller_module_info *moduleInfo;
		if (get_module(name, (module_info**)&moduleInfo) != B_OK)
			continue;

		// add it to the list
		PICModule *module = new(nothrow) PICModule(moduleInfo);
		if (!module) {
			put_module(((module_info*)moduleInfo)->name);
			continue;
		}
		list.Add(module);
	}
}


static bool
probe_pic_device(device_node *node, PICModuleList &picModules)
{
	for (PICModule *module = picModules.Head();
		 module;
		 module = picModules.GetNext(module)) {
		if (module->module->info.supports_device(node) > 0) {
			if (module->module->info.register_device(node) == B_OK)
				return true;
		}
	}

	return false;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	// get the interrupt controller driver modules
	PICModuleList picModules;
	get_interrupt_controller_modules(picModules);
	if (picModules.IsEmpty()) {
		panic("arch_int_init_post_device_manager(): Found no PIC modules!");
		return B_ENTRY_NOT_FOUND;
	}

	// get the device manager module
	device_manager_info *deviceManager;
	status_t error = get_module(B_DEVICE_MANAGER_MODULE_NAME,
		(module_info**)&deviceManager);
	if (error != B_OK) {
		panic("arch_int_init_post_device_manager(): Failed to get device "
			"manager: %s", strerror(error));
		return error;
	}
	Module<device_manager_info> _deviceManager(deviceManager);	// auto put

	// iterate through the device tree and probe the interrupt controllers
	DeviceTreeIterator iterator(deviceManager);
	while (device_node *node = iterator.Next())
		probe_pic_device(node, picModules);

	// iterate through the tree again and get an interrupt controller node
	iterator.Rewind();
	while (device_node *node = iterator.Next()) {
		const char *deviceType;
		if (deviceManager->get_attr_string(node, B_DEVICE_TYPE,
				&deviceType, false) == B_OK) {
			bool isPIC = false;

			/*
			bool isPIC
				= (strcmp(deviceType, B_INTERRUPT_CONTROLLER_DRIVER_TYPE) == 0);
			free(deviceType);
			*/

			if (isPIC) {
				driver_module_info *driver;
				void *driverCookie;

				deviceManager->get_driver(node, (driver_module_info **)&driver, (void **)&driverCookie);

				sPIC = (interrupt_controller_module_info *)driver;
				sPICCookie = driverCookie;
				return B_OK;
			}
		}
	}

	// no PIC found
	panic("arch_int_init_post_device_manager(): Found no supported PIC!");

	return B_ENTRY_NOT_FOUND;
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
	phys_addr_t physicalPage;
	addr_t inPageOffset = (addr_t)context & (B_PAGE_SIZE - 1);
	status_t error = vm_get_page_mapping(VMAddressSpace::KernelID(),
		(addr_t)context - inPageOffset, &physicalPage);
	if (error != B_OK) {
		panic("ppc_set_current_cpu_exception_context(): Failed to get physical "
			"address!");
		return;
	}

	asm volatile("mtsprg0 %0" : : "r"(physicalPage + inPageOffset));
}

