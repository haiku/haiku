/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <cstddef>


extern "C" void*
memcpy(void* destination, const void* source, size_t length)
{
	auto returnValue = destination;
	__asm__ __volatile__("rep movsb"
		: "+D" (destination), "+S" (source), "+c" (length)
		: : "memory");
	return returnValue;
}


extern "C" void*
memset(void* destination, int value, size_t length)
{
	auto returnValue = destination;
	__asm__ __volatile__("rep stosb"
		: "+D" (destination), "+c" (length)
		: "a" (value)
		: "memory");
	return returnValue;
}

