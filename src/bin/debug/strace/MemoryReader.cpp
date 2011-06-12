/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debugger.h>

#include "MemoryReader.h"


MemoryReader::MemoryReader()
	:
	fNubPort(-1),
	fReplyPort(-1)
{
}


MemoryReader::~MemoryReader()
{
	if (fReplyPort >= 0)
		delete_port(fReplyPort);
}


status_t
MemoryReader::Init(port_id nubPort)
{
	if (fReplyPort >= 0)
		delete_port(fReplyPort);

	fNubPort = nubPort;

	fReplyPort = create_port(1, "memory reader reply");
	if (fReplyPort < 0) {
		fprintf(stderr, "Failed to create memory reader reply port: %s\n",
			strerror(fReplyPort));
		return fReplyPort;
	}

	return B_OK;
}


status_t
MemoryReader::Read(void *_address, void *_buffer, int32 size, int32 &bytesRead)
{
	char *address = (char*)_address;
	char *buffer = (char*)_buffer;
	bytesRead = 0;

	while (size > 0) {
		int32 toRead = size;
		if (toRead > B_MAX_READ_WRITE_MEMORY_SIZE)
			toRead = B_MAX_READ_WRITE_MEMORY_SIZE;

		int32 actuallyRead = 0;
		status_t error = _Read(address, buffer, toRead, actuallyRead);

		// If reading fails, we only fail, if we haven't read anything yet.
		if (error != B_OK) {
			if (bytesRead > 0)
				return B_OK;
			return error;
		}

		bytesRead += actuallyRead;
		address += actuallyRead;
		buffer += actuallyRead;
		size -= actuallyRead;
	}

	return B_OK;
}


status_t
MemoryReader::_Read(void *address, void *buffer, int32 size, int32 &bytesRead)
{
	// prepare message
	debug_nub_read_memory message;
	message.reply_port = fReplyPort;
	message.address = address;
	message.size = size;

	// send message
	while (true) {
		status_t error = write_port(fNubPort, B_DEBUG_MESSAGE_READ_MEMORY,
			&message, sizeof(message));
		if (error == B_OK)
			break;
		if (error != B_INTERRUPTED)
			return error;
	}

	// get reply
	int32 code;
	debug_nub_read_memory_reply reply;
	while (true) {
		ssize_t bytesRead = read_port(fReplyPort, &code, &reply, sizeof(reply));
		if (bytesRead > 0)
			break;
		if (bytesRead != B_INTERRUPTED)
			return bytesRead;
	}

	if (reply.error != B_OK)
		return reply.error;

	bytesRead = reply.size;
	memcpy(buffer, reply.data, bytesRead);

	return B_OK;
}
