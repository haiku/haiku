/*
 * Copyright 2003-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <int.h>

#include <arch_platform.h>
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

#warning M68K: writeme!


//#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

typedef void (*m68k_exception_handler)(void);
#define M68K_EXCEPTION_VECTOR_COUNT 256
#warning M68K: align on 4 ?
//m68k_exception_handler gExceptionVectors[M68K_EXCEPTION_VECTOR_COUNT];
m68k_exception_handler *gExceptionVectors;

// defined in arch_exceptions.S
extern "C" void __m68k_exception_noop(void);
extern "C" void __m68k_exception_common(void);

extern int __irqvec_start;
extern int __irqvec_end;

extern"C" void m68k_exception_tail(void);

// current fault handler
addr_t gFaultHandler;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;

// interrupt controller interface (initialized
// in arch_int_init_post_device_manager())
//static struct interrupt_controller_module_info *sPIC;
//static void *sPICCookie;


void
arch_int_enable_io_interrupt(int irq)
{
	//if (!sPIC)
	//	return;

	// TODO: I have no idea, what IRQ type is appropriate.
	//sPIC->enable_io_interrupt(sPICCookie, irq, IRQ_TYPE_LEVEL);
	M68KPlatform::Default()->EnableIOInterrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	//if (!sPIC)
	//	return;

	//sPIC->disable_io_interrupt(sPICCookie, irq);
	M68KPlatform::Default()->DisableIOInterrupt(irq);
}


/* arch_int_*_interrupts() and friends are in arch_asm.S */


