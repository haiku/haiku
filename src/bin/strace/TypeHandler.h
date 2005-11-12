/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRACE_TYPE_HANDLER_H
#define STRACE_TYPE_HANDLER_H

#include <string>

#include <arch_config.h>
#include <SupportDefs.h>

using std::string;

class MemoryReader;

typedef FUNCTION_CALL_PARAMETER_ALIGNMENT_TYPE align_t;

// TypeHandler
class TypeHandler {
public:
	TypeHandler() {}
	virtual ~TypeHandler() {}

	virtual string GetParameterValue(const void *address, bool getContents,
		MemoryReader &reader) = 0;

	virtual string GetReturnValue(uint64 value, bool getContents,
		MemoryReader &reader) = 0;
};

// templatized TypeHandler factory class
// (I tried a simple function first, but then the compiler complains for
// the partial instantiation. Not sure, if I'm missing something or this is
// a compiler bug).
template<typename Type>
struct TypeHandlerFactory {
	static TypeHandler *Create();
};

extern TypeHandler *create_pointer_type_handler();
extern TypeHandler *create_string_type_handler();

// specialization for "const char*"
template<>
struct TypeHandlerFactory<const char*> {
	static inline TypeHandler *Create()
	{
		return create_string_type_handler();
	}
};

// partial specialization for generic pointers
template<typename Type>
struct TypeHandlerFactory<Type*> {
	static inline TypeHandler *Create()
	{
		return create_pointer_type_handler();
	}
};

#endif	// STRACE_TYPE_HANDLER_H
