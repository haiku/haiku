/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRACE_MEMORY_READER_H
#define STRACE_MEMORY_READER_H

#include <OS.h>

class MemoryReader {
public:
	MemoryReader(port_id nubPort);
	~MemoryReader();

	status_t Read(void *address, void *buffer, int32 size, int32 &bytesRead);

private:
	status_t _Read(void *address, void *buffer, int32 size, int32 &bytesRead);

	port_id	fNubPort;
	port_id	fReplyPort;
};


#endif	// STRACE_MEMORY_READER_H
