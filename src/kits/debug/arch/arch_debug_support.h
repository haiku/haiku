/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_DEBUG_SUPPORT_H
#define _ARCH_DEBUG_SUPPORT_H

#include <debug_support.h>

#ifdef __cplusplus
extern "C" {
#endif

status_t arch_debug_get_instruction_pointer(debug_context *context,
			thread_id thread, void **ip, void **stackFrameAddress);
status_t arch_debug_get_stack_frame(debug_context *context,
			void *stackFrameAddress, debug_stack_frame_info *stackFrameInfo);

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	// _ARCH_DEBUG_SUPPORT_H
