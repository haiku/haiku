/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/debug.h>

#include <stdio.h>
#include <stdlib.h>

#include <ByteOrder.h>
#include <TypeConstants.h>

#include <cpu.h>
#include <debug.h>
#include <debug_heap.h>
#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>


struct stack_frame {
	stack_frame*	previous;
	addr_t			return_address;
};

#define NUM_PREVIOUS_LOCATIONS 32


static bool is_kernel_stack_address(Thread* thread, addr_t address);


static bool
already_visited(addr_t* visited, int32* _last, int32* _num, addr_t bp)
{
	int32 last = *_last;
	int32 num = *_num;
	int32 i;

	for (i = 0; i < num; i++) {
		if (visited[(NUM_PREVIOUS_LOCATIONS + last - i) % NUM_PREVIOUS_LOCATIONS] == bp)
			return true;
	}

	*_last = last = (last + 1) % NUM_PREVIOUS_LOCATIONS;
	visited[last] = bp;

	if (num < NUM_PREVIOUS_LOCATIONS)
		*_num = num + 1;

	return false;
}


/*!	Safe to be called only from outside the debugger.
*/
static status_t
get_next_frame_no_debugger(addr_t bp, addr_t* _next, addr_t* _ip,
	bool onKernelStack, Thread* thread)
{
	// TODO: Do this more efficiently in assembly.
	stack_frame frame;
	if (onKernelStack
			&& is_kernel_stack_address(thread, bp + sizeof(frame) - 1)) {
		memcpy(&frame, (void*)bp, sizeof(frame));
	} else if (!IS_USER_ADDRESS(bp)
			|| user_memcpy(&frame, (void*)bp, sizeof(frame)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	*_ip = frame.return_address;
	*_next = (addr_t)frame.previous;

	return B_OK;
}


/*!	Safe to be called only from inside the debugger.
*/
static status_t
get_next_frame_debugger(addr_t bp, addr_t* _next, addr_t* _ip)
{
	stack_frame frame;
	if (debug_memcpy(B_CURRENT_TEAM, &frame, (void*)bp, sizeof(frame)) != B_OK)
		return B_BAD_ADDRESS;

	*_ip = frame.return_address;
	*_next = (addr_t)frame.previous;

	return B_OK;
}


static status_t
lookup_symbol(Thread* thread, addr_t address, addr_t* _baseAddress,
	const char** _symbolName, const char** _imageName, bool* _exactMatch)
{
	status_t status = B_ENTRY_NOT_FOUND;

	if (IS_KERNEL_ADDRESS(address)) {
		// a kernel symbol
		status = elf_debug_lookup_symbol_address(address, _baseAddress,
			_symbolName, _imageName, _exactMatch);
	} else if (thread != NULL && thread->team != NULL) {
		// try a lookup using the userland runtime loader structures
		status = elf_debug_lookup_user_symbol_address(thread->team, address,
			_baseAddress, _symbolName, _imageName, _exactMatch);

		if (status != B_OK) {
			// try to locate the image in the images loaded into user space
			status = image_debug_lookup_user_symbol_address(thread->team,
				address, _baseAddress, _symbolName, _imageName, _exactMatch);
		}
	}

	return status;
}


#ifndef __x86_64__


static void
set_debug_argument_variable(int32 index, uint64 value)
{
	char name[8];
	snprintf(name, sizeof(name), "_arg%ld", index);
	set_debug_variable(name, value);
}


template<typename Type>
static Type
read_function_argument_value(void* argument, bool& _valueKnown)
{
	Type value;
	if (debug_memcpy(B_CURRENT_TEAM, &value, argument, sizeof(Type)) == B_OK) {
		_valueKnown = true;
		return value;
	}

	_valueKnown = false;
	return 0;
}


static status_t
print_demangled_call(const char* image, const char* symbol, addr_t args,
	bool noObjectMethod, bool addDebugVariables)
{
	static const size_t kBufferSize = 256;
	char* buffer = (char*)debug_malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	bool isObjectMethod;
	const char* name = debug_demangle_symbol(symbol, buffer, kBufferSize,
		&isObjectMethod);
	if (name == NULL) {
		debug_free(buffer);
		return B_ERROR;
	}

	uint32* arg = (uint32*)args;

	if (noObjectMethod)
		isObjectMethod = false;
	if (isObjectMethod) {
		const char* lastName = strrchr(name, ':') - 1;
		int namespaceLength = lastName - name;

		uint32 argValue = 0;
		if (debug_memcpy(B_CURRENT_TEAM, &argValue, arg, 4) == B_OK) {
			kprintf("<%s> %.*s<\33[32m%#" B_PRIx32 "\33[0m>%s", image,
				namespaceLength, name, argValue, lastName);
		} else
			kprintf("<%s> %.*s<\?\?\?>%s", image, namespaceLength, name, lastName);

		if (addDebugVariables)
			set_debug_variable("_this", argValue);
		arg++;
	} else
		kprintf("<%s> %s", image, name);

	kprintf("(");

	size_t length;
	int32 type, i = 0;
	uint32 cookie = 0;
	while (debug_get_next_demangled_argument(&cookie, symbol, buffer,
			kBufferSize, &type, &length) == B_OK) {
		if (i++ > 0)
			kprintf(", ");

		// retrieve value and type identifier

		uint64 value;
		bool valueKnown = false;

		switch (type) {
			case B_INT64_TYPE:
				value = read_function_argument_value<int64>(arg, valueKnown);
				if (valueKnown)
					kprintf("int64: \33[34m%Ld\33[0m", value);
				break;
			case B_INT32_TYPE:
				value = read_function_argument_value<int32>(arg, valueKnown);
				if (valueKnown)
					kprintf("int32: \33[34m%ld\33[0m", (int32)value);
				break;
			case B_INT16_TYPE:
				value = read_function_argument_value<int16>(arg, valueKnown);
				if (valueKnown)
					kprintf("int16: \33[34m%d\33[0m", (int16)value);
				break;
			case B_INT8_TYPE:
				value = read_function_argument_value<int8>(arg, valueKnown);
				if (valueKnown)
					kprintf("int8: \33[34m%d\33[0m", (int8)value);
				break;
			case B_UINT64_TYPE:
				value = read_function_argument_value<uint64>(arg, valueKnown);
				if (valueKnown) {
					kprintf("uint64: \33[34m%#Lx\33[0m", value);
					if (value < 0x100000)
						kprintf(" (\33[34m%Lu\33[0m)", value);
				}
				break;
			case B_UINT32_TYPE:
				value = read_function_argument_value<uint32>(arg, valueKnown);
				if (valueKnown) {
					kprintf("uint32: \33[34m%#lx\33[0m", (uint32)value);
					if (value < 0x100000)
						kprintf(" (\33[34m%lu\33[0m)", (uint32)value);
				}
				break;
			case B_UINT16_TYPE:
				value = read_function_argument_value<uint16>(arg, valueKnown);
				if (valueKnown) {
					kprintf("uint16: \33[34m%#x\33[0m (\33[34m%u\33[0m)",
						(uint16)value, (uint16)value);
				}
				break;
			case B_UINT8_TYPE:
				value = read_function_argument_value<uint8>(arg, valueKnown);
				if (valueKnown) {
					kprintf("uint8: \33[34m%#x\33[0m (\33[34m%u\33[0m)",
						(uint8)value, (uint8)value);
				}
				break;
			case B_BOOL_TYPE:
				value = read_function_argument_value<uint8>(arg, valueKnown);
				if (valueKnown)
					kprintf("\33[34m%s\33[0m", value ? "true" : "false");
				break;
			default:
				if (buffer[0])
					kprintf("%s: ", buffer);

				if (length == 4) {
					value = read_function_argument_value<uint32>(arg,
						valueKnown);
					if (valueKnown) {
						if (value == 0
							&& (type == B_POINTER_TYPE || type == B_REF_TYPE))
							kprintf("NULL");
						else
							kprintf("\33[34m%#lx\33[0m", (uint32)value);
					}
					break;
				}


				if (length == 8) {
					value = read_function_argument_value<uint64>(arg,
						valueKnown);
				} else
					value = (uint64)arg;

				if (valueKnown)
					kprintf("\33[34m%#Lx\33[0m", value);
				break;
		}

		if (!valueKnown)
			kprintf("???");

		if (valueKnown && type == B_STRING_TYPE) {
			if (value == 0)
				kprintf(" \33[31m\"<NULL>\"\33[0m");
			else if (debug_strlcpy(B_CURRENT_TEAM, buffer, (char*)(addr_t)value,
					kBufferSize) < B_OK) {
				kprintf(" \33[31m\"<\?\?\?>\"\33[0m");
			} else
				kprintf(" \33[36m\"%s\"\33[0m", buffer);
		}

		if (addDebugVariables)
			set_debug_argument_variable(i, value);
		arg = (uint32*)((uint8*)arg + length);
	}

	debug_free(buffer);

	kprintf(")");
	return B_OK;
}


#else	// __x86_64__


static status_t
print_demangled_call(const char* image, const char* symbol, addr_t args,
	bool noObjectMethod, bool addDebugVariables)
{
	// Since x86_64 uses registers rather than the stack for the first 6
	// arguments we cannot use the same method as x86 to read the function
	// arguments. Maybe we need DWARF support in the kernel debugger. For now
	// just print out the function signature without the argument values.

	static const size_t kBufferSize = 256;
	char* buffer = (char*)debug_malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	bool isObjectMethod;
	const char* name = debug_demangle_symbol(symbol, buffer, kBufferSize,
		&isObjectMethod);
	if (name == NULL) {
		debug_free(buffer);
		return B_ERROR;
	}

	kprintf("<%s> %s(", image, name);

	size_t length;
	int32 type, i = 0;
	uint32 cookie = 0;
	while (debug_get_next_demangled_argument(&cookie, symbol, buffer,
			kBufferSize, &type, &length) == B_OK) {
		if (i++ > 0)
			kprintf(", ");

		if (buffer[0])
			kprintf("%s", buffer);
		else
			kprintf("???");
	}

	debug_free(buffer);

	kprintf(")");
	return B_OK;
}


#endif	// __x86_64__


static void
print_stack_frame(Thread* thread, addr_t ip, addr_t bp, addr_t nextBp,
	int32 callIndex, bool demangle)
{
	const char* symbol;
	const char* image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status;
	addr_t diff;

	diff = nextBp - bp;

	// MSB set = kernel space/user space switch
	if (diff & ~((addr_t)-1 >> 1))
		diff = 0;

	status = lookup_symbol(thread, ip, &baseAddress, &symbol, &image,
		&exactMatch);

	kprintf("%2" B_PRId32 " %0*lx (+%4ld) %0*lx   ", callIndex,
		B_PRINTF_POINTER_WIDTH, bp, diff, B_PRINTF_POINTER_WIDTH, ip);

	if (status == B_OK) {
		if (exactMatch && demangle) {
			status = print_demangled_call(image, symbol,
				nextBp + sizeof(stack_frame), false, false);
		}

		if (!exactMatch || !demangle || status != B_OK) {
			if (symbol != NULL) {
				kprintf("<%s> %s%s", image, symbol,
					exactMatch ? "" : " (nearest)");
			} else
				kprintf("<%s@%p> <unknown>", image, (void*)baseAddress);
		}

		kprintf(" + %#04lx\n", ip - baseAddress);
	} else {
		VMArea *area = NULL;
		if (thread != NULL && thread->team != NULL
			&& thread->team->address_space != NULL) {
			area = thread->team->address_space->LookupArea(ip);
		}
		if (area != NULL) {
			kprintf("%" B_PRId32 ":%s@%p + %#lx\n", area->id, area->name,
				(void*)area->Base(), ip - area->Base());
		} else
			kprintf("\n");
	}
}


static void
print_iframe(iframe* frame)
{
	bool isUser = IFRAME_IS_USER(frame);

#ifdef __x86_64__
	kprintf("%s iframe at %p (end = %p)\n", isUser ? "user" : "kernel", frame,
		frame + 1);

	kprintf(" rax %#-18lx    rbx %#-18lx    rcx %#lx\n", frame->ax,
		frame->bx, frame->cx);
	kprintf(" rdx %#-18lx    rsi %#-18lx    rdi %#lx\n", frame->dx,
		frame->si, frame->di);
	kprintf(" rbp %#-18lx     r8 %#-18lx     r9 %#lx\n", frame->bp,
		frame->r8, frame->r9);
	kprintf(" r10 %#-18lx    r11 %#-18lx    r12 %#lx\n", frame->r10,
		frame->r11, frame->r12);
	kprintf(" r13 %#-18lx    r14 %#-18lx    r15 %#lx\n", frame->r13,
		frame->r14, frame->r15);
	kprintf(" rip %#-18lx    rsp %#-18lx rflags %#lx\n", frame->ip,
		frame->sp, frame->flags);
#else
	kprintf("%s iframe at %p (end = %p)\n", isUser ? "user" : "kernel", frame,
		isUser ? (void*)(frame + 1) : (void*)&frame->user_sp);

	kprintf(" eax %#-10lx    ebx %#-10lx     ecx %#-10lx  edx %#lx\n",
		frame->ax, frame->bx, frame->cx, frame->dx);
	kprintf(" esi %#-10lx    edi %#-10lx     ebp %#-10lx  esp %#lx\n",
		frame->si, frame->di, frame->bp, frame->sp);
	kprintf(" eip %#-10lx eflags %#-10lx", frame->ip, frame->flags);
	if (isUser) {
		// from user space
		kprintf("user esp %#lx", frame->user_sp);
	}
	kprintf("\n");
#endif

	kprintf(" vector: %#lx, error code: %#lx\n", frame->vector,
		frame->error_code);
}


static bool
setup_for_thread(char* arg, Thread** _thread, addr_t* _bp,
	phys_addr_t* _oldPageDirectory)
{
	Thread* thread = NULL;

	if (arg != NULL) {
		thread_id id = strtoul(arg, NULL, 0);
		thread = Thread::GetDebug(id);
		if (thread == NULL) {
			kprintf("could not find thread %" B_PRId32 "\n", id);
			return false;
		}

		if (id != thread_get_current_thread_id()) {
			// switch to the page directory of the new thread to be
			// able to follow the stack trace into userland
			phys_addr_t newPageDirectory = x86_next_page_directory(
				thread_get_current_thread(), thread);

			if (newPageDirectory != 0) {
				*_oldPageDirectory = x86_read_cr3();
				x86_write_cr3(newPageDirectory);
			}

			if (thread->state == B_THREAD_RUNNING) {
				// The thread is currently running on another CPU.
				if (thread->cpu == NULL)
					return false;
				arch_debug_registers* registers = debug_get_debug_registers(
					thread->cpu->cpu_num);
				if (registers == NULL)
					return false;
				*_bp = registers->bp;
			} else {
				// Read frame pointer from the thread's stack.
				*_bp = thread->arch_info.GetFramePointer();
			}
		} else
			thread = NULL;
	}

	if (thread == NULL) {
		// if we don't have a thread yet, we want the current one
		// (ebp has been set by the caller for this case already)
		thread = thread_get_current_thread();
	}

	*_thread = thread;
	return true;
}


static bool
is_double_fault_stack_address(int32 cpu, addr_t address)
{
	size_t size;
	addr_t bottom = (addr_t)x86_get_double_fault_stack(cpu, &size);
	return address >= bottom && address < bottom + size;
}


static bool
is_kernel_stack_address(Thread* thread, addr_t address)
{
	// We don't have a thread pointer in the early boot process, but then we are
	// on the kernel stack for sure.
	if (thread == NULL)
		return IS_KERNEL_ADDRESS(address);

	// Also in the early boot process we might have a thread structure, but it
	// might not have its kernel stack attributes set yet.
	if (thread->kernel_stack_top == 0)
		return IS_KERNEL_ADDRESS(address);

	return (address >= thread->kernel_stack_base
			&& address < thread->kernel_stack_top)
		|| (thread->cpu != NULL
			&& is_double_fault_stack_address(thread->cpu->cpu_num, address));
}


static bool
is_iframe(Thread* thread, addr_t frame)
{
	if (!is_kernel_stack_address(thread, frame))
		return false;

	addr_t previousFrame = *(addr_t*)frame;
	return ((previousFrame & ~(addr_t)IFRAME_TYPE_MASK) == 0
		&& previousFrame != 0);
}


static iframe*
find_previous_iframe(Thread* thread, addr_t frame)
{
	// iterate backwards through the stack frames, until we hit an iframe
	while (is_kernel_stack_address(thread, frame)) {
		if (is_iframe(thread, frame))
			return (iframe*)frame;

		frame = *(addr_t*)frame;
	}

	return NULL;
}


static iframe*
get_previous_iframe(Thread* thread, iframe* frame)
{
	if (frame == NULL)
		return NULL;

	return find_previous_iframe(thread, frame->bp);
}


static iframe*
get_current_iframe(Thread* thread)
{
	if (thread == thread_get_current_thread())
		return x86_get_current_iframe();

	// NOTE: This doesn't work, if the thread is running (on another CPU).
	return find_previous_iframe(thread, thread->arch_info.GetFramePointer());
}


#define CHECK_DEBUG_VARIABLE(_name, _member, _settable) \
	if (strcmp(variableName, _name) == 0) { \
		settable = _settable; \
		return &_member; \
	}


static size_t*
find_debug_variable(const char* variableName, bool& settable)
{
	iframe* frame = get_current_iframe(debug_get_debugged_thread());
	if (frame == NULL)
		return NULL;

#ifdef __x86_64__
	CHECK_DEBUG_VARIABLE("cs", frame->cs, false);
	CHECK_DEBUG_VARIABLE("ss", frame->ss, false);
	CHECK_DEBUG_VARIABLE("r15", frame->r15, true);
	CHECK_DEBUG_VARIABLE("r14", frame->r14, true);
	CHECK_DEBUG_VARIABLE("r13", frame->r13, true);
	CHECK_DEBUG_VARIABLE("r12", frame->r12, true);
	CHECK_DEBUG_VARIABLE("r11", frame->r11, true);
	CHECK_DEBUG_VARIABLE("r10", frame->r10, true);
	CHECK_DEBUG_VARIABLE("r9", frame->r9, true);
	CHECK_DEBUG_VARIABLE("r8", frame->r8, true);
	CHECK_DEBUG_VARIABLE("rbp", frame->bp, true);
	CHECK_DEBUG_VARIABLE("rsi", frame->si, true);
	CHECK_DEBUG_VARIABLE("rdi", frame->di, true);
	CHECK_DEBUG_VARIABLE("rdx", frame->dx, true);
	CHECK_DEBUG_VARIABLE("rcx", frame->cx, true);
	CHECK_DEBUG_VARIABLE("rbx", frame->bx, true);
	CHECK_DEBUG_VARIABLE("rax", frame->ax, true);
	CHECK_DEBUG_VARIABLE("rip", frame->ip, true);
	CHECK_DEBUG_VARIABLE("rflags", frame->flags, true);
	CHECK_DEBUG_VARIABLE("rsp", frame->sp, true);
#else
	CHECK_DEBUG_VARIABLE("gs", frame->gs, false);
	CHECK_DEBUG_VARIABLE("fs", frame->fs, false);
	CHECK_DEBUG_VARIABLE("es", frame->es, false);
	CHECK_DEBUG_VARIABLE("ds", frame->ds, false);
	CHECK_DEBUG_VARIABLE("cs", frame->cs, false);
	CHECK_DEBUG_VARIABLE("edi", frame->di, true);
	CHECK_DEBUG_VARIABLE("esi", frame->si, true);
	CHECK_DEBUG_VARIABLE("ebp", frame->bp, true);
	CHECK_DEBUG_VARIABLE("esp", frame->sp, true);
	CHECK_DEBUG_VARIABLE("ebx", frame->bx, true);
	CHECK_DEBUG_VARIABLE("edx", frame->dx, true);
	CHECK_DEBUG_VARIABLE("ecx", frame->cx, true);
	CHECK_DEBUG_VARIABLE("eax", frame->ax, true);
	CHECK_DEBUG_VARIABLE("orig_eax", frame->orig_eax, true);
	CHECK_DEBUG_VARIABLE("orig_edx", frame->orig_edx, true);
	CHECK_DEBUG_VARIABLE("eip", frame->ip, true);
	CHECK_DEBUG_VARIABLE("eflags", frame->flags, true);

	if (IFRAME_IS_USER(frame)) {
		CHECK_DEBUG_VARIABLE("user_esp", frame->user_sp, true);
		CHECK_DEBUG_VARIABLE("user_ss", frame->user_ss, false);
	}
#endif

	return NULL;
}


static int
stack_trace(int argc, char** argv)
{
	static const char* usage = "usage: %s [-d] [ <thread id> ]\n"
		"Prints a stack trace for the current, respectively the specified\n"
		"thread.\n"
		"  -d           -  Disables the demangling of the symbols.\n"
		"  <thread id>  -  The ID of the thread for which to print the stack\n"
		"                  trace.\n";
	bool demangle = true;
	int32 threadIndex = 1;
	if (argc > 1 && !strcmp(argv[1], "-d")) {
		demangle = false;
		threadIndex++;
	}

	if (argc > threadIndex + 1
		|| (argc == 2 && strcmp(argv[1], "--help") == 0)) {
		kprintf(usage, argv[0]);
		return 0;
	}

	addr_t previousLocations[NUM_PREVIOUS_LOCATIONS];
	Thread* thread = NULL;
	phys_addr_t oldPageDirectory = 0;
	addr_t bp = x86_get_stack_frame();
	int32 num = 0, last = 0;

	if (!setup_for_thread(argc == threadIndex + 1 ? argv[threadIndex] : NULL,
			&thread, &bp, &oldPageDirectory))
		return 0;

	DebuggedThreadSetter threadSetter(thread);

	if (thread != NULL) {
		kprintf("stack trace for thread %" B_PRId32 " \"%s\"\n", thread->id,
			thread->name);

		kprintf("    kernel stack: %p to %p\n",
			(void*)thread->kernel_stack_base,
			(void*)(thread->kernel_stack_top));
		if (thread->user_stack_base != 0) {
			kprintf("      user stack: %p to %p\n",
				(void*)thread->user_stack_base,
				(void*)(thread->user_stack_base + thread->user_stack_size));
		}
	}

	kprintf("%-*s            %-*s   <image>:function + offset\n",
		B_PRINTF_POINTER_WIDTH, "frame", B_PRINTF_POINTER_WIDTH, "caller");

	bool onKernelStack = true;

	for (int32 callIndex = 0;; callIndex++) {
		onKernelStack = onKernelStack
			&& is_kernel_stack_address(thread, bp);

		if (onKernelStack && is_iframe(thread, bp)) {
			iframe* frame = (iframe*)bp;

			print_iframe(frame);
			print_stack_frame(thread, frame->ip, bp, frame->bp, callIndex,
				demangle);

 			bp = frame->bp;
		} else {
			addr_t ip, nextBp;

			if (get_next_frame_debugger(bp, &nextBp, &ip) != B_OK) {
				kprintf("%0*lx -- read fault\n", B_PRINTF_POINTER_WIDTH, bp);
				break;
			}

			if (ip == 0 || bp == 0)
				break;

			print_stack_frame(thread, ip, bp, nextBp, callIndex, demangle);
			bp = nextBp;
		}

		if (already_visited(previousLocations, &last, &num, bp)) {
			kprintf("circular stack frame: %p!\n", (void*)bp);
			break;
		}
		if (bp == 0)
			break;
	}

	if (oldPageDirectory != 0) {
		// switch back to the previous page directory to no cause any troubles
		x86_write_cr3(oldPageDirectory);
	}

	return 0;
}


#ifndef __x86_64__
static void
print_call(Thread *thread, addr_t eip, addr_t ebp, addr_t nextEbp,
	int32 argCount)
{
	const char *symbol, *image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status;
	bool demangled = false;
	int32 *arg = (int32 *)(nextEbp + 8);

	status = lookup_symbol(thread, eip, &baseAddress, &symbol, &image,
		&exactMatch);

	kprintf("%08lx %08lx   ", ebp, eip);

	if (status == B_OK) {
		if (symbol != NULL) {
			if (exactMatch && (argCount == 0 || argCount == -1)) {
				status = print_demangled_call(image, symbol, (addr_t)arg,
					argCount == -1, true);
				if (status == B_OK)
					demangled = true;
			}
			if (!demangled) {
				kprintf("<%s>:%s%s", image, symbol,
					exactMatch ? "" : " (nearest)");
			}
		} else {
			kprintf("<%s@%p>:unknown + 0x%04lx", image,
				(void *)baseAddress, eip - baseAddress);
		}
	} else {
		VMArea *area = NULL;
		if (thread->team->address_space != NULL)
			area = thread->team->address_space->LookupArea(eip);
		if (area != NULL) {
			kprintf("%ld:%s@%p + %#lx", area->id, area->name,
				(void *)area->Base(), eip - area->Base());
		}
	}

	if (!demangled) {
		kprintf("(");

		for (int32 i = 0; i < argCount; i++) {
			if (i > 0)
				kprintf(", ");
			kprintf("%#lx", *arg);
			if (*arg > -0x10000 && *arg < 0x10000)
				kprintf(" (%ld)", *arg);

			set_debug_argument_variable(i + 1, *(uint32 *)arg);
			arg++;
		}

		kprintf(")\n");
	} else
		kprintf("\n");

	set_debug_variable("_frame", nextEbp);
}


static int
show_call(int argc, char **argv)
{
	static const char* usage
		= "usage: %s [ <thread id> ] <call index> [ -<arg count> ]\n"
		"Prints a function call with parameters of the current, respectively\n"
		"the specified thread.\n"
		"  <thread id>   -  The ID of the thread for which to print the call.\n"
		"  <call index>  -  The index of the call in the stack trace.\n"
		"  <arg count>   -  The number of call arguments to print (use 'c' to\n"
		"                   force the C++ demangler to use class methods,\n"
		"                   use 'd' to disable demangling).\n";
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		kprintf(usage, argv[0]);
		return 0;
	}

	Thread *thread = NULL;
	phys_addr_t oldPageDirectory = 0;
	addr_t ebp = x86_get_stack_frame();
	int32 argCount = 0;

	if (argc >= 2 && argv[argc - 1][0] == '-') {
		if (argv[argc - 1][1] == 'c')
			argCount = -1;
		else if (argv[argc - 1][1] == 'd')
			argCount = -2;
		else
			argCount = strtoul(argv[argc - 1] + 1, NULL, 0);

		if (argCount < -2 || argCount > 16) {
			kprintf("Invalid argument count \"%ld\".\n", argCount);
			return 0;
		}
		argc--;
	}

	if (argc < 2 || argc > 3) {
		kprintf(usage, argv[0]);
		return 0;
	}

	if (!setup_for_thread(argc == 3 ? argv[1] : NULL, &thread, &ebp,
			&oldPageDirectory))
		return 0;

	DebuggedThreadSetter threadSetter(thread);

	int32 callIndex = strtoul(argv[argc == 3 ? 2 : 1], NULL, 0);

	if (thread != NULL)
		kprintf("thread %ld, %s\n", thread->id, thread->name);

	bool onKernelStack = true;

	for (int32 index = 0; index <= callIndex; index++) {
		onKernelStack = onKernelStack
			&& is_kernel_stack_address(thread, ebp);

		if (onKernelStack && is_iframe(thread, ebp)) {
			struct iframe *frame = (struct iframe *)ebp;

			if (index == callIndex)
				print_call(thread, frame->ip, ebp, frame->bp, argCount);

 			ebp = frame->bp;
		} else {
			addr_t eip, nextEbp;

			if (get_next_frame_debugger(ebp, &nextEbp, &eip) != B_OK) {
				kprintf("%08lx -- read fault\n", ebp);
				break;
			}

			if (eip == 0 || ebp == 0)
				break;

			if (index == callIndex)
				print_call(thread, eip, ebp, nextEbp, argCount);

			ebp = nextEbp;
		}

		if (ebp == 0)
			break;
	}

	if (oldPageDirectory != 0) {
		// switch back to the previous page directory to not cause any troubles
		x86_write_cr3(oldPageDirectory);
	}

	return 0;
}
#endif


static int
dump_iframes(int argc, char** argv)
{
	static const char* usage = "usage: %s [ <thread id> ]\n"
		"Prints the iframe stack for the current, respectively the specified\n"
		"thread.\n"
		"  <thread id>  -  The ID of the thread for which to print the iframe\n"
		"                  stack.\n";
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		kprintf(usage, argv[0]);
		return 0;
	}

	Thread* thread = NULL;

	if (argc < 2) {
		thread = thread_get_current_thread();
	} else if (argc == 2) {
		thread_id id = strtoul(argv[1], NULL, 0);
		thread = Thread::GetDebug(id);
		if (thread == NULL) {
			kprintf("could not find thread %" B_PRId32 "\n", id);
			return 0;
		}
	} else if (argc > 2) {
		kprintf(usage, argv[0]);
		return 0;
	}

	if (thread != NULL) {
		kprintf("iframes for thread %" B_PRId32 " \"%s\"\n", thread->id,
			thread->name);
	}

	DebuggedThreadSetter threadSetter(thread);

	iframe* frame = find_previous_iframe(thread, x86_get_stack_frame());
	while (frame != NULL) {
		print_iframe(frame);
		frame = get_previous_iframe(thread, frame);
	}

	return 0;
}


