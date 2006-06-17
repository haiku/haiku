/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef REFERENCABLE_H
#define REFERENCABLE_H

#include <SupportDefs.h>

class Referenceable {
 public:
								Referenceable()
									: fReferenceCount(1)
								{}
	virtual						~Referenceable()
								{}

	inline	void				Acquire();
	inline	bool				Release();

 private:
			vint32				fReferenceCount;
};

// Acquire
inline void
Referenceable::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}

// Release
inline bool
Referenceable::Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1) {
		delete this;
		return true;
	}

	return false;
}

#endif // REFERENCABLE_H
