/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <WeakReferenceable.h>

#include <stdio.h>
#include <OS.h>


namespace BPrivate {


WeakPointer::WeakPointer(BWeakReferenceable* object)
	:
	fUseCount(1),
	fObject(object)
{
}


WeakPointer::~WeakPointer()
{
}


BWeakReferenceable*
WeakPointer::Get()
{
	int32 count;
	do {
		count = atomic_get(&fUseCount);
		if (count == 0)
			return NULL;
		if (count < 0)
			debugger("reference (use) count is negative");
	} while (atomic_test_and_set(&fUseCount, count + 1, count) != count);

	return fObject;
}


bool
WeakPointer::Put()
{
	const int32 count = atomic_add(&fUseCount, -1);
	if (count == 1) {
		delete fObject;
		return true;
	}
	if (count <= 0)
		debugger("reference (use) count is negative");

	return false;
}


int32
WeakPointer::UseCount() const
{
	return fUseCount;
}


void
WeakPointer::GetUnchecked()
{
	atomic_add(&fUseCount, 1);
}


//	#pragma -


BWeakReferenceable::BWeakReferenceable()
	:
	fPointer(new(std::nothrow) WeakPointer(this))
{
}


BWeakReferenceable::~BWeakReferenceable()
{
	if (fPointer->UseCount() == 1)
		atomic_test_and_set(&fPointer->fUseCount, 0, 1);

	if (fPointer->UseCount() != 0) {
		char message[256];
		snprintf(message, sizeof(message), "deleting referenceable object %p with "
			"reference count (%" B_PRId32 ")", this, fPointer->UseCount());
		debugger(message);
	}

	fPointer->ReleaseReference();
}


status_t
BWeakReferenceable::InitCheck()
{
	if (fPointer == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


WeakPointer*
BWeakReferenceable::GetWeakPointer()
{
	fPointer->AcquireReference();
	return fPointer;
}


}	// namespace BPrivate