static bool
is_calling(Thread* thread, addr_t ip, const char* pattern, addr_t start,
	addr_t end)
{
	if (pattern == NULL)
		return ip >= start && ip < end;

	if (!IS_KERNEL_ADDRESS(ip))
		return false;

	const char* symbol;
	if (lookup_symbol(thread, ip, NULL, &symbol, NULL, NULL) != B_OK)
		return false;

	return strstr(symbol, pattern);
}


static int
cmd_in_context(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// get the thread ID
	const char* commandLine = argv[1];
	char threadIDString[16];
	if (parse_next_debug_command_argument(&commandLine, threadIDString,
			sizeof(threadIDString)) != B_OK) {
		kprintf("Failed to parse thread ID.\n");
		return 0;
	}

	if (commandLine == NULL) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 threadID;
	if (!evaluate_debug_expression(threadIDString, &threadID, false))
		return 0;

	// get the thread
	Thread* thread = Thread::GetDebug(threadID);
	if (thread == NULL) {
		kprintf("Could not find thread with ID \"%s\".\n", threadIDString);
		return 0;
	}

	// switch the page directory, if necessary
	phys_addr_t oldPageDirectory = 0;
	if (thread != thread_get_current_thread()) {
		phys_addr_t newPageDirectory = x86_next_page_directory(
			thread_get_current_thread(), thread);

		if (newPageDirectory != 0) {
			oldPageDirectory = x86_read_cr3();
			x86_write_cr3(newPageDirectory);
		}
	}

	// execute the command
	{
		DebuggedThreadSetter threadSetter(thread);
		evaluate_debug_command(commandLine);
	}

	// reset the page directory
	if (oldPageDirectory)
		x86_write_cr3(oldPageDirectory);

	return 0;
}


