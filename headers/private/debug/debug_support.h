/*
 * Copyright 2005-2009, Ingo Weinhold, bonefish@users.sf.net.
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
	team_id	team;
	port_id	nub_port;
	port_id	reply_port;
} debug_context;


status_t init_debug_context(debug_context *context, team_id team,
	port_id nubPort);
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
ssize_t debug_write_memory_partial(debug_context *context, const void *address,
			void *buffer, size_t size);
ssize_t debug_write_memory(debug_context *context, const void *address,
			void *buffer, size_t size);

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


// symbol lookup support

typedef struct debug_symbol_lookup_context debug_symbol_lookup_context;
typedef struct debug_symbol_iterator debug_symbol_iterator;

status_t debug_create_symbol_lookup_context(team_id team,
			debug_symbol_lookup_context **lookupContext);
void debug_delete_symbol_lookup_context(
			debug_symbol_lookup_context *lookupContext);

status_t debug_get_symbol(debug_symbol_lookup_context* lookupContext,
			image_id image, const char* name, int32 symbolType,
			void** _symbolLocation, size_t* _symbolSize, int32* _symbolType);
status_t debug_lookup_symbol_address(debug_symbol_lookup_context *lookupContext,
			const void *address, void **baseAddress, char *symbolName,
			int32 symbolNameSize, char *imageName, int32 imageNameSize,
			bool *exactMatch);

status_t debug_create_image_symbol_iterator(
			debug_symbol_lookup_context* lookupContext, image_id imageID,
			debug_symbol_iterator** _iterator);
status_t debug_create_file_symbol_iterator(const char* path,
			debug_symbol_iterator** _iterator);
void debug_delete_symbol_iterator(debug_symbol_iterator* iterator);

status_t debug_next_image_symbol(debug_symbol_iterator* iterator,
			char* nameBuffer, size_t nameBufferLength, int32* _symbolType,
			void** _symbolLocation, size_t* _symbolSize);
status_t debug_get_symbol_iterator_image_info(debug_symbol_iterator* iterator,
			image_info* info);

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _DEBUG_SUPPORT_H
