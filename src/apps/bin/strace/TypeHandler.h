/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRACE_TYPE_HANDLER_H
#define STRACE_TYPE_HANDLER_H

#include <string>

#include <arch_config.h>
#include <SupportDefs.h>

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

// TypeHandlerImpl
template<typename Type>
class TypeHandlerImpl : public TypeHandler {
public:
	virtual string GetParameterValue(const void *address, bool getContents,
		MemoryReader &reader);

	virtual string GetReturnValue(uint64 value, bool getContents,
		MemoryReader &reader);
};


// TypeHandlerImpl
template<typename Type>
class TypeHandlerImpl<Type*> : public TypeHandler {
public:
	virtual string GetParameterValue(const void *address, bool getContents,
		MemoryReader &reader);

	virtual string GetReturnValue(uint64 value, bool getContents,
		MemoryReader &reader);
};

//// TypeHandlerImpl
//template<>
//class TypeHandlerImpl<const char*> : public TypeHandler {
//public:
//	virtual string GetParameterValue(const void *address, bool getContents,
//		MemoryReader &reader);
//
//	virtual string GetReturnValue(uint64 value, bool getContents,
//		MemoryReader &reader);
//};

// get_number_value
template<typename value_t>
static inline
string
get_number_value(const void *address, const char *format)
{
	if (sizeof(align_t) > sizeof(value_t))
		return get_number_value<value_t>(value_t(*(align_t*)address), format);
	else
		return get_number_value<value_t>(*(value_t*)address, format);
}

// get_number_value
template<typename value_t>
static inline
string
get_number_value(value_t value, const char *format)
{
	char buffer[32];
	sprintf(buffer, format, value);
	return buffer;
}

// get_pointer_value
static inline
string
get_pointer_value(const void *address)
{
	char buffer[32];
	sprintf(buffer, "%p", *(void **)address);
	return buffer;
}

// get_pointer_value
static inline
string
get_pointer_value(uint64 value)
{
	char buffer[32];
	sprintf(buffer, "%p", (void*)value);
	return buffer;
}

// generic pointer
template<typename Type>
string
TypeHandlerImpl<Type*>::GetParameterValue(const void *address, bool getContents,
		MemoryReader &reader)
{
	return get_pointer_value(address);
}

template<typename Type>
string
TypeHandlerImpl<Type*>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_pointer_value(value);
}

#endif	// STRACE_TYPE_HANDLER_H