//	#pragma mark -


void
arch_debug_save_registers(arch_debug_registers* registers)
{
	// get the caller's frame pointer
	stack_frame* frame = (stack_frame*)x86_get_stack_frame();
	registers->bp = (addr_t)frame->previous;
}


void
arch_debug_stack_trace(void)
{
	stack_trace(0, NULL);
}


bool
arch_debug_contains_call(Thread* thread, const char* symbol, addr_t start,
	addr_t end)
{
	DebuggedThreadSetter threadSetter(thread);

	addr_t bp;
	if (thread == thread_get_current_thread())
		bp = x86_get_stack_frame();
	else {
		if (thread->state == B_THREAD_RUNNING) {
			// The thread is currently running on another CPU.
			if (thread->cpu == NULL)
				return false;
			arch_debug_registers* registers = debug_get_debug_registers(
				thread->cpu->cpu_num);
			if (registers == NULL)
				return false;
			bp = registers->bp;
		} else {
			// thread not running
			bp = thread->arch_info.GetFramePointer();
		}
	}

	for (;;) {
		if (!is_kernel_stack_address(thread, bp))
			break;

		if (is_iframe(thread, bp)) {
			iframe* frame = (iframe*)bp;

			if (is_calling(thread, frame->ip, symbol, start, end))
				return true;

 			bp = frame->bp;
		} else {
			addr_t ip, nextBp;

			if (get_next_frame_no_debugger(bp, &nextBp, &ip, true,
					thread) != B_OK
				|| ip == 0 || bp == 0)
				break;

			if (is_calling(thread, ip, symbol, start, end))
				return true;

			bp = nextBp;
		}

		if (bp == 0)
			break;
	}

	return false;
}


