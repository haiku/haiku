/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_LTRACE_STUB_H
#define ARCH_LTRACE_STUB_H


extern "C" {

size_t	arch_call_stub_size();
void	arch_init_call_stub(void* stub,
			void* (*callback)(const void* stub, const void* args),
			void* function);

}	// extern "C"


#endif	// ARCH_LTRACE_STUB_H
