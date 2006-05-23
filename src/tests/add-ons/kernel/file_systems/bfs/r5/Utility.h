/* Utility - some helper classes
 *
 * Copyright 2001-2006, Axel Dörfler, axeld@pinc-software.de
 * This file may be used under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include "bfs_endian.h"


// Simple array, used for the duplicate handling in the B+Tree,
// and for the log entries.

struct sorted_array {
	public:
		off_t	count;
		
#if __MWERKS__
		off_t	values[1];
#else
		off_t	values[0];
#endif

		off_t CountItems() const { return BFS_ENDIAN_TO_HOST_INT64(count); }
		off_t ValueAt(int32 index) const { return BFS_ENDIAN_TO_HOST_INT64(values[index]); }
		inline int32 Find(off_t value) const;
		void Insert(off_t value);
		bool Remove(off_t value);

	private:
		bool _FindInternal(off_t value, int32 &index) const;
};


inline int32
sorted_array::Find(off_t value) const
{
	int32 i;
	return _FindInternal(value, i) ? i : -1;
}


// The BlockArray reserves a multiple of "blockSize" and
// maintain array size for new entries.
// This is used for the in-memory log entries before they
// are written to disk.

class BlockArray {
	public:
		BlockArray(int32 blockSize);
		~BlockArray();

		int32 Find(off_t value);
		status_t Insert(off_t value);
		status_t Remove(off_t value);

		void MakeEmpty();

		int32 CountItems() const { return fArray != NULL ? fArray->CountItems() : 0; }
		int32 BlocksUsed() const { return fArray != NULL ? ((fArray->CountItems() + 1) * sizeof(off_t) + fBlockSize - 1) / fBlockSize : 0; }
		sorted_array *Array() const { return fArray; }
		int32 Size() const { return fSize; }

	private:
		sorted_array *fArray;
		int32	fBlockSize;
		int32	fSize;
		int32	fMaxBlocks;
};


// Some atomic operations that are somehow missing in BeOS:
//
//	_atomic_test_and_set(value, newValue, testAgainst)
//		sets "value" to "newValue", if "value" is equal to "testAgainst"
//	_atomic_set(value, newValue)
//		sets "value" to "newValue"

#if _NO_INLINE_ASM
	// Note that these atomic versions *don't* work as expected!
	// They are only used for single processor user space tests
	// (and don't even work correctly there)
	inline int32
	_atomic_test_and_set(volatile int32 *value, int32 newValue, int32 testAgainst)
	{
		int32 oldValue = *value;
		if (oldValue == testAgainst)
			*value = newValue;

		return oldValue;
	}

	inline void
	_atomic_set(volatile int32 *value, int32 newValue)
	{
		*value = newValue;
	}
#elif __INTEL__
	inline int32
	_atomic_test_and_set(volatile int32 *value, int32 newValue, int32 testAgainst)
	{
		int32 oldValue;
		asm volatile("lock; cmpxchg %%ecx, (%%edx)"
			: "=a" (oldValue) : "a" (testAgainst), "c" (newValue), "d" (value));
		return oldValue;
	}

	inline void
	_atomic_set(volatile int32 *value, int32 newValue)
	{
		asm volatile("lock; xchg %%eax, (%%edx)"
			: : "a" (newValue), "d" (value));
	}
#elif __POWERPC__ && __MWERKS__ /* GCC has different assembler syntax */
inline asm int32
	_atomic_set(volatile int32 *value, int32)
	{
		loop:
			dcbf	r0, r3;
			lwarx	r0, 0, r3;
			stwcx.	r4, 0, r3;
			bc        5, 2, loop
		mr r3,r5;
		isync;
		blr;	
	}
	
inline asm int32
	_atomic_test_and_set(volatile int32 *value, int32 newValue, int32 testAgainst)
	{
		loop:
			dcbf	r0, r3;
			lwarx	r0, 0, r3;
			cmpw	r5, r0;
			bne		no_dice;
			stwcx.	r4, 0, r3;
			bc      5, 2, loop
			
		mr 		r3,r0;
		isync;
		blr;
		
		no_dice:
			stwcx.	r0, 0, r3;
			mr 		r3,r0;
			isync;
			blr;
	}
			
#else
#	error The macros _atomic_set(), and _atomic_test_and_set() are not defined for the target processor
#endif


extern "C" size_t strlcpy(char *dest, char const *source, size_t length);


#endif	/* UTILITY_H */
