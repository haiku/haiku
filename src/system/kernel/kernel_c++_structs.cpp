/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <computed_asm_macros.h>

#include <condition_variable.h>


// NOTE: Don't include <kernel_c++_structs.h>, since that would result in a
// circular dependency.


#define DEFINE_SIZE_MACRO(name) \
    DEFINE_COMPUTED_ASM_MACRO(_KERNEL_CPP_STRUCT_SIZE_##name, sizeof(name));


void
dummy()
{
	DEFINE_SIZE_MACRO(ConditionVariable)
}