void*
arch_debug_get_caller(void)
{
	stack_frame* frame = (stack_frame*)x86_get_stack_frame();
	return (void*)frame->previous->return_address;
}


/*!	Captures a stack trace (the return addresses) of the current thread.
	\param returnAddresses The array the return address shall be written to.
	\param maxCount The maximum number of return addresses to be captured.
	\param skipIframes The number of interrupt frames that shall be skipped. If
		greater than 0, \a skipFrames is ignored.
	\param skipFrames The number of stack frames that shall be skipped.
	\param flags A combination of one or two of the following:
		- \c STACK_TRACE_KERNEL: Capture kernel return addresses.
		- \c STACK_TRACE_USER: Capture user return addresses.
	\return The number of return addresses written to the given array.
*/
int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
	// Keep skipping normal stack frames until we've skipped the iframes we're
	// supposed to skip.
	if (skipIframes > 0)
		skipFrames = INT_MAX;

	Thread* thread = thread_get_current_thread();
	int32 count = 0;
	addr_t bp = x86_get_stack_frame();
	bool onKernelStack = true;

	while (bp != 0 && count < maxCount) {
		onKernelStack = onKernelStack
			&& is_kernel_stack_address(thread, bp);
		if (!onKernelStack && (flags & STACK_TRACE_USER) == 0)
			break;

		addr_t ip;
		addr_t nextBp;

		if (onKernelStack && is_iframe(thread, bp)) {
			iframe* frame = (iframe*)bp;
			ip = frame->ip;
			nextBp = frame->bp;

			if (skipIframes > 0) {
				if (--skipIframes == 0)
					skipFrames = 0;
			}
		} else {
			if (get_next_frame_no_debugger(bp, &nextBp, &ip,
					onKernelStack, thread) != B_OK) {
				break;
			}
		}

		if (ip == 0)
			break;

		if (skipFrames <= 0
			&& ((flags & STACK_TRACE_KERNEL) != 0 || onKernelStack)) {
			returnAddresses[count++] = ip;
		} else
			skipFrames--;

		bp = nextBp;
	}

	return count;
}


