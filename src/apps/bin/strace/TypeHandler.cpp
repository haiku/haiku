/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "MemoryReader.h"
#include "TypeHandler.h"

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
