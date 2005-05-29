/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <debug_support.h>

#include "arch_debug_support.h"


// init_debug_context
status_t
init_debug_context(debug_context *context, port_id nubPort)
{
	if (!context || nubPort < 0)
		return B_BAD_VALUE;

	context->nub_port = nubPort;

	// create the reply port
	context->reply_port = create_port(1, "debug reply port");
	if (context->reply_port < 0)
		return context->reply_port;

	return B_OK;
}

// destroy_debug_context
void
destroy_debug_context(debug_context *context)
{
	if (context) {
		if (context->reply_port >= 0)
			delete_port(context->reply_port);
	}
}

// send_debug_message
status_t
send_debug_message(debug_context *context, int32 messageCode,
	const void *message, int32 messageSize, void *reply, int32 replySize)
{
	if (!context)
		return B_BAD_VALUE;

	// send message
	while (true) {
		status_t result = write_port(context->nub_port, messageCode, message,
			messageSize);
		if (result == B_OK)
			break;
		if (result != B_INTERRUPTED)
			return result;
	}

	if (!reply)
		return B_OK;

	// read reply
	while (true) {
		int32 code;
		ssize_t bytesRead = read_port(context->reply_port, &code, reply,
			replySize);
		if (bytesRead > 0)
			return B_OK;
		if (bytesRead != B_INTERRUPTED)
			return bytesRead;
	}
}

// debug_read_memory_partial
ssize_t
debug_read_memory_partial(debug_context *context, const void *address,
	void *buffer, size_t size)
{
	if (!context)
		return B_BAD_VALUE;

	if (size == 0)
		return 0;
	if (size > B_MAX_READ_WRITE_MEMORY_SIZE)
		size = B_MAX_READ_WRITE_MEMORY_SIZE;

	// prepare the message
	debug_nub_read_memory message;
	message.reply_port = context->reply_port;
	message.address = (void*)address;
	message.size = size;

	// send the message
	debug_nub_read_memory_reply reply;
	status_t error = send_debug_message(context, B_DEBUG_MESSAGE_READ_MEMORY,
		&message, sizeof(message), &reply, sizeof(reply));

	if (error != B_OK)
		return error;
	if (reply.error != B_OK)
		return reply.error;

	// copy the read data
	memcpy(buffer, reply.data, reply.size);
	return reply.size;
}

// debug_read_memory
ssize_t
debug_read_memory(debug_context *context, const void *_address, void *_buffer,
	size_t size)
{
	const char *address = (const char *)_address;
	char *buffer = (char*)_buffer;

	// check parameters
	if (!context || !address || !buffer)
		return B_BAD_VALUE;
	if (size == 0)
		return 0;

	// read as long as we can read data
	ssize_t sumRead = 0;
	while (size > 0) {
		ssize_t bytesRead = debug_read_memory_partial(context, address, buffer,
			size);
		if (bytesRead < 0) {
			if (sumRead > 0)
				return sumRead;
			return bytesRead;
		}

		address += bytesRead;
		buffer += bytesRead;
		sumRead += bytesRead;
		size -= bytesRead;
	}

	return sumRead;
}

// debug_read_string
ssize_t
debug_read_string(debug_context *context, const void *_address, char *buffer,
	size_t size)
{
	const char *address = (const char *)_address;

	// check parameters
	if (!context || !address || !buffer || size == 0)
		return B_BAD_VALUE;

	// read as long as we can read data
	ssize_t sumRead = 0;
	while (size > 0) {
		ssize_t bytesRead = debug_read_memory_partial(context, address, buffer,
			size);
		if (bytesRead < 0) {
			// always null-terminate what we have (even, if it is an empty
			// string) and be done
			*buffer = '\0';
			return (sumRead > 0 ? sumRead : bytesRead);
		}

		int chunkSize = strnlen(buffer, bytesRead);
		if (chunkSize < bytesRead) {
			// we found a terminating null
			sumRead += chunkSize;
			return sumRead;
		}

		address += bytesRead;
		buffer += bytesRead;
		sumRead += bytesRead;
		size -= bytesRead;
	}

	// We filled the complete buffer without encountering a terminating null
	// replace the last char. But nevertheless return the full size to indicate
	// that the buffer was too small.
	buffer[-1] = '\0';

	return sumRead;
}


status_t
debug_get_cpu_state(debug_context *context, thread_id thread,
	debug_debugger_message *messageCode, debug_cpu_state *cpuState)
{
	if (!context || !cpuState)
		return B_BAD_VALUE;

	// prepare message
	debug_nub_get_cpu_state message;
	message.reply_port = context->reply_port;
	message.thread = thread;

	// send message
	debug_nub_get_cpu_state_reply reply;
	status_t error = send_debug_message(context, B_DEBUG_MESSAGE_GET_CPU_STATE,
		&message, sizeof(message), &reply, sizeof(reply));
	if (error == B_OK)
		error = reply.error;

	// get state
	if (error == B_OK) {
		*cpuState = reply.cpu_state;
		if (messageCode)
			*messageCode = reply.message;
	}

	return error;
}


status_t
debug_get_instruction_pointer(debug_context *context, thread_id thread,
	void **ip, void **stackFrameAddress)
{
	if (!context || !ip || !stackFrameAddress)
		return B_BAD_VALUE;

	return arch_debug_get_instruction_pointer(context, thread, ip,
		stackFrameAddress);
}


status_t
debug_get_stack_frame(debug_context *context, void *stackFrameAddress,
	debug_stack_frame_info *stackFrameInfo)
{
	if (!context || !stackFrameAddress || !stackFrameInfo)
		return B_BAD_VALUE;

	return arch_debug_get_stack_frame(context, stackFrameAddress,
		stackFrameInfo);
}
