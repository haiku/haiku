/*
 * Copyright 2021-2024, Ilya Chugin, danger_mail@list.ru.
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

#include "RISCV64VMTranslationMap.h"


extern "C" void SVecRet();
extern "C" void SVecURet();


static bool
iframe_is_user(iframe* frame)
{
	return SstatusReg{.val = frame->status}.spp == modeU;
}


static iframe*
get_user_iframe()
{
	Thread* thread = thread_get_current_thread();
	return thread->arch_info.userFrame;
}


static phys_addr_t
get_thread_page_directory(Thread* thread)
{
	auto map = (RISCV64VMTranslationMap*)thread->team->address_space->TranslationMap();
	return map->Satp();
}


struct stack_frame {
	addr_t previous;
	addr_t return_address;
};

#define NUM_PREVIOUS_LOCATIONS 32


static bool is_kernel_stack_address(Thread* thread, addr_t address);


static bool
already_visited(addr_t* visited, int32* _last, int32* _num, addr_t fp)
{
	int32 last = *_last;
	int32 num = *_num;
	int32 i;

	for (i = 0; i < num; i++) {
		if (visited[(NUM_PREVIOUS_LOCATIONS + last - i) % NUM_PREVIOUS_LOCATIONS] == fp)
			return true;
	}

	*_last = last = (last + 1) % NUM_PREVIOUS_LOCATIONS;
	visited[last] = fp;

	if (num < NUM_PREVIOUS_LOCATIONS)
		*_num = num + 1;

	return false;
}


/*!	Safe to be called only from outside the debugger.
*/
static status_t
get_next_frame_no_debugger(addr_t fp, addr_t* _next, addr_t* _pc,
	bool onKernelStack, Thread* thread)
{
	// TODO: Do this more efficiently in assembly.
	stack_frame frame;
	if (onKernelStack
			&& is_kernel_stack_address(thread, fp - 1)) {
		memcpy(&frame, (stack_frame*)fp - 1, sizeof(frame));
	} else if (!IS_USER_ADDRESS(fp)
			|| user_memcpy(&frame, (stack_frame*)fp - 1, sizeof(frame)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	*_pc = frame.return_address;
	*_next = frame.previous;

	return B_OK;
}


/*!	Safe to be called only from inside the debugger.
*/
static status_t
get_next_frame_debugger(addr_t fp, addr_t* _next, addr_t* _pc)
{
	stack_frame frame;
	if (debug_memcpy(B_CURRENT_TEAM, &frame, (stack_frame*)fp - 1, sizeof(frame)) != B_OK)
		return B_BAD_ADDRESS;

	*_pc = frame.return_address;
	*_next = frame.previous;

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


static status_t
print_demangled_call(const char* image, const char* symbol, addr_t args,
	bool noObjectMethod, bool addDebugVariables)
{
	// Since riscv64 uses registers rather than the stack for the first 8
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


static void
print_stack_frame(Thread* thread, addr_t pc, addr_t calleeFp, addr_t fp,
	int32 callIndex, bool demangle)
{
	const char* symbol;
	const char* image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status;
	addr_t diff;

	diff = fp - calleeFp;

	// kernel space/user space switch
	if (calleeFp > fp)
		diff = 0;

	status = lookup_symbol(thread, pc, &baseAddress, &symbol, &image,
		&exactMatch);

	kprintf("%2" B_PRId32 " %0*lx (+%4ld) %0*lx   ", callIndex,
		B_PRINTF_POINTER_WIDTH, fp, diff, B_PRINTF_POINTER_WIDTH, pc);

	if (status == B_OK) {
		if (exactMatch && demangle) {
			status = print_demangled_call(image, symbol,
				fp, false, false);
		}

		if (!exactMatch || !demangle || status != B_OK) {
			if (symbol != NULL) {
				kprintf("<%s> %s%s", image, symbol,
					exactMatch ? "" : " (nearest)");
			} else
				kprintf("<%s@%p> <unknown>", image, (void*)baseAddress);
		}

		kprintf(" + %#04lx\n", pc - baseAddress);
	} else {
		VMArea *area = NULL;
		if (thread != NULL && thread->team != NULL
			&& thread->team->address_space != NULL) {
			area = thread->team->address_space->LookupArea(pc);
		}
		if (area != NULL) {
			kprintf("%" B_PRId32 ":%s@%p + %#lx\n", area->id, area->name,
				(void*)area->Base(), pc - area->Base());
		} else
			kprintf("\n");
	}
}


static void
write_mode(int mode)
{
	switch (mode) {
		case modeU: kprintf("u"); break;
		case modeS: kprintf("s"); break;
		case modeM: kprintf("m"); break;
		default: kprintf("%d", mode);
	}
}


static void
write_mode_set(uint32_t val)
{
	bool first = true;
	kprintf("{");
	for (int i = 0; i < 32; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else kprintf(", ");
			write_mode(i);
		}
	}
	kprintf("}");
}


static void
write_ext(uint64_t val)
{
	switch (val) {
		case 0: kprintf("off"); break;
		case 1: kprintf("initial"); break;
		case 2: kprintf("clean"); break;
		case 3: kprintf("dirty"); break;
		default: kprintf("%" B_PRId64, val);
	}
}


static void
write_sstatus(uint64_t val)
{
	SstatusReg status{.val = val};
	kprintf("(");
	kprintf("ie: "); write_mode_set(status.ie);
	kprintf(", pie: "); write_mode_set(status.pie);
	kprintf(", spp: "); write_mode(status.spp);
	kprintf(", fs: "); write_ext(status.fs);
	kprintf(", xs: "); write_ext(status.xs);
	kprintf(", sum: %d", (int)status.sum);
	kprintf(", mxr: %d", (int)status.mxr);
	kprintf(", uxl: %d", (int)status.uxl);
	kprintf(", sd: %d", (int)status.sd);
	kprintf(")");
}


static void
write_interrupt(uint64_t val)
{
	switch (val) {
		case 0 + modeU: kprintf("uSoft"); break;
		case 0 + modeS: kprintf("sSoft"); break;
		case 0 + modeM: kprintf("mSoft"); break;
		case 4 + modeU: kprintf("uTimer"); break;
		case 4 + modeS: kprintf("sTimer"); break;
		case 4 + modeM: kprintf("mTimer"); break;
		case 8 + modeU: kprintf("uExtern"); break;
		case 8 + modeS: kprintf("sExtern"); break;
		case 8 + modeM: kprintf("mExtern"); break;
		default: kprintf("%" B_PRId64, val);
	}
}


static void
write_interrupt_set(uint64_t val)
{
	bool first = true;
	kprintf("{");
	for (int i = 0; i < 64; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else kprintf(", ");
			write_interrupt(i);
		}
	}
	kprintf("}");
}


static void
write_cause(uint64_t cause)
{
	if ((cause & causeInterrupt) == 0) {
		kprintf("exception ");
		switch (cause) {
			case causeExecMisalign: kprintf("execMisalign"); break;
			case causeExecAccessFault: kprintf("execAccessFault"); break;
			case causeIllegalInst: kprintf("illegalInst"); break;
			case causeBreakpoint: kprintf("breakpoint"); break;
			case causeLoadMisalign: kprintf("loadMisalign"); break;
			case causeLoadAccessFault: kprintf("loadAccessFault"); break;
			case causeStoreMisalign: kprintf("storeMisalign"); break;
			case causeStoreAccessFault: kprintf("storeAccessFault"); break;
			case causeUEcall: kprintf("uEcall"); break;
			case causeSEcall: kprintf("sEcall"); break;
			case causeMEcall: kprintf("mEcall"); break;
			case causeExecPageFault: kprintf("execPageFault"); break;
			case causeLoadPageFault: kprintf("loadPageFault"); break;
			case causeStorePageFault: kprintf("storePageFault"); break;
			default: kprintf("%" B_PRId64, cause);
			}
	} else {
		kprintf("interrupt "); write_interrupt(cause & ~causeInterrupt);
	}
}


const static char* registerNames[] = {
	" ra", " t6", " sp", " gp",
	" tp", " t0", " t1", " t2",
	" t5", " s1", " a0", " a1",
	" a2", " a3", " a4", " a5",
	" a6", " a7", " s2", " s3",
	" s4", " s5", " s6", " s7",
	" s8", " s9", "s10", "s11",
	" t3", " t4", " fp", "epc"
};


static void
print_iframe(iframe* frame)
{
	bool isUser = iframe_is_user(frame);

	kprintf("%s iframe at %p (end = %p)\n", isUser ? "user" : "kernel", frame,
		frame + 1);

	uint64* regs = &frame->ra;
	for (int i = 0; i < 32; i += 4) {
		kprintf(
			"  %s: 0x%016" B_PRIx64
			"  %s: 0x%016" B_PRIx64
			"  %s: 0x%016" B_PRIx64
			"  %s: 0x%016" B_PRIx64 "\n",
			registerNames[i + 0], regs[i + 0],
			registerNames[i + 1], regs[i + 1],
			registerNames[i + 2], regs[i + 2],
			registerNames[i + 3], regs[i + 3]
		);
	}

	kprintf(" status: ");
	write_sstatus(frame->status);
	kprintf(", cause: ");
	write_cause(frame->cause);
	kprintf(", tval: %#" B_PRIx64 "\n", frame->tval);
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
			phys_addr_t newPageDirectory = get_thread_page_directory(thread);

			if (newPageDirectory != 0) {
				*_oldPageDirectory = Satp();
				SetSatp(newPageDirectory);
				FlushTlbAllAsid(0);
			}

			if (thread->state == B_THREAD_RUNNING) {
				// The thread is currently running on another CPU.
				if (thread->cpu == NULL)
					return false;
				arch_debug_registers* registers = debug_get_debug_registers(
					thread->cpu->cpu_num);
				if (registers == NULL)
					return false;
				*_bp = registers->fp;
			} else {
				// Read frame pointer from the thread's stack.
				*_bp = thread->arch_info.context.s[0];
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
#if 0
	size_t size;
	addr_t bottom = (addr_t)x86_get_double_fault_stack(cpu, &size);
	return address >= bottom && address < bottom + size;
#endif
	return false;
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

	addr_t pc = ((stack_frame*)frame - 1)->return_address;
	return pc == (addr_t)&SVecRet || pc == (addr_t)&SVecURet;
}


static iframe*
find_previous_iframe(Thread* thread, addr_t frame)
{
	// iterate backwards through the stack frames, until we hit an iframe
	while (is_kernel_stack_address(thread, frame)) {
		if (is_iframe(thread, frame))
			return (iframe*)frame;

		frame = ((stack_frame*)frame - 1)->previous;
	}

	return NULL;
}


static iframe*
get_previous_iframe(Thread* thread, iframe* frame)
{
	if (frame == NULL)
		return NULL;

	return find_previous_iframe(thread, frame->fp);
}


static struct iframe*
get_current_iframe(void)
{
	return find_previous_iframe(thread_get_current_thread(), Fp());
}


static iframe*
get_current_iframe(Thread* thread)
{
	if (thread == thread_get_current_thread())
		return get_current_iframe();

	// NOTE: This doesn't work, if the thread is running (on another CPU).
	return find_previous_iframe(thread, thread->arch_info.context.s[0]);
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

	CHECK_DEBUG_VARIABLE("status", frame->status, true);
	CHECK_DEBUG_VARIABLE("cause", frame->cause, true);
	CHECK_DEBUG_VARIABLE("tval", frame->tval, true);

	CHECK_DEBUG_VARIABLE("ra", frame->ra, true);
	CHECK_DEBUG_VARIABLE("t6", frame->t6, true);
	CHECK_DEBUG_VARIABLE("sp", frame->sp, true);
	CHECK_DEBUG_VARIABLE("gp", frame->gp, true);
	CHECK_DEBUG_VARIABLE("tp", frame->tp, true);
	CHECK_DEBUG_VARIABLE("t0", frame->t0, true);
	CHECK_DEBUG_VARIABLE("t1", frame->t1, true);
	CHECK_DEBUG_VARIABLE("t2", frame->t2, true);
	CHECK_DEBUG_VARIABLE("t5", frame->t5, true);
	CHECK_DEBUG_VARIABLE("s1", frame->s1, true);
	CHECK_DEBUG_VARIABLE("a0", frame->a0, true);
	CHECK_DEBUG_VARIABLE("a1", frame->a1, true);
	CHECK_DEBUG_VARIABLE("a2", frame->a2, true);
	CHECK_DEBUG_VARIABLE("a3", frame->a3, true);
	CHECK_DEBUG_VARIABLE("a4", frame->a4, true);
	CHECK_DEBUG_VARIABLE("a5", frame->a5, true);
	CHECK_DEBUG_VARIABLE("a6", frame->a6, true);
	CHECK_DEBUG_VARIABLE("a7", frame->a7, true);
	CHECK_DEBUG_VARIABLE("s2", frame->s2, true);
	CHECK_DEBUG_VARIABLE("s3", frame->s3, true);
	CHECK_DEBUG_VARIABLE("s4", frame->s4, true);
	CHECK_DEBUG_VARIABLE("s5", frame->s5, true);
	CHECK_DEBUG_VARIABLE("s6", frame->s6, true);
	CHECK_DEBUG_VARIABLE("s7", frame->s7, true);
	CHECK_DEBUG_VARIABLE("s8", frame->s8, true);
	CHECK_DEBUG_VARIABLE("s9", frame->s9, true);
	CHECK_DEBUG_VARIABLE("s10", frame->s10, true);
	CHECK_DEBUG_VARIABLE("s11", frame->s11, true);
	CHECK_DEBUG_VARIABLE("t3", frame->t3, true);
	CHECK_DEBUG_VARIABLE("t4", frame->t4, true);
	CHECK_DEBUG_VARIABLE("fp", frame->fp, true);
	CHECK_DEBUG_VARIABLE("epc", frame->epc, true);

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
	addr_t fp = Fp();
	int32 num = 0, last = 0;

	if (!setup_for_thread(argc == threadIndex + 1 ? argv[threadIndex] : NULL,
			&thread, &fp, &oldPageDirectory))
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
			&& is_kernel_stack_address(thread, fp);

		if (onKernelStack && is_iframe(thread, fp)) {
			iframe* frame = (iframe*)fp;

			print_iframe(frame);
			print_stack_frame(thread, frame->epc, fp, frame->fp, callIndex,
				demangle);

 			fp = frame->fp;
		} else {
			addr_t pc, nextFp;

			if (get_next_frame_debugger(fp, &nextFp, &pc) != B_OK) {
				kprintf("%0*lx -- read fault\n", B_PRINTF_POINTER_WIDTH, fp);
				break;
			}

			if (pc == 0 || fp == 0)
				break;

			print_stack_frame(thread, pc, fp, nextFp, callIndex, demangle);
			fp = nextFp;
		}

		if (already_visited(previousLocations, &last, &num, fp)) {
			kprintf("circular stack frame: %p!\n", (void*)fp);
			break;
		}
		if (fp == 0)
			break;
	}

	if (oldPageDirectory != 0) {
		// switch back to the previous page directory to no cause any troubles
		SetSatp(oldPageDirectory);
		FlushTlbAllAsid(0);
	}

	return 0;
}


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

	iframe* frame = find_previous_iframe(thread, Fp());
	while (frame != NULL) {
		print_iframe(frame);
		frame = get_previous_iframe(thread, frame);
	}

	return 0;
}


static bool
is_calling(Thread* thread, addr_t pc, const char* pattern, addr_t start,
	addr_t end)
{
	if (pattern == NULL)
		return pc >= start && pc < end;

	if (!IS_KERNEL_ADDRESS(pc))
		return false;

	const char* symbol;
	if (lookup_symbol(thread, pc, NULL, &symbol, NULL, NULL) != B_OK)
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
		phys_addr_t newPageDirectory = get_thread_page_directory(thread);

		if (newPageDirectory != 0) {
			oldPageDirectory = Satp();
			SetSatp(newPageDirectory);
			FlushTlbAllAsid(0);
		}
	}

	// execute the command
	{
		DebuggedThreadSetter threadSetter(thread);
		evaluate_debug_command(commandLine);
	}

	// reset the page directory
	if (oldPageDirectory) {
		SetSatp(oldPageDirectory);
		FlushTlbAllAsid(0);
	}

	return 0;
}


