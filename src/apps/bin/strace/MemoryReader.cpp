/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <debugger.h>

#include "MemoryReader.h"

// constructor
MemoryReader::MemoryReader(port_id nubPort)
	: fNubPort(nubPort),
	  fReplyPort(-1)
{
	fReplyPort = create_port(1, "memory reader reply");
	if (fReplyPort < 0) {
		fprintf(stderr, "Failed to create memory reader reply port: %s\n",
			strerror(fReplyPort));
		exit(1);
	}
}

// constructor
MemoryReader::~MemoryReader()
{
	if (fReplyPort >= 0)
		delete_port(fReplyPort);
}

// Read
status_t
MemoryReader::Read(void *_address, void *_buffer, int32 size, int32 &bytesRead)
{
	char *address = (char*)_address;
	char *buffer = (char*)_buffer;
	bytesRead = 0;

	// If the region to be read crosses page boundaries, we split it up into
	// smaller chunks.
	while (size > 0) {
		int32 toRead = size;
		if (toRead > B_MAX_READ_WRITE_MEMORY_SIZE)
			toRead = B_MAX_READ_WRITE_MEMORY_SIZE;
		if ((uint32)address % B_PAGE_SIZE + toRead > B_PAGE_SIZE)
			toRead = B_PAGE_SIZE - (uint32)address % B_PAGE_SIZE;

		status_t error = _Read(address, buffer, toRead);

		// If reading fails, we only fail, if we haven't read something yet.
		if (error != B_OK) {
			if (bytesRead > 0)
				return B_OK;
			return error;
		}

		bytesRead += toRead;
		address += toRead;
		buffer += toRead;
		size -= toRead;
	}

	return B_OK;
}

// _Read
status_t
MemoryReader::_Read(void *address, void *buffer, int32 size)
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

	memcpy(buffer, reply.data, size);

	return B_OK;
}
