/*
 * Copyright 2001-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_DELETER_H
#define _AUTO_DELETER_H


/*!	Scope-based automatic deletion of objects/arrays.
	ObjectDeleter  - deletes an object
	ArrayDeleter   - deletes an array
	MemoryDeleter  - free()s malloc()ed memory
	CObjectDeleter - calls an arbitrary specified destructor function
	HandleDeleter  - use arbitrary handle type and destructor function
	FileDescriptorCloser - closes a file descriptor, based on HandleDeleter
*/


#include <stdlib.h>
#include <unistd.h>


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
		DeleteFunc destructor;
		destructor(fObject);
	}

	inline void SetTo(C *object)
	{
		if (object != fObject) {
			DeleteFunc destructor;
			destructor(fObject);
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

	inline C *Get() const
	{
		return fObject;
	}

	inline C *Detach()
	{
		C *object = fObject;
		fObject = NULL;
		return object;
	}

	inline C *operator->() const
	{
		return fObject;
	}

protected:
	C			*fObject;

private:
	AutoDeleter(const AutoDeleter&);
	AutoDeleter& operator=(const AutoDeleter&);
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

	inline C& operator[](size_t index) const
	{
		return this->Get()[index];
	}
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


// CObjectDeleter

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (*Destructor)(Type*)>
struct CObjectDelete
{
	inline void operator()(Type *object)
	{
		if (object != NULL)
			Destructor(object);
	}
};

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (*Destructor)(Type*)>
struct CObjectDeleter
	: AutoDeleter<Type, CObjectDelete<Type, DestructorReturnType, Destructor> >
{
	typedef AutoDeleter<Type,
		CObjectDelete<Type, DestructorReturnType, Destructor> > Base;

	CObjectDeleter() : Base()
	{
	}

	CObjectDeleter(Type *object) : Base(object)
	{
	}
};


// MethodDeleter

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (Type::*Destructor)()>
struct MethodDelete
{
	inline void operator()(Type *object)
	{
		if (object != NULL)
			(object->*Destructor)();
	}
};


template<typename Type, typename DestructorReturnType,
	DestructorReturnType (Type::*Destructor)()>
struct MethodDeleter
	: AutoDeleter<Type, MethodDelete<Type, DestructorReturnType, Destructor> >
{
	typedef AutoDeleter<Type,
		MethodDelete<Type, DestructorReturnType, Destructor> > Base;

	MethodDeleter() : Base()
	{
	}

	MethodDeleter(Type *object) : Base(object)
	{
	}
};


// HandleDeleter

template<typename C, typename DestructorResult,
	DestructorResult (*Destructor)(C), C nullValue = -1>
class HandleDeleter {
public:
	inline HandleDeleter()
		: fHandle(nullValue)
	{
	}

	inline HandleDeleter(C handle)
		: fHandle(handle)
	{
	}

	inline ~HandleDeleter()
	{
		Destructor(fHandle);
	}

	inline void SetTo(C handle)
	{
		if (handle != fHandle) {
			Destructor(fHandle);
			fHandle = handle;
		}
	}

	inline void Unset()
	{
		SetTo(nullValue);
	}

	inline void Delete()
	{
		SetTo(nullValue);
	}

	inline C Get() const
	{
		return fHandle;
	}

	inline C Detach()
	{
		C handle = fHandle;
		fHandle = nullValue;
		return handle;
	}

protected:
	C			fHandle;

private:
	HandleDeleter(const HandleDeleter&);
	HandleDeleter& operator=(const HandleDeleter&);
};


// FileDescriptorCloser

typedef HandleDeleter<int, int, close, -1> FileDescriptorCloser;


}	// namespace BPrivate


using ::BPrivate::ObjectDeleter;
using ::BPrivate::ArrayDeleter;
using ::BPrivate::MemoryDeleter;
using ::BPrivate::CObjectDeleter;
using ::BPrivate::MethodDeleter;
using ::BPrivate::HandleDeleter;
using ::BPrivate::FileDescriptorCloser;


#endif	// _AUTO_DELETER_H
