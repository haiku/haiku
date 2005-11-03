/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SHARED_OBJECT_H
#define SHARED_OBJECT_H


#include <SupportDefs.h>


/*!
	\class SharedObject SharedObject.h
	\brief Base class for shared objects
	
	SharedObjects track dependencies upon a particular object. In this way, is is
	possible to ensure that a shared resource is not deleted if something else
	needs it. How the dependency tracking is done largely depends on the child class.
*/
class SharedObject {
	public:
		SharedObject()
			: fReferenceCount(1) {}
		virtual ~SharedObject() {}

		inline void Acquire();
		inline bool Release();

	private:
		int32 fReferenceCount;
};


inline void
SharedObject::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}

inline bool
SharedObject::Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1) {
		delete this;
		return true;
	}

	return false;
}

#endif	/* SHARED_OBJECT_H */