//	#pragma mark -


static void __attribute__((naked))
handle_fault()
{
	asm volatile("ld a0, 0(sp)");
	asm volatile("li a1, 1");
	asm volatile("call longjmp");
}


void
arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
	void (*function)(void*), void* parameter)
{
	cpu->fault_handler = (addr_t)&handle_fault;
	cpu->fault_handler_stack_pointer = (addr_t)&jumpBuffer;
	function(parameter);
}


void
arch_debug_save_registers(arch_debug_registers* registers)
{
	// get the caller's frame pointer
	stack_frame* frame = (stack_frame*)Fp() - 1;
	registers->fp = (addr_t)frame->previous;
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

	addr_t fp;
	if (thread == thread_get_current_thread())
		fp = Fp();
	else {
		if (thread->state == B_THREAD_RUNNING) {
			// The thread is currently running on another CPU.
			if (thread->cpu == NULL)
				return false;
			arch_debug_registers* registers = debug_get_debug_registers(
				thread->cpu->cpu_num);
			if (registers == NULL)
				return false;
			fp = registers->fp;
		} else {
			// thread not running
			fp = thread->arch_info.context.s[0];
		}
	}

	for (;;) {
		if (!is_kernel_stack_address(thread, fp))
			break;

		if (is_iframe(thread, fp)) {
			iframe* frame = (iframe*)fp;

			if (is_calling(thread, frame->epc, symbol, start, end))
				return true;

 			fp = frame->fp;
		} else {
			addr_t pc, nextFp;

			if (get_next_frame_no_debugger(fp, &nextFp, &pc, true,
					thread) != B_OK
				|| pc == 0 || fp == 0)
				break;

			if (is_calling(thread, pc, symbol, start, end))
				return true;

			fp = nextFp;
		}

		if (fp == 0)
			break;
	}

	return false;
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
	addr_t fp = Fp();
	bool onKernelStack = true;

	if ((flags & (STACK_TRACE_KERNEL | STACK_TRACE_USER)) == STACK_TRACE_USER) {
		iframe* frame = get_user_iframe();
		if (frame == NULL)
			return 0;

		fp = (addr_t)frame;
	}

	while (fp != 0 && count < maxCount) {
		onKernelStack = onKernelStack
			&& is_kernel_stack_address(thread, fp);
		if (!onKernelStack && (flags & STACK_TRACE_USER) == 0)
			break;

		addr_t pc;
		addr_t nextFp;

		if (onKernelStack && is_iframe(thread, fp)) {
			iframe* frame = (iframe*)fp;
			pc = frame->epc;
			nextFp = frame->fp;

			if (skipIframes > 0) {
				if (--skipIframes == 0)
					skipFrames = 0;
			}
		} else {
			if (get_next_frame_no_debugger(fp, &nextFp, &pc,
					onKernelStack, thread) != B_OK) {
				break;
			}
		}

		if (pc == 0)
			break;

		if (skipFrames > 0)
			skipFrames--;
		else
			returnAddresses[count++] = pc;

		fp = nextFp;
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
		*_isSyscall = frame->cause == causeUEcall;

	return (void*)(addr_t)frame->epc;
}


/*!	Sets the current thread to \c NULL.
	Invoked in the kernel debugger only.
*/
void
arch_debug_unset_current_thread(void)
{
	arch_thread_set_current_thread(NULL);
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

	static const int32 kRegisterCount = 33;
	uint64 registers[kRegisterCount] = {
	  0,
	  frame->ra,
	  frame->sp,
	  frame->gp,
	  frame->tp,
	  frame->t0,
	  frame->t1,
	  frame->t2,
	  frame->fp,
	  frame->s1,
	  frame->a0,
	  frame->a1,
	  frame->a2,
	  frame->a3,
	  frame->a4,
	  frame->a5,
	  frame->a6,
	  frame->a7,
	  frame->s2,
	  frame->s3,
	  frame->s4,
	  frame->s5,
	  frame->s6,
	  frame->s7,
	  frame->s8,
	  frame->s9,
	  frame->s10,
	  frame->s11,
	  frame->t3,
	  frame->t4,
	  frame->t5,
	  frame->t6,
	  frame->epc
	};

	const char* const bufferStart = buffer;

	for (int32 i = 0; i < kRegisterCount; i++) {
		// For some reason gdb wants the register dump in *big endian* format.
		int result = snprintf(buffer, bufferSize, "%016" B_PRIx64,
				(uint64)B_HOST_TO_BENDIAN_INT64(registers[i]));

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
	add_debugger_command_etc("in_context", &cmd_in_context,
		"Executes a command in the context of a given thread",
		"<thread ID> <command> ...\n"
		"Executes a command in the context of a given thread.\n",
		B_KDEBUG_DONT_PARSE_ARGUMENTS);

	return B_NO_ERROR;
}
