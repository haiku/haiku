/*
 * Copyright 2007-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

// This file is used to get C structure offsets into assembly code.
// The build system assembles the file and processes the output to create
// a header file with macro definitions, that can be included from assembly
// code.


#include <computed_asm_macros.h>

#include <arch_cpu.h>
#include <ksyscalls.h>
#include <thread_types.h>


#define DEFINE_MACRO(macro, value) DEFINE_COMPUTED_ASM_MACRO(macro, value)

#define DEFINE_OFFSET_MACRO(prefix, structure, member) \
    DEFINE_MACRO(prefix##_##member, offsetof(struct structure, member));

#define DEFINE_SIZEOF_MACRO(prefix, structure) \
    DEFINE_MACRO(prefix##_sizeof, sizeof(struct structure));


void
dummy()
{
	// struct Thread
	DEFINE_OFFSET_MACRO(THREAD, Thread, kernel_time);
	DEFINE_OFFSET_MACRO(THREAD, Thread, user_time);
	DEFINE_OFFSET_MACRO(THREAD, Thread, last_time);
	DEFINE_OFFSET_MACRO(THREAD, Thread, in_kernel);
	DEFINE_OFFSET_MACRO(THREAD, Thread, flags);
	DEFINE_OFFSET_MACRO(THREAD, Thread, kernel_stack_top);
	DEFINE_OFFSET_MACRO(THREAD, Thread, fault_handler);

	// struct iframe
	DEFINE_OFFSET_MACRO(IFRAME, iframe, fp);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, fpc);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, fpu);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, d);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, a);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, cpu);
#if 0
	// struct iframe
	DEFINE_OFFSET_MACRO(IFRAME, iframe, cs);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, eax);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, edx);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, orig_eax);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, vector);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, eip);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, flags);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, user_esp);
#endif

	DEFINE_SIZEOF_MACRO(FPU_STATE, mc680x0_fpu_state);
	DEFINE_SIZEOF_MACRO(FPU_DATA_REG, mc680x0_fp_data_reg);

	// struct syscall_info
	DEFINE_SIZEOF_MACRO(SYSCALL_INFO, syscall_info);
	DEFINE_OFFSET_MACRO(SYSCALL_INFO, syscall_info, function);
	DEFINE_OFFSET_MACRO(SYSCALL_INFO, syscall_info, parameter_size);
}
