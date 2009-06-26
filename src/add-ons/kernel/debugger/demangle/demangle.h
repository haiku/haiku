/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEMANGLE_H
#define DEMANGLE_H

#include <SupportDefs.h>


const char* demangle_symbol(const char* mangledName, char* buffer, size_t bufferSize,
	bool* _isObjectMethod);

status_t 	get_next_argument(uint32* _cookie, const char* mangledName, char* name,
	size_t nameSize, int32* _type, size_t* _argumentLength);

// gcc 2
const char*	demangle_symbol_gcc2(const char* name, char* buffer,
				size_t bufferSize, bool* _isObjectMethod);
status_t	get_next_argument_gcc2(uint32* _cookie, const char* symbol,
				char* name, size_t nameSize, int32* _type,
				size_t* _argumentLength);


// gcc 3+
const char*	demangle_symbol_gcc3(const char* name, char* buffer,
				size_t bufferSize, bool* _isObjectMethod);
status_t	get_next_argument_gcc3(uint32* _cookie, const char* symbol,
				char* name, size_t nameSize, int32* _type,
				size_t* _argumentLength);

#ifndef _KERNEL_MODE
const char*	demangle_name_gcc3(const char* name, char* buffer,
				size_t bufferSize);
#endif


#endif	// DEMANGLE_H
