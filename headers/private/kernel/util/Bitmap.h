/*
 * Copyright 2013-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef KERNEL_UTIL_BITMAP_H
#define KERNEL_UTIL_BITMAP_H


#ifdef _KERNEL_MODE
#	include <debug.h>
#else
#	include <Debug.h>
#endif

#include <string.h>

#include <SupportDefs.h>

namespace BKernel {

class Bitmap {
public:
						Bitmap(size_t bitCount);
						~Bitmap();

			status_t	InitCheck();

			status_t	Resize(size_t bitCount);
			void		Shift(ssize_t bitCount);

	inline	bool		Get(size_t index) const;
	inline	void		Set(size_t index);
	inline	void		Clear(size_t index);

			ssize_t		GetHighestSet() const;

	template<typename T>
	static	void		Shift(T* bits, size_t bitCount, ssize_t shift);
private:
			size_t		fElementsCount;
			size_t		fSize;
			addr_t*		fBits;

	static	const int	kBitsPerElement = (sizeof(addr_t) * 8);
};


bool
Bitmap::Get(size_t index) const
{
	ASSERT(index < fSize);

	const size_t kArrayElement = index / kBitsPerElement;
	const addr_t kBitMask = addr_t(1) << (index % kBitsPerElement);
	return fBits[kArrayElement] & kBitMask;
}


void
Bitmap::Set(size_t index)
{
	ASSERT(index < fSize);

	const size_t kArrayElement = index / kBitsPerElement;
	const addr_t kBitMask = addr_t(1) << (index % kBitsPerElement);
	fBits[kArrayElement] |= kBitMask;
}


void
Bitmap::Clear(size_t index)
{
	ASSERT(index < fSize);

	const size_t kArrayElement = index / kBitsPerElement;
	const addr_t kBitMask = addr_t(1) << (index % kBitsPerElement);
	fBits[kArrayElement] &= ~addr_t(kBitMask);
}


template<typename T>
void
Bitmap::Shift(T* bits, size_t bitCount, ssize_t shift)
{
	if (shift == 0)
		return;

	const size_t bitsPerElement = sizeof(T) * 8;
	const size_t elementsCount = (bitCount + bitsPerElement - 1) / bitsPerElement;
	const size_t absoluteShift = (shift > 0) ? shift : -shift;
	const size_t nElements = absoluteShift / bitsPerElement;
	const size_t nBits = absoluteShift % bitsPerElement;
	if (nElements != 0) {
		if (shift > 0) {
			// "Left" shift.
			memmove(&bits[nElements], bits, sizeof(T) * (elementsCount - nElements));
			memset(bits, 0, sizeof(T) * nElements);
		} else if (shift < 0) {
			// "Right" shift.
			memmove(bits, &bits[nElements], sizeof(T) * (elementsCount - nElements));
			memset(&bits[elementsCount - nElements], 0, sizeof(T) * nElements);
		}
	}

	// If the shift was by a multiple of the element size, nothing more to do.
	if (nBits == 0)
		return;

	// One set of bits comes from the "current" element and are shifted in the
	// direction of the shift; the other set comes from the next-processed
	// element and are shifted in the opposite direction.
	if (shift > 0) {
		// "Left" shift.
		for (ssize_t i = elementsCount - 1; i >= 0; i--) {
			T low = 0;
			if (i != 0)
				low = bits[i - 1] >> (bitsPerElement - nBits);
			const T high = bits[i] << nBits;
			bits[i] = low | high;
		}
	} else if (shift < 0) {
		// "Right" shift.
		for (size_t i = 0; i < elementsCount; i++) {
			const T low = bits[i] >> nBits;
			T high = 0;
			if (i != (elementsCount - 1))
				high = bits[i + 1] << (bitsPerElement - nBits);
			bits[i] = low | high;
		}
	}
}

} // namespace BKernel


using BKernel::Bitmap;


#endif	// KERNEL_UTIL_BITMAP_H

