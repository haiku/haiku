/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INLINE_REFERENCEABLE_H
#define INLINE_REFERENCEABLE_H


#include <SupportDefs.h>
#include <kernel/debug.h>


/*! This class should behave very similarly to BReferenceable, but
 * whereas sizeof(BReferenceable) == (sizeof(void*) * 2), this class
 * is only sizeof(int32) == 4. */
class InlineReferenceable {
public:
	InlineReferenceable()
		:
		fReferenceCount(1)
	{
	}

	~InlineReferenceable()
	{
		ASSERT(fReferenceCount == 0 || fReferenceCount == 1);
	}

	int32
	AcquireReference()
	{
		const int32 previousCount = atomic_add(&fReferenceCount, 1);
		ASSERT(previousCount > 0);
		return previousCount;
	}

	int32
	ReleaseReference()
	{
		const int32 previousCount = atomic_add(&fReferenceCount, -1);
		ASSERT(previousCount > 0);
		return previousCount;
	}

private:
	int32	fReferenceCount;
};


#define DEFINE_REFERENCEABLE_ACQUIRE_RELEASE(CLASS, InlineReferenceable) \
	void CLASS::AcquireReference()	{ InlineReferenceable.AcquireReference(); } \
	void CLASS::ReleaseReference()	{ if (InlineReferenceable.ReleaseReference() == 1) delete this; }


#endif	// INLINE_REFERENCEABLE_H
