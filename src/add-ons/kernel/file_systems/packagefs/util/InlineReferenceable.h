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
	inline InlineReferenceable()
		:
		fReferenceCount(1)
	{
	}

	inline ~InlineReferenceable()
	{
		ASSERT(fReferenceCount == 0 || fReferenceCount == 1);
	}

	inline int32
	AcquireReference()
	{
		const int32 previousCount = atomic_add(&fReferenceCount, 1);
		ASSERT(previousCount > 0);
		return previousCount;
	}

	inline int32
	ReleaseReference()
	{
		const int32 previousCount = atomic_add(&fReferenceCount, -1);
		ASSERT(previousCount > 0);
		return previousCount;
	}

	inline int32
	CountReferences()
	{
		return atomic_get(&fReferenceCount);
	}

private:
	int32	fReferenceCount;
};


#define DEFINE_INLINE_REFERENCEABLE_METHODS(CLASS, InlineReferenceable) \
	void CLASS::AcquireReference()	{ InlineReferenceable.AcquireReference(); } \
	void CLASS::ReleaseReference()	{ if (InlineReferenceable.ReleaseReference() == 1) delete this; } \
	int32 CLASS::CountReferences()	{ return InlineReferenceable.CountReferences(); }


#endif	// INLINE_REFERENCEABLE_H
