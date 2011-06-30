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
	FileDescriptorCloser - closes a file descriptor
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


// CObjectDeleter

template<typename Type, typename DestructorReturnType>
struct CObjectDelete
{
	inline void operator()(Type *object)
	{
		if (fDestructor != NULL && object != NULL)
			fDestructor(object);
	}

	template<typename Destructor>
	inline void operator=(Destructor destructor)
	{
		fDestructor = destructor;
	}

private:
	DestructorReturnType (*fDestructor)(Type*);
};

template<typename Type, typename DestructorReturnType = void>
struct CObjectDeleter
	: AutoDeleter<Type, CObjectDelete<Type, DestructorReturnType> >
{
	typedef AutoDeleter<Type, CObjectDelete<Type, DestructorReturnType> > Base;

	template<typename Destructor>
	CObjectDeleter(Destructor destructor) : Base()
	{
		Base::fDelete = destructor;
	}

	template<typename Destructor>
	CObjectDeleter(Type *object, Destructor destructor) : Base(object)
	{
		Base::fDelete = destructor;
	}
};


// MethodDeleter

template<typename Type, typename DestructorReturnType>
struct MethodDelete
{
	inline void operator()(Type *object)
	{
		if (fDestructor && object != NULL)
			(object->*fDestructor)();
	}

	template<typename Destructor>
	inline void operator=(Destructor destructor)
	{
		fDestructor = destructor;
	}

private:
	DestructorReturnType (Type::*fDestructor)();
};


template<typename Type, typename DestructorReturnType = void>
struct MethodDeleter
	: AutoDeleter<Type, MethodDelete<Type, DestructorReturnType> >
{
	typedef AutoDeleter<Type, MethodDelete<Type, DestructorReturnType> > Base;

	template<typename Destructor>
	MethodDeleter(Destructor destructor) : Base()
	{
		Base::fDelete = destructor;
	}

	template<typename Destructor>
	MethodDeleter(Type *object, Destructor destructor) : Base(object)
	{
		Base::fDelete = destructor;
	}
};


// FileDescriptorCloser

struct FileDescriptorCloser {
	inline FileDescriptorCloser()
		:
		fDescriptor(-1)
	{
	}

	inline FileDescriptorCloser(int descriptor)
		:
		fDescriptor(descriptor)
	{
	}

	inline ~FileDescriptorCloser()
	{
		SetTo(-1);
	}

	inline void SetTo(int descriptor)
	{
		if (fDescriptor >= 0)
			close(fDescriptor);

		fDescriptor = descriptor;
	}

	inline void Unset()
	{
		SetTo(-1);
	}

	inline int Detach()
	{
		int descriptor = fDescriptor;
		fDescriptor = -1;
		return descriptor;
	}

private:
	int	fDescriptor;
};


}	// namespace BPrivate


using BPrivate::ObjectDeleter;
using BPrivate::ArrayDeleter;
using BPrivate::MemoryDeleter;
using BPrivate::CObjectDeleter;
using BPrivate::MethodDeleter;
using BPrivate::FileDescriptorCloser;


#endif	// _AUTO_DELETER_H