/*!	Returns the program counter of the currently debugged (respectively this)
	thread where the innermost interrupts happened. \a _isSyscall, if specified,
	is set to whether this interrupt frame was created by a syscall. Returns
	\c NULL, if there's no such frame or a problem occurred retrieving it;
	\a _isSyscall won't be set in this case.
*/
void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
	iframe* frame = get_current_iframe(debug_get_debugged_thread());
	if (frame == NULL)
		return NULL;

	if (_isSyscall != NULL)
		*_isSyscall = frame->type == IFRAME_TYPE_SYSCALL;

	return (void*)(addr_t)frame->ip;
}


/*!	Sets the current thread to \c NULL.
	Invoked in the kernel debugger only.
*/
void
arch_debug_unset_current_thread(void)
{
	// Can't just write 0 to the GS base, that will cause the read from %gs:0
	// to fault. Instead point it at a NULL pointer, %gs:0 will get this value.
	static Thread* unsetThread = NULL;
#ifdef __x86_64__
	x86_write_msr(IA32_MSR_GS_BASE, (addr_t)&unsetThread);
#else
	asm volatile("mov %0, %%gs:0" : : "r" (unsetThread) : "memory");
#endif
}


bool
arch_is_debug_variable_defined(const char* variableName)
{
	bool settable;
	return find_debug_variable(variableName, settable);
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	bool settable;
	size_t* variable = find_debug_variable(variableName, settable);
	if (variable == NULL)
		return B_ENTRY_NOT_FOUND;

	if (!settable)
		return B_NOT_ALLOWED;

	*variable = (size_t)value;
	return B_OK;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	bool settable;
	size_t* variable = find_debug_variable(variableName, settable);
	if (variable == NULL)
		return B_ENTRY_NOT_FOUND;

	*value = *variable;
	return B_OK;
}


