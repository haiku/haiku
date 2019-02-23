/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BYTE_POINTER_H
#define _BYTE_POINTER_H


#include <stdlib.h>


// Behaves like a char* pointer, but -> and & return the right pointed type.
// Assumes the offsets passed to + and += maintain the alignment for the type.
template<class T> struct BytePointer {
	char* address;

	BytePointer(void* base) { address = (char*)base; }

	T* operator&() { return reinterpret_cast<T*>(address); }
	T* operator->() { return reinterpret_cast<T*>(address); }
	void operator+=(size_t offset) { address += offset; }
	char* operator+(size_t offset) const { return address + offset; }
};


#endif
