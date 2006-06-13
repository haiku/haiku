/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		IngoWeinhold <bonefish@cs.tu-berlin.de>
 */

/**	Scope-based automatic deletion of objects/arrays.
 *					ObjectDeleter - deletes an object
 *					ArrayDeleter  - deletes an array
 *					MemoryDeleter - free()s malloc()ed memory
 */

#ifndef _AUTO_DELETER_H
#define _AUTO_DELETER_H

#include <stdlib.h>

namespace BPrivate {

// AutoDeleter

template<typename C, typename DeleteFunc>
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
		if (object != fObject) {
			fDelete(fObject);
			fObject = object;
		}
	}

	inline void Unset()
	{
		SetTo(NULL);
	}

	inline void Delete()
	{
		SetTo(NULL);
	}

	inline C *Detach()
	{
		C *object = fObject;
		fObject = NULL;
		return object;
	}

private:
	C			*fObject;
	DeleteFunc	fDelete;
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
