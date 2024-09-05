/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUMP_ALLOCATOR_H
#define _BUMP_ALLOCATOR_H


#include <stdlib.h>

#include <OS.h>
#include <SupportDefs.h>


template<size_t InlineDataSize = (16 * sizeof(void*)), size_t SlabSize = 4096>
class BumpAllocator {
public:
	BumpAllocator()
		:
		fCurrentSlab((Slab*)fInlineData)
	{
		fCurrentSlab->previous = NULL;
		fCurrentSlab->total = sizeof(fInlineData) - sizeof(Slab);
		fCurrentSlab->remaining = fCurrentSlab->total;
	}

	~BumpAllocator()
	{
		if (fCurrentSlab != (Slab*)fInlineData)
			Free(NULL);

		if (!IsEmpty())
			debugger("BumpAllocator: deleted with allocations still active!");
	}

	bool IsEmpty() const
	{
		return fCurrentSlab == (Slab*)fInlineData
			&& fCurrentSlab->remaining == fCurrentSlab->total;
	}

	void* Allocate(size_t _size)
	{
		const size_t size = _size + sizeof(Allocation);
		if (size > SlabSize)
			debugger("BumpAllocator: can't allocate larger than the slab size");
		if (fCurrentSlab->remaining < size) {
			// We need a new slab.
			Slab* newSlab = (Slab*)malloc(SlabSize);
			if (newSlab == NULL)
				return NULL;

			newSlab->previous = fCurrentSlab;
			newSlab->total = SlabSize - sizeof(Slab);
			newSlab->remaining = newSlab->total;
			fCurrentSlab = newSlab;
		}

		fCurrentSlab->remaining -= size;
		uint8* pointer = fCurrentSlab->data + fCurrentSlab->remaining;

		Allocation* allocation = (Allocation*)pointer;
		allocation->size = size;
		return allocation->data;
	}

	void Free(void* _pointer)
	{
		if (fCurrentSlab->remaining == fCurrentSlab->total) {
			// Free the current slab.
			Slab* previous = fCurrentSlab->previous;
			free(fCurrentSlab);
			fCurrentSlab = previous;
		}
		if (_pointer == NULL)
			return;

		Allocation* allocation = (((Allocation*)_pointer) - 1);

		// This needs to be the last thing allocated.
		uint8* last = fCurrentSlab->data + fCurrentSlab->remaining;
		if ((uint8*)allocation != last) {
			debugger("BumpAllocator: out-of-order free");
			return;
		}

		fCurrentSlab->remaining += allocation->size;
	}

private:
#if __cplusplus >= 201103L
#	define FLA_SIZE
#else
#	define FLA_SIZE 0
#endif
	struct Slab {
		Slab*		previous;
		uint32		total;
		uint32		remaining;
		uint8		data[FLA_SIZE];
	};
	struct Allocation {
		uint32		size;	/*!< includes sizeof(Allocation) */
		uint32		_pad;
		uint8		data[FLA_SIZE];
#undef FLA_SIZE
	};
	Slab*			fCurrentSlab;
	uint8			fInlineData[InlineDataSize - sizeof(Slab*)];
};

#endif	/* _BUMP_ALLOCATOR_H */
