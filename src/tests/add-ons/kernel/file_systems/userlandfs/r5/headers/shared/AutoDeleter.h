//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AutoDeleter.h
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Scope-based automatic deletion of objects/arrays.
//					ObjectDeleter - deletes an object
//					ArrayDeleter  - deletes an array
//					MemoryDeleter - free()s malloc()ed memory
//------------------------------------------------------------------------------

#ifndef _AUTO_DELETER_H
#define _AUTO_DELETER_H

#include <stdlib.h>

namespace BPrivate {

// AutoDeleter

template<typename C, typename Delete>
class AutoDeleter {
public:
	inline AutoDeleter()
		: fObject(NULL)
	{
	}

	inline AutoDeleter(C *object)
		: fObject(object)
	{
	}

	inline ~AutoDeleter()
	{
		fDelete(fObject);
	}

	inline void SetTo(C *object)
	{
		fDelete(fObject);
		fObject = object;
	}

	inline C *Detach()
	{
		C *object = fObject;
		fObject = NULL;
		return object;
	}

private:
	C		*fObject;
	Delete	fDelete;
};


// ObjectDeleter

template<typename C>
struct ObjectDelete
{
	inline void operator()(C *object)
	{
		delete object;
	}
};

template<typename C>
struct ObjectDeleter : AutoDeleter<C, ObjectDelete<C> >
{
	ObjectDeleter() : AutoDeleter<C, ObjectDelete<C> >() {}
	ObjectDeleter(C *object) : AutoDeleter<C, ObjectDelete<C> >(object) {}
};


// ArrayDeleter

template<typename C>
struct ArrayDelete
{
	inline void operator()(C *array)
	{
		delete[] array;
	}
};

template<typename C>
struct ArrayDeleter : AutoDeleter<C, ArrayDelete<C> >
{
	ArrayDeleter() : AutoDeleter<C, ArrayDelete<C> >() {}
	ArrayDeleter(C *array) : AutoDeleter<C, ArrayDelete<C> >(array) {}
};


// MemoryDeleter

struct MemoryDelete
{
	inline void operator()(void *memory)
	{
		free(memory);
	}
};

struct MemoryDeleter : AutoDeleter<void, MemoryDelete >
{
	MemoryDeleter() : AutoDeleter<void, MemoryDelete >() {}
	MemoryDeleter(void *memory) : AutoDeleter<void, MemoryDelete >(memory) {}
};

}	// namespace BPrivate

using BPrivate::ObjectDeleter;
using BPrivate::ArrayDeleter;
using BPrivate::MemoryDeleter;

#endif	// _AUTO_DELETER_H
