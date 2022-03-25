/*
 * Copyright 2013-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 *		Augustin Cavalier <waddlesplash>
 */
#include <util/Bitmap.h>

#include <stdlib.h>
#include <string.h>

#include <util/BitUtils.h>


namespace BKernel {


Bitmap::Bitmap(size_t bitCount)
	:
	fElementsCount(0),
	fSize(0),
	fBits(NULL)
{
	Resize(bitCount);
}


Bitmap::~Bitmap()
{
	free(fBits);
}


status_t
Bitmap::InitCheck()
{
	return (fBits != NULL) ? B_OK : B_NO_MEMORY;
}


status_t
Bitmap::Resize(size_t bitCount)
{
	const size_t count = (bitCount + kBitsPerElement - 1) / kBitsPerElement;
	if (count == fElementsCount) {
		fSize = bitCount;
		return B_OK;
	}

	void* bits = realloc(fBits, sizeof(addr_t) * count);
	if (bits == NULL)
		return B_NO_MEMORY;
	fBits = (addr_t*)bits;

	if (fElementsCount < count)
		memset(&fBits[fElementsCount], 0, sizeof(addr_t) * (count - fElementsCount));

	fSize = bitCount;
	fElementsCount = count;
	return B_OK;
}


void
Bitmap::Shift(ssize_t bitCount)
{
	if (bitCount == 0)
		return;

	const size_t shift = (bitCount > 0) ? bitCount : -bitCount;
	const size_t nElements = shift / kBitsPerElement, nBits = shift % kBitsPerElement;
	if (nElements != 0) {
		if (bitCount > 0) {
			// "Left" shift.
			memmove(&fBits[nElements], fBits, sizeof(addr_t) * (fElementsCount - nElements));
			memset(fBits, 0, sizeof(addr_t) * nElements);
		} else if (bitCount < 0) {
			// "Right" shift.
			memmove(fBits, &fBits[nElements], sizeof(addr_t) * (fElementsCount - nElements));
			memset(&fBits[fElementsCount - nElements], 0, sizeof(addr_t) * nElements);
		}
	}

	// If the shift was by a multiple of the element size, nothing more to do.
	if (nBits == 0)
		return;

	// One set of bits comes from the "current" element and are shifted in the
	// direction of the shift; the other set comes from the next-processed
	// element and are shifted in the opposite direction.
	if (bitCount > 0) {
		// "Left" shift.
		for (ssize_t i = fElementsCount - 1; i >= 0; i--) {
			addr_t low = 0;
			if (i != 0)
				low = fBits[i - 1] >> (kBitsPerElement - nBits);
			const addr_t high = fBits[i] << nBits;
			fBits[i] = low | high;
		}
	} else if (bitCount < 0) {
		// "Right" shift.
		for (size_t i = 0; i < fElementsCount; i++) {
			const addr_t low = fBits[i] >> nBits;
			addr_t high = 0;
			if (i != (fElementsCount - 1))
				high = fBits[i + 1] << (kBitsPerElement - nBits);
			fBits[i] = low | high;
		}
	}
}


ssize_t
Bitmap::GetHighestSet() const
{
	size_t i = fElementsCount - 1;
	while (i >= 0 && fBits[i] == 0)
		i--;

	if (i < 0)
		return -1;

	STATIC_ASSERT(sizeof(addr_t) == sizeof(uint64)
		|| sizeof(addr_t) == sizeof(uint32));
	if (sizeof(addr_t) == sizeof(uint32))
		return log2(fBits[i]) + i * kBitsPerElement;

	uint32 v = (uint64)fBits[i] >> 32;
	if (v != 0)
		return log2(v) + sizeof(uint32) * 8 + i * kBitsPerElement;
	return log2(fBits[i]) + i * kBitsPerElement;
}


} // namespace BKernel
