/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "x86_signals.h"

#include <string.h>

#include <KernelExport.h>

#include <commpage.h>
#include <cpu.h>
#include <elf.h>
#include <smp.h>
#include <arch_altcodepatch.h>

#include "syscall_numbers.h"


extern "C" void __attribute__((noreturn))
x86_64_user_signal_handler(signal_frame_data* data)
{
	if (data->siginfo_handler) {
		auto handler = (void (*)(int, siginfo_t*, void*, void*))data->handler;
		handler(data->info.si_signo, &data->info, &data->context, data->user_data);
	} else {
		auto handler = (void (*)(int, void*, vregs*))data->handler;
		handler(data->info.si_signo, data->user_data, &data->context.uc_mcontext);
	}

	// _kern_restore_signal_frame(data)
	uint64 result = 0;
	asm volatile(
		"movq $" STRINGIFY(SYSCALL_RESTORE_SIGNAL_FRAME) ", %%rax;"
		"syscall;"
		: "=a" (result)
		: "D" (data)
	);

	// Now we actually restore the frame and return.
	iframe frame = {};
	frame.ax = data->context.uc_mcontext.rax;
	frame.bx = data->context.uc_mcontext.rbx;
	frame.cx = data->context.uc_mcontext.rcx;
	frame.dx = data->context.uc_mcontext.rdx;
	frame.di = data->context.uc_mcontext.rdi;
	frame.si = data->context.uc_mcontext.rsi;
	frame.bp = data->context.uc_mcontext.rbp;
	frame.r8 = data->context.uc_mcontext.r8;
	frame.r9 = data->context.uc_mcontext.r9;
	frame.r10 = data->context.uc_mcontext.r10;
	frame.r11 = data->context.uc_mcontext.r11;
	frame.r12 = data->context.uc_mcontext.r12;
	frame.r13 = data->context.uc_mcontext.r13;
	frame.r14 = data->context.uc_mcontext.r14;
	frame.r15 = data->context.uc_mcontext.r15;
	frame.user_sp = data->context.uc_mcontext.rsp;
	frame.ip = data->context.uc_mcontext.rip;
	frame.flags = data->context.uc_mcontext.rflags;

	// Note: the error_code and vector fields are not restored. These are provided to the signal
	// handler for information purposes only, and are not used after the signal handling is
	// complete.

	frame.cs = data->context.uc_mcontext.fpu.fp_fxsave.cs;
	frame.ss = data->context.uc_mcontext.fpu.fp_fxsave.ss;

	frame.fpu = &data->context.uc_mcontext.fpu;

	asm volatile(
		"leaq %0, %%rsp;"

		// If XSAVE is in use, just restore everything in the FPU state.
		"mov 		$0xFFFFFFFF, %%eax;"
		"mov 		$0xFFFFFFFF, %%edx;"
		CODEPATCH_START
		"fxrstorq	(%%rdi);"
		CODEPATCH_END(ALTCODEPATCH_TAG_XRSTOR) ";"

		// Restore the saved registers (similar to RESTORE_IFRAME).
		"add $16, %%rsp;"
		"pop %%r15;"
		"pop %%r14;"
		"pop %%r13;"
		"pop %%r12;"
		"pop %%r11;"
		"pop %%r10;"
		"pop %%r9;"
		"pop %%r8;"
		"pop %%rbp;"
		"pop %%rsi;"
		"pop %%rdi;"
		"pop %%rdx;"
		"pop %%rcx;"
		"pop %%rbx;"
		"pop %%rax;"
		"addq $24, %%rsp;"

		"iretq"
		:
		: "m" (frame), "D" (frame.fpu)
	);

	__builtin_unreachable();
}


static void
register_commpage_function(const char* functionName, int32 commpageIndex,
	const char* commpageSymbolName, addr_t expectedAddress)
{
	// get address and size of function
	elf_symbol_info symbolInfo;
	if (elf_lookup_kernel_symbol(functionName, &symbolInfo)	!= B_OK) {
		panic("register_commpage_function(): Failed to find "
			"signal frame function \"%s\"!", functionName);
	}

	ASSERT(expectedAddress == symbolInfo.address);

	// fill in the commpage table entry
	addr_t position = fill_commpage_entry(commpageIndex,
		(void*)symbolInfo.address, symbolInfo.size);

	// add symbol to the commpage image
	image_id image = get_commpage_image();
	elf_add_memory_image_symbol(image, commpageSymbolName, position,
		symbolInfo.size, B_SYMBOL_TYPE_TEXT);
}


void
x86_initialize_commpage_signal_handler()
{
	register_commpage_function("x86_64_user_signal_handler",
		COMMPAGE_ENTRY_X86_SIGNAL_HANDLER, "commpage_signal_handler",
		(addr_t)&x86_64_user_signal_handler);
}
