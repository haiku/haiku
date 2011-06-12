/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRACE_MEMORY_READER_H
#define STRACE_MEMORY_READER_H


#include <OS.h>


class MemoryReader {
public:
								MemoryReader();
								~MemoryReader();

			status_t			Init(port_id nubPort);

			status_t			Read(void *address, void *buffer, int32 size,
									int32 &bytesRead);

private:
			status_t			_Read(void *address, void *buffer, int32 size,
									int32 &bytesRead);

private:
			port_id				fNubPort;
			port_id				fReplyPort;
};


#endif	// STRACE_MEMORY_READER_H
