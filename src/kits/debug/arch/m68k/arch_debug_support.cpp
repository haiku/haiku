/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


#include <debug_support.h>

#include "arch_debug_support.h"


struct stack_frame {
	struct stack_frame	*previous;
	void				*return_address;
};


status_t
arch_debug_get_instruction_pointer(debug_context *context, thread_id thread,
	void **ip, void **stackFrameAddress)
{
	// get the CPU state
#warning M68K: TODO
	return B_ERROR;
}


status_t
arch_debug_get_stack_frame(debug_context *context, void *stackFrameAddress,
	debug_stack_frame_info *stackFrameInfo)
{
#warning M68K: TODO
	return B_ERROR;
}