struct gdb_register {
	int32 type;
	uint64 value;
};


/*!	Writes the contents of the CPU registers at some fixed outer stack frame or
	iframe into the given buffer in the format expected by gdb.

	This function is called in response to gdb's 'g' command.

	\param buffer The buffer to write the registers to.
	\param bufferSize The size of \a buffer in bytes.
	\return When successful, the number of bytes written to \a buffer, or a
		negative error code on error.
*/
ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	iframe* frame = get_current_iframe(debug_get_debugged_thread());
	if (frame == NULL)
		return B_NOT_SUPPORTED;

#ifdef __x86_64__
	// For x86_64 the register order is:
	//
	//    rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp,
	//    r8, r9, r10, r11, r12, r13, r14, r15,
	//    rip, rflags, cs, ss, ds, es, fs, gs
	//
	// Annoyingly, GDB wants all the registers as 64-bit values, but then
	// RFLAGS and the segment registers as 32-bit values, hence the need for
	// the type information.
	static const int32 kRegisterCount = 24;
	gdb_register registers[kRegisterCount] = {
		{ B_UINT64_TYPE, frame->ax },  { B_UINT64_TYPE, frame->bx },
		{ B_UINT64_TYPE, frame->cx },  { B_UINT64_TYPE, frame->dx },
		{ B_UINT64_TYPE, frame->si },  { B_UINT64_TYPE, frame->di },
		{ B_UINT64_TYPE, frame->bp },  { B_UINT64_TYPE, frame->sp },
		{ B_UINT64_TYPE, frame->r8 },  { B_UINT64_TYPE, frame->r9 },
		{ B_UINT64_TYPE, frame->r10 }, { B_UINT64_TYPE, frame->r11 },
		{ B_UINT64_TYPE, frame->r12 }, { B_UINT64_TYPE, frame->r13 },
		{ B_UINT64_TYPE, frame->r14 }, { B_UINT64_TYPE, frame->r15 },
		{ B_UINT64_TYPE, frame->ip },  { B_UINT32_TYPE, frame->flags },
		{ B_UINT32_TYPE, frame->cs },  { B_UINT32_TYPE, frame->ss },
		{ B_UINT32_TYPE, 0 }, { B_UINT32_TYPE, 0 },
		{ B_UINT32_TYPE, 0 }, { B_UINT32_TYPE, 0 },
	};
