/*
 * Copyright 2003-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 */


#include <arch/debug.h>

#include <arch_cpu.h>
#include <debug.h>
#include <debug_heap.h>
#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <thread.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>

#define NUM_PREVIOUS_LOCATIONS 32

extern struct iframe_stack gBootFrameStack;


static bool
already_visited(addr_t *visited, int32 *_last, int32 *_num, addr_t fp)
{
	int32 last = *_last;
	int32 num = *_num;
	int32 i;

	for (i = 0; i < num; i++) {
		if (visited[(NUM_PREVIOUS_LOCATIONS + last - i)
				% NUM_PREVIOUS_LOCATIONS] == fp) {
			return true;
		}
	}

	*_last = last = (last + 1) % NUM_PREVIOUS_LOCATIONS;
	visited[last] = fp;

	if (num < NUM_PREVIOUS_LOCATIONS)
		*_num = num + 1;

	return false;
}


static status_t
get_next_frame(addr_t fp, addr_t *next, addr_t *ip)
{
	if (fp != 0) {
	        addr_t _fp = *(((addr_t*)fp) -3);
	        addr_t _sp = *(((addr_t*)fp) -2);
	        addr_t _lr = *(((addr_t*)fp) -1);
	        addr_t _pc = *(((addr_t*)fp) -0);

		*ip = (_fp != 0) ? _lr : _pc;
		*next = _fp;

		return B_OK;
	}

	return B_BAD_VALUE;
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
			kprintf("<%s> %.*s<???>%s", image, namespaceLength, name, lastName);

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
				kprintf(" \33[31m\"<???>\"\33[0m");
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



static void
print_stack_frame(Thread *thread, addr_t ip, addr_t fp, addr_t next,
	int32 callIndex, bool demangle)
{
	const char* symbol;
	const char* image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status;
	addr_t diff;

	diff = next - fp;

	// MSB set = kernel space/user space switch
	if (diff & ~((addr_t)-1 >> 1))
		diff = 0;

	status = lookup_symbol(thread, ip, &baseAddress, &symbol, &image,
		&exactMatch);

	kprintf("%2" B_PRId32 " %0*lx (+%4ld) %0*lx   ", callIndex,
		B_PRINTF_POINTER_WIDTH, fp, diff, B_PRINTF_POINTER_WIDTH, ip);

	if (status == B_OK) {
		if (exactMatch && demangle) {
			status = print_demangled_call(image, symbol,
				next, false, false);
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

static int
stack_trace(int argc, char **argv)
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
	addr_t fp = arm_get_fp();
	int32 num = 0, last = 0;
	struct iframe_stack *frameStack;

	// We don't have a thread pointer early in the boot process
	if (thread != NULL)
		frameStack = &thread->arch_info.iframes;
	else
		frameStack = &gBootFrameStack;

	int32 i;
	for (i = 0; i < frameStack->index; i++) {
		kprintf("iframe %p (end = %p)\n",
			frameStack->frames[i], frameStack->frames[i] + 1);
	}

	if (thread != NULL) {
		kprintf("stack trace for thread 0x%lx \"%s\"\n", thread->id,
			thread->name);

		kprintf("    kernel stack: %p to %p\n",
			(void *)thread->kernel_stack_base,
			(void *)(thread->kernel_stack_top));
		if (thread->user_stack_base != 0) {
			kprintf("      user stack: %p to %p\n",
				(void *)thread->user_stack_base,
				(void *)(thread->user_stack_base + thread->user_stack_size));
		}
	}

	kprintf("frame            caller     <image>:function + offset\n");

	for (int32 callIndex = 0;; callIndex++) {
		// see if the frame pointer matches the iframe
		struct iframe *frame = NULL;
		for (i = 0; i < frameStack->index; i++) {
			if (fp == (addr_t)frameStack->frames[i]) {
				// it's an iframe
				frame = frameStack->frames[i];
				break;
			}
		}

		if (frame) {
			kprintf("iframe at %p\n", frame);
			kprintf("   r0 0x%08lx    r1 0x%08lx    r2 0x%08lx    r3 0x%08lx\n",
				frame->r0, frame->r1, frame->r2, frame->r3);
			kprintf("   r4 0x%08lx    r5 0x%08lx    r6 0x%08lx    r7 0x%08lx\n",
				frame->r4, frame->r5, frame->r6, frame->r7);
			kprintf("   r8 0x%08lx    r9 0x%08lx    r10 0x%08lx    r11 0x%08lx\n",
				frame->r8, frame->r9, frame->r10, frame->r11);
			kprintf("   r12 0x%08lx   sp 0x%08lx    lr 0x%08lx    pc 0x%08lx\n",
				frame->r12, frame->svc_sp, frame->svc_lr, frame->pc);

 			fp = frame->svc_sp;
			print_stack_frame(thread, frame->pc, frame->svc_sp, frame->svc_lr, callIndex, demangle);
		} else {
			addr_t ip, next;

			if (get_next_frame(fp, &next, &ip) != B_OK) {
				kprintf("%08lx -- read fault\n", fp);
				break;
			}

			if (ip == 0 || fp == 0)
				break;

			print_stack_frame(thread, ip, fp, next, callIndex, demangle);
			fp = next;
		}

		if (already_visited(previousLocations, &last, &num, fp)) {
			kprintf("circular stack frame: %p!\n", (void *)fp);
			break;
		}
		if (fp == 0)
			break;
	}

	return 0;
}


// #pragma mark -


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{
}


bool
arch_debug_contains_call(Thread *thread, const char *symbol,
	addr_t start, addr_t end)
{
	return false;
}


void
arch_debug_stack_trace(void)
{
	stack_trace(0, NULL);
}


void *
arch_debug_get_caller(void)
{
	/* Return the thread id as the kernel (for example the lock code) actually
	   gets a somewhat valid indication of the caller back. */
	return (void*) thread_get_current_thread_id();
}


int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
	// TODO: Implement!
	return 0;
}


void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
	// TODO: Implement!
	return NULL;
}


bool
arch_is_debug_variable_defined(const char* variableName)
{
	// TODO: Implement!
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	// TODO: Implement!
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	// TODO: Implement!
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_debug_init(kernel_args *args)
{
	add_debugger_command("where", &stack_trace, "Same as \"sc\"");
	add_debugger_command("bt", &stack_trace, "Same as \"sc\" (as in gdb)");
	add_debugger_command("sc", &stack_trace, "Stack crawl for current thread");

	return B_NO_ERROR;
}


/* arch_debug_call_with_fault_handler is in arch_asm.S */

void
arch_debug_unset_current_thread(void)
{
	// TODO: Implement!
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}
