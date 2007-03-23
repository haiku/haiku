/*
 * Copyright 2005-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		Hugo Santos <hugosantos@gmail.com>
 */

#include "MemoryReader.h"
#include "TypeHandler.h"


// TypeHandlerImpl
template<typename Type>
class TypeHandlerImpl : public TypeHandler {
public:
	virtual string GetParameterValue(const void *address, bool getContents,
		MemoryReader &reader);

	virtual string GetReturnValue(uint64 value, bool getContents,
		MemoryReader &reader);
};


// #pragma mark -

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

// create_pointer_type_handler
TypeHandler *
create_pointer_type_handler()
{
	return new TypeHandlerImpl<const void*>();
}

// create_string_type_handler
TypeHandler *
create_string_type_handler()
{
	return new TypeHandlerImpl<const char*>();
}

TypeHandler *
create_fdset_type_handler()
{
	return new TypeHandlerImpl<struct fd_set *>();
}


// #pragma mark -

// complete specializations

// void
template<>
string
TypeHandlerImpl<void>::GetParameterValue(const void *address, bool getContents,
		MemoryReader &reader)
{
	return "void";
}

template<>
string
TypeHandlerImpl<void>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return "";
}

template<>
TypeHandler *
TypeHandlerFactory<void>::Create()
{
	return new TypeHandlerImpl<void>();
}

// bool
template<>
string
TypeHandlerImpl<bool>::GetParameterValue(const void *address, bool getContents,
	MemoryReader &reader)
{
	return (*(align_t*)address ? "true" : "false");
}

template<>
string
TypeHandlerImpl<bool>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return (value ? "true" : "false");
}

template<>
TypeHandler *
TypeHandlerFactory<bool>::Create()
{
	return new TypeHandlerImpl<bool>();
}

// char
template<>
string
TypeHandlerImpl<char>::GetParameterValue(const void *address, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<char>(address, "0x%x");
}

template<>
string
TypeHandlerImpl<char>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<char>(value, "0x%x");
}

template<>
TypeHandler *
TypeHandlerFactory<char>::Create()
{
	return new TypeHandlerImpl<char>();
}

// short
template<>
string
TypeHandlerImpl<short>::GetParameterValue(const void *address, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<short>(address, "0x%x");
}

template<>
string
TypeHandlerImpl<short>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<short>(value, "0x%x");
}

template<>
TypeHandler *
TypeHandlerFactory<short>::Create()
{
	return new TypeHandlerImpl<short>();
}


// unsigned short
template<>
string
TypeHandlerImpl<unsigned short>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	return get_number_value<unsigned short>(address, "0x%x");
}

template<>
string
TypeHandlerImpl<unsigned short>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<unsigned short>(value, "0x%x");
}

template<>
TypeHandler *
TypeHandlerFactory<unsigned short>::Create()
{
	return new TypeHandlerImpl<unsigned short>();
}

// int
template<>
string
TypeHandlerImpl<int>::GetParameterValue(const void *address, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<int>(address, "0x%x");
}

template<>
string
TypeHandlerImpl<int>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<int>(value, "0x%x");
}

template<>
TypeHandler *
TypeHandlerFactory<int>::Create()
{
	return new TypeHandlerImpl<int>();
}

// unsigned int
template<>
string
TypeHandlerImpl<unsigned int>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	return get_number_value<unsigned int>(address, "0x%x");
}

template<>
string
TypeHandlerImpl<unsigned int>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<unsigned int>(value, "0x%x");
}

template<>
TypeHandler *
TypeHandlerFactory<unsigned int>::Create()
{
	return new TypeHandlerImpl<unsigned int>();
}

// long
template<>
string
TypeHandlerImpl<long>::GetParameterValue(const void *address, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<long>(address, "0x%lx");
}

template<>
string
TypeHandlerImpl<long>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<long>(value, "0x%lx");
}

template<>
TypeHandler *
TypeHandlerFactory<long>::Create()
{
	return new TypeHandlerImpl<long>();
}

// unsigned long
template<>
string
TypeHandlerImpl<unsigned long>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	return get_number_value<unsigned long>(address, "0x%lx");
}

template<>
string
TypeHandlerImpl<unsigned long>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<unsigned long>(value, "0x%lx");
}

template<>
TypeHandler *
TypeHandlerFactory<unsigned long>::Create()
{
	return new TypeHandlerImpl<unsigned long>();
}