#else
	// For x86 the register order is:
	//
	//    eax, ecx, edx, ebx,
	//    esp, ebp, esi, edi,
	//    eip, eflags,
	//    cs, ss, ds, es, fs, gs
	//
	// Note that even though the segment descriptors are actually 16 bits wide,
	// gdb requires them as 32 bit integers.
	static const int32 kRegisterCount = 16;
	gdb_register registers[kRegisterCount] = {
		{ B_UINT32_TYPE, frame->ax }, { B_UINT32_TYPE, frame->cx },
		{ B_UINT32_TYPE, frame->dx }, { B_UINT32_TYPE, frame->bx },
		{ B_UINT32_TYPE, frame->sp }, { B_UINT32_TYPE, frame->bp },
		{ B_UINT32_TYPE, frame->si }, { B_UINT32_TYPE, frame->di },
		{ B_UINT32_TYPE, frame->ip }, { B_UINT32_TYPE, frame->flags },
		{ B_UINT32_TYPE, frame->cs }, { B_UINT32_TYPE, frame->ds },
			// assume ss == ds
		{ B_UINT32_TYPE, frame->ds }, { B_UINT32_TYPE, frame->es },
		{ B_UINT32_TYPE, frame->fs }, { B_UINT32_TYPE, frame->gs },
	};
