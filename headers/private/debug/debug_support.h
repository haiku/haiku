/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEBUG_SUPPORT_H
#define _DEBUG_SUPPORT_H

#include <debugger.h>
#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct debug_context {
	port_id	nub_port;
	port_id	reply_port;
} debug_context;


status_t init_debug_context(debug_context *context, port_id nubPort);
void destroy_debug_context(debug_context *context);

status_t send_debug_message(debug_context *context, int32 messageCode,
			const void *message, int32 messageSize, void *reply,
			int32 replySize);

ssize_t debug_read_memory_partial(debug_context *context, const void *address,
			void *buffer, size_t size);
ssize_t debug_read_memory(debug_context *context, const void *address,
			void *buffer, size_t size);
ssize_t debug_read_string(debug_context *context, const void *_address,
			char *buffer, size_t size);

status_t debug_get_cpu_state(debug_context *context, thread_id thread,
			debug_debugger_message *messageCode, debug_cpu_state *cpuState);


// stack trace support

typedef struct debug_stack_frame_info {
	void	*frame;
	void	*parent_frame;
	void	*return_address;
} debug_stack_frame_info;

status_t debug_get_instruction_pointer(debug_context *context, thread_id thread,
			void **ip, void **stackFrameAddress);
status_t debug_get_stack_frame(debug_context *context,
			void *stackFrameAddress, debug_stack_frame_info *stackFrameInfo);


#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _DEBUG_SUPPORT_H