// long long
template<>
string
TypeHandlerImpl<long long>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	return get_number_value<long long>(address, "0x%llx");
}

template<>
string
TypeHandlerImpl<long long>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_number_value<long long>(value, "0x%llx");
}

template<>
TypeHandler *
TypeHandlerFactory<long long>::Create()
{
	return new TypeHandlerImpl<long long>();
}

// unsigned long
template<>
string
TypeHandlerImpl<unsigned long long>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	return get_number_value<unsigned long>(address, "0x%llx");
}

template<>
string
TypeHandlerImpl<unsigned long long>::GetReturnValue(uint64 value,
	bool getContents, MemoryReader &reader)
{
	return get_number_value<unsigned long long>(value, "0x%llx");
}

template<>
TypeHandler *
TypeHandlerFactory<unsigned long long>::Create()
{
	return new TypeHandlerImpl<unsigned long long>();
}

// read_string
static
string
read_string(MemoryReader &reader, void *data)
{
	char buffer[256];
	int32 bytesRead;
	status_t error = reader.Read(data, buffer, sizeof(buffer), bytesRead);
	if (error == B_OK) {
//		return string("\"") + string(buffer, bytesRead) + "\"";
//string result("\"");
//result += string(buffer, bytesRead);
//result += "\"";
//return result;

// TODO: Unless I'm missing something obvious, our STL string class is broken.
// The appended "\"" doesn't appear in either of the above cases.

		int32 len = strnlen(buffer, sizeof(buffer));
		char largeBuffer[259];
		largeBuffer[0] = '"';
		memcpy(largeBuffer + 1, buffer, len);
		largeBuffer[len + 1] = '"';
		largeBuffer[len + 2] = '\0';
		return largeBuffer;
	}
	return get_pointer_value(&data) + " (" + strerror(error) + ")";
}

static string
read_fdset(MemoryReader &reader, void *data)
{
	/* default FD_SETSIZE is 1024 */
	unsigned long tmp[1024 / (sizeof(unsigned long) * 8)];
	int32 bytesRead;

	status_t err = reader.Read(data, &tmp, sizeof(tmp), bytesRead);
	if (err != B_OK)
		return get_pointer_value(&data);

	/* implicitly align to unsigned long lower boundary */
	int count = bytesRead / sizeof(unsigned long);
	int added = 0;

	string r = "[";

	for (int i = 0; i < count && added < 8; i++) {
		for (int j = 0;
			 j < (int)(sizeof(unsigned long) * 8) && added < 8; j++) {
			if (tmp[i] & (1 << j)) {
				if (added > 0)
					r += ", ";
				r += get_number_value<unsigned long>(
						i * (sizeof(unsigned long) * 8) + j,
						"%u");
				added++;
			}
		}
	}

	if (added >= 8)
		r += " ...";

	return r + "]";
}

// const void*
template<>
string
TypeHandlerImpl<const void*>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	return get_pointer_value(address);
}

template<>
string
TypeHandlerImpl<const void*>::GetReturnValue(uint64 value, bool getContents,
	MemoryReader &reader)
{
	return get_pointer_value(value);
}

// const char*
template<>
string
TypeHandlerImpl<const char*>::GetParameterValue(const void *address,
	bool getContents, MemoryReader &reader)
{
	void *data = *(void**)address;
	if (getContents && data)
		return read_string(reader, data);

	return get_pointer_value(&data);
}

template<>
string
TypeHandlerImpl<const char*>::GetReturnValue(uint64 value,
	bool getContents, MemoryReader &reader)
{
	void *data = (void*)value;
	if (getContents && data)
		return read_string(reader, data);

	return get_pointer_value(&data);
}

// struct fd_set *
template<>
string
TypeHandlerImpl<struct fd_set *>::GetParameterValue(const void *address,
		bool getContents, MemoryReader &reader)
{
	void *data = *(void **)address;
	if (getContents && data)
		return read_fdset(reader, data);
	return get_pointer_value(&data);
}

template<>
string
TypeHandlerImpl<struct fd_set *>::GetReturnValue(uint64 value,
	bool getContents, MemoryReader &reader)
{
	void *data = (void *)value;
	if (getContents && data)
		return read_fdset(reader, data);

	return get_pointer_value(&data);
}
