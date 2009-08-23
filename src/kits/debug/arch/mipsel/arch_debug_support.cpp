/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "arch_debug_support.h"

#include <debug_support.h>


status_t
arch_debug_get_instruction_pointer(debug_context *context, thread_id thread,
	void **ip, void **stackFrameAddress)
{
#warning IMPLEMENT arch_debug_get_instruction_pointer
	return B_ERROR;
}


status_t
arch_debug_get_stack_frame(debug_context *context, void *stackFrameAddress,
	debug_stack_frame_info *stackFrameInfo)
{
#warning IMPLEMENT arch_debug_get_stack_frame
	return B_ERROR;
}