#endif

	const char* const bufferStart = buffer;

	for (int32 i = 0; i < kRegisterCount; i++) {
		// For some reason gdb wants the register dump in *big endian* format.
		int result = 0;
		switch (registers[i].type) {
			case B_UINT64_TYPE:
				result = snprintf(buffer, bufferSize, "%016" B_PRIx64,
					(uint64)B_HOST_TO_BENDIAN_INT64(registers[i].value));
				break;
			case B_UINT32_TYPE:
				result = snprintf(buffer, bufferSize, "%08" B_PRIx32,
					(uint32)B_HOST_TO_BENDIAN_INT32((uint32)registers[i].value));
				break;
		}
		if (result >= (int)bufferSize)
			return B_BUFFER_OVERFLOW;

		buffer += result;
		bufferSize -= result;
	}

	return buffer - bufferStart;
}


status_t
arch_debug_init(kernel_args* args)
{
	// at this stage, the debugger command system is alive

	add_debugger_command("where", &stack_trace, "Same as \"sc\"");
	add_debugger_command("bt", &stack_trace, "Same as \"sc\" (as in gdb)");
	add_debugger_command("sc", &stack_trace,
		"Stack crawl for current thread (or any other)");
	add_debugger_command("iframe", &dump_iframes,
		"Dump iframes for the specified thread");
#ifndef __x86_64__
	add_debugger_command("call", &show_call, "Show call with arguments");
#endif
	add_debugger_command_etc("in_context", &cmd_in_context,
		"Executes a command in the context of a given thread",
		"<thread ID> <command> ...\n"
		"Executes a command in the context of a given thread.\n",
		B_KDEBUG_DONT_PARSE_ARGUMENTS);

	return B_NO_ERROR;
}

