/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef REFERENCE_COUNTING_H
#define REFERENCE_COUNTING_H


#include <SupportDefs.h>


/*!
	\class ReferenceCounting ReferenceCounting.h
	\brief Base class for reference counting objects

	ReferenceCounting objects track dependencies upon a particular object. In this way,
	it is possible to ensure that a shared resource is not deleted if something else
	needs it. How the dependency tracking is done largely depends on the child class.
*/
class ReferenceCounting {
	public:
		ReferenceCounting()
			: fReferenceCount(1) {}
		virtual ~ReferenceCounting() {}

		inline void Acquire();
		inline bool Release();

	private:
		int32 fReferenceCount;
};


inline void
ReferenceCounting::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}

inline bool
ReferenceCounting::Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1) {
		delete this;
		return true;
	}

	return false;
}

#endif	/* REFERENCE_COUNTING_H */
