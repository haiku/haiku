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
#include <cpu.h>
#include <ksignal.h>
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
	DEFINE_OFFSET_MACRO(THREAD, Thread, arch_info);

	// struct arch_thread
	DEFINE_OFFSET_MACRO(ARCH_THREAD, arch_thread, context);
	DEFINE_OFFSET_MACRO(ARCH_THREAD, arch_thread, fpuContext);

	DEFINE_OFFSET_MACRO(ARCH_CONTEXT, arch_context, sp);

	DEFINE_OFFSET_MACRO(ARCH_STACK, arch_stack, thread);

	DEFINE_SIZEOF_MACRO(IFRAME, iframe);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, status);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, cause);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, tval);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, ra);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, sp);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, tp);
	DEFINE_OFFSET_MACRO(IFRAME, iframe, epc);
}