static void
print_iframe(struct iframe *frame)
{
	dprintf("iframe at %p:\n", frame);
	dprintf("   d0 0x%08lx    d1 0x%08lx    d2 0x%08lx    d3 0x%08lx\n",
				frame->d[0], frame->d[1], frame->d[2], frame->d[3]);
			kprintf("   d4 0x%08lx    d5 0x%08lx    d6 0x%08lx    d7 0x%08lx\n",
				frame->d[4], frame->d[5], frame->d[6], frame->d[7]);
			kprintf("   a0 0x%08lx    a1 0x%08lx    a2 0x%08lx    a3 0x%08lx\n",
				frame->a[0], frame->a[1], frame->a[2], frame->a[3]);
			kprintf("   a4 0x%08lx    a5 0x%08lx    a6 0x%08lx    "/*"a7 0x%08lx (sp)"*/"\n",
				frame->a[4], frame->a[5], frame->a[6]/*, frame->a[7]*/);

			/*kprintf("   pc 0x%08lx   ccr 0x%02x\n",
			  frame->pc, frame->ccr);*/
			kprintf("   pc 0x%08lx        sr 0x%04x\n",
				frame->cpu.pc, frame->cpu.sr);
#if 0
	dprintf("r0-r3:   0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->d0, frame->d1, frame->d2, frame->d3);
	dprintf("r4-r7:   0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->d4, frame->d5, frame->d6, frame->d7);
	dprintf("r8-r11:  0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->a0, frame->a1, frame->a2, frame->a3);
	dprintf("r12-r15: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->a4, frame->a5, frame->a6, frame->a7);
	dprintf("      pc 0x%08lx         sr 0x%08lx\n", frame->pc, frame->sr);
#endif
}


static addr_t
fault_address(struct iframe *iframe)
{
	switch (iframe->cpu.type) {
		case 0x0:
		case 0x1:
			return 0;
		case 0x2:
			return iframe->cpu.type_2.instruction_address;
		case 0x3:
			return iframe->cpu.type_3.effective_address;
		case 0x7:
			return iframe->cpu.type_7.effective_address;
		case 0x9:
			return iframe->cpu.type_9.instruction_address;
		case 0xa:
			return iframe->cpu.type_a.fault_address;
		case 0xb:
			return iframe->cpu.type_b.fault_address;
		default:
			return 0;
	}
}


static bool
fault_was_write(struct iframe *iframe)
{
	switch (iframe->cpu.type) {
		case 0x7:
			return !iframe->cpu.type_7.ssw.rw;
		case 0xa:
			return !iframe->cpu.type_a.ssw.rw;
		case 0xb:
			return !iframe->cpu.type_b.ssw.rw;
		default:
			panic("can't determine r/w from iframe type %d\n",
				iframe->cpu.type);
			return false;
	}
}


extern "C" void m68k_exception_entry(struct iframe *iframe);
void
m68k_exception_entry(struct iframe *iframe)
{
	int vector = iframe->cpu.vector >> 2;
	bool hardwareInterrupt = false;

	if (vector != -1) {
		dprintf("m68k_exception_entry: time %lld vector 0x%x, iframe %p, "
			"pc: %p\n", system_time(), vector, iframe, (void*)iframe->cpu.pc);
	}

	Thread *thread = thread_get_current_thread();

	// push iframe
	if (thread)
		m68k_push_iframe(&thread->arch_info.iframes, iframe);
	else
		m68k_push_iframe(&gBootFrameStack, iframe);

	switch (vector) {
		case 0: // system reset
			panic("system reset exception\n");
			break;
		case 2: // bus error
		case 3: // address error
		{
			bool kernelDebugger = debug_debugger_running();

			if (kernelDebugger) {
				// if this thread has a fault handler, we're allowed to be here
				if (thread && thread->fault_handler != 0) {
					iframe->cpu.pc = thread->fault_handler;
					break;
				}


				// otherwise, not really
				panic("page fault in debugger without fault handler! Touching "
					"address %p from ip %p\n", (void *)fault_address(iframe),
					(void *)iframe->cpu.pc);
				break;
			} else if ((iframe->cpu.sr & SR_IP_MASK) != 0) {
				// interrupts disabled

				// If a page fault handler is installed, we're allowed to be here.
				// TODO: Now we are generally allowing user_memcpy() with interrupts
				// disabled, which in most cases is a bug. We should add some thread
				// flag allowing to explicitly indicate that this handling is desired.
				if (thread && thread->fault_handler != 0) {
					iframe->cpu.pc = thread->fault_handler;
						return;
				}

				// if the interrupts were disabled, and we are not running the
				// kernel startup the page fault was not allowed to happen and
				// we must panic
				panic("page fault, but interrupts were disabled. Touching "
					"address %p from ip %p\n", (void *)fault_address(iframe),
					(void *)iframe->cpu.pc);
				break;
			} else if (thread != NULL && thread->page_faults_allowed < 1) {
				panic("page fault not allowed at this place. Touching address "
					"%p from ip %p\n", (void *)fault_address(iframe),
					(void *)iframe->cpu.pc);
			}

			enable_interrupts();

			addr_t newip;

			vm_page_fault(fault_address(iframe), iframe->cpu.pc,
				fault_was_write(iframe), // store or load
				iframe->cpu.sr & SR_S, // was the system in user or supervisor
				&newip);
			if (newip != 0) {
				// the page fault handler wants us to modify the iframe to set the
				// IP the cpu will return to to be this ip
				iframe->cpu.pc = newip;
			}
 			break;
		}

		case 24: // spurious interrupt
			dprintf("spurious interrupt\n");
			break;
		case 25: // autovector interrupt
		case 26: // autovector interrupt
		case 27: // autovector interrupt
		case 28: // autovector interrupt
		case 29: // autovector interrupt
		case 30: // autovector interrupt
		case 31: // autovector interrupt
		{
#if 0
			if (!sPIC) {
				panic("m68k_exception_entry(): external interrupt although we "
					"don't have a PIC driver!");
				break;
			}
#endif
			M68KPlatform::Default()->AcknowledgeIOInterrupt(vector);

dprintf("handling I/O interrupts...\n");
			int_io_interrupt_handler(vector, true);
#if 0
			while ((irq = sPIC->acknowledge_io_interrupt(sPICCookie)) >= 0) {
// TODO: correctly pass level-triggered vs. edge-triggered to the handler!
				int_io_interrupt_handler(irq, true);
			}
#endif
dprintf("handling I/O interrupts done\n");
			hardwareInterrupt = true;
			break;
		}

		case 9: // trace
		default:
			// vectors >= 64 are user defined vectors, used for IRQ
			if (vector >= 64) {
				if (M68KPlatform::Default()->AcknowledgeIOInterrupt(vector)) {
					int_io_interrupt_handler(vector, true);
					break;
				}
			}
			dprintf("unhandled exception type 0x%x\n", vector);
			print_iframe(iframe);
			panic("unhandled exception type\n");
	}

	int state = disable_interrupts();
	if (thread->cpu->invoke_scheduler) {
		SpinLocker schedulerLocker(gSchedulerLock);
		scheduler_reschedule();
		schedulerLocker.Unlock();
		restore_interrupts(state);
	} else if (hardwareInterrupt && thread->post_interrupt_callback != NULL) {
		void (*callback)(void*) = thread->post_interrupt_callback;
		void* data = thread->post_interrupt_data;

		thread->post_interrupt_callback = NULL;
		thread->post_interrupt_data = NULL;

		restore_interrupts(state);

		callback(data);
	}

	// pop iframe
	if (thread)
		m68k_pop_iframe(&thread->arch_info.iframes);
	else
		m68k_pop_iframe(&gBootFrameStack);
}


status_t
arch_int_init(kernel_args *args)
{
	status_t err;
	addr_t vbr;
	int i;

	gExceptionVectors = (m68k_exception_handler *)args->arch_args.vir_vbr;

	/* fill in the vector table */
	for (i = 0; i < M68K_EXCEPTION_VECTOR_COUNT; i++)
		gExceptionVectors[i] = &__m68k_exception_common;

	vbr = args->arch_args.phys_vbr;
	/* point VBR to the new table */
	asm volatile  ("movec %0,%%vbr" : : "r"(vbr):);

	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	status_t err;
	err = M68KPlatform::Default()->InitPIC(args);
	return err;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


#if 0 /* PIC modules */
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
			fDeviceManager->put_device_node(fParent);
		if (fNode != NULL)
			fDeviceManager->put_device_node(fNode);
	}

	void Rewind()
	{
		fNode = fDeviceManager->get_root();
	}

	bool HasNext() const
	{
		return (fNode != NULL);
	}

	device_node_handle Next()
	{
		if (fNode == NULL)
			return NULL;

		device_node_handle foundNode = fNode;

		// get first child
		device_node_handle child = NULL;
		if (fDeviceManager->get_next_child_device(fNode, &child, NULL)
				== B_OK) {
			// move to the child node
			if (fParent != NULL)
				fDeviceManager->put_device_node(fParent);
			fParent = fNode;
			fNode = child;

		// no more children; backtrack to find the next sibling
		} else {
			while (fParent != NULL) {
				if (fDeviceManager->get_next_child_device(fParent, &fNode, NULL)
						== B_OK) {
						// get_next_child_device() always puts the node
					break;
				}
				fNode = fParent;
				fParent = fDeviceManager->get_parent(fNode);
			}

			// if we hit the root node again, we're done
			if (fParent == NULL) {
				fDeviceManager->put_device_node(fNode);
				fNode = NULL;
			}
		}

		return foundNode;
	}

private:
	device_manager_info *fDeviceManager;
	device_node_handle	fNode;
	device_node_handle	fParent;
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
probe_pic_device(device_node_handle node, PICModuleList &picModules)
{
	for (PICModule *module = picModules.Head();
		 module;
		 module = picModules.GetNext(module)) {
		bool noConnection;
		if (module->module->info.supports_device(node, &noConnection) > 0) {
			if (module->module->info.register_device(node) == B_OK)
				return true;
		}
	}

	return false;
}
#endif /* PIC modules */

status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
#if 0 /* PIC modules */
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
	while (device_node_handle node = iterator.Next())
		probe_pic_device(node, picModules);

	// iterate through the tree again and get an interrupt controller node
	iterator.Rewind();
	while (device_node_handle node = iterator.Next()) {
		char *deviceType;
		if (deviceManager->get_attr_string(node, B_DRIVER_DEVICE_TYPE,
				&deviceType, false) == B_OK) {
			bool isPIC
				= (strcmp(deviceType, B_INTERRUPT_CONTROLLER_DRIVER_TYPE) == 0);
			free(deviceType);

			if (isPIC) {
				driver_module_info *driver;
				void *driverCookie;
				error = deviceManager->init_driver(node, NULL, &driver,
					&driverCookie);
				if (error == B_OK) {
					sPIC = (interrupt_controller_module_info *)driver;
					sPICCookie = driverCookie;
					return B_OK;
				}
			}
		}
	}

#endif /* PIC modules */

	// no PIC found
	panic("arch_int_init_post_device_manager(): Found no supported PIC!");

	return B_ENTRY_NOT_FOUND;
}


#if 0//PPC
// #pragma mark -

struct m68k_cpu_exception_context *
m68k_get_cpu_exception_context(int cpu)
{
	return sCPUExceptionContexts + cpu;
}


void
m68k_set_current_cpu_exception_context(struct m68k_cpu_exception_context *context)
{
	// translate to physical address
	addr_t physicalPage;
	addr_t inPageOffset = (addr_t)context & (B_PAGE_SIZE - 1);
	status_t error = vm_get_page_mapping(VMAddressSpace::KernelID(),
		(addr_t)context - inPageOffset, &physicalPage);
	if (error != B_OK) {
		panic("m68k_set_current_cpu_exception_context(): Failed to get physical "
			"address!");
		return;
	}

	asm volatile("mtsprg0 %0" : : "r"(physicalPage + inPageOffset));
}

#endif
