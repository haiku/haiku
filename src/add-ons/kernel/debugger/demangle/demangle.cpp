/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "demangle.h"

#include <string.h>

#ifdef _KERNEL_MODE
#	include <debug.h>
#endif


static inline bool
looks_like_gcc3_symbol(const char* symbol)
{
	return strncmp(symbol, "_Z", 2) == 0;
}


const char*
demangle_symbol(const char* mangledName, char* buffer, size_t bufferSize,
	bool* _isObjectMethod)
{
	// try the gcc3 demangler, if it looks like a gcc3 symbol
	const char* demangled = NULL;
	if (looks_like_gcc3_symbol(mangledName)) {
		demangled = demangle_symbol_gcc3(mangledName, buffer, bufferSize,
			_isObjectMethod);
		if (demangled != NULL)
			return demangled;
	}

	// fallback is gcc2
	return demangle_symbol_gcc2(mangledName, buffer, bufferSize,
		_isObjectMethod);
}


status_t
get_next_argument(uint32* _cookie, const char* mangledName, char* name,
	size_t nameSize, int32* _type, size_t* _argumentLength)
{
	// try the gcc3 demangler, if it looks like a gcc3 symbol
	if (looks_like_gcc3_symbol(mangledName)) {
		status_t error = get_next_argument_gcc3(_cookie, mangledName, name,
			nameSize, _type, _argumentLength);
		if (error == B_OK || error == B_BAD_INDEX)
			return error;
	}

	// fallback is gcc2
	return get_next_argument_gcc2(_cookie, mangledName, name, nameSize, _type,
		_argumentLength);
}


#ifdef _KERNEL_MODE

static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_BAD_VALUE;
}


static struct debugger_demangle_module_info sModuleInfo = {
	{
		"debugger/demangle/v1",
		0,
		std_ops
	},

	demangle_symbol,
	get_next_argument,
};

module_info* modules[] = {
	(module_info*)&sModuleInfo,
	NULL
};

#endif // _KERNEL_MODE
